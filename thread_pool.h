#ifndef THREAD_POOL_THREAD_POOL_H
#define THREAD_POOL_THREAD_POOL_H

#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <vector>

class ThreadPool {
public:
    explicit ThreadPool(size_t);
    template<class F, class... Args>
    auto Enqueue(F &&f, Args &&...args) -> std::future<typename std::result_of<F(Args...)>::type>;
    void Barrier();
    ~ThreadPool();

private:
    // thread pool
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mutex;
    std::condition_variable queue_condition_;
    std::atomic<bool> is_stop_;

    // barriers
    std::atomic<bool> is_can_pop_;
    std::atomic<bool> is_barriers_ready_;
    std::mutex barrier_start_mutex_;
    std::condition_variable barrier_start_condition_;
    std::atomic<std::size_t> num_barrier_running_threads_;
};

inline ThreadPool::ThreadPool(size_t threads)
    : is_stop_(false), is_can_pop_(true), is_barriers_ready_(false), num_barrier_running_threads_(0) {
    for (size_t i = 0; i < threads; ++i)
        workers_.emplace_back([this] {
            for (;;) {
                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(queue_mutex);
                    queue_condition_.wait(lock, [this] {
                        return (is_stop_ || !tasks_.empty()) && is_can_pop_;
                    });
                    if (is_stop_ && tasks_.empty()) {
                        return;
                    }
                    task = std::move(tasks_.front());
                    tasks_.pop();
                }

                task();
            }
        });
}

template<class F, class... Args>
auto ThreadPool::Enqueue(F &&f, Args &&...args) -> std::future<typename std::result_of<F(Args...)>::type> {
    using return_type = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex);

        if (is_stop_) throw std::runtime_error("Enqueue on stopped ThreadPool");

        tasks_.emplace([task]() {
            (*task)();
        });
    }
    queue_condition_.notify_one();
    return res;
}

void ThreadPool::Barrier() {
    for (std::size_t i = 0; i < workers_.size(); i++) {
        Enqueue([this]() {
            for (;;) {
                num_barrier_running_threads_++;

                if (num_barrier_running_threads_ == workers_.size()) {
                    is_barriers_ready_ = true;
                    break;
                }
                std::unique_lock<std::mutex> lock(barrier_start_mutex_);
                barrier_start_condition_.wait(lock, [this]() -> bool {
                    return is_barriers_ready_;
                });
                break;
            }
            barrier_start_condition_.notify_all();
            num_barrier_running_threads_--;
            if (num_barrier_running_threads_ == 0) {
                is_barriers_ready_ = false;
                is_can_pop_ = true;
                queue_condition_.notify_all();
            } else {
                is_can_pop_ = false;
            }
        });
    }
}

inline ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        is_stop_ = true;
    }
    queue_condition_.notify_all();
    for (std::thread &worker: workers_) worker.join();
}

#endif// THREAD_POOL_THREAD_POOL_H
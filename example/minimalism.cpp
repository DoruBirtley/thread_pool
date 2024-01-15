#include <iostream>

#include "./../thread_pool.h"

ThreadPool pool(8);

int main() {
    for (int j = 0; j < 10; j++) {
        for (int i = 0; i < 10; ++i) {
            pool.Enqueue([] {
                // std::cout not thread safe
                std::cout << std::this_thread::get_id() << std::endl;
            });
        }
        pool.Barrier();
    }
    return 0;
}
# thread_pool

## finished work

### Implemented the basic functions of the memory pool

#### minimalism
```c++
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
```

#### example:
```c++
#include <chrono>
#include <iostream>
#include <vector>

#include "random"
#include "thread_pool.h"


std::default_random_engine engine{std::random_device{}()};
std::uniform_int_distribution<int> distribution(1, 5);

ThreadPool pool(8);

volatile double function() {
    volatile double d = 0;
    for (double value = 0; value < 1024 * 1024 * 128; value += 0.1) {
        d += sqrt(value);
        d = sqrt(d);
    }
    return d;
}

int main() {
    std::vector<std::future<int>> results;
    for (int j = 0; j < 10; j++) {
        for (int i = 0; i < 10; ++i) {
            results.emplace_back(pool.Enqueue([i] {
                function();
                std::this_thread::sleep_for(std::chrono::seconds(i));
                return i * i;
            }));
        }
        pool.Barrier();
    }
    for (auto &&result: results) {
        std::cout << result.get() << std::endl;
    }
    return 0;
}
```
#### cpu usage chart corresponding to the example:
<img src="./images/frame_graph.jpg">
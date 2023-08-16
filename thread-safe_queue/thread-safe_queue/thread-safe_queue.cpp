#include <iostream>
#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>

using namespace std::chrono_literals;
std::queue <std::function<void()>> s_queue;


class safe_queue {
public:
    safe_queue() = default;
    safe_queue(const safe_queue&) = delete;
    safe_queue& operator = (const safe_queue&) = delete;
    void push(std::function <void()>& func) {
        std::unique_lock<std::mutex> lock1(m);
        s_queue.push(func);
        cond.notify_one();
    }
    std::function <void()> pop() {
        std::unique_lock<std::mutex> lock2(m);
        cond.wait(lock2, [this] {return !s_queue.empty(); });
        std::function <void()> func_elem = s_queue.front();
        s_queue.pop();
        return func_elem;
    }
private:
    mutable std::mutex m;
    std::condition_variable cond;
};

class thread_pool {
public:
    thread_pool(const thread_pool&) = delete;
    thread_pool& operator = (const thread_pool&) = delete;
    thread_pool() {
        const int size_thread = std::thread::hardware_concurrency() - 2;
        for (size_t i = 0; i < size_thread; i++) {
            threads.push_back(std::thread(&thread_pool::work, this));
        }
    }
    
    void work() {
        if (!s_queue.empty()) {
            std::function<void()> task = sf.pop();
            task();
        }
    }
    void submit(std::function<void()> pool) {
        sf.push(pool);
        thread_pool();
    }
    ~thread_pool() {
        for (size_t i = 0; i < threads.size(); i++) {
            if (threads[i].joinable()) {
                threads[i].join();
            };
        }
    }
private:
    std::vector<std::thread> threads;
    safe_queue sf;
};



void func1() {
    std::cout << "Function1 id = " << std::this_thread::get_id() << "\n";
}
void func2() {
    std::cout << "Function2 id = " << std::this_thread::get_id() << "\n";
}

int main()
{
    thread_pool t_pool;
    for (size_t i = 0; i < 15; i++) {
        t_pool.submit(func1);
        t_pool.submit(func2);
        std::this_thread::sleep_for(1s);
    }
    return 0;
}

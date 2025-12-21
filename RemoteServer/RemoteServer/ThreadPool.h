#pragma once

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include <atomic>

class ThreadPool {
public:
    // 构造函数：启动指定数量的线程
    explicit ThreadPool(size_t threads) : m_stop(false) {
        for (size_t i = 0; i < threads; ++i) {
            m_workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;

                    {
                        // 创建一个互斥锁
                        std::unique_lock<std::mutex> lock(this->m_queue_mutex);

                        // 等待条件：停止线程池 或者 任务队列不为空
                        // wait 会自动解锁 m_queue_mutex 并阻塞，被唤醒后重新加锁
                        this->m_condition.wait(lock, [this] {
                            return this->m_stop || !this->m_tasks.empty();
                            });

                        // 如果停止且队列空，则退出线程
                        if (this->m_stop && this->m_tasks.empty())
                            return;

                        // 取出任务
                        task = std::move(this->m_tasks.front());
                        this->m_tasks.pop();
                    }

                    // 执行任务 (在锁外执行，防止阻塞其他线程入队)
                    task();
                }
                });
        }
    }

    // 析构函数：停止所有线程
    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            m_stop = true;
        }
        m_condition.notify_all(); // 唤醒所有线程让它们退出

        for (std::thread& worker : m_workers) {
            if (worker.joinable())
                worker.join(); // 等待线程结束
        }
    }

    // 核心方法：投递任务
    // 支持 Lambda、函数指针、类成员函数
    // F: 函数类型, Args: 参数包
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::invoke_result<F, Args...>::type>
    {
        // 推导任务的返回类型
        using return_type = typename std::invoke_result<F, Args...>::type;

        // 将任务打包成 packaged_task，以便通过 future 获取返回值
        // 使用 shared_ptr 是为了能拷贝进 lambda (std::function 要求可拷贝)
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        // 获取 future 对象
        std::future<return_type> res = task->get_future();

        {
            std::unique_lock<std::mutex> lock(m_queue_mutex);

            // 如果线程池已经停止，抛出异常
            if (m_stop)
                throw std::runtime_error("enqueue on stopped ThreadPool");

            // 将任务放入队列
            // 这里的 lambda 捕获 task 指针并执行，这就把任意返回类型的任务统一成了 void()
            m_tasks.emplace([task]() { (*task)(); });
        }

        m_condition.notify_one(); // 唤醒一个空闲线程去干活
        return res;
    }

private:
    // 工作线程组
    std::vector<std::thread> m_workers;

    // 任务队列
    std::queue<std::function<void()>> m_tasks;

    // 同步机制
    std::mutex m_queue_mutex;
    std::condition_variable m_condition;

    // 停止标记
    bool m_stop;
};
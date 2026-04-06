#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <future>

class FJobSystem {
public:
    FJobSystem(uint32_t numThreads = std::thread::hardware_concurrency()) {
        for (uint32_t i = 0; i < numThreads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> job;
                    {
                        std::unique_lock<std::mutex> lock(queueMutex);
                        condition.wait(lock, [this] { return stop || !jobs.empty(); });
                        if (stop && jobs.empty()) return;

                        job = std::move(jobs.front());
                        jobs.pop();
                    }
                    job();
                }
                });
        }
    }

    ~FJobSystem() {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread& worker : workers) worker.join();
    }

    // 수정된 PushJob: std::future를 반환하여 대기 가능하게 함
    template<class F, class... Args>
    auto PushJob(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type> {
        using return_type = typename std::invoke_result<F, Args...>::type;

        // 작업을 캡슐화하여 결과값을 추후에 가져올 수 있게 함
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            if (stop) throw std::runtime_error("PushJob on stopped JobSystem");

            // 람다로 감싸서 큐에 삽입
            jobs.emplace([task]() { (*task)(); });
        }
        condition.notify_one();
        return res;
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> jobs;
    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop = false;
};

extern FJobSystem GJobSystem;

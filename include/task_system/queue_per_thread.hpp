//
// Created by Justin Durkan on 07/05/2016.
//

#ifndef TASK_SYSTEM_QUEUE_PER_THREAD_HPP
#define TASK_SYSTEM_QUEUE_PER_THREAD_HPP

#include <thread>
#include <vector>
#include <memory>
#include <queue>
#include <functional>
#include <boost/optional.hpp>

#include <utility>

namespace nitro {
    namespace concurrency {


        class queue_per_thread_task_queue {
        public:
            auto pop() -> std::function<void()> {
                std::unique_lock<std::mutex> lock{mtx};
                while (task_queue.empty() && !terminate_queue) {
                    queue_can_pop.wait(lock);
                }

                if (!terminate_queue) {
                    auto task_fn = move(task_queue.front());
                    task_queue.pop();

                    return task_fn;
                }
                return std::function<void()>([](){});
            }

            template <typename F>
            void push(F&& f) {
                {
                    std::unique_lock<std::mutex> lock{mtx};
                    task_queue.emplace(std::forward<F>(f));
                }

                queue_can_pop.notify_one();
            }

            void done() {
                {
                    std::unique_lock<std::mutex> lock{mtx};
                    task_queue.emplace([this]() {
                        terminate_queue = true;
                    });
                }
                queue_can_pop.notify_all();
            }

            auto is_terminated() -> bool {
                std::unique_lock<std::mutex> lock{mtx};
                return terminate_queue;
            }

        private:
            std::queue<std::function<void()>> task_queue;
            bool terminate_queue{false};
            std::mutex mtx;
            std::condition_variable queue_can_pop;
        };


        class queue_per_thread_scheduler  {
        public:
            queue_per_thread_scheduler() {
                for (unsigned n = 0; n != hw_num_cores; ++n) {
                    core_threads.emplace_back([&, n] {
                        run(n);
                    });
                }
            }

            ~queue_per_thread_scheduler() {
                for (auto& q : task_queues) {
                    q.done();
                }
                for (auto& t : core_threads) {
                    t.join();
                }
            }

            template <typename F>
            void async(F&& f)  {
                auto i = queue_index++;
                task_queues[i % hw_num_cores].push(std::forward<F>(f));
            }

        private:
            void run(unsigned index)  noexcept {
                auto& q = task_queues[index];
                while (!q.is_terminated()) {
                    auto task_fn = q.pop();
                    task_fn();
                    tasks_executed++;
                }
            }

            const unsigned hw_num_cores = std::thread::hardware_concurrency();
            std::vector<std::thread> core_threads;
            std::vector<queue_per_thread_task_queue> task_queues{hw_num_cores};
            std::atomic<unsigned int> queue_index{0};
            std::atomic<int> tasks_executed{0};
        };
    }
}

#endif //TASK_SYSTEM_QUEUE_PER_THREAD_HPP

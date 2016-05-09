//
// Created by Justin Durkan on 07/05/2016.
//

#ifndef NITRO_TASK_SYSTEM_TYPICAL_TASK_SYSTEM_HPP
#define NITRO_TASK_SYSTEM_TYPICAL_TASK_SYSTEM_HPP

#include <thread>
#include <vector>
#include <memory>
#include <deque>
#include <queue>
#include <functional>
#include <boost/optional.hpp>

#include <utility>

#include "scheduler.hpp"

namespace nitro {
    namespace concurrency {

        class typical_task_queue {
        public:
            ~typical_task_queue() {
                ;
            }

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
                    task_queue.emplace(std::forward<const std::function<void()>>(f));
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


        class typical_scheduler  {
        public:
            typical_scheduler() {
                for (unsigned n = 0; n != hw_num_cores; ++n) {
                    core_threads.emplace_back([&, n] {
                        run(n);
                    });
                }
            }

            ~typical_scheduler() {
                task_queue.done();
                for (auto& t : core_threads) {
                    t.join();
                }
            }

            template <typename F>
            void async(F&& f)  {
                task_queue.push(std::forward<F>(f));
            }

        private:
            void run(unsigned index)  noexcept {
                while (!task_queue.is_terminated()) {
                    auto task_fn = task_queue.pop();
                    task_fn();
                }
            }

            const unsigned hw_num_cores = std::thread::hardware_concurrency();
            std::vector<std::thread> core_threads;
            typical_task_queue task_queue;
        };

    }
}


#endif //NITRO_TASK_SYSTEM_TYPICAL_TASK_SYSTEM_HPP

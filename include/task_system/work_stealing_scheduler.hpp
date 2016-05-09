//
// Created by Justin Durkan on 06/05/2016.
//

#ifndef NITRO_TASK_SYSTEM_WORK_STEALING_SCHEDULER_HPP
#define NITRO_TASK_SYSTEM_WORK_STEALING_SCHEDULER_HPP


#include <thread>
#include <vector>
#include <memory>
#include <deque>
#include <functional>
#include <boost/optional.hpp>

#include <utility>

namespace nitro {
    namespace concurrency {

        class work_stealing_task_queue {
        public:
            auto pop_opt() -> boost::optional<std::function<void()>> {
                boost::optional<std::function<void()>> ret;

                std::unique_lock<std::mutex> lock{mtx, std::try_to_lock};

                if (lock.owns_lock() && !task_queue.empty()) {
                    ret = move(task_queue.front());
                    task_queue.pop();
                }

                return ret;
            }

            template <typename F>
            auto try_push(F&& f) -> bool {
                {
                    std::unique_lock<std::mutex> lock{mtx, std::try_to_lock};
                    if (!lock) {
                        return false;
                    }
                    task_queue.emplace(std::forward<F>(f));
                }
                queue_can_pop.notify_one();
                return true;
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

        private:
            std::queue<std::function<void()>> task_queue;
            bool terminate_queue{false};
            std::mutex mtx;
            std::condition_variable queue_can_pop;
        };

        constexpr int SPIN_FACTOR = 64;

        class work_stealing_scheduler {
        public:
            work_stealing_scheduler() {
                for (unsigned n = 0; n != hw_num_cores; ++n) {
                    core_threads.emplace_back([&, n] {
                        run(n);
                    });
                }
            }

            ~work_stealing_scheduler() {
                for (auto& e : task_queues) {
                    e.done();
                }
                for (auto& e : core_threads) {
                    e.join();
                }
            }

            template <typename F>
            void async(F&& f)  {
                auto curr_task_count = queue_index++;

                for (unsigned i = 0; i != hw_num_cores; ++i) {
                    auto push_succeeded = task_queues[(curr_task_count + i) % hw_num_cores].try_push(
                            std::forward<F>(f));
                    if (push_succeeded) {
                        return;
                    }
                }

                task_queues[curr_task_count % hw_num_cores].push(
                        std::forward<F>(f));
            }

        private:
            void run(unsigned index)  noexcept {
                auto& q = task_queues[index];
                while (!q.is_terminated()) {
                    std::function<void()> task_fn;

                    const unsigned N = hw_num_cores * SPIN_FACTOR;
                    for (unsigned i = 0; i != N; ++i) {
                        auto q_index = (index + i) % hw_num_cores;
                        auto& curr_q = task_queues[q_index];

                        auto task_fn_opt = curr_q.pop_opt();
                        if (task_fn_opt) {
                            task_fn = *task_fn_opt;
                            break;
                        }
                    }

                    if (!task_fn) {
                        task_fn = q.pop();
                    }

                    task_fn();
                }
            }

            const unsigned hw_num_cores = std::thread::hardware_concurrency();
            std::vector<std::thread> core_threads;
            std::vector<work_stealing_task_queue> task_queues{hw_num_cores};

            std::atomic<unsigned> queue_index{0};
        };
    }
}


#endif //NITRO_TASK_SYSTEM_WORK_STEALING_SCHEDULER_HPP

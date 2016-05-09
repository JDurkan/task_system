//
// Created by Justin Durkan on 06/05/2016.
//

#ifndef NITRO_TASK_SYSTEM_SCHEDULER_HPP
#define NITRO_TASK_SYSTEM_SCHEDULER_HPP

namespace nitro {
    namespace concurrency {

        class scheduler {
        public:
            virtual void async(const std::function<void()>&& f) = 0;
        };

    }
}

#endif //NITRO_TASK_SYSTEM_SCHEDULER_HPP

//
// Created by Justin Durkan on 06/05/2016.
//
#include <gtest/gtest.h>

#include "task_system/typical_task_system.hpp"
#include "task_system/queue_per_thread.hpp"
#include "task_system/work_stealing_scheduler.hpp"

#include <boost/multiprecision/cpp_int.hpp>

template<typename T, typename N, typename O>
T power(T x, N n, O op) {
    if (n == 0) {
        return identity_element(op);
    }

    while ((n & 1) == 0) {
        n >>= 1;
        x = op(x, x);
    }

    T result = x;
    n >>= 1;
    while (n != 0) {
        x = op(x, x);
        if ((n & 1) != 0) {
            result = op(result, x);
        }
        n >>= 1;
    }
    return result;
}

template<typename N>
struct multiply_2x2 {
    std::array<N, 4> operator()(const std::array<N, 4>& x, const std::array<N, 4>& y) {
        return {x[0] * y[0] + x[1] * y[2], x[0] * y[1] + x[1] * y[3],
                x[2] * y[0] + x[3] * y[2], x[2] * y[1] + x[3] * y[3]};
    }
};

template<typename N>
std::array<N, 4> identity_element(const multiply_2x2<N>&) { return {N(1), N(0), N(0), N(1)}; }

template<typename R, typename N>
R fibonacci(N n) {
    if (n == 0) {
        return R(0);
    }
    return power(std::array<R, 4>{1, 1, 1, 0}, N(n - 1), multiply_2x2<R>())[0];
}


/**
 ************** test function has side effects only ************
 **/
void test_func() {
    fibonacci<boost::multiprecision::cpp_int>(100);
}


constexpr int NUM_TEST_ITERATIONS = 1000000;

TEST(task_system_tests, single_threaded_test) {
    std::cout << "Number of hardware threads: 1 and " ;

    auto begin = std::chrono::system_clock::now();
    for (int i = 0; i < NUM_TEST_ITERATIONS; i++) {
        test_func();
    }
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - begin;
    std::cout << "Duration (seconds): " << elapsed_seconds.count() << std::endl;
}

TEST(task_system_tests, typical_task_system_test) {
    std::cout << "\nNumber of hardware threads: " << std::thread::hardware_concurrency()  << " and ";

    std::chrono::time_point<std::chrono::system_clock> begin;
    {
        nitro::concurrency::typical_scheduler typical_sched;

        begin = std::chrono::system_clock::now();

        for (int i = 0; i < NUM_TEST_ITERATIONS; i++) {
            typical_sched.async(test_func);
        }
    }
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - begin;
    std::cout << "typical scheduler duration (seconds): " << elapsed_seconds.count() << std::endl;
}


TEST(task_system_tests, queue_per_thread_test) {
    std::cout << "\nNumber of hardware threads: " << std::thread::hardware_concurrency()  << " and ";

    std::chrono::time_point<std::chrono::system_clock> begin;
    {
        nitro::concurrency::queue_per_thread_scheduler queue_per_thread_sched;

        begin = std::chrono::system_clock::now();
        for (int i = 0; i < NUM_TEST_ITERATIONS; i++) {
            queue_per_thread_sched.async(test_func);
        }
    }
    auto end = std::chrono::system_clock::now();

    std::chrono::duration<double> elapsed_seconds = end - begin;
    std::cout << "queue per thread scheduler duration (seconds): " << elapsed_seconds.count() << std::endl;
}


TEST(task_system_tests, work_stealing_test) {
    std::cout << "\nNumber of hardware threads: " << std::thread::hardware_concurrency()  << " and ";

    std::chrono::time_point<std::chrono::system_clock> begin;
    {
        nitro::concurrency::work_stealing_scheduler work_stealing_sched;

        begin = std::chrono::system_clock::now();
        for (int i = 0; i < NUM_TEST_ITERATIONS; i++) {
            work_stealing_sched.async(test_func);
        }
    }

    auto end = std::chrono::system_clock::now();

    std::chrono::duration<double> elapsed_seconds = end - begin;
    std::cout << "work stealing scheduler duration (seconds): " << elapsed_seconds.count() << std::endl;
}

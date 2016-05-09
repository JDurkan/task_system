cmake_minimum_required(VERSION 3.3)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

enable_testing()

include_directories(${googletest_SOURCE_DIR}/googletest/googlemock/gtest)
include_directories("../include")

link_directories("../googletest")

SET(testSRCs gtestmain.cpp tast_system_tests.cpp)

add_executable(task_system_unit_tests ${testSRCs})

target_link_libraries(task_system_unit_tests gtest gmock)

if(Boost_FOUND)
    target_link_libraries(task_system_unit_tests ${Boost_LIBRARIES})
endif()


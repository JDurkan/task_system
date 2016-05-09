cmake_minimum_required(VERSION 3.5)
project(task_system)

message(STATUS "Loading Boost files")
find_package(Boost 1.60.0 COMPONENTS thread regex serialization system filesystem REQUIRED)

if(Boost_FOUND)
    message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
    include_directories(${Boost_INCLUDE_DIRS})
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

add_subdirectory(googletest)

add_subdirectory(unit_tests)

cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(GA VERSION 1.0)
add_executable(GA_run main.cpp
                      lib/core.cpp
                      lib/solution.cpp)
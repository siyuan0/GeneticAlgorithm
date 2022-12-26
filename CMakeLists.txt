cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(THREADS_PREFER_PTHREAD_FLAG ON)


project(GA VERSION 1.0)

add_executable(GA_run main.cpp
                      lib/solution.cpp)
find_package(Threads REQUIRED)
target_link_libraries(GA_run PRIVATE Threads::Threads)
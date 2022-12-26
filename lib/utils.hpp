#ifndef INCLUDE_UTILS
#define INCLUDE_UTILS

#include <sstream>
#include <iostream>
#include <mutex>
#include <chrono>

std::timed_mutex coutGuard; // use for thread-safe cout
const std::chrono::duration<double, std::milli> default_timeout(1); // default timeout for waiting for cout lock

#define CONCAT2(a, b) a ## b

#define CONCAT(a, b) CONCAT2(a, b)

#define LOG(...) std::cout << __VA_ARGS__ ; 

// thread-safe printing to iostream for multiple arguments: THREADPRINT(a << b << c)
#define THREADPRINT(...) \
    coutGuard.try_lock_for(default_timeout); \
    std::cout << __VA_ARGS__; \
    coutGuard.unlock();

// thread-blocking reserve of iostream
#define RESERVECOUT coutGuard.try_lock_for(default_timeout);

#define UNRESERVECOUT coutGuard.unlock();

#endif // INCLUDE_UTILS
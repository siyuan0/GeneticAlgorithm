#ifndef INCLUDE_UTILS
#define INCLUDE_UTILS

#include <sstream>
#include <iostream>
#include <mutex>
#include <chrono>
#include <random>

std::timed_mutex coutGuard; // use for thread-safe cout
const std::chrono::duration<double, std::milli> default_timeout(10); // default timeout for waiting for cout lock
std::random_device rd{};
std::mt19937 gen{rd()};

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

struct randNormal
{
    float _mean;
    float _stddev;
    std::normal_distribution<> _dist;

    randNormal(float mean, float stddev)
    {
        _mean = mean;
        _stddev = stddev;
        _dist = std::normal_distribution<>{mean, stddev};
    }

    float rand(std::mt19937& generator)
    {
       return  _dist(generator);
    }
};

int intRand(const int & min, const int & max, std::mt19937& generator) {
    std::uniform_int_distribution<int> distribution(min,max);
    return distribution(generator);
}

#endif // INCLUDE_UTILS
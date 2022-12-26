#ifndef INCLUDE_UTILS
#define INCLUDE_UTILS

#include <sstream>
#include <iostream>

#define CONCAT2(a, b) a ## b

#define CONCAT(a, b) CONCAT2(a, b)

// thread-safe printing to iostream for multiple arguments: THREADPRINT(a << b << c)
#define THREADPRINT(...) \
    std::stringstream CONCAT(ss, __LINE__); \
    CONCAT(ss, __LINE__) << __VA_ARGS__; \
    std::cout << CONCAT(ss, __LINE__).str();

#endif // INCLUDE_UTILS
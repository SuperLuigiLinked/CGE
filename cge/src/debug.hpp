/**
 * @file debug.hpp
 */

#pragma once

#include <cstdlib>
#define ASSERT(expr) if (expr) {} else std::abort()

#if defined(CGE_DEBUG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Weverything"
    #include <cstdio>
    #include <fmt/core.h>
    #define LOG(...) fmt::print(stderr, __VA_ARGS__)
    #pragma clang diagnostic pop
    
    #define LOG(...) fmt::print(stderr, __VA_ARGS__)
#else
    #define LOG(...) ((void)0)
#endif

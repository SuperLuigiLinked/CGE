/**
 * @file cge/debug.hpp
 */

#pragma once


#include <cstdlib>
#if defined(__WIN32) && !defined(NDEBUG)
    #define CGE_ASSERT(expr) if (expr) {} else (__debugbreak(), std::abort())
#else
    #define CGE_ASSERT(expr) if (expr) {} else std::abort()
#endif

#if defined(CGE_DEBUG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Weverything"
    #include <cstdio>
    #include <fmt/core.h>
    #define CGE_LOG(...) fmt::print(stderr, __VA_ARGS__)
    #pragma clang diagnostic pop
    
    #define CGE_LOG(...) fmt::print(stderr, __VA_ARGS__)
#else
    #define CGE_LOG(...) ((void)0)
#endif

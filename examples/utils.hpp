/**
 * @file examples/utils.hpp
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cmath>

#include <array>
#include <vector>
#include <string>
#include <span>

#if 1
    #include <cstdio>
    #pragma clang diagnostic ignored "-Weverything"
    #pragma clang diagnostic push
    #include <fmt/core.h>
    #pragma clang diagnostic pop
    #define LOG(...) fmt::print(stderr, __VA_ARGS__)
#else
    #define LOG(...) ((void)0)
#endif

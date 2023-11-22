/**
 * @file examples/utils.hpp
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cmath>

#include <numbers>
#include <span>
#include <array>
#include <vector>
#include <string>

#if 0
    #include <cstdio>
    #pragma clang diagnostic ignored "-Weverything"
    #pragma clang diagnostic push
    #include <fmt/core.h>
    #include <fmt/color.h>
    #pragma clang diagnostic pop
    #define LOG(...) fmt::print(stderr, __VA_ARGS__)
#else
    #define LOG(...) ((void)0)
#endif

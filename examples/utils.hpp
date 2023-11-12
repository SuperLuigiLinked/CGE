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

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include <cstdio>
#include <fmt/core.h>
#define LOG(...) fmt::print(stderr, __VA_ARGS__)
#pragma clang diagnostic pop

/**
 * @file cge/vec.hpp
 */

#pragma once

#ifndef CGE_VEC_H
#define CGE_VEC_H

#include <cstdint>

namespace cge
{
    struct ivec2 { int32_t x, y; }; 
    struct ivec3 { int32_t x, y, z; }; 
    struct ivec4 { int32_t x, y, z, w; }; 

    struct uvec2 { uint32_t x, y; }; 
    struct uvec3 { uint32_t x, y, z; }; 
    struct uvec4 { uint32_t x, y, z, w; }; 

    struct vec2 { float x, y; }; 
    struct vec3 { float x, y, z; }; 
    struct vec4 { float x, y, z, w; }; 

    struct dvec2 { double x, y; }; 
    struct dvec3 { double x, y, z; }; 
    struct dvec4 { double x, y, z, w; }; 

    inline constexpr vec4 rgba() noexcept { return {}; }
    inline constexpr vec4 rgba(const float v) noexcept { return { v, v, v, 1.0f}; }
    inline constexpr vec4 rgba(const float v, const float a) noexcept { return { v, v, v, a}; }
    inline constexpr vec4 rgba(const float r, const float g, const float b) noexcept { return { r, g, b, 1.0f }; }
    inline constexpr vec4 rgba(const float r, const float g, const float b, const float a) noexcept { return { r, g, b, a }; }
}

#endif

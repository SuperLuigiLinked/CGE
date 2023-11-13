/**
 * @file cge/types.hpp
 */

#pragma once

#ifndef CGE_TYPES_H
#define CGE_TYPES_H

#include <cstddef>
#include <cstdint>
#include <bit>

namespace cge
{
    // --------------------------------------------------------------------------------------------------------------------------------

    using Coord = double;

    using Extent = double;

    struct Point
    {
        Coord x, y;
    };
    
    struct Size
    {
        Extent w, h;
    };
    
    struct Rect
    {
        Point origin;
        Size size;
    };

    // --------------------------------------------------------------------------------------------------------------------------------

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

    // --------------------------------------------------------------------------------------------------------------------------------

    struct Vertex
    {
        vec4 xyzw;
        vec2 uv;
        uvec2 st;
    };

    // --------------------------------------------------------------------------------------------------------------------------------

    struct RGBA
    {
        using byte = std::uint8_t;
        using uint = std::uint32_t;

        byte r, g, b, a;

        static inline constexpr byte min{ 0x00 };
        static inline constexpr byte max{ 0xFF };
        static inline constexpr uint shift_r{ 16 };
        static inline constexpr uint shift_g{  8 };
        static inline constexpr uint shift_b{    };
        static inline constexpr uint shift_a{ 24 };

        static inline constexpr float to_float(const auto bval) noexcept
        { return float(bval) / float(max); }
        
        static inline constexpr byte to_byte(const auto fval) noexcept
        { return byte(float(fval) * float(max)); }

        inline constexpr uint as_uint() const noexcept
        {
            return (uint(r) << shift_r)
                | (uint(g) << shift_g)
                | (uint(b) << shift_b)
                | (uint(a) << shift_a)
            ;
        }

        static inline constexpr RGBA from_uint(const uint val) noexcept
        {
            return {
                .r = byte(val >> shift_r),
                .g = byte(val >> shift_g),
                .b = byte(val >> shift_b),
                .a = byte(val >> shift_a)
            };
        }

        inline constexpr vec4 as_vec() const noexcept
        { return { to_float(r), to_float(g), to_float(b), to_float(a) }; }

        static inline constexpr RGBA from_vec(const vec4 vec) noexcept
        { return { to_byte(vec.x), to_byte(vec.y), to_byte(vec.z), to_byte(vec.w) }; }

        static inline constexpr RGBA splat() noexcept
        { return { min, min, min, min }; }

        static inline constexpr RGBA splat(const float v) noexcept
        { return { to_byte(v), to_byte(v), to_byte(v), max }; }

        static inline constexpr RGBA splat(const float v, const float a) noexcept
        { return { to_byte(v), to_byte(v), to_byte(v), to_byte(a) }; }

        static inline constexpr RGBA splat(const float r, const float g, const float b) noexcept
        { return { to_byte(r), to_byte(g), to_byte(b), max }; }

        static inline constexpr RGBA splat(const float r, const float g, const float b, const float a) noexcept
        { return { to_byte(r), to_byte(g), to_byte(b), to_byte(a) }; }
    };

    struct Texture
    {
        std::size_t width;
        std::size_t height;
        const RGBA* data;
    };

    // --------------------------------------------------------------------------------------------------------------------------------
}

#endif

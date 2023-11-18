/**
 * @file cge/types.hpp
 */

#pragma once

#ifndef CGE_TYPES_H
#define CGE_TYPES_H

#include <cstddef>
#include <cstdint>
#include <bit>
#include <span>
#include <vector>

namespace cge
{
    // --------------------------------------------------------------------------------------------------------------------------------

    struct ivec2 { std::int32_t x, y; }; 
    struct ivec3 { std::int32_t x, y, z; }; 
    struct ivec4 { std::int32_t x, y, z, w; }; 

    struct uvec2 { std::uint32_t x, y; }; 
    struct uvec3 { std::uint32_t x, y, z; }; 
    struct uvec4 { std::uint32_t x, y, z, w; }; 

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

    using Index = std::uint16_t;

    // --------------------------------------------------------------------------------------------------------------------------------

    using Color = std::uint32_t;

    static inline constexpr Color shift_r{ 16 };
    static inline constexpr Color shift_g{  8 };
    static inline constexpr Color shift_b{  0 };
    static inline constexpr Color shift_a{ 24 };

    static inline constexpr Color rgba(const float r, const float g, const float b, const float a = 1.0f) noexcept
    {
        return (Color(r * 255.0f) << shift_r)
             | (Color(g * 255.0f) << shift_g)
             | (Color(b * 255.0f) << shift_b)
             | (Color(a * 255.0f) << shift_a)
        ;
    }

    static inline constexpr Color rgba(const int r, const int g, const int b, const int a = 255) noexcept
    {
        return (Color(r) << shift_r)
             | (Color(g) << shift_g)
             | (Color(b) << shift_b)
             | (Color(a) << shift_a)
        ;
    }

    // --------------------------------------------------------------------------------------------------------------------------------

    struct Texture
    {
        std::uint32_t width;
        std::uint32_t height;
        const Color* data; // #AARRGGBB format (BGRA little-endian, ARGB big-endian)

        inline constexpr std::size_t size() const noexcept
        { return std::size_t(this->width) * std::size_t(this->height); }

        inline constexpr bool empty() const noexcept
        { return !this->width || !this->height || !this->data; }

        inline constexpr std::span<const Color> elems() const noexcept
        { return std::span{ this->data, this->size() }; }

        inline constexpr std::span<const std::byte> as_bytes() const noexcept
        { return std::as_bytes(this->elems()); }
    };

    // --------------------------------------------------------------------------------------------------------------------------------
}

#endif

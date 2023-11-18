/**
 * @file cge/cge.hpp
 * @brief Simple C++ Game Engine.
 */

#pragma once

#ifndef CGE_H
#define CGE_H

#include <cstddef>
#include <cstdint>
#include <span>
#include <array>
#include <vector>

namespace cge
{
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
}

namespace cge
{
    struct Vertex
    {
        vec4 xyzw;
        vec2 uv;
        uvec2 st;
    };

    using Index = std::uint16_t;
}

namespace cge
{
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
}

namespace cge
{
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
}

namespace cge
{
    struct GameSettings
    {
        const char* name;
        double width;
        double height;
        double fps;
        bool vsync;
        bool fullscreen;
    };

    struct RenderSettings
    {
    private:
    
        friend class Engine;

        inline static constexpr Index sentinel_idx{ 0xFFFF };
        inline static constexpr Color default_clr{ 0xFF000000 };

        struct Primitives
        {
            Color background_clr;
            std::vector<Vertex> point_vtx;
            std::vector<Vertex> line_vtx;
            std::vector<Vertex> triangle_vtx;
            std::vector<Vertex> line_strip_vtx;
            std::vector<Index> line_strip_idx;
            std::vector<Vertex> triangle_strip_vtx;
            std::vector<Index> triangle_strip_idx;
            std::vector<Vertex> triangle_fan_vtx;
            std::vector<Index> triangle_fan_idx;
        };

        Primitives self;

    public:

        inline constexpr void clear(const Color color = default_clr) noexcept
        {
            self.background_clr = color;
            self.point_vtx.clear();
            self.line_vtx.clear();
            self.triangle_vtx.clear();
            self.line_strip_vtx.clear();
            self.line_strip_idx.clear();
            self.triangle_strip_vtx.clear();
            self.triangle_strip_idx.clear();
            self.triangle_fan_vtx.clear();
            self.triangle_fan_idx.clear();
        }

        inline constexpr void point(const Vertex vtx)
        {
            self.point_vtx.push_back(vtx);
        }

        inline constexpr void line(const std::span<const Vertex, 2> vtx)
        {
            self.line_vtx.push_back(vtx[0]);
            self.line_vtx.push_back(vtx[1]);
        }

        inline constexpr void triangle(const std::span<const Vertex, 3> vtx)
        {
            self.triangle_vtx.push_back(vtx[0]);
            self.triangle_vtx.push_back(vtx[1]);
            self.triangle_vtx.push_back(vtx[2]);
        }

        inline constexpr void line_strip(const std::span<const Vertex /* 2+ */> vtx)
        {
            if (vtx.size() >= 2)
            {
                Index idx{ static_cast<Index>(self.line_strip_vtx.size()) };
                for (const Vertex& v : vtx)
                {
                    self.line_strip_vtx.push_back(v);
                    self.line_strip_idx.push_back(idx);
                    ++idx;
                }
                self.line_strip_idx.push_back(sentinel_idx);
            }
        }

        inline constexpr void triangle_strip(const std::span<const Vertex /* 3+ */> vtx)
        {
            if (vtx.size() >= 3)
            {
                Index idx{ static_cast<Index>(self.triangle_strip_vtx.size()) };
                for (const Vertex& v : vtx)
                {
                    self.triangle_strip_vtx.push_back(v);
                    self.triangle_strip_idx.push_back(idx);
                    ++idx;
                }
                self.triangle_strip_idx.push_back(sentinel_idx);
            }
        }

        inline constexpr void triangle_fan(const std::span<const Vertex /* 3+ */> vtx)
        {
            if (vtx.size() >= 3)
            {
            #ifdef __APPLE__
                const Index size{ static_cast<Index>(self.triangle_fan_vtx.size()) };
                for (Index idx{ 1 }; idx < size - 1; ++idx)
                {
                    self.triangle_fan_vtx.push_back(vtx[idx]);
                    self.triangle_fan_vtx.push_back(vtx[idx + 1]);
                    self.triangle_fan_vtx.push_back(vtx[0]);
                }
            #else
                Index idx{ static_cast<Index>(self.triangle_fan_vtx.size()) };
                for (const Vertex& v : vtx)
                {
                    self.triangle_fan_vtx.push_back(v);
                    self.triangle_fan_idx.push_back(idx);
                    ++idx;
                }
                self.triangle_fan_idx.push_back(sentinel_idx);
            #endif
            }
        }
    };
}

namespace cge
{
    class Engine;

    extern void quit(Engine& engine) noexcept;
    extern bool quitting(const Engine& engine) noexcept;

    extern GameSettings& settings(Engine& engine) noexcept;
    extern RenderSettings& renderer(Engine& engine) noexcept;

    extern double elapsed_seconds(const Engine& engine) noexcept;
}

namespace cge
{
    class Game
    {
    public:
        virtual ~Game() = default;
        virtual void update(Engine& engine);
        virtual void render(Engine& engine);
    };
}

namespace cge
{
    /**
     * @brief Runs the game loop until the game terminates.
     * @warning MUST be called on the Main Thread.
     *          MUST NOT be called while the Engine is already running.
     */
    void run(Game& game, const GameSettings& settings);
}

#endif

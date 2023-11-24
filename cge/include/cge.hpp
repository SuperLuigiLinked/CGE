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
    using sint = std::int32_t;
    using uint = std::uint32_t;
    
    struct ivec2 { sint x, y; }; 
    struct ivec3 { sint x, y, z; }; 
    struct ivec4 { sint x, y, z, w; }; 

    struct uvec2 { uint x, y; }; 
    struct uvec3 { uint x, y, z; }; 
    struct uvec4 { uint x, y, z, w; }; 

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

    using Index = std::uint32_t;

    using Color = std::uint32_t;
}

namespace cge
{
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
    struct Settings
    {
        const char* name;
        double width;
        double height;
        double fps;
        bool vsync;
        bool fullscreen;
    };

    struct Scene
    {
    public:

        std::uint32_t res_w;
        std::uint32_t res_h;
        cge::Color backcolor;

        std::vector<cge::Vertex> vertices;
        std::vector<cge::Index> indices;

    public:

        inline constexpr void clear() noexcept
        {
            vertices.clear();
            indices.clear();
        }

        inline constexpr void draw_tri(const std::span<const cge::Vertex, 3> vtx_list)
        {
            const cge::Index base{ static_cast<cge::Index>(vertices.size()) };
            
            vertices.push_back(vtx_list[0]);
            vertices.push_back(vtx_list[1]);
            vertices.push_back(vtx_list[2]);

            indices.push_back(base);
            indices.push_back(base + 1);
            indices.push_back(base + 2);
        }

        inline constexpr void draw_strip(const std::span<const cge::Vertex /* 3+ */> vtx_list)
        {
            if (vtx_list.size() < 3) return;

            const cge::Index base{ static_cast<cge::Index>(vertices.size()) };
            
            vertices.push_back(vtx_list[0]);
            vertices.push_back(vtx_list[1]);
            for (cge::Index rel{ 2 }; rel < vtx_list.size(); ++rel)
            {
                vertices.push_back(vtx_list[rel]);
                indices.push_back(base + rel - 2);
                indices.push_back(base + rel - 1);
                indices.push_back(base + rel);
            }
        }

        inline constexpr void draw_fan(const std::span<const cge::Vertex /* 3+ */> vtx_list)
        {
            if (vtx_list.size() < 3) return;

            const cge::Index base{ static_cast<cge::Index>(vertices.size()) };

            vertices.push_back(vtx_list[0]);
            vertices.push_back(vtx_list[1]);
            for (cge::Index rel{ 2 }; rel < vtx_list.size(); ++rel)
            {
                vertices.push_back(vtx_list[rel]);
                indices.push_back(base + rel - 1);
                indices.push_back(base + rel);
                indices.push_back(base);
            }
        }
    };
}

namespace cge
{
    class Engine;

    extern void quit(Engine& engine) noexcept;
    extern bool quitting(const Engine& engine) noexcept;

    extern Settings& settings(Engine& engine) noexcept;
    extern Scene& scene(Engine& engine) noexcept;

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
    void run(Game& game, const Settings& settings);
}

#endif

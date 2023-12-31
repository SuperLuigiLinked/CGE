/**
 * @file cge/cge.hpp
 * @brief Simple C++ Game Engine.
 */

#pragma once

#ifndef CGE_H
#define CGE_H

#include <cstddef>
#include <cstdint>
#include <cmath>
#include <numeric>
#include <variant>
#include <vector>
#include <array>
#include <span>

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

    using Index = cge::uint;

    using Color = cge::uint;
}

namespace cge
{
    static inline constexpr cge::Color shift_r{ 16 };
    static inline constexpr cge::Color shift_g{  8 };
    static inline constexpr cge::Color shift_b{  0 };
    static inline constexpr cge::Color shift_a{ 24 };

    static inline constexpr cge::Color rgba(const float r, const float g, const float b, const float a = 1.0f) noexcept
    {
        return (cge::Color(r * 255.0f) << shift_r)
             | (cge::Color(g * 255.0f) << shift_g)
             | (cge::Color(b * 255.0f) << shift_b)
             | (cge::Color(a * 255.0f) << shift_a)
        ;
    }

    static inline constexpr Color rgba(const int r, const int g, const int b, const int a = 255) noexcept
    {
        return (cge::Color(r) << shift_r)
             | (cge::Color(g) << shift_g)
             | (cge::Color(b) << shift_b)
             | (cge::Color(a) << shift_a)
        ;
    }

    static inline float from_srgb(const float val) noexcept
    {
        return val < 0.04045f ? val / 12.92f : std::pow((val + 0.055f) / 1.055f, 2.4f);
    }

    static inline float to_srgb(const float val) noexcept
    {
        return val < 0.04045f / 12.92f ? val * 12.92f : std::pow(val, 1.0f / 2.4f) * 1.055f - 0.055f;
    }
}

namespace cge
{
    struct Texture
    {
        cge::uint width;
        cge::uint height;
        const cge::Color* data; // #AARRGGBB format (BGRA little-endian, ARGB big-endian)

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
    enum class Scaling
    {
        none, ///< No scaling is performed.
        fit, ///< The scene is scaled to fit the window exactly.
        aspect, ///< The scene is scaled to fit at least one window dimension, preserving Aspect Ratio as closely as possible.
        aspect_exact, ///< The scene is scaled, preserving Aspect Ratio exactly.
        exact, ///< The scene is scaled only by integer multiples.
    };

    struct Viewport { cge::sint x, y; cge::uint w, h; };
    
    extern cge::Viewport viewport(cge::uint window_w, cge::uint window_h, cge::uint render_w, cge::uint render_h, cge::Scaling scaling) noexcept;
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

        cge::uint res_w;
        cge::uint res_h;
        cge::Scaling scaling;

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
                if (rel % 2 == 0)
                {
                    indices.push_back(base + rel - 2);
                    indices.push_back(base + rel - 1);
                    indices.push_back(base + rel);
                }
                else
                {
                    indices.push_back(base + rel - 2);
                    indices.push_back(base + rel);
                    indices.push_back(base + rel - 1);
                }
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
    struct EventInit {};
    struct EventFocus { bool focused; };
    struct EventReposition { double x, y, w, h; double scale; };
    struct EventCursor { double x, y; };
    struct EventCursorExit {};
    struct EventScroll { double x, y; };
    struct EventMouse  { unsigned short button; bool pressed; };
    struct EventKeyboard { unsigned short keycode; bool pressed; };
    struct EventText { const unsigned char* text; };

    using Event = std::variant
    <
        cge::EventInit,
        cge::EventFocus,
        cge::EventReposition,
        cge::EventCursor,
        cge::EventCursorExit,
        cge::EventScroll,
        cge::EventMouse,
        cge::EventKeyboard,
        cge::EventText
    >;
}

namespace cge
{
    class Engine;

    extern void quit(cge::Engine& engine) noexcept;
    extern bool quitting(const cge::Engine& engine) noexcept;

    extern cge::Settings& settings(cge::Engine& engine) noexcept;

    extern double elapsed_seconds(const cge::Engine& engine) noexcept;
}

namespace cge
{
    class Game
    {
    public:
        virtual ~Game() = default;
        virtual void event(cge::Engine& engine, cge::Event event);
        virtual void update(cge::Engine& engine);
        virtual void render(cge::Engine& engine, cge::Scene& scene);
    };
}

namespace cge
{
    /**
     * @brief Runs the game loop until the game terminates.
     * @warning MUST be called on the Main Thread.
     *          MUST NOT be called while the Engine is already running.
     */
    void run(cge::Game& game, const cge::Settings& settings);
}

#endif

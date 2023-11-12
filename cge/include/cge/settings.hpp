/**
 * @file cge/settings.hpp
 */

#pragma once

#ifndef CGE_SETTINGS_H
#define CGE_SETTINGS_H

#include <cstdint>
#include <vector>
#include <span>

#include "cge/types.hpp"
#include "cge/vec.hpp"

namespace cge
{
    struct InitSettings
    {
        Extent width;
        Extent height;
        double fps;
        bool vsync;
        bool fullscreen;
    };

    struct WindowSettings
    {
        const char* name;
        Rect rect;
        bool fullscreen;
    };

    struct Vertex
    {
        vec4 xyzw;
        vec4 rgba;
    };

    struct VertexUV
    {
        vec4 xyzw;
        vec4 rgba;
        vec2 uv;
    };

    using Index = std::uint16_t;

    struct RenderSettings
    {
    public:

        inline static constexpr Index sentinel_idx{ 0xFFFF };

    public:

        vec4 backcolor;

        std::vector<Vertex> point_vtx;
        std::vector<Vertex> line_vtx;
        std::vector<Vertex> triangle_vtx;
        std::vector<Vertex> line_strip_vtx;
        std::vector<Index> line_strip_idx;
        std::vector<Vertex> triangle_strip_vtx;
        std::vector<Index> triangle_strip_idx;
        std::vector<Vertex> triangle_fan_vtx;
        std::vector<Index> triangle_fan_idx;

        std::vector<VertexUV> uv_point_vtx;
        std::vector<VertexUV> uv_line_vtx;
        std::vector<VertexUV> uv_triangle_vtx;
        std::vector<VertexUV> uv_line_strip_vtx;
        std::vector<Index> uv_line_strip_idx;
        std::vector<VertexUV> uv_triangle_strip_vtx;
        std::vector<Index> uv_triangle_strip_idx;
        std::vector<VertexUV> uv_triangle_fan_vtx;
        std::vector<Index> uv_triangle_fan_idx;

    private:

        std::size_t updates;
        std::size_t renders;
        double fps;
        bool vsync;

    public:

        void clear();

        void point(const Vertex vtx);
        void line(const std::span<const Vertex, 2> vtx);
        void triangle(const std::span<const Vertex, 3> vtx);
        void line_strip(const std::span<const Vertex /* 2+ */> vtx);
        void triangle_strip(const std::span<const Vertex /* 3+ */> vtx);
        void triangle_fan(const std::span<const Vertex /* 3+ */> vtx);

        void uv_point(const Vertex vtx);
        void uv_line(const std::span<const Vertex, 2> vtx);
        void uv_triangle(const std::span<const Vertex, 3> vtx);
        void uv_line_strip(const std::span<const Vertex /* 2+ */> vtx);
        void uv_triangle_strip(const std::span<const Vertex /* 3+ */> vtx);
        void uv_triangle_fan(const std::span<const Vertex /* 3+ */> vtx);

    };
}

#endif

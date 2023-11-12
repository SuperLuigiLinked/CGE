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

        inline constexpr void clear() noexcept
        {
            this->backcolor = cge::vec4(0.0f, 0.0f, 0.0f, 1.0f);

            this->point_vtx.clear();
            this->line_vtx.clear();
            this->triangle_vtx.clear();
            this->line_strip_vtx.clear();
            this->line_strip_idx.clear();
            this->triangle_strip_vtx.clear();
            this->triangle_strip_idx.clear();
            this->triangle_fan_vtx.clear();
            this->triangle_fan_idx.clear();

            this->uv_point_vtx.clear();
            this->uv_line_vtx.clear();
            this->uv_triangle_vtx.clear();
            this->uv_line_strip_vtx.clear();
            this->uv_line_strip_idx.clear();
            this->uv_triangle_strip_vtx.clear();
            this->uv_triangle_strip_idx.clear();
            this->uv_triangle_fan_vtx.clear();
            this->uv_triangle_fan_idx.clear();
        }

        inline constexpr void point(const Vertex vtx)
        {
            this->point_vtx.push_back(vtx);
        }

        inline constexpr void line(const std::span<const Vertex, 2> vtx)
        {
            this->line_vtx.push_back(vtx[0]);
            this->line_vtx.push_back(vtx[1]);
        }

        inline constexpr void triangle(const std::span<const Vertex, 3> vtx)
        {
            this->triangle_vtx.push_back(vtx[0]);
            this->triangle_vtx.push_back(vtx[1]);
            this->triangle_vtx.push_back(vtx[2]);
        }

        inline constexpr void line_strip(const std::span<const Vertex /* 2+ */> vtx)
        {
            if (vtx.size() >= 2)
            {
                Index idx{ static_cast<Index>(this->line_strip_vtx.size()) };
                for (const Vertex& v : vtx)
                {
                    this->line_strip_vtx.push_back(v);
                    this->line_strip_idx.push_back(idx);
                    ++idx;
                }
                this->line_strip_idx.push_back(sentinel_idx);
            }
        }

        inline constexpr void triangle_strip(const std::span<const Vertex /* 3+ */> vtx)
        {
            if (vtx.size() >= 3)
            {
                Index idx{ static_cast<Index>(this->triangle_strip_vtx.size()) };
                for (const Vertex& v : vtx)
                {
                    this->triangle_strip_vtx.push_back(v);
                    this->triangle_strip_idx.push_back(idx);
                    ++idx;
                }
                this->triangle_strip_idx.push_back(sentinel_idx);
            }
        }

        inline constexpr void triangle_fan(const std::span<const Vertex /* 3+ */> vtx)
        {
            if (vtx.size() >= 3)
            {
                Index idx{ static_cast<Index>(this->triangle_fan_vtx.size()) };
                for (const Vertex& v : vtx)
                {
                    this->triangle_fan_vtx.push_back(v);
                    this->triangle_fan_idx.push_back(idx);
                    ++idx;
                }
                this->triangle_fan_idx.push_back(sentinel_idx);
            }
        }

        inline constexpr void uv_point(const VertexUV vtx)
        {
            this->uv_point_vtx.push_back(vtx);
        }

        inline constexpr void uv_line(const std::span<const VertexUV, 2> vtx)
        {
            this->uv_line_vtx.push_back(vtx[0]);
            this->uv_line_vtx.push_back(vtx[1]);
        }

        inline constexpr void uv_triangle(const std::span<const VertexUV, 3> vtx)
        {
            this->uv_triangle_vtx.push_back(vtx[0]);
            this->uv_triangle_vtx.push_back(vtx[1]);
            this->uv_triangle_vtx.push_back(vtx[2]);
        }

        inline constexpr void uv_line_strip(const std::span<const VertexUV /* 2+ */> vtx)
        {
            if (vtx.size() >= 2)
            {
                Index idx{ static_cast<Index>(this->uv_line_strip_vtx.size()) };
                for (const VertexUV& v : vtx)
                {
                    this->uv_line_strip_vtx.push_back(v);
                    this->uv_line_strip_idx.push_back(idx);
                    ++idx;
                }
                this->uv_line_strip_idx.push_back(sentinel_idx);
            }
        }

        inline constexpr void uv_triangle_strip(const std::span<const VertexUV /* 3+ */> vtx)
        {
            if (vtx.size() >= 3)
            {
                Index idx{ static_cast<Index>(this->uv_triangle_strip_vtx.size()) };
                for (const VertexUV& v : vtx)
                {
                    this->uv_triangle_strip_vtx.push_back(v);
                    this->uv_triangle_strip_idx.push_back(idx);
                    ++idx;
                }
                this->uv_triangle_strip_idx.push_back(sentinel_idx);
            }
        }

        inline constexpr void uv_triangle_fan(const std::span<const VertexUV /* 3+ */> vtx)
        {
            if (vtx.size() >= 3)
            {
                Index idx{ static_cast<Index>(this->uv_triangle_fan_vtx.size()) };
                for (const VertexUV& v : vtx)
                {
                    this->uv_triangle_fan_vtx.push_back(v);
                    this->uv_triangle_fan_idx.push_back(idx);
                    ++idx;
                }
                this->uv_triangle_fan_idx.push_back(sentinel_idx);
            }
        }
    };
}

#endif

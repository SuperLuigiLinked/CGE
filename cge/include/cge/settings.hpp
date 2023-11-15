/**
 * @file cge/settings.hpp
 */

#pragma once

#ifndef CGE_SETTINGS_H
#define CGE_SETTINGS_H

#include <vector>
#include <span>

#include "cge/types.hpp"

namespace cge
{
    struct InitSettings
    {
        double width;
        double height;
        double fps;
        bool vsync;
        bool fullscreen;
    };

    struct WindowSettings
    {
        const char* name;
        double width;
        double height;
        bool fullscreen;
    };

    struct RenderSettings
    {
    public:

        inline static constexpr Index sentinel_idx{ 0xFFFF };

    public:

        RGBA backcolor;

    private:
        
        friend class Renderer;
        friend class Renderer_VK;

        std::vector<Vertex> point_vtx;
        std::vector<Vertex> line_vtx;
        std::vector<Vertex> triangle_vtx;
        std::vector<Vertex> line_strip_vtx;
        std::vector<Index> line_strip_idx;
        std::vector<Vertex> triangle_strip_vtx;
        std::vector<Index> triangle_strip_idx;
        std::vector<Vertex> triangle_fan_vtx;
        std::vector<Index> triangle_fan_idx;

    public:

        inline constexpr void clear() noexcept
        {
            this->backcolor = { 0x00, 0x00, 0x00, 0xFF };

            this->point_vtx.clear();
            this->line_vtx.clear();
            this->triangle_vtx.clear();
            this->line_strip_vtx.clear();
            this->line_strip_idx.clear();
            this->triangle_strip_vtx.clear();
            this->triangle_strip_idx.clear();
            this->triangle_fan_vtx.clear();
            this->triangle_fan_idx.clear();
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
            #ifdef __APPLE__
                const Index size{ static_cast<Index>(this->triangle_fan_vtx.size()) };
                for (Index idx{ 1 }; idx < size - 1; ++idx)
                {
                    this->triangle_fan_vtx.push_back(vtx[idx]);
                    this->triangle_fan_vtx.push_back(vtx[idx + 1]);
                    this->triangle_fan_vtx.push_back(vtx[0]);
                }
            #else
                Index idx{ static_cast<Index>(this->triangle_fan_vtx.size()) };
                for (const Vertex& v : vtx)
                {
                    this->triangle_fan_vtx.push_back(v);
                    this->triangle_fan_idx.push_back(idx);
                    ++idx;
                }
                this->triangle_fan_idx.push_back(sentinel_idx);
            #endif
            }
        }
    };

    struct Settings
    {
        std::size_t updates;
        std::size_t renders;
        double fps;
        bool vsync;
    };
}

#endif

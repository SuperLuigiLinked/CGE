/**
 * @file cge/settings.hpp
 */

#pragma once

#ifndef CGE_SETTINGS_H
#define CGE_SETTINGS_H

#include "cge/types.hpp"

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
    
        friend class EngineImpl;

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

        inline constexpr void clear() noexcept
        {
            self.background_clr = default_clr;
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

        inline constexpr void backcolor(const Color color) noexcept
        {
            self.background_clr = color;
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

#endif

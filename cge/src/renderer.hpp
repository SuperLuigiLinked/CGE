/**
 * @file renderer.hpp
 */

#pragma once

#include "cge/cge.hpp"

#include <wyn.h>

namespace cge
{
    class Renderer
    {
    public:
        virtual ~Renderer() = default;

        virtual void target_window(EngineImpl& impl, wyn_window_t window);

        virtual void render(EngineImpl& impl);
    };

    extern Renderer* renderer_vk();
}

/**
 * @file renderer.hpp
 */

#pragma once

#include "cge/cge.hpp"

namespace cge
{
    class Renderer
    {
    public:
        virtual ~Renderer() = default;
        virtual void render(EngineImpl& impl) = 0;
    };

    extern Renderer* renderer_vk();
}

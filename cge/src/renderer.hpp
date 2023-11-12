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
        virtual void render(EngineImpl& impl);
    };

    extern Renderer* renderer_vk();
}

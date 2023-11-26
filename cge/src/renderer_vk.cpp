/**
 * @file cge/renderer_vk.cpp
 */

#include "engine.hpp"
#include "cvk/cvk.hpp"

namespace cge
{
    class Renderer_VK final : public cge::Renderer
    {
    private:

        cvk::Vulkan self;

    public:

        Renderer_VK() = default;
        ~Renderer_VK() final;

        void target_window(cge::Engine& engine, wyn_window_t window) final;
        void render(cge::Engine& engine) final;

    };
}

namespace cge
{
    Renderer_VK::~Renderer_VK()
    {
        cvk::destroy_renderable(self.ctx, self.gfx);
        cvk::destroy_context(self.ctx);
    }

    extern cge::Renderer* renderer_vk()
    {
        return new Renderer_VK();
    }
}

namespace cge
{
    void Renderer_VK::target_window(cge::Engine& engine, wyn_window_t const window)
    {
        if (self.ctx.instance == nullptr)
        {
            cvk::create_context(self.ctx);
        }

        if (self.gfx.window != nullptr)
        {
            cvk::destroy_renderable(self.ctx, self.gfx);
        }

        if (window)
        {
            cvk::create_renderable(self.ctx, self.gfx, window, engine.settings.vsync);
        }
    }

    void Renderer_VK::render(cge::Engine& engine)
    {
        constexpr unsigned max_attempts{ 8 };

        const VkExtent2D cur_extent{ cvk::full_resolution(self.ctx, self.gfx, true) };
        if (!(cur_extent.width && cur_extent.height)) return;

        if ((cur_extent.width != self.gfx.surface_extent.width) || (cur_extent.height != self.gfx.surface_extent.height) || (engine.cached_vsync != self.gfx.surface_vsync))
        {
            cvk::remake_swapchain(self.ctx, self.gfx, engine.cached_vsync);
            if (!(self.gfx.surface_extent.width && self.gfx.surface_extent.height)) return;
        }

        for (unsigned attempts{}; attempts < max_attempts; ++attempts)
        {
            const VkResult res_render{ cvk::render_frame(self.ctx, self.gfx, engine.scene) };
            if (res_render == VK_SUCCESS) return;
            if (cge::quitting(engine)) return;

            cvk::remake_swapchain(self.ctx, self.gfx, engine.cached_vsync);
            if (!(self.gfx.surface_extent.width && self.gfx.surface_extent.height)) return;

            if (res_render == VK_ERROR_OUT_OF_DATE_KHR)
            {
                continue;
            }
            else if (res_render == VK_SUBOPTIMAL_KHR)
            {
                return;
            }
            else
            {
                CGE_LOG("[CGE] RENDER {} : VkResult {}\n", attempts, +res_render);
                CGE_ASSERT(res_render == VK_SUCCESS);
            }
        }
        CGE_LOG("[CGE] RENDER FAILED {} TIMES. ABORTING...\n", max_attempts);
    }
}

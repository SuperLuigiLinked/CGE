/**
 * @file cge/renderer_vk.cpp
 */

#include "engine.hpp"
#include "cvk/cvk.hpp"

namespace cge
{
    class Renderer_VK final : public Renderer
    {
    private:

        cvk::Vulkan self;

    public:

        Renderer_VK() = default;
        ~Renderer_VK() final;

        void target_window(Engine& engine, wyn_window_t window) final;
        void render(Engine& engine) final;

    };
}

namespace cge
{
    Renderer_VK::~Renderer_VK()
    {
        cvk::destroy_renderable(self.ctx, self.gfx);
        cvk::destroy_context(self.ctx);
    }

    extern Renderer* renderer_vk()
    {
        return new Renderer_VK();
    }
}

namespace cge
{
    void Renderer_VK::target_window(Engine& engine, wyn_window_t const window)
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
            cvk::create_renderable(self.ctx, self.gfx, window, engine.game_settings.vsync);
        }
    }

    void Renderer_VK::render(Engine& engine)
    {
        for (unsigned attempts{}; attempts < 10; ++attempts)
        {
            const bool res_render{ cvk::render_frame(self.ctx, self.gfx, engine.primitives(), attempts) };
            if (res_render) return;

            if (cge::quitting(engine)) return;

            cvk::update_surface_info(self.ctx, self.gfx, self.gfx.sel_device);
            cvk::remake_swapchain(self.ctx, self.gfx, engine.cached_vsync);

            // if ((self.surface_size.width == 0) || (self.surface_size.height == 0)) return;
        }
        CGE_LOG("[CGE] RENDER ABORTING...\n");
    }
}

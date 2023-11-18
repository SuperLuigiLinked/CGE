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

        cvk::Context context;
        cvk::Renderable renderable;
        cvk::Atlas atlas;

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
        cvk::destroy_atlas(this->context, this->renderable, this->atlas);
        cvk::destroy_renderable(this->context, this->renderable);
        cvk::destroy_context(this->context);
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
        if (this->context.instance == nullptr)
        {
            this->context = cvk::create_context();
        }

        if (this->renderable.window != nullptr)
        {
            cvk::destroy_renderable(this->context, this->renderable);
            this->renderable = cvk::Renderable{};
        }

        if (window)
        {
            this->renderable = cvk::create_renderable(this->context, wyn_window_t(window), engine.game_settings.vsync);
            
            this->atlas = cvk::create_atlas(this->context, this->renderable, cvk::default_texture);
            
            cvk::upload_atlas(this->context, this->renderable, this->atlas);
        }
    }

    void Renderer_VK::render(Engine& engine)
    {
        for (unsigned attempts{}; ; ++attempts)
        {
            const bool res_render{ cvk::render_frame(this->renderable, engine.primitives(), attempts) };
            if (res_render) break;

            if (cge::quitting(engine)) return;

            if (attempts > 10)
            {
                CGE_LOG("[CGE] Renderer aborting...\n");
                return;
            }
            cvk::update_surface_info(this->context, this->renderable);
            cvk::remake_swapchain(this->renderable, engine.cached_vsync);

            if ((this->renderable.surface_size.width == 0) || (this->renderable.surface_size.height == 0)) return;
        }
    }
}

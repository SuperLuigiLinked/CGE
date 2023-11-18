/**
 * @file cge/impl.hpp
 */

#pragma once

#include "cge/cge.hpp"

#include <wyn.h>
#include <wyt.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <condition_variable>

namespace cge
{
    extern wyt_retval_t WYT_ENTRY update_main(void* arg) noexcept;
    extern wyt_retval_t WYT_ENTRY render_main(void* arg) noexcept;
}

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

namespace cge
{
    using SignalImpl = unsigned char;

    enum Signal : SignalImpl
    {
        signal_quit   = 0b1,
        signal_render = 0b10,
        signal_update = 0b100,
    };
}

namespace cge
{
    class EngineImpl final
    {
    public:

        using Primitives = RenderSettings::Primitives;

        inline constexpr Engine engine() noexcept { return Engine(this); }
        inline constexpr Primitives& primitives() noexcept { return gfx_settings.self; }

    public:

        Game& game;
        
        GameSettings game_settings;
        RenderSettings gfx_settings;
        
        double cached_fps;
        bool cached_vsync;

        wyt_time_t epoch;
        wyn_window_t window;
        wyt_thread_t update_thread;
        wyt_thread_t render_thread;

        std::unique_ptr<Renderer> renderer;

        std::atomic<SignalImpl> signal;
    
    };

    using Primitives = cge::EngineImpl::Primitives;
}

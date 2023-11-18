/**
 * @file cge/events.cpp
 */

#include "cge/cge.hpp"
#include "impl.hpp"
#include "debug.hpp"

#include <wyn.h>

extern "C"
{
    using namespace cge;

    void wyn_on_start(void* const userdata)
    {
        EngineImpl& impl{ *static_cast<EngineImpl*>(userdata) };
        
        impl.window = wyn_window_open();
        if (!impl.window) return impl.engine().quit();

        wyn_window_resize(impl.window, { impl.game_settings.width, impl.game_settings.height });
        impl.renderer->target_window(impl, impl.window);

        wyn_window_show(impl.window);

        impl.epoch = wyt_nanotime();

        impl.render_thread = wyt_spawn(render_main, userdata);
        if (!impl.render_thread) return impl.engine().quit();

        impl.update_thread = wyt_spawn(update_main, userdata);
        if (!impl.update_thread) return impl.engine().quit();
    }

    void wyn_on_stop(void* const userdata)
    {
        EngineImpl& impl{ *static_cast<EngineImpl*>(userdata) };

        impl.signal.store(cge::signal_quit, std::memory_order::relaxed);
        impl.signal.notify_all();

        if (impl.update_thread)
        {
            wyt_join(impl.update_thread);
        }

        if (impl.render_thread)
        {
            wyt_join(impl.render_thread);
        }

        if (impl.window)
        {
            wyn_window_close(impl.window);
        }
    }

    void wyn_on_signal(void* const userdata)
    {
        EngineImpl& impl{ *static_cast<EngineImpl*>(userdata) };
        const Engine engine{ impl.engine() };

        const SignalImpl signal{ impl.signal.load(std::memory_order::acquire) };
        if (!signal) return;

        if (signal & cge::signal_quit)
        {
            wyn_quit();
            return;
        }

        if (signal & cge::signal_render)
        {
            impl.game.render(engine);
            impl.cached_vsync = impl.game_settings.vsync;
        }

        if (signal & cge::signal_update)
        {
            impl.game.update(engine);
            impl.cached_fps = impl.game_settings.fps;
        }

        {
           // wyn_window_resize(impl.window, { impl.game_settings.width, impl.game_settings.height });
        }

        impl.signal.store(signal & cge::signal_quit, std::memory_order::release);
        impl.signal.notify_all();
    }

    void wyn_on_window_close(void* const userdata, wyn_window_t const window)
    {
        EngineImpl& impl{ *static_cast<EngineImpl*>(userdata) };

        if (window == impl.window)
        {
            impl.engine().quit();
        }
    }
}

namespace cge
{
    static bool await_signal(EngineImpl& impl, const SignalImpl signal) noexcept
    {
        SignalImpl cached = signal | impl.signal.fetch_or(signal, std::memory_order::relaxed);
        if (cached & cge::signal_quit) return false;

        wyn_signal();

        while (cached & signal)
        {
            impl.signal.wait(cached);
            cached = impl.signal.load(std::memory_order_acquire);
            if (cached & cge::signal_quit) return false;
        }

        return true;
    }

    extern wyt_retval_t WYT_ENTRY render_main(void* const arg) noexcept
    {
        EngineImpl& impl{ *static_cast<EngineImpl*>(arg) };
        
        for(;;)
        {
            if (!await_signal(impl, cge::signal_render)) return {};

            impl.renderer->render(impl);
        }
    }

    extern wyt_retval_t WYT_ENTRY update_main(void* const arg) noexcept
    {
        EngineImpl& impl{ *static_cast<EngineImpl*>(arg) };
        
        const wyt_time_t epoch{ impl.epoch };
        wyt_time_t last_tick{ epoch };

        for (;;)
        {
           if (!await_signal(impl, cge::signal_update)) return {};

            const double fps{ impl.cached_fps };

            if (fps > 0)
            {
                const wyt_time_t frame_nanos{ static_cast<wyt_time_t>(1'000'000'000.0 / fps) };
                const wyt_time_t last_nanos{ last_tick - epoch };
                const wyt_time_t last_frame{ last_nanos / frame_nanos };
                const wyt_time_t next_frame{ last_frame + 1 };
                const wyt_time_t next_nanos{ next_frame * frame_nanos };
                const wyt_time_t next_tick{ epoch + next_nanos };
                wyt_nanosleep_until(next_tick);
            }
            else
            {
                wyt_yield();
            }
            last_tick = wyt_nanotime();
        }
    }
}

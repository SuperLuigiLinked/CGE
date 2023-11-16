/**
 * @file engine.cpp
 */

#include "cge/cge.hpp"
#include "impl.hpp"

#include <wyn.h>

extern "C"
{
    using namespace cge;

    void wyn_on_start(void* const userdata)
    {
        EngineImpl& impl{ *static_cast<EngineImpl*>(userdata) };
        
        impl.window = wyn_window_open();
        if (!impl.window) return impl.engine().quit();

        impl.sem_game = wyt_sem_create(1, 1);
        if (!impl.sem_game) return impl.engine().quit();
        
        wyn_window_resize(impl.window, { impl.win_settings.width, impl.win_settings.height });
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

        impl.win_updates.fetch_add(1, std::memory_order::relaxed);
        impl.win_cond.notify_all();

        if (impl.update_thread)
        {
            wyt_join(impl.update_thread);
        }

        if (impl.render_thread)
        {
            wyt_join(impl.render_thread);
        }

        if (impl.sem_game)
        {
            wyt_sem_destroy(impl.sem_game);
        }

        if (impl.window)
        {
            wyn_window_close(impl.window);
        }
    }

    void wyn_on_signal(void* const userdata)
    {
        EngineImpl& impl{ *static_cast<EngineImpl*>(userdata) };
        
        if (impl.engine().quitting()) return wyn_quit();

        {
            const std::unique_lock<std::mutex> lock(impl.win_mutex);

            const wyn_size_t window_size{ wyn_window_size(impl.window) };
            impl.win_settings.fullscreen = false;
            impl.win_settings.width = window_size.w;
            impl.win_settings.height = window_size.h;
            impl.win_settings.name = "";
            
            impl.win_updates.fetch_add(1, std::memory_order::relaxed);
            impl.win_cond.notify_all();
        }
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

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

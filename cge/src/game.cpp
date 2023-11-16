/**
 * @file render.cpp
 */

#include "cge/cge.hpp"
#include "impl.hpp"

namespace cge
{
    static void render_loop(EngineImpl& impl)
    {
        while (!impl.engine().quitting())
        {
            {
                const SemLock lock{ impl.sem_game };
                ++impl.settings.renders;
                impl.game.render(impl.engine());
            }
            impl.renderer->render(impl);
        }
    }

    extern wyt_retval_t WYT_ENTRY render_main(void* const arg) noexcept
    {
        EngineImpl& impl{ *static_cast<EngineImpl*>(arg) };
        
        try { render_loop(impl); } catch (...) {}

        impl.engine().quit();
        return wyt_retval_t{};
    }
}

namespace cge
{
    static void update_loop(EngineImpl& impl)
    {
        double fps_cache{};
        const wyt_time_t epoch{ impl.epoch };
        wyt_time_t last_tick{ epoch };

        while (!impl.engine().quitting())
        {
            {
                const SemLock lock{ impl.sem_game };
                ++impl.settings.updates;
                impl.game.update(impl.engine());

                fps_cache = impl.settings.fps;
            }

            if (fps_cache > 0)
            {
                const wyt_time_t frame_nanos{ static_cast<wyt_time_t>(1'000'000'000.0 / fps_cache) };
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

    extern wyt_retval_t WYT_ENTRY update_main(void* const arg) noexcept
    {
        EngineImpl& impl{ *static_cast<EngineImpl*>(arg) };
        
        try { update_loop(impl); } catch (...) {}

        impl.engine().quit();
        return wyt_retval_t{};
    }
}

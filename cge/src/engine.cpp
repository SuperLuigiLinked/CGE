/**
 * @file engine.cpp
 */

#include "cge/cge.hpp"
#include "impl.hpp"

namespace cge
{
    void run(Game& game, const InitSettings& settings)
    {
        EngineImpl impl
        {
            .game = game,
            .init = settings,
            .window = {},
            .update_thread = {},
            .render_thread = {},
        };
        wyn_run(static_cast<void*>(&impl));
    }

    void Engine::quit() noexcept
    {
        const bool was_quitting{ impl->quit_flag.test_and_set(std::memory_order::relaxed) };

        if (!was_quitting)
        {
            if (wyn_is_this_thread())
            {
                wyn_quit();
            }
            else if (!wyn_quitting())
            {
                wyn_signal();
            }
        }
    }

    bool Engine::quitting() const noexcept
    {
        return impl->quit_flag.test(std::memory_order::relaxed);
    }

    WindowSettings& Engine::window() noexcept
    {
        return impl->win_settings;
    }

    RenderSettings& Engine::render() noexcept
    {
        return impl->gfx_settings;
    }
    
    double Engine::elapsed_seconds() const noexcept
    {
        return double(wyt_nanotime() - impl->epoch) / 1'000'000'000.0;
    }

}

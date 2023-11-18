/**
 * @file cge/engine.cpp
 */

#include "cge/cge.hpp"
#include "impl.hpp"

namespace cge
{
    void Game::update([[maybe_unused]] Engine engine) {}
    void Game::render([[maybe_unused]] Engine engine) {}
    void Renderer::target_window([[maybe_unused]] EngineImpl& impl, [[maybe_unused]] wyn_window_t window) {}
    void Renderer::render([[maybe_unused]] EngineImpl& impl) {}
}

namespace cge
{
    void run(Game& game, const GameSettings& settings)
    {
        EngineImpl impl{
            .game = game,
            .game_settings = settings,
        };
        impl.renderer.reset(cge::renderer_vk());
        wyn_run(static_cast<void*>(&impl));
    }

    void Engine::quit() noexcept
    {
        const bool was_quitting{ (impl->signal.fetch_or(cge::signal_quit, std::memory_order::relaxed) & cge::signal_quit) != 0 };
        impl->signal.notify_all();

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
        return (impl->signal.load(std::memory_order::relaxed) & cge::signal_quit) != 0;
    }

    GameSettings& Engine::settings() noexcept
    {
        return impl->game_settings;
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

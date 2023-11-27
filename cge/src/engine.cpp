/**
 * @file cge/engine.cpp
 */

#include "engine.hpp"

namespace cge
{
    void Game::update([[maybe_unused]] Engine& engine) {}
    void Game::render([[maybe_unused]] Engine& engine) {}
    void Renderer::target_window([[maybe_unused]] Engine& engine, [[maybe_unused]] wyn_window_t window) {}
    void Renderer::render([[maybe_unused]] Engine& engine) {}
}

namespace cge
{
    extern Viewport viewport(
        const std::uint32_t window_w,
        const std::uint32_t window_h,
        const std::uint32_t render_w,
        const std::uint32_t render_h,
        const Scaling scaling
    ) noexcept
    {
        Viewport view{};

        if (render_w && render_h)
        {
            switch (scaling)
            {
            case cge::Scaling::none:
            {
                view.w = render_w;
                view.h = render_h;
            }
            break;
            case cge::Scaling::fit:
            {
                view.w = window_w;
                view.h = window_h;
            }
            break;
            case cge::Scaling::aspect:
            {
                const float scale_x{ float(window_w) / float(render_w) };
                const float scale_y{ float(window_h) / float(render_h) };
                const float scale_aspect{ std::min(scale_x, scale_y) };

                view.w = std::uint32_t(float(render_w) * float(scale_aspect));
                view.h = std::uint32_t(float(render_h) * float(scale_aspect));
            }
            break;
            case cge::Scaling::aspect_exact:
            {
                const std::uint32_t aspect_factor{ std::gcd(render_w, render_h) };
                const std::uint32_t aspect_w{ render_w / aspect_factor };
                const std::uint32_t aspect_h{ render_h / aspect_factor };
                const float scale_x{ (float(window_w) - float(render_w)) / float(aspect_w) };
                const float scale_y{ (float(window_h) - float(render_h)) / float(aspect_h) };
                const float scale_min{ std::min(scale_x, scale_y) };
                const float scale_exact{ std::floor(scale_min) };

                view.w = std::uint32_t(std::int32_t(render_w) + std::int32_t(aspect_w) * std::int32_t(scale_exact));
                view.h = std::uint32_t(std::int32_t(render_h) + std::int32_t(aspect_h) * std::int32_t(scale_exact));

                view.w = std::max(view.w, aspect_w);
                view.h = std::max(view.h, aspect_h);
            }
            break;
            case cge::Scaling::exact:
            {
                const std::uint32_t scale_x{ window_w / render_w };
                const std::uint32_t scale_y{ window_h / render_h };
                const std::uint32_t scale_min{ std::min(scale_x, scale_y) };
                const std::uint32_t scale_nearest{ std::max(1u, scale_min) };

                view.w = render_w * scale_nearest;
                view.h = render_h * scale_nearest;
            }
            break;
            }
        }
        
        view.w = std::max(view.w, 1u);
        view.h = std::max(view.h, 1u);
        view.x = std::int32_t(window_w - view.w) / 2;
        view.y = std::int32_t(window_h - view.h) / 2;

        return view;
    }
}

namespace cge
{
    void run(cge::Game& game, const cge::Settings& settings)
    {
        cge::Engine engine{
            .game = game,
            .settings = settings,
        };
        engine.renderer.reset(cge::renderer_vk());
        wyn_run(static_cast<void*>(&engine));
    }

    void quit(cge::Engine& engine) noexcept
    {
        const bool was_quitting{ (engine.signal.fetch_or(cge::signal_quit, std::memory_order::relaxed) & cge::signal_quit) != 0 };
        engine.signal.notify_all();

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

    bool quitting(const cge::Engine& engine) noexcept
    {
        return (engine.signal.load(std::memory_order::relaxed) & cge::signal_quit) != 0;
    }

    cge::Settings& settings(cge::Engine& engine) noexcept
    {
        return engine.settings;
    }

    cge::Scene& scene(cge::Engine& engine) noexcept
    {
        return engine.scene;
    }
    
    double elapsed_seconds(const cge::Engine& engine) noexcept
    {
        return double(wyt_nanotime() - engine.epoch) / 1'000'000'000.0;
    }
}

extern "C"
{
    void wyn_on_start(void* const userdata)
    {
        cge::Engine& engine{ *static_cast<cge::Engine*>(userdata) };
        
        engine.window = wyn_window_open();
        if (!engine.window) return cge::quit(engine);

        engine.scene.res_w = static_cast<std::uint32_t>(engine.settings.width);
        engine.scene.res_h = static_cast<std::uint32_t>(engine.settings.height);

        const double scale{ wyn_window_scale(engine.window) };
        wyn_window_resize(engine.window, { engine.settings.width * scale, engine.settings.height * scale });
        engine.renderer->target_window(engine, engine.window);

        wyn_window_show(engine.window);

        engine.epoch = wyt_nanotime();

        engine.render_thread = wyt_spawn(cge::render_main, userdata);
        if (!engine.render_thread) return cge::quit(engine);

        engine.update_thread = wyt_spawn(cge::update_main, userdata);
        if (!engine.update_thread) return cge::quit(engine);
    }

    void wyn_on_stop(void* const userdata)
    {
        cge::Engine& engine{ *static_cast<cge::Engine*>(userdata) };

        engine.signal.store(cge::signal_quit, std::memory_order::relaxed);
        engine.signal.notify_all();

        if (engine.update_thread)
        {
            wyt_join(engine.update_thread);
        }

        if (engine.render_thread)
        {
            wyt_join(engine.render_thread);
        }

        if (engine.window)
        {
            wyn_window_close(engine.window);
        }
    }

    void wyn_on_signal(void* const userdata)
    {
        cge::Engine& engine{ *static_cast<cge::Engine*>(userdata) };

        const cge::Signal signal{ engine.signal.load(std::memory_order::acquire) };
        if (!signal) return;

        if (signal & cge::signal_quit)
        {
            wyn_quit();
            return;
        }

        if (signal & cge::signal_render)
        {
            engine.game.render(engine);
            engine.cached_vsync = engine.settings.vsync;
        }

        if (signal & cge::signal_update)
        {
            engine.game.update(engine);
            engine.cached_fps = engine.settings.fps;
        }

        {
           // wyn_window_resize(engine.window, { engine.settings.width, engine.settings.height });
        }

        (void)engine.signal.fetch_and(~signal, std::memory_order::release);
        engine.signal.notify_all();
    }

    void wyn_on_window_close(void* const userdata, wyn_window_t const window)
    {
        cge::Engine& engine{ *static_cast<cge::Engine*>(userdata) };

        if (window == engine.window)
        {
            cge::quit(engine);
        }
    }
}

namespace cge
{
    static bool await_signal(Engine& engine, const Signal signal) noexcept
    {
        Signal cached = signal | engine.signal.fetch_or(signal, std::memory_order::relaxed);
        if (cached & cge::signal_quit) return false;

        wyn_signal();

        while (cached & signal)
        {
            engine.signal.wait(cached);
            cached = engine.signal.load(std::memory_order_acquire);
            if (cached & cge::signal_quit) return false;
        }

        return true;
    }

    extern wyt_retval_t WYT_ENTRY render_main(void* const arg) noexcept
    {
        Engine& engine{ *static_cast<Engine*>(arg) };
        
        for(;;)
        {
            if (!await_signal(engine, cge::signal_render)) return {};

            engine.renderer->render(engine);
        }
    }

    extern wyt_retval_t WYT_ENTRY update_main(void* const arg) noexcept
    {
        Engine& engine{ *static_cast<Engine*>(arg) };
        
        const wyt_time_t epoch{ engine.epoch };
        wyt_time_t last_tick{ epoch };

        for (;;)
        {
           if (!await_signal(engine, cge::signal_update)) return {};

            const double fps{ engine.cached_fps };

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

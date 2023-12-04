/**
 * @file cge/engine.cpp
 */

#include "engine.hpp"

namespace cge
{
    void Game::event([[maybe_unused]] cge::Engine& engine, [[maybe_unused]] cge::Event event) {}
    void Game::update([[maybe_unused]] cge::Engine& engine) {}
    void Game::render([[maybe_unused]] cge::Engine& engine, [[maybe_unused]] cge::Scene& scene) {}
    
    void Renderer::target_window([[maybe_unused]] cge::Engine& engine, [[maybe_unused]] wyn_window_t window) {}
    void Renderer::render([[maybe_unused]] cge::Engine& engine) {}
}

namespace cge
{
    extern cge::Viewport viewport(
        const cge::uint window_w,
        const cge::uint window_h,
        const cge::uint render_w,
        const cge::uint render_h,
        const cge::Scaling scaling
    ) noexcept
    {
        cge::Viewport view{};

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
                const float scale{ std::min(scale_x, scale_y) };

                view.w = cge::uint(float(render_w) * float(scale));
                view.h = cge::uint(float(render_h) * float(scale));
            }
            break;
            case cge::Scaling::aspect_exact:
            {
                const cge::uint aspect_scale{ std::gcd(render_w, render_h) };
                const cge::uint aspect_w{ render_w / aspect_scale };
                const cge::uint aspect_h{ render_h / aspect_scale };

                const cge::uint scale_x{ window_w / aspect_w };
                const cge::uint scale_y{ window_h / aspect_h };
                const cge::uint scale_min{ std::min(scale_x, scale_y) };
                const cge::uint scale{ std::max(1u, scale_min) };

                view.w = aspect_w * scale;
                view.h = aspect_h * scale;

            }
            break;
            case cge::Scaling::exact:
            {
                const cge::uint scale_x{ window_w / render_w };
                const cge::uint scale_y{ window_h / render_h };
                const cge::uint scale_min{ std::min(scale_x, scale_y) };
                const cge::uint scale{ std::max(1u, scale_min) };

                view.w = render_w * scale;
                view.h = render_h * scale;
            }
            break;
            }
        }
        
        view.w = std::max(view.w, 1u);
        view.h = std::max(view.h, 1u);
        view.x = cge::sint(window_w - view.w) / 2;
        view.y = cge::sint(window_h - view.h) / 2;

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

        engine.scene.res_w = static_cast<cge::uint>(engine.settings.width);
        engine.scene.res_h = static_cast<cge::uint>(engine.settings.height);

        const double scale{ wyn_window_scale(engine.window) };
        wyn_window_resize(engine.window, { engine.settings.width * scale, engine.settings.height * scale });
        engine.renderer->target_window(engine, engine.window);

        wyn_window_retitle(engine.window, reinterpret_cast<const wyn_utf8_t*>(engine.settings.name));
        wyn_window_show(engine.window);

        engine.epoch = wyt_nanotime();

        engine.game.event(engine, cge::EventInit{});

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
            engine.game.render(engine, engine.scene);
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
        if (window != engine.window) return;

        cge::quit(engine);
    }

    void wyn_on_window_reposition(void* const userdata, wyn_window_t const window, wyn_rect_t const content, wyn_coord_t const scale)
    {
        cge::Engine& engine{ *static_cast<cge::Engine*>(userdata) };
        if (window != engine.window) return;
        
        (void)scale;
        const cge::Event event{ cge::EventReposition { content.origin.x, content.origin.y, content.extent.w, content.extent.h } };
        engine.game.event(engine, event);
    }

    void wyn_on_cursor(void*const  userdata, wyn_window_t const window, wyn_coord_t const sx, wyn_coord_t const sy)
    {
        cge::Engine& engine{ *static_cast<cge::Engine*>(userdata) };
        if (window != engine.window) return;

        const wyn_rect_t content{ wyn_window_position(engine.window) };
    #if defined(WYN_COCOA)
        const wyn_coord_t vx{ sx };
        const wyn_coord_t vy{ content.extent.h - sy };
    #else
        const wyn_coord_t vx{ sx };
        const wyn_coord_t vy{ sy };
    #endif

        const cge::Viewport view{
            cge::viewport(
                cge::uint(content.extent.w), cge::uint(content.extent.h),
                cge::uint(engine.scene.res_w), cge::uint(engine.scene.res_h),
                engine.scene.scaling
            )
        };

        const wyn_coord_t rel_x{ wyn_coord_t(vx) - wyn_coord_t(view.x) };
        const wyn_coord_t rel_y{ wyn_coord_t(vy) - wyn_coord_t(view.y) };
        const wyn_coord_t nrm_x{ (rel_x / wyn_coord_t(view.w)) * 2 - 1 };
        const wyn_coord_t nrm_y{ (rel_y / wyn_coord_t(view.h)) * 2 - 1 };

        const cge::Event event{ cge::EventCursor { nrm_x, nrm_y } };
        engine.game.event(engine, event);
    }
    
    void wyn_on_scroll(void* const userdata, wyn_window_t const window, wyn_coord_t const dx, wyn_coord_t const dy)
    {
        cge::Engine& engine{ *static_cast<cge::Engine*>(userdata) };
        if (window != engine.window) return;

        const cge::Event event{ cge::EventScroll { dx, dy } };
        engine.game.event(engine, event);
    }
    
    void wyn_on_mouse(void* const userdata, wyn_window_t const window, wyn_button_t const button, wyn_bool_t const pressed)
    {
        cge::Engine& engine{ *static_cast<cge::Engine*>(userdata) };
        if (window != engine.window) return;

        const cge::Event event{ cge::EventMouse { button, pressed } };
        engine.game.event(engine, event);
    }
    
    void wyn_on_keyboard(void* const userdata, wyn_window_t const window, wyn_keycode_t const keycode, wyn_bool_t const pressed)
    {
        cge::Engine& engine{ *static_cast<cge::Engine*>(userdata) };
        if (window != engine.window) return;

        const cge::Event event{ cge::EventKeyboard { keycode, pressed } };
        engine.game.event(engine, event);
    }
    
    void wyn_on_text(void* const userdata, wyn_window_t const window, const wyn_utf8_t* const text)
    {
        cge::Engine& engine{ *static_cast<cge::Engine*>(userdata) };
        if (window != engine.window) return;

        const cge::Event event{ cge::EventText { text } };
        engine.game.event(engine, event);
    }
}

namespace cge
{
    static bool await_signal(cge::Engine& engine, const cge::Signal signal) noexcept
    {
        cge::Signal cached = signal | engine.signal.fetch_or(signal, std::memory_order::relaxed);
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
        cge::Engine& engine{ *static_cast<cge::Engine*>(arg) };
        
        for(;;)
        {
            if (!cge::await_signal(engine, cge::signal_render)) return {};

            engine.renderer->render(engine);
        }
    }

    extern wyt_retval_t WYT_ENTRY update_main(void* const arg) noexcept
    {
        cge::Engine& engine{ *static_cast<cge::Engine*>(arg) };
        
        const wyt_time_t epoch{ engine.epoch };
        wyt_time_t last_tick{ epoch };

        for (;;)
        {
           if (!cge::await_signal(engine, cge::signal_update)) return {};

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

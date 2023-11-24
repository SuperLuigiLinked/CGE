/**
 * @file cge/engine.hpp
 */

#pragma once

#include <atomic>
#include <memory>

#include <cstdlib>
#if defined(__WIN32) && !defined(NDEBUG)
    #define CGE_ASSERT(expr) if (expr) {} else (__debugbreak(), std::abort())
#else
    #define CGE_ASSERT(expr) if (expr) {} else std::abort()
#endif

#if defined(CGE_DEBUG)
    #include <cstdio>
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Weverything"
    #include <fmt/core.h>
    #pragma clang diagnostic pop
    #define CGE_LOG(...) fmt::print(stderr, __VA_ARGS__)
#else
    #define CGE_LOG(...) ((void)0)
#endif

#include <cge.hpp>
#include <wyn.h>
#include <wyt.h>

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
        virtual void target_window(Engine& engine, wyn_window_t window);
        virtual void render(Engine& engine);
    };

    extern Renderer* renderer_vk();
}

namespace cge
{
    using Signal = unsigned char;

    enum SignalBits : Signal
    {
        signal_quit   = 0b1,
        signal_render = 0b10,
        signal_update = 0b100,
    };
}

namespace cge
{
    class Engine final
    {
    public:

        Game& game;
        
        Settings settings;
        Scene scene;
        
        wyt_time_t epoch;
        wyn_window_t window;
        wyt_thread_t update_thread;
        wyt_thread_t render_thread;

        std::unique_ptr<Renderer> renderer;
        double cached_fps;
        bool cached_vsync;
        
        std::atomic<Signal> signal;
    
    };
}

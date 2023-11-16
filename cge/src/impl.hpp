/**
 * @file impl.hpp
 */

#pragma once

#include "cge/cge.hpp"
#include "renderer.hpp"

#include <wyn.h>
#include <wyt.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <condition_variable>

namespace cge
{
    class EngineImpl final
    {
    public:

        inline constexpr Engine engine() noexcept { return Engine(this); }

    public:

        Game& game;
        wyt_sem_t sem_game;
        
        Settings settings;
        std::mutex win_mutex;
        std::condition_variable win_cond;
        std::atomic<std::uint64_t> win_updates;
        WindowSettings win_settings;
        RenderSettings gfx_settings;

        wyn_window_t window;
        wyt_thread_t update_thread;
        wyt_thread_t render_thread;

        wyt_time_t epoch;

        std::unique_ptr<Renderer> renderer;

        std::atomic_flag quit_flag;
    
    };

    struct SemLock final
    {
        const wyt_sem_t& m_sem;

        inline SemLock(const wyt_sem_t& sem) noexcept : m_sem{ sem } { wyt_sem_acquire(m_sem); }
        inline ~SemLock() noexcept { (void)wyt_sem_release(m_sem); }
    };

    extern wyt_retval_t WYT_ENTRY update_main(void* arg) noexcept;
    extern wyt_retval_t WYT_ENTRY render_main(void* arg) noexcept;
}


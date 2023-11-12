/**
 * @file impl.hpp
 */

#pragma once

#include "cge/cge.hpp"
#include <wyn.h>
#include <wyt.h>

#include <atomic>

namespace cge
{
    class EngineImpl final
    {
    public:

        inline constexpr Engine engine() noexcept { return Engine(this); }

    public:
        Game& game;
        InitSettings init;
        WindowSettings win_settings;
        RenderSettings gfx_settings;
        
        wyt_time_t epoch;

        wyn_window_t window;
        wyt_thread_t update_thread;
        wyt_thread_t render_thread;

        wyt_sem_t sem_game;

        std::atomic_flag quit_flag;
    };

    struct SemLock final
    {
        wyt_sem_t& m_sem;

        inline SemLock(wyt_sem_t& sem) noexcept : m_sem{ sem } { wyt_sem_acquire(m_sem); }
        inline ~SemLock() noexcept { (void)wyt_sem_release(m_sem); }
    };

    extern wyt_retval_t WYT_ENTRY update_main(void* arg) noexcept;
    extern wyt_retval_t WYT_ENTRY render_main(void* arg) noexcept;
}


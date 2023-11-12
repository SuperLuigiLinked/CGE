/**
 * @file update.cpp
 */

#include "cge/cge.hpp"
#include "impl.hpp"

namespace cge
{
    static void update_loop(EngineImpl& impl)
    {
        const wyt_duration_t frame_nanos{ static_cast<wyt_duration_t>(1'000'000'000.0 / impl.init.fps) };

        while (!impl.engine().quitting())
        {
            wyt_nanosleep_for(frame_nanos);
            {
                const SemLock lock{ impl.sem_game };
                impl.game.update(impl.engine());
            }
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

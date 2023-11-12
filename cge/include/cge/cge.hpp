/**
 * @file cge/cge.hpp
 * @brief Simple C++ Game Engine.
 */

#pragma once

#ifndef CGE_H
#define CGE_H

#include "cge/types.hpp"
#include "cge/vec.hpp"
#include "cge/settings.hpp"
#include "cge/engine.hpp"

namespace cge
{
    /**
     * @brief Runs the game loop until the game terminates.
     * @warning MUST be called on the Main Thread.
     *          MUST NOT be called while the Engine is already running.
     */
    void run(Game& game, const InitSettings& settings);
}

#endif

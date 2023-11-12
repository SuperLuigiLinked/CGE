/**
 * @file cge/engine.hpp
 */

#pragma once

#ifndef CGE_ENGINE_H
#define CGE_ENGINE_H

namespace cge
{
    class Engine final
    {
    private:

        class EngineImpl* impl;
        friend class EngineImpl;

        explicit inline constexpr Engine(EngineImpl* ptr) noexcept : impl{ ptr } {}
        
    public:

        void quit() noexcept;
        bool quitting() const noexcept;

        double elapsed_seconds() const noexcept;

    };

    class Game
    {
    public:
        virtual ~Game() = default;

        virtual void update(Engine engine) = 0;
        virtual void render(Engine engine) = 0;
    };
}

#endif

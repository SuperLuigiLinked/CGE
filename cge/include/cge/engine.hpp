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

        Engine(const Engine&) = delete;
        Engine(Engine&&) = delete;
        Engine& operator=(const Engine&) = delete;
        Engine& operator=(Engine&&) = delete;

    public:

    };

    class Game
    {
    public:
        virtual ~Game() = default;

        virtual void update(Engine& engine) = 0;
        virtual void render(Engine& engine) = 0;
    };
}

#endif

/**
 * @file examples/demo1/main.cpp
 */

#include <cge/cge.hpp>

// ================================================================================================================================

#include <print>
#define LOG(...) std::print(stderr, __VA_ARGS__)

// ================================================================================================================================

class Example final : public cge::Game
{
public:

    std::uint64_t updates;
    std::uint64_t renders;

public:

    void update(cge::Engine engine) final
    {
        const double secs{ engine.elapsed_seconds() };
        ++this->updates;
        LOG("[UPDATE] [{:.3f}] {}\n", secs, this->updates);
        
        (void)engine;
    }

    void render(cge::Engine engine) final
    {
        const double secs{ engine.elapsed_seconds() };
        ++this->renders;
        LOG("[RENDER] [{:.3f}] {}\n", secs, this->renders);

        (void)engine;
    }
};

// ================================================================================================================================

int main()
{
    Example example{};

    const cge::InitSettings settings
    {
        .width = 1280.0,
        .height = 720.0,
        .fps = 60.0,
        .vsync = true,
        .fullscreen = false,
    };

    cge::run(example, settings);

    return 0;
}

// ================================================================================================================================


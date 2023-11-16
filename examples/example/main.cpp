/**
 * @file examples/demo1/main.cpp
 */

#include <cge/cge.hpp>
#include "../utils.hpp"

// ================================================================================================================================

class App final : public cge::Game
{
public:

    std::uint64_t updates;
    std::uint64_t renders;

public:

    void update(cge::Engine engine) final;
    void render(cge::Engine engine) final;

};

// --------------------------------------------------------------------------------------------------------------------------------

void App::update(cge::Engine engine)
{
    [[maybe_unused]] const double secs{ engine.elapsed_seconds() };
    ++this->updates;
    //LOG("[UPDATE] [{:.3f}] {}\n", secs, this->updates);
    
    (void)engine;
}

// --------------------------------------------------------------------------------------------------------------------------------

void App::render(cge::Engine engine)
{
    [[maybe_unused]] const double secs{ engine.elapsed_seconds() };
    ++this->renders;
    LOG("[RENDER] [{:.3f}] {}\n", secs, this->renders);

    //cge::WindowSettings& window{ engine.window() };
    cge::RenderSettings& render{ engine.render() };

    render.clear();

    render.backcolor = cge::RGBA::splat(
        1.0f - float(std::abs(((std::int64_t(this->updates) / 2) % 510) - 255)) / 255.0f,
        1.0f - float(std::abs(((std::int64_t(this->updates)    ) % 510) - 255)) / 255.0f,
        1.0f - float(std::abs(((std::int64_t(this->updates) * 2) % 510) - 255)) / 255.0f
    ); 

    //render.backcolor = cge::RGBA::splat(0.0f, 0.0f, 0.25f);

    render.triangle(
        std::array
        {
            cge::Vertex {
                .xyzw = { -1.0f, -1.0f },
                .st = { 0xFFFF0000 },
            },
            cge::Vertex {
                .xyzw = { 0.5f, 0.0f },
                .st = { 0x00FF0000 },
            },
            cge::Vertex {
                .xyzw = { -1.0f, 1.0f },
                .st = { 0xFFFF0000 },
            }
        }
    );

    render.triangle(
        std::array
        {
            cge::Vertex {
                .xyzw = { 1.0f, -1.0f },
                .st = { 0xFF00FF00 },
            },
            cge::Vertex {
                .xyzw = { -0.5f, 0.0f },
                .st = { 0x0000FF00 },
            },
            cge::Vertex {
                .xyzw = { 1.0f, 1.0f },
                .st = { 0xFF00FF00 },
            }
        }
    );
}

// ================================================================================================================================

int main()
{
    App app{};

    const cge::InitSettings settings
    {
        .width = 1280.0,
        .height = 720.0,
        .fps = 60.0,
        .vsync = true,
        .fullscreen = false,
    };

    cge::run(app, settings);

    return 0;
}

// ================================================================================================================================


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

void App::update(cge::Engine engine)
{
    const double secs{ engine.elapsed_seconds() };
    ++this->updates;
    LOG("[UPDATE] [{:.3f}] {}\n", secs, this->updates);
    
    (void)engine;
}

void App::render(cge::Engine engine)
{
    const double secs{ engine.elapsed_seconds() };
    ++this->renders;
    LOG("[RENDER] [{:.3f}] {}\n", secs, this->renders);

    //cge::WindowSettings& window{ engine.window() };
    cge::RenderSettings& render{ engine.render() };

    render.clear();

    render.backcolor = cge::rgba(
        float(this->updates / 2 * 1 % 256) / 256.0f,
        float(this->updates / 2 * 2 % 256) / 256.0f,
        float(this->updates / 2 * 4 % 256) / 256.0f
    );
    // render.backcolor = cge::rgba(0.0f, 0.0f, 0.1f);

    render.triangle(
        std::array
        {
            cge::Vertex {
                .xyzw = cge::vec4{ 1.0f, -1.0f },
                .rgba = cge::rgba(0.0f, 1.0f, 0.0f),
            },
            cge::Vertex {
                .xyzw = cge::vec4{ -0.5f, 0.0f },
                .rgba = cge::rgba(0.0f, 1.0f, 0.0f),
            },
            cge::Vertex {
                .xyzw = cge::vec4{1.0f, 1.0f},
                .rgba = cge::rgba(0.0f, 1.0f, 0.0f),
            }
        }
    );

    render.triangle(
        std::array
        {
            cge::Vertex {
                .xyzw = cge::vec4{ -1.0f, -1.0f },
                .rgba = cge::rgba(1.0f, 0.0f, 0.0f, 1.0f),
            },
            cge::Vertex {
                .xyzw = cge::vec4{ 0.5f, 0.0f },
                .rgba = cge::rgba(1.0f, 0.0f, 0.0f, 0.0f),
            },
            cge::Vertex {
                .xyzw = cge::vec4{ -1.0f, 1.0f },
                .rgba = cge::rgba(1.0f, 0.0f, 0.0f, 1.0f),
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


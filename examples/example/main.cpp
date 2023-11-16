/**
 * @file examples/example/main.cpp
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

void App::update(cge::Engine engine)
{
    [[maybe_unused]] const double secs{ engine.elapsed_seconds() };
    cge::Settings& settings{ engine.settings() };

    LOG(fmt::fg(fmt::color::orange), "[UPDATE] [{:.3f}] <{:.2f}> {}\n", secs, secs * settings.fps, this->updates);
    ++this->updates;
    
    (void)engine;
}

// ================================================================================================================================

void App::render(cge::Engine engine)
{
    [[maybe_unused]] const double secs{ engine.elapsed_seconds() };
    cge::Settings& settings{ engine.settings() };
    // cge::WindowSettings& window{ engine.window() };
    cge::RenderSettings& render{ engine.render() };

    LOG(fmt::fg(fmt::color::purple), "[RENDER] [{:.3f}] <{:.2f}> {}\n", secs, secs * settings.fps, this->renders);
    ++this->renders;

    render.clear();

    render.backcolor = cge::RGBA::splat(
        1.0f - float(std::abs(((std::int64_t(this->updates) / 2) % 510) - 255)) / 255.0f,
        1.0f - float(std::abs(((std::int64_t(this->updates)    ) % 510) - 255)) / 255.0f,
        1.0f - float(std::abs(((std::int64_t(this->updates) * 2) % 510) - 255)) / 255.0f
    ); 

    //render.backcolor = cge::RGBA::splat(0.0f, 0.0f, 0.25f);

    constexpr double pi{ std::numbers::pi };
    constexpr double tau{ pi + pi };

    constexpr double rad{ 0.75 };
    render.triangle(
        std::array
        {
            cge::Vertex {
                .xyzw = { std::cos((secs + 0.0 / 3.0) * tau) * rad, std::sin((secs + 0.0 / 3.0) * tau) * rad },
                .st = { 0xFFFF0000 },
            },
            cge::Vertex {
                .xyzw = { std::cos((secs + 1.0 / 3.0) * tau) * rad, std::sin((secs + 1.0 / 3.0) * tau) * rad },
                .st = { 0xFF00FF00 },
            },
            cge::Vertex {
                .xyzw = { std::cos((secs + 2.0 / 3.0) * tau) * rad, std::sin((secs + 2.0 / 3.0) * tau) * rad },
                .st = { 0xFF0000FF },
            }
        }
    );

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


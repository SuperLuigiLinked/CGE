/**
 * @file examples/example/main.cpp
 */

#include <cge.hpp>
#include "../utils.hpp"

// ================================================================================================================================

class App final : public cge::Game
{
public:

    std::uint64_t updates;
    std::uint64_t renders;

public:

    void update(cge::Engine& engine) final;
    void render(cge::Engine& engine) final;

};

// ================================================================================================================================

int main()
{
    App app{};

    const cge::GameSettings settings
    {
        .name = "CGE - Example",
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

void App::update([[maybe_unused]] cge::Engine& engine)
{
    [[maybe_unused]] const double secs{ cge::elapsed_seconds(engine) };
    [[maybe_unused]] cge::GameSettings& settings{ cge::settings(engine) };

    LOG(fmt::fg(fmt::color::orange), "[UPDATE] [{:.3f}] <{:.2f}> {}\n", secs, secs * settings.fps, this->updates);
    ++this->updates;
}

// ================================================================================================================================

void App::render([[maybe_unused]] cge::Engine& engine)
{
    [[maybe_unused]] const double secs{ cge::elapsed_seconds(engine) };
    [[maybe_unused]] cge::GameSettings& settings{ cge::settings(engine) };
    [[maybe_unused]] cge::RenderSettings& render{ cge::renderer(engine) };

    LOG(fmt::fg(fmt::color::purple), "[RENDER] [{:.3f}] <{:.2f}> {}\n", secs, secs * settings.fps, this->renders);
    ++this->renders;

    render.clear();

    render.backcolor(
        cge::rgba(
            255 - std::abs(255 - int((this->updates / 2) % 510)),
            255 - std::abs(255 - int((this->updates    ) % 510)),
            255 - std::abs(255 - int((this->updates * 2) % 510)),
            255
        )
    ); 
    //render.backcolor(0xFF000040);

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


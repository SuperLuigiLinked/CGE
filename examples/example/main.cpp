/**
 * @file examples/example/main.cpp
 */

#include <cge.hpp>
#include "../utils.hpp"

// ================================================================================================================================

class App final : public cge::Game
{
public:

    std::uint64_t events;
    std::uint64_t updates;
    std::uint64_t renders;

    bool cursor_focus;
    double cursor_rot;
    cge::dvec2 cursor_pos;

public:

    void event(cge::Engine& engine, cge::Event event) final;
    void update(cge::Engine& engine) final;
    void render(cge::Engine& engine, cge::Scene& scene) final;

};

// ================================================================================================================================

int main()
{
    App app{};

    const cge::Settings settings
    {
        .name = "CGE - Example",
        .width  = 1280.0,
        .height =  720.0,
        .fps = 60.0,
        .vsync = true,
        .fullscreen = false,
    };

    cge::run(app, settings);

    return 0;
}

// ================================================================================================================================

void App::event([[maybe_unused]] cge::Engine& engine, const cge::Event event)
{
    [[maybe_unused]] const double secs{ cge::elapsed_seconds(engine) };
    [[maybe_unused]] cge::Settings& settings{ cge::settings(engine) };
    LOG(fmt::fg(fmt::color::lime_green), "[EVENT]  [{:.3f}] <{:.2f}> {}\n", secs, secs * settings.fps, this->events);
    ++this->events;

    if (const auto* const evt = std::get_if<cge::EventReposition>(&event))
    {

    }
    else if (const auto* const evt = std::get_if<cge::EventCursor>(&event))
    {
        this->cursor_focus = true;
        this->cursor_pos = { evt->x, evt->y };
    }
    else if (const auto* const evt = std::get_if<cge::EventScroll>(&event))
    {
        this->cursor_rot -= evt->y / 16.0;
    }
    else if (const auto* const evt = std::get_if<cge::EventMouse>(&event))
    {

    }
    else if (const auto* const evt = std::get_if<cge::EventKeyboard>(&event))
    {

    }
    else if (const auto* const evt = std::get_if<cge::EventText>(&event))
    {

    }
}

// ================================================================================================================================

void App::update([[maybe_unused]] cge::Engine& engine)
{
    [[maybe_unused]] const double secs{ cge::elapsed_seconds(engine) };
    [[maybe_unused]] cge::Settings& settings{ cge::settings(engine) };
    LOG(fmt::fg(fmt::color::orange), "[UPDATE] [{:.3f}] <{:.2f}> {}\n", secs, secs * settings.fps, this->updates);
    ++this->updates;
}

// ================================================================================================================================

void App::render([[maybe_unused]] cge::Engine& engine, cge::Scene& scene)
{
    [[maybe_unused]] const double secs{ cge::elapsed_seconds(engine) };
    [[maybe_unused]] cge::Settings& settings{ cge::settings(engine) };
    LOG(fmt::fg(fmt::color::purple), "[RENDER] [{:.3f}] <{:.2f}> {}\n", secs, secs * settings.fps, this->renders);
    ++this->renders;
    
    scene.clear();
    
    scene.res_w = 1280;
    scene.res_h =  720;
    scene.scaling = cge::Scaling::aspect;
    scene.backcolor = 0xFF000000;

    const cge::Color backcolor = cge::rgba(
        255 - std::abs(255 - int((this->updates / 2) % 510)),
        255 - std::abs(255 - int((this->updates    ) % 510)),
        255 - std::abs(255 - int((this->updates * 2) % 510)),
        255
    );
    scene.draw_strip(
        std::array
        {
            cge::Vertex{ .xyzw = {  1.0f, -1.0f, 1.0f }, .st = { backcolor } },
            cge::Vertex{ .xyzw = { -1.0f, -1.0f, 1.0f }, .st = { backcolor } },
            cge::Vertex{ .xyzw = {  1.0f,  1.0f, 1.0f }, .st = { backcolor } },
            cge::Vertex{ .xyzw = { -1.0f,  1.0f, 1.0f }, .st = { backcolor } },
        }
    );

    [[maybe_unused]] constexpr float pi{ std::numbers::pi_v<float> };
    [[maybe_unused]] constexpr float tau{ pi + pi };
    [[maybe_unused]] const float aspect{ float(scene.res_w) / float(scene.res_h) };

    {
        [[maybe_unused]] constexpr float rad{ 0.4375f };
        [[maybe_unused]] const float rot{ float(secs) };
        [[maybe_unused]] const float cx{ 0.0f };
        [[maybe_unused]] const float cy{ 0.0f };

        scene.draw_tri(
            std::array
            {
                cge::Vertex {
                    .xyzw = { cx + std::cos((rot - 0.0f / 3.0f) * tau) * rad, cy + std::sin((rot - 0.0f / 3.0f) * tau) * rad * aspect },
                    .st = { 0xFFFF0000 },
                },
                cge::Vertex {
                    .xyzw = { cx + std::cos((rot - 1.0f / 3.0f) * tau) * rad, cy + std::sin((rot - 1.0f / 3.0f) * tau) * rad * aspect },
                    .st = { 0xFF00FF00 },
                },
                cge::Vertex {
                    .xyzw = { cx + std::cos((rot - 2.0f / 3.0f) * tau) * rad, cy + std::sin((rot - 2.0f / 3.0f) * tau) * rad * aspect },
                    .st = { 0xFF0000FF },
                },
            }
        );
    }    

    scene.draw_strip(
        std::array
        {
            cge::Vertex { .xyzw = { -0.50f,  0.75f }, .st = { 0xFFFFFF00 }, },
            cge::Vertex { .xyzw = { -0.50f, -0.75f }, .st = { 0xFF00FF00 }, },
            cge::Vertex { .xyzw = { -1.00f,  0.75f }, .st = { 0xFF0000FF }, },
            cge::Vertex { .xyzw = { -1.00f, -0.75f }, .st = { 0xFFFF0000 }, },
        }
    );

    scene.draw_fan(
        std::array
        {
            cge::Vertex { .xyzw = {  0.75f,  0.00f }, .st = { 0xFF7F7F7F }, },
            cge::Vertex { .xyzw = {  1.00f,  0.00f }, .st = { 0xFFFF0000 }, },
            cge::Vertex { .xyzw = {  0.75f, -0.75f }, .st = { 0xFFFFFF00 }, },
            cge::Vertex { .xyzw = {  0.50f,  0.00f }, .st = { 0xFF0000FF }, },
            cge::Vertex { .xyzw = {  0.75f,  0.75f }, .st = { 0xFF00FF00 }, },
            cge::Vertex { .xyzw = {  1.00f,  0.00f }, .st = { 0xFFFF0000 }, },
        }
    );

    if (this->cursor_focus)
    {
        [[maybe_unused]] constexpr float rad{ 1.0f / 32.0f };
        [[maybe_unused]] const float rot{ float(this->cursor_rot) };
        [[maybe_unused]] const float cx{ float(this->cursor_pos.x) };
        [[maybe_unused]] const float cy{ float(this->cursor_pos.y) };

        scene.draw_tri(
            std::array
            {
                cge::Vertex {
                    .xyzw = { cx + std::cos((rot - 0.0f / 3.0f) * tau) * rad, cy + std::sin((rot - 0.0f / 3.0f) * tau) * rad * aspect },
                    .st = { 0xFF000000 },
                },
                cge::Vertex {
                    .xyzw = { cx + std::cos((rot - 1.0f / 3.0f) * tau) * rad, cy + std::sin((rot - 1.0f / 3.0f) * tau) * rad * aspect },
                    .st = { 0xFFFFFFFF },
                },
                cge::Vertex {
                    .xyzw = { cx + std::cos((rot - 2.0f / 3.0f) * tau) * rad, cy + std::sin((rot - 2.0f / 3.0f) * tau) * rad * aspect },
                    .st = { 0xFFFFFFFF },
                },
            }
        );
    }
}

// ================================================================================================================================


/**
 * @file examples/example/main.cpp
 */

#include <wyc.h>
#include <cge.hpp>
#include "../utils.hpp"

// ================================================================================================================================

class App final : public cge::Game
{
public:

    std::uint64_t events;
    std::uint64_t updates;
    std::uint64_t renders;

    const wyn_vb_mapping_t* vb_map;
    const wyn_vk_mapping_t* vk_map;

    double window_x;
    double window_y;
    double window_w;
    double window_h;

    bool window_focus;
    bool cursor_focus;
    bool cursor_hover;

    bool mb_L;
    bool mb_R;
    bool mb_M;
    bool kb_Space;

    bool toggle_fs;
    
    double cursor_rot = { 11.0 / 16.0 };
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

    if (const auto* const evt = std::get_if<cge::EventInit>(&event))
    {
        this->vb_map = wyn_vb_mapping();
        this->vk_map = wyn_vk_mapping();
    }
    else if (const auto* const evt = std::get_if<cge::EventFocus>(&event))
    {
        this->window_focus = evt->focused;
    }
    else if (const auto* const evt = std::get_if<cge::EventReposition>(&event))
    {
        this->window_x = evt->x;
        this->window_y = evt->y;
        this->window_w = evt->w;
        this->window_h = evt->h;
    }
    else if (const auto* const evt = std::get_if<cge::EventCursor>(&event))
    {
        this->cursor_hover = (evt->x >= -1.0) && (evt->y >= -1.0) && (evt->x <= 1.0) && (evt->y <= 1.0);
        this->cursor_pos = { evt->x, evt->y };
    }
    else if (const auto* const evt = std::get_if<cge::EventCursorExit>(&event))
    {
        this->cursor_hover = false;
    }
    else if (const auto* const evt = std::get_if<cge::EventScroll>(&event))
    {
        this->cursor_rot -= evt->y / 16.0;
    }
    else if (const auto* const evt = std::get_if<cge::EventMouse>(&event))
    {
        if (evt->button == (*this->vb_map)[wyn_vb_left]) this->mb_L = evt->pressed;
        if (evt->button == (*this->vb_map)[wyn_vb_right]) this->mb_R = evt->pressed;
        if (evt->button == (*this->vb_map)[wyn_vb_middle]) this->mb_M = evt->pressed;
    }
    else if (const auto* const evt = std::get_if<cge::EventKeyboard>(&event))
    {
        if (evt->keycode == (*this->vk_map)[wyn_vk_Space])
        {
            this->kb_Space = evt->pressed;
        }

        if (evt->pressed && (evt->keycode == (*this->vk_map)[wyn_vk_Escape]))
        {
            this->toggle_fs = true;
        }
    }
    else if (const auto* const evt = std::get_if<cge::EventText>(&event))
    {

    }

    this->cursor_focus = this->window_focus && (this->cursor_hover || (this->mb_L || this->mb_R || this->mb_M));
    
    if (!this->window_focus)
    {
        this->mb_L = false;
        this->mb_R = false;
        this->mb_M = false;
        this->kb_Space = false;
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

    if (this->toggle_fs)
    {
        settings.fullscreen = !settings.fullscreen;
        this->toggle_fs = false;
    }
    
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
    [[maybe_unused]] const bool invert{ this->kb_Space };

    {
        constexpr float rad{ 0.4375f };
        const float rot{ float(secs) };
        const float cx{ 0.0f };
        const float cy{ 0.0f };

        scene.draw_tri(
            std::array
            {
                cge::Vertex {
                    .xyzw = { cx + std::cos((rot - 0.0f / 3.0f) * tau) * rad, cy + std::sin((rot - 0.0f / 3.0f) * tau) * rad * aspect },
                    .st = { invert ? 0xFF00FFFF : 0xFFFF0000 },
                },
                cge::Vertex {
                    .xyzw = { cx + std::cos((rot - 1.0f / 3.0f) * tau) * rad, cy + std::sin((rot - 1.0f / 3.0f) * tau) * rad * aspect },
                    .st = { invert ? 0xFFFF00FF : 0xFF00FF00 },
                },
                cge::Vertex {
                    .xyzw = { cx + std::cos((rot - 2.0f / 3.0f) * tau) * rad, cy + std::sin((rot - 2.0f / 3.0f) * tau) * rad * aspect },
                    .st = { invert ? 0xFFFFFF00 : 0xFF0000FF },
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
        constexpr float rad{ 1.0f / 32.0f };
        const float rot{ float(this->cursor_rot) };
        const float cx{ float(this->cursor_pos.x) };
        const float cy{ float(this->cursor_pos.y) };

        const cge::Color mA{ 0xFF000000 };
        const cge::Color mR{ this->mb_L ? 0x00FF0000 : 0 };
        const cge::Color mG{ this->mb_M ? 0x0000FF00 : 0 };
        const cge::Color mB{ this->mb_R ? 0x000000FF : 0 };
        const cge::Color c1{ mA };
        const cge::Color c2{ mA | mR | mG | mB };

        scene.draw_tri(
            std::array
            {
                cge::Vertex {
                    .xyzw = { cx + std::cos((rot - 0.0f / 3.0f) * tau) * rad, cy + std::sin((rot - 0.0f / 3.0f) * tau) * rad * aspect },
                    .st = { c1 },
                },
                cge::Vertex {
                    .xyzw = { cx + std::cos((rot - 1.0f / 3.0f) * tau) * rad, cy + std::sin((rot - 1.0f / 3.0f) * tau) * rad * aspect },
                    .st = { c2 },
                },
                cge::Vertex {
                    .xyzw = { cx + std::cos((rot - 2.0f / 3.0f) * tau) * rad, cy + std::sin((rot - 2.0f / 3.0f) * tau) * rad * aspect },
                    .st = { c2 },
                },
            }
        );
    }

    if (!this->window_focus)
    {
        constexpr cge::Color fade{ 0xA0000000 };

        scene.draw_tri(
            std::array
            {
                cge::Vertex {
                    .xyzw = { -1.0, -1.0 },
                    .st = { fade },
                },
                cge::Vertex {
                    .xyzw = { -1.0, 3.0 },
                    .st = { fade },
                },
                cge::Vertex {
                    .xyzw = { 3.0, -1.0 },
                    .st = { fade },
                },
            }
        );
    }
}

// ================================================================================================================================


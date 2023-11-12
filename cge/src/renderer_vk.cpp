/**
 * @file renderer_vk.cpp
 */

#include <optional>

#include <vulkan/vulkan.h>

#include "cge/cge.hpp"
#include "impl.hpp"
#include "renderer.hpp"
#include "debug.hpp"

namespace cge
{
    class Renderer_VK final : public Renderer 
    {
    private:

        struct Context
        {
            VkInstance instance;
        };

        struct Renderable
        {
            wyn_window_t window;
        };

    private:

        Context context;
        std::optional<Renderable> renderable;

    private:
    
        void reinit_context();
        void deinit_context();

        void create_renderable(wyn_window_t window);
        void destroy_renderable();

    public:

        Renderer_VK();
        ~Renderer_VK() final;

        void render(EngineImpl& impl) final;
    };
}

namespace cge
{
    Renderer_VK::Renderer_VK()
        : context{}, renderable{}
    {
        this->reinit_context();
    }

    Renderer_VK::~Renderer_VK()
    {
        this->destroy_renderable();
        this->deinit_context();
    }

    extern Renderer* renderer_vk()
    {
        return new Renderer_VK();
    }
}

namespace cge
{
    /**
     * @see Vulkan:
     * - https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkApplicationInfo.html
     * - https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkInstanceCreateInfo.html
     * - https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCreateInstance.html
     */
    void Renderer_VK::reinit_context()
    {
        const VkApplicationInfo app_info{
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = {},
            .pApplicationName = {},
            .applicationVersion = {},
            .pEngineName = {},
            .engineVersion = {},
            .apiVersion = VK_VERSION_1_0,
        };
        const VkInstanceCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = {},
            .flags = {},
            .pApplicationInfo = &app_info,
            .enabledLayerCount = {},
            .ppEnabledLayerNames = {},
            .enabledExtensionCount = {},
            .ppEnabledExtensionNames = {},
        };
        vkCreateInstance(&create_info, nullptr, &context.instance);
        ASSERT(context.instance);

        LOG("[VK] SUCCESS!\n");
    }

    void Renderer_VK::deinit_context()
    {

    }
}

namespace cge
{
    void Renderer_VK::create_renderable(wyn_window_t const window)
    {
        if (this->renderable.has_value())
        {
            this->destroy_renderable();
        }
        else
        {
            Renderable& gfx{ *this->renderable };
            gfx.window = window;
        }
    }

    void Renderer_VK::destroy_renderable()
    {
        if (this->renderable.has_value())
        {
            this->renderable.reset();
        }
    }
}

namespace cge
{
    void Renderer_VK::render(EngineImpl& impl)
    {
        (void)impl;
    }
}

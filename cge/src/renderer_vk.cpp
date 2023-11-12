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

        static inline constexpr std::array instance_exts{
            "VK_KHR_portability_enumeration",
            "VK_KHR_surface",
        #if defined(WYN_WIN32)
            "VK_KHR_win32_surface",
        #elif defined(WYN_COCOA)
            "VK_EXT_metal_surface",
        #elif defined(WYN_XLIB)
            "VK_KHR_xlib_surface",
        #elif defined(WYN_XCB)
            "VK_KHR_xcb_surface",
        #endif
        #if !defined(NDEBUG)
            "VK_EXT_debug_utils"
        #endif
        };

        static inline constexpr std::array device_exts{
            "VK_KHR_swapchain"
        };
        
        static inline constexpr std::array instance_layers{
            "VK_LAYER_KHRONOS_validation"
        };
        
        static inline constexpr std::array device_layers{
            "VK_LAYER_KHRONOS_validation"
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
            .apiVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
        };

        const VkInstanceCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = {},
            .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
            .pApplicationInfo = &app_info,
            .enabledLayerCount = uint32_t(instance_layers.size()),
            .ppEnabledLayerNames = instance_layers.data(),
            .enabledExtensionCount = uint32_t(instance_exts.size()),
            .ppEnabledExtensionNames = instance_exts.data(),
        };
        
        const VkResult res{ vkCreateInstance(&create_info, nullptr, &context.instance) };
        ASSERT(res == VK_SUCCESS);
        ASSERT(context.instance);

        LOG("[CGE] VK INSTANCE!\n");
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

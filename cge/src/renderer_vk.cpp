/**
 * @file renderer_vk.cpp
 */

#include <optional>
#include <array>
#include <vector>

#include <vulkan/vulkan.h>

#include "cge/cge.hpp"
#include "impl.hpp"
#include "renderer.hpp"
#include "debug.hpp"

#if !defined(NDEBUG)
    #define CGE_DEBUG_VK
#endif

#define CGE_LOAD_INSTANCE(handle, var, func) var.func = reinterpret_cast<PFN_##func>(vkGetInstanceProcAddr(handle, #func))
//#define CGE_LOAD_DEVICE(handle, var, func) var.func = reinterpret_cast<PFN_##func>(vkGetDeviceProcAddr(handle, #func))

namespace cge
{
    class Renderer_VK final : public Renderer 
    {
    private:

        struct Functions
        {
            PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
        };

        struct Context
        {
            VkInstance instance; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkInstance.html
            uint32_t instance_extension_count;
            std::vector<VkExtensionProperties> instance_extensions; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkExtensionProperties.html
            uint32_t instance_layer_count;
            std::vector<VkLayerProperties> instance_layers; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkLayerProperties.html

        #if defined(CGE_DEBUG_VK)
            VkDebugUtilsMessengerEXT messenger; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDebugUtilsMessengerEXT.html
        #endif

            uint32_t device_count;
            std::vector<VkPhysicalDevice> device_handles; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDevice.html
            std::vector<uint32_t> device_extension_counts;
            std::vector<std::vector<VkExtensionProperties>> device_extensions; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkExtensionProperties.html
            std::vector<uint32_t> device_layer_counts;
            std::vector<std::vector<VkLayerProperties>> device_layers; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkLayerProperties.html
            std::vector<VkPhysicalDeviceProperties> device_properties; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceProperties
            std::vector<VkPhysicalDeviceFeatures> device_features; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceFeatures.html
            std::vector<VkPhysicalDeviceMemoryProperties> device_memory; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceMemoryProperties.html
            std::vector<uint32_t> device_queue_family_counts;
            std::vector<std::vector<VkQueueFamilyProperties>> device_queue_families; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkQueueFamilyProperties.html
        };

        struct Renderable
        {
            wyn_window_t window;
        };

        static inline constexpr std::array instance_extensions{
            "VK_KHR_portability_enumeration", ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_portability_enumeration.html
            "VK_KHR_surface", ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_surface.html
        #if defined(WYN_WIN32)
            "VK_KHR_win32_surface", ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_win32_surface.html
        #elif defined(WYN_COCOA)
            "VK_EXT_metal_surface", ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_metal_surface.html
        #elif defined(WYN_XLIB)
            "VK_KHR_xlib_surface", ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_xlib_surface.html
        #elif defined(WYN_XCB)
            "VK_KHR_xcb_surface", ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_xcb_surface.html
        #elif defined(WYN_WAYLAND)
            "VK_KHR_wayland_surface", ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_wayland_surface.html
        #endif
        #if defined(CGE_DEBUG_VK)
            "VK_EXT_debug_utils", ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_debug_utils.html
        #endif
        };

        static inline constexpr std::array device_extensions{
            "VK_KHR_swapchain", ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_swapchain.html
        };
        
        static inline constexpr std::array instance_layers{
            "VK_LAYER_KHRONOS_validation", ///< https://vulkan.lunarg.com/doc/view/1.3.204.1/windows/khronos_validation_layer.html
        };
        
        static inline constexpr std::array device_layers{
            "VK_LAYER_KHRONOS_validation", ///< https://vulkan.lunarg.com/doc/view/1.3.204.1/windows/khronos_validation_layer.html
        };

    private:

        Functions pfn;
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
    static constexpr const char* unknown_str{ "???" };

    static constexpr const char* vk_msg_severity(const VkDebugUtilsMessageSeverityFlagBitsEXT val) noexcept
    {
        switch (val)
        {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: return "ERROR";
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: return "WARNING";
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: return "INFO";
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: return "VERBOSE";
        default: return unknown_str;
        }
    }

    static constexpr const char* vk_msg_type(const VkDebugUtilsMessageTypeFlagsEXT val) noexcept
    {
        switch (val)
        {
        case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: return "GENERAL";
        case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: return "VALIDATION";
        case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: return "PERFORMANCE";
        default: return unknown_str;
        }
    }

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/PFN_vkDebugUtilsMessengerCallbackEXT.html
    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT svrt,
        VkDebugUtilsMessageTypeFlagsEXT types,
        const VkDebugUtilsMessengerCallbackDataEXT* data,
        [[maybe_unused]] void* user
    ) noexcept
    {
        const char* const str_svrt{ vk_msg_severity(svrt) };
        const char* const str_type{ vk_msg_type(types) };
        const char* const str_id{ data ? data->pMessageIdName : unknown_str };
        const char* const str_msg{ data ? data->pMessage : "" };
        LOG("\n[VULKAN DEBUG - {} {}] <{}>\n{}\n", str_type, str_svrt, str_id, str_msg);
        return VK_FALSE;
    }
}

namespace cge
{
    void Renderer_VK::reinit_context()
    {
        LOG("[CGE] Initializing Vulkan Context...\n");
        {
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkApplicationInfo.html
            const VkApplicationInfo app_info{
                .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                .pNext = {},
                .pApplicationName = {},
                .applicationVersion = {},
                .pEngineName = {},
                .engineVersion = {},
                .apiVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
            };
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkInstanceCreateInfo.html
            const VkInstanceCreateInfo create_info{
                .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                .pNext = {},
                .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
                .pApplicationInfo = &app_info,
                .enabledLayerCount = uint32_t(instance_layers.size()),
                .ppEnabledLayerNames = instance_layers.data(),
                .enabledExtensionCount = uint32_t(instance_extensions.size()),
                .ppEnabledExtensionNames = instance_extensions.data(),
            };
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCreateInstance.html
            const VkResult res_instance{ vkCreateInstance(&create_info, nullptr, &context.instance) };
            ASSERT(res_instance == VK_SUCCESS);

            LOG("[CGE] VK INSTANCE\n");
        }
        {
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkEnumerateInstanceExtensionProperties.html
            const VkResult res_extension_count{ vkEnumerateInstanceExtensionProperties(nullptr, &context.instance_extension_count, nullptr) };
            ASSERT(res_extension_count == VK_SUCCESS);
            context.instance_extensions.resize(context.instance_extension_count);
            const VkResult res_extensions{ vkEnumerateInstanceExtensionProperties(nullptr, &context.instance_extension_count, context.instance_extensions.data()) };
            ASSERT((res_extensions == VK_SUCCESS) || (res_extensions == VK_INCOMPLETE));

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkEnumerateInstanceLayerProperties.html
            const VkResult res_layer_count{ vkEnumerateInstanceLayerProperties(&context.instance_layer_count, nullptr) };
            ASSERT(res_layer_count == VK_SUCCESS);
            context.instance_layers.resize(context.instance_layer_count);
            const VkResult res_layers{ vkEnumerateInstanceLayerProperties(&context.instance_layer_count, context.instance_layers.data()) };
            ASSERT((res_layers == VK_SUCCESS) || (res_layers == VK_INCOMPLETE));

            LOG("[CGE] VK INSTANCE PROPERTIES\n");
        }
        {
            ASSERT((CGE_LOAD_INSTANCE(context.instance, pfn, vkCreateDebugUtilsMessengerEXT)));

            LOG("[CGE] VK INSTANCE FUNCTIONS\n");
        }
        #if defined(CGE_DEBUG_VK)
        {
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDebugUtilsMessengerCreateInfoEXT.html
            const VkDebugUtilsMessengerCreateInfoEXT create_info{
                .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                .pNext = {},
                .flags = {},
                .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT,
                .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
                .pfnUserCallback = debug_callback,
                .pUserData = {},
            };
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCreateDebugUtilsMessengerEXT.html
            const VkResult res_debug{ pfn.vkCreateDebugUtilsMessengerEXT(context.instance, &create_info, nullptr, &context.messenger) };
            ASSERT(res_debug == VK_SUCCESS);

            LOG("[CGE] VK DEBUG\n");
        }
        #endif
        {
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkEnumeratePhysicalDevices.html
            const VkResult res_count{ vkEnumeratePhysicalDevices(context.instance, &context.device_count, nullptr) };
            ASSERT(res_count == VK_SUCCESS);
            context.device_handles.resize(context.device_count);
            context.device_extension_counts.resize(context.device_count);
            context.device_extensions.resize(context.device_count);
            context.device_layer_counts.resize(context.device_count);
            context.device_layers.resize(context.device_count);
            context.device_properties.resize(context.device_count);
            context.device_features.resize(context.device_count);
            context.device_memory.resize(context.device_count);
            context.device_queue_family_counts.resize(context.device_count);
            context.device_queue_families.resize(context.device_count);
            const VkResult res_handles{ vkEnumeratePhysicalDevices(context.instance, &context.device_count, context.device_handles.data()) };
            ASSERT((res_handles == VK_SUCCESS) || (res_handles == VK_INCOMPLETE));
            
            LOG("[CGE] VK PHYSICAL DEVICES\n");
        }
        {
            ASSERT(context.device_count > 0);

            for (uint32_t idx{}; idx < context.device_count; ++idx)
            {
                VkPhysicalDevice& handle{ context.device_handles[idx] };
                uint32_t& extension_count{ context.device_extension_counts[idx] };
                uint32_t& layer_count{ context.device_layer_counts[idx] };
                uint32_t& queue_family_count{ context.device_queue_family_counts[idx] };
                std::vector<VkExtensionProperties>& extensions{ context.device_extensions[idx] };
                std::vector<VkLayerProperties>& layers{ context.device_layers[idx] };
                VkPhysicalDeviceProperties& properties{ context.device_properties[idx] };
                VkPhysicalDeviceFeatures& features{ context.device_features[idx] };
                VkPhysicalDeviceMemoryProperties& memory{ context.device_memory[idx] };
                std::vector<VkQueueFamilyProperties>& queue_families{ context.device_queue_families[idx] };

                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkEnumerateDeviceExtensionProperties.html
                const VkResult res_extension_count{ vkEnumerateDeviceExtensionProperties(handle, nullptr, &extension_count, nullptr) };
                ASSERT(res_extension_count == VK_SUCCESS);
                extensions.resize(extension_count);
                const VkResult res_extensions{ vkEnumerateDeviceExtensionProperties(handle, nullptr, &extension_count, extensions.data()) };
                ASSERT((res_extensions == VK_SUCCESS) || (res_extensions == VK_INCOMPLETE));

                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkEnumerateDeviceLayerProperties.html
                const VkResult res_layer_count{ vkEnumerateDeviceLayerProperties(handle, &layer_count, nullptr) };
                ASSERT(res_layer_count == VK_SUCCESS);
                layers.resize(layer_count);
                const VkResult res_layers{ vkEnumerateDeviceLayerProperties(handle, &layer_count, layers.data()) };
                ASSERT((res_layers == VK_SUCCESS) || (res_layers == VK_INCOMPLETE));

                //https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetPhysicalDeviceProperties.html
                vkGetPhysicalDeviceProperties(handle, &properties);

                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetPhysicalDeviceFeatures.html
                vkGetPhysicalDeviceFeatures(handle, &features);

                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetPhysicalDeviceMemoryProperties.html
                vkGetPhysicalDeviceMemoryProperties(handle, &memory);

                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetPhysicalDeviceQueueFamilyProperties.html
                vkGetPhysicalDeviceQueueFamilyProperties(handle, &queue_family_count, nullptr);
                queue_families.resize(queue_family_count);
                vkGetPhysicalDeviceQueueFamilyProperties(handle, &queue_family_count, queue_families.data());
            }

            LOG("[CGE] VK PHYSICAL DEVICE PROPERTIES\n");
        }
    }

    void Renderer_VK::deinit_context()
    {

    }
}

namespace cge
{
    void Renderer_VK::create_renderable(wyn_window_t const window)
    {
        LOG("[CGE] Initializing Vulkan Window...\n");

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

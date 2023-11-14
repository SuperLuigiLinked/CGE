/**
 * @file renderer_vk.cpp
 */

#include <cstring>
#include <optional>
#include <array>
#include <vector>
#include <span>

#if defined(WYN_WIN32)
    #define VK_USE_PLATFORM_WIN32_KHR
#elif defined(WYN_COCOA)
    #define VK_USE_PLATFORM_METAL_EXT
#elif defined(WYN_X11)
    #error "Unimplemented"
#elif defined(WYN_XLIB)
    #define VK_USE_PLATFORM_XLIB_KHR
#elif defined(WYN_XCB)
    #define VK_USE_PLATFORM_XCB_KHR
#elif defined(WYN_WAYLAND)
    #define VK_USE_PLATFORM_WAYLAND_KHR
#endif
#include <vulkan/vulkan.h>

#include "cge/cge.hpp"
#include "impl.hpp"
#include "mvk.hpp"
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
            PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCreateDebugUtilsMessengerEXT.html
        };
        
        struct Atlas
        {
            VkDeviceMemory device_memory; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDeviceMemory.html
            VkDeviceSize device_capacity; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDeviceSize.html

            VkImage image_handle; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImage.html
            VkDeviceSize image_offs; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDeviceSize.html
            VkDeviceSize image_size; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDeviceSize.html
            VkMemoryRequirements image_mem; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkMemoryRequirements

            VkImageView view; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImageView.html
            VkSampler sampler; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSampler.html
        };

        struct Context
        {
            VkInstance instance; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkInstance.html
            std::uint32_t instance_extension_count;
            std::uint32_t instance_layer_count;
            std::vector<VkExtensionProperties> instance_extensions; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkExtensionProperties.html
            std::vector<VkLayerProperties> instance_layers; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkLayerProperties.html

        #if defined(CGE_DEBUG_VK)
            VkDebugUtilsMessengerEXT messenger; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDebugUtilsMessengerEXT.html
        #endif

            std::uint32_t device_count;
            std::vector<VkPhysicalDevice> device_handles; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDevice.html
            std::vector<std::uint32_t> device_extension_counts;
            std::vector<std::uint32_t> device_layer_counts;
            std::vector<std::uint32_t> device_queue_family_counts;
            std::vector<std::vector<VkExtensionProperties>> device_extensions; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkExtensionProperties.html
            std::vector<std::vector<VkLayerProperties>> device_layers; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkLayerProperties.html
            std::vector<std::vector<VkQueueFamilyProperties>> device_queue_families; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkQueueFamilyProperties.html
            std::vector<VkPhysicalDeviceProperties> device_properties; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceProperties
            std::vector<VkPhysicalDeviceFeatures> device_features; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceFeatures.html
            std::vector<VkPhysicalDeviceMemoryProperties> device_memory; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceMemoryProperties.html
        };

        struct Renderable
        {
            void* window;  ///< HWND      | Window   | xcb_window_t      | wl_surface* | NSWindow*
            void* context; ///< HINSTANCE | Display* | xcb_connection_t* | wl_display* | NSView*

            VkSurfaceKHR surface_handle; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSurfaceKHR.html
            VkExtent2D surface_size; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkExtent2D.html

            VkDevice device; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDevice.html
            VkQueue queue_present; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkQueue.html
            VkQueue queue_graphics; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkQueue.html
            std::uint32_t device_idx;
            std::uint32_t queue_graphics_idx;
            std::uint32_t queue_present_idx;

            std::uint32_t ds_mode_count;
            std::uint32_t ds_format_count;
            std::vector<std::vector<VkBool32>> ds_present_supports; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkBool32.html
            std::vector<std::vector<VkPresentModeKHR>> ds_present_modes; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPresentModeKHR.html
            std::vector<std::vector<VkSurfaceFormatKHR>> ds_formats; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSurfaceFormatKHR.html
            std::vector<VkSurfaceCapabilitiesKHR> ds_capabilities; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSurfaceCapabilitiesKHR.html

            VkBuffer buffer_staging; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkBuffer.html
            VkBuffer buffer_index; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkBuffer.html
            VkBuffer buffer_vertex; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkBuffer.html
            VkDeviceMemory buffer_memory; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDeviceMemory.html
            VkDeviceSize buffer_capacity; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDeviceSize.html

            VkCommandPool command_pool; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkCommandPool.html

            std::optional<Atlas> atlas;

            VkRenderPass render_pass; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkRenderPass.html
            VkShaderModule module_vertex; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkShaderModule.html
            VkShaderModule module_fragment; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkShaderModule.html
            VkDescriptorSetLayout descriptor_layout; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDescriptorSetLayout.html
            VkDescriptorPool descriptor_pool; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDescriptorPool.html
            VkDescriptorSet descriptor_set; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDescriptorSet.html
            VkPipelineLayout pipeline_layout; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPipelineLayout.html
            VkPipeline pipeline_graphics; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPipeline.html

            VkSwapchainKHR swapchain; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSwapchainKHR.html
            
            std::vector<VkFence> frame_fence; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkFence.html
            std::vector<VkSemaphore> frame_sem_render; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSemaphore.html
            std::vector<VkSemaphore> frame_sem_image; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSemaphore.html
            std::vector<VkImage> frame_images; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImage.html
            std::vector<VkImageView> frame_views; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImageView.html
            std::vector<VkFramebuffer> frame_buffers; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkFramebuffer.html
            std::vector<VkCommandBuffer> frame_commands; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkCommandBuffer.html
            std::size_t frame_idx;
        };

        static inline constexpr std::array req_instance_extensions{
            "VK_KHR_portability_enumeration", ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_portability_enumeration.html
            "VK_KHR_surface", ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_surface.html
        #if defined(WYN_WIN32)
            "VK_KHR_win32_surface", ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_win32_surface.html
        #elif defined(WYN_XLIB)
            "VK_KHR_xlib_surface", ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_xlib_surface.html
        #elif defined(WYN_XCB)
            "VK_KHR_xcb_surface", ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_xcb_surface.html
        #elif defined(WYN_WAYLAND)
            "VK_KHR_wayland_surface", ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_wayland_surface.html
        #elif defined(WYN_COCOA)
            "VK_EXT_metal_surface", ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_metal_surface.html
        #else
            #error "Unimplemented"
        #endif
        #if defined(CGE_DEBUG_VK)
            "VK_EXT_debug_utils", ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_debug_utils.html
        #endif
        };

        static inline constexpr std::array req_device_extensions{
            "VK_KHR_swapchain", ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_swapchain.html
        };
        
        static inline constexpr std::array req_instance_layers{
            "VK_LAYER_KHRONOS_validation", ///< https://vulkan.lunarg.com/doc/view/1.3.204.1/windows/khronos_validation_layer.html
        };
        
        static inline constexpr std::array req_device_layers{
            "VK_LAYER_KHRONOS_validation", ///< https://vulkan.lunarg.com/doc/view/1.3.204.1/windows/khronos_validation_layer.html
        };

    private:

        Functions pfn;
        Context context;
        std::optional<Renderable> renderable;

    private:
    
        void reinit_context();
        void deinit_context();

        void create_renderable(wyn_window_t window, bool vsync);
        void destroy_renderable();

        static std::uint64_t rank_device(Context& ctx, Renderable& gfx, std::uint32_t device_idx);
        static std::uint64_t rank_device_graphics(Context& ctx, Renderable& gfx, std::uint32_t device_idx, std::uint32_t queue_idx);
        static std::uint64_t rank_device_present(Context& ctx, Renderable& gfx, std::uint32_t device_idx, std::uint32_t queue_idx);

    public:

        Renderer_VK();
        ~Renderer_VK() final;

        void target_window(EngineImpl& impl, wyn_window_t window) final;
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
    static constexpr const char* vk_msg_severity(const VkDebugUtilsMessageSeverityFlagBitsEXT val) noexcept
    {
        switch (val)
        {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT  : return "ERROR  ";
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: return "WARNING";
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT   : return "INFO   ";
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: return "VERBOSE";
                                                     default: return "???    ";
        }
    }

    static constexpr const char* vk_msg_type(const VkDebugUtilsMessageTypeFlagsEXT val) noexcept
    {
        switch (val)
        {
        case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT    : return "GENERAL    ";
        case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT : return "VALIDATION ";
        case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: return "PERFORMANCE";
                                                     default: return "???        ";
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
        const char* const str_id{ data ? data->pMessageIdName : "???" };
        const char* const str_msg{ data ? data->pMessage : "" };
        CGE_LOG("[ VULKAN DEBUG - {} {} ] {} | {}\n", str_type, str_svrt, str_id, str_msg);
        return VK_FALSE;
    }
}

namespace cge
{
    void Renderer_VK::reinit_context()
    {
        CGE_LOG("[CGE] Initializing Vulkan Context...\n");
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
                .enabledLayerCount = std::uint32_t(req_instance_layers.size()),
                .ppEnabledLayerNames = req_instance_layers.data(),
                .enabledExtensionCount = std::uint32_t(req_instance_extensions.size()),
                .ppEnabledExtensionNames = req_instance_extensions.data(),
            };
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCreateInstance.html
            const VkResult res_instance{ vkCreateInstance(&create_info, nullptr, &context.instance) };
            CGE_ASSERT(res_instance == VK_SUCCESS);

            CGE_LOG("[CGE] VK INSTANCE\n");
        }
        {
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkEnumerateInstanceExtensionProperties.html
            const VkResult res_extension_count{ vkEnumerateInstanceExtensionProperties(nullptr, &context.instance_extension_count, nullptr) };
            CGE_ASSERT(res_extension_count == VK_SUCCESS);
            context.instance_extensions.resize(context.instance_extension_count);
            const VkResult res_extensions{ vkEnumerateInstanceExtensionProperties(nullptr, &context.instance_extension_count, context.instance_extensions.data()) };
            CGE_ASSERT((res_extensions == VK_SUCCESS) || (res_extensions == VK_INCOMPLETE));

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkEnumerateInstanceLayerProperties.html
            const VkResult res_layer_count{ vkEnumerateInstanceLayerProperties(&context.instance_layer_count, nullptr) };
            CGE_ASSERT(res_layer_count == VK_SUCCESS);
            context.instance_layers.resize(context.instance_layer_count);
            const VkResult res_layers{ vkEnumerateInstanceLayerProperties(&context.instance_layer_count, context.instance_layers.data()) };
            CGE_ASSERT((res_layers == VK_SUCCESS) || (res_layers == VK_INCOMPLETE));

            CGE_LOG("[CGE] VK INSTANCE PROPERTIES\n");
        }
        {
            CGE_ASSERT((CGE_LOAD_INSTANCE(context.instance, pfn, vkCreateDebugUtilsMessengerEXT)));

            CGE_LOG("[CGE] VK INSTANCE FUNCTIONS\n");
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
            CGE_ASSERT(res_debug == VK_SUCCESS);

            CGE_LOG("[CGE] VK DEBUG\n");
        }
        #endif
        {
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkEnumeratePhysicalDevices.html
            const VkResult res_count{ vkEnumeratePhysicalDevices(context.instance, &context.device_count, nullptr) };
            CGE_ASSERT(res_count == VK_SUCCESS);
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
            CGE_ASSERT((res_handles == VK_SUCCESS) || (res_handles == VK_INCOMPLETE));
            
            CGE_LOG("[CGE] VK PHYSICAL DEVICES\n");
        }
        {
            CGE_ASSERT(context.device_count > 0);

            for (std::uint32_t idx{}; idx < context.device_count; ++idx)
            {
                VkPhysicalDevice& handle{ context.device_handles[idx] };
                std::uint32_t& extension_count{ context.device_extension_counts[idx] };
                std::uint32_t& layer_count{ context.device_layer_counts[idx] };
                std::uint32_t& queue_family_count{ context.device_queue_family_counts[idx] };
                std::vector<VkExtensionProperties>& extensions{ context.device_extensions[idx] };
                std::vector<VkLayerProperties>& layers{ context.device_layers[idx] };
                std::vector<VkQueueFamilyProperties>& queue_families{ context.device_queue_families[idx] };
                VkPhysicalDeviceProperties& properties{ context.device_properties[idx] };
                VkPhysicalDeviceFeatures& features{ context.device_features[idx] };
                VkPhysicalDeviceMemoryProperties& memory{ context.device_memory[idx] };

                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkEnumerateDeviceExtensionProperties.html
                const VkResult res_extension_count{ vkEnumerateDeviceExtensionProperties(handle, nullptr, &extension_count, nullptr) };
                CGE_ASSERT(res_extension_count == VK_SUCCESS);
                extensions.resize(extension_count);
                const VkResult res_extensions{ vkEnumerateDeviceExtensionProperties(handle, nullptr, &extension_count, extensions.data()) };
                CGE_ASSERT((res_extensions == VK_SUCCESS) || (res_extensions == VK_INCOMPLETE));

                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkEnumerateDeviceLayerProperties.html
                const VkResult res_layer_count{ vkEnumerateDeviceLayerProperties(handle, &layer_count, nullptr) };
                CGE_ASSERT(res_layer_count == VK_SUCCESS);
                layers.resize(layer_count);
                const VkResult res_layers{ vkEnumerateDeviceLayerProperties(handle, &layer_count, layers.data()) };
                CGE_ASSERT((res_layers == VK_SUCCESS) || (res_layers == VK_INCOMPLETE));

                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetPhysicalDeviceQueueFamilyProperties.html
                vkGetPhysicalDeviceQueueFamilyProperties(handle, &queue_family_count, nullptr);
                queue_families.resize(queue_family_count);
                vkGetPhysicalDeviceQueueFamilyProperties(handle, &queue_family_count, queue_families.data());

                //https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetPhysicalDeviceProperties.html
                vkGetPhysicalDeviceProperties(handle, &properties);

                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetPhysicalDeviceFeatures.html
                vkGetPhysicalDeviceFeatures(handle, &features);

                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetPhysicalDeviceMemoryProperties.html
                vkGetPhysicalDeviceMemoryProperties(handle, &memory);
            }

            CGE_LOG("[CGE] VK PHYSICAL DEVICE PROPERTIES\n");
        }
    }

    void Renderer_VK::deinit_context()
    {
        if (this->renderable.has_value()) destroy_renderable();
    }
}

namespace cge
{
    void Renderer_VK::create_renderable(wyn_window_t const window, bool const vsync)
    {
        CGE_LOG("[CGE] Initializing Vulkan Window...\n");
        Renderable& gfx{ this->renderable.emplace() };
        {
            gfx.window = static_cast<void*>(window);
            CGE_ASSERT(gfx.window);

            gfx.context = wyn_native_context(window);
            CGE_ASSERT(gfx.context);

            CGE_LOG("[CGE] VK WINDOW CONTEXT\n");
        }
        {
        #if defined(WYN_WIN32)
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkWin32SurfaceCreateInfoKHR.html
            const VkWin32SurfaceCreateInfoKHR create_info{
                .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
                .pNext = {},
                .flags = {},
                .hinstance = static_cast<HINSTANCE>(gfx.context),
                .hwnd = static_cast<HWND>(gfx.window),
            };
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCreateWin32SurfaceKHR.html
            const VkResult res_surface{ vkCreateWin32SurfaceKHR(context.instance, &create_info, nullptr, &gfx.surface_handle) };
        #elif defined(WYN_XLIB)
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkXlibSurfaceCreateInfoKHR.html
            const VkXlibSurfaceCreateInfoKHR create_info{
                .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
                .pNext = {},
                .flags = {},
                .dpy = static_cast<Display*>(gfx.context),
                .window = static_cast<Window>(std::uintptr_t(gfx.window)),
            };
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCreateXlibSurfaceKHR.html
            const VkResult res_surface{ vkCreateXlibSurfaceKHR(context.instance, &create_info, nullptr, &gfx.surface_handle) };
        #elif defined(WYN_XCB) || defined(WYN_X11)
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkXcbSurfaceCreateInfoKHR.html
            const VkXcbSurfaceCreateInfoKHR create_info{
                .sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
                .pNext = {},
                .flags = {},
                .connection = static_cast<xcb_connection_t*>(gfx.context),
                .window = static_cast<xcb_window_t>(std::uintptr_t(gfx.window)),
            };
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCreateXcbSurfaceKHR.html
            const VkResult res_surface{ vkCreateXcbSurfaceKHR(context.instance, &create_info, nullptr, &gfx.surface_handle) };
        #elif defined(WYN_WAYLAND)
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkWaylandSurfaceCreateInfoKHR.html
            const VkWaylandSurfaceCreateInfoKHR create_info{
                .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
                .pNext = {},
                .flags = {},
                .display = static_cast<wl_display*>(gfx.context),
                .surface = static_cast<wl_surface*>(gfx.window),
            };
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCreateWaylandSurfaceKHR.html
            const VkResult res_surface{ vkCreateWaylandSurfaceKHR(context.instance, &create_info, nullptr, &gfx.surface_handle) };
        #elif defined(WYN_COCOA)
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkMetalSurfaceCreateInfoEXT.html
            const VkMetalSurfaceCreateInfoEXT create_info{
                .sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT,
                .pNext = {},
                .flags = {},
                .pLayer = static_cast<const CAMetalLayer*>(cge::mvk::create_layer(gfx.window)),
            };
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCreateMetalSurfaceEXT.html
            const VkResult res_surface{ vkCreateMetalSurfaceEXT(context.instance, &create_info, nullptr, &gfx.surface_handle) };
        #else
            #error "Unimplemented"
        #endif
            CGE_ASSERT(res_surface == VK_SUCCESS);

            const wyn_size_t size{ wyn_window_size(gfx.window) };
            gfx.surface_size = { .width = uint32_t(size.w), .height = uint32_t(size.h) };

            CGE_LOG("[CGE] VK SURFACE\n");
        }
        {
            const std::uint32_t device_count{  context.device_count };
            gfx.ds_present_supports.resize(device_count);
            gfx.ds_present_modes.resize(device_count);
            gfx.ds_formats.resize(device_count);
            gfx.ds_capabilities.resize(device_count);
            
            for (std::uint32_t di{}; di < device_count; ++di)
            {
                VkPhysicalDevice& device{ context.device_handles[di] };
                VkSurfaceKHR& surface{ gfx.surface_handle };
                std::vector<VkBool32>& present_supports{ gfx.ds_present_supports[di] };
                std::vector<VkPresentModeKHR>& present_modes{ gfx.ds_present_modes[di] };
                std::vector<VkSurfaceFormatKHR>& formats{ gfx.ds_formats[di] };
                VkSurfaceCapabilitiesKHR& capabilities{ gfx.ds_capabilities[di] };
                std::uint32_t& mode_count{ gfx.ds_mode_count };
                std::uint32_t& format_count{ gfx.ds_format_count };

                const std::uint32_t queue_count{ context.device_queue_family_counts[di] };
                present_supports.resize(queue_count);

                for (std::uint32_t qi{}; qi < queue_count; ++qi)
                {
                    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetPhysicalDeviceSurfaceSupportKHR.html
                    const VkResult res_support{ vkGetPhysicalDeviceSurfaceSupportKHR(device, qi, surface, &present_supports[qi]) };
                    CGE_ASSERT(res_support == VK_SUCCESS);
                }

                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetPhysicalDeviceSurfacePresentModesKHR.html
                const VkResult res_mode_count{ vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &mode_count, nullptr) };
                CGE_ASSERT(res_mode_count == VK_SUCCESS);
                present_modes.resize(mode_count);
                const VkResult res_modes{ vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &mode_count, present_modes.data()) };
                CGE_ASSERT((res_modes == VK_SUCCESS) || (res_modes == VK_INCOMPLETE));
                
                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetPhysicalDeviceSurfaceFormatsKHR.html
                const VkResult res_format_count{ vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr) };
                CGE_ASSERT(res_format_count == VK_SUCCESS);
                formats.resize(format_count);
                const VkResult res_formats{ vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, formats.data()) };
                CGE_ASSERT((res_formats == VK_SUCCESS) || (res_formats == VK_INCOMPLETE));

                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetPhysicalDeviceSurfaceCapabilitiesKHR.html
                const VkResult res_caps{ vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &capabilities) };
                CGE_ASSERT(res_caps == VK_SUCCESS);
            }

            CGE_LOG("[CGE] VK DEVICE SURFACE\n");
        }
        {
            std::uint32_t device_idx{};
            std::uint32_t graphics_idx{};
            std::uint32_t present_idx{};
            std::uint64_t device_rank{};
            std::uint64_t graphics_rank{};
            std::uint64_t present_rank{};

            const std::uint32_t device_count{ context.device_count };
            for (std::uint32_t di{}; di < device_count; ++di)
            {
                const uint64_t rank_d{ rank_device(context, gfx, di) };
                {
                    const auto& dev_props{ context.device_properties[di] };
                    CGE_LOG("[CGE] Device: ({:2}) \"{}\"\n", rank_d, dev_props.deviceName);
                }

                if (rank_d > device_rank)
                {
                    device_idx = di;
                    device_rank = rank_d;
                    graphics_idx = 0;
                    graphics_rank = 0;
                    present_idx = 0;
                    present_rank = 0;

                    const std::uint32_t queue_count{ context.device_queue_family_counts[di] };
                    for (std::uint32_t qi{}; qi < queue_count; ++qi)
                    {
                        const std::uint64_t rank_g{ rank_device_graphics(context, gfx, di, qi) };
                        if (rank_g > graphics_rank)
                        {
                            graphics_idx = qi;
                            graphics_rank = rank_g;
                        }

                        const std::uint64_t rank_p{ rank_device_present(context, gfx, di, qi) };
                        if (rank_p > present_rank)
                        {
                            present_idx = qi;
                            present_rank = rank_g;
                        }
                    }
                }
            }

            gfx.device_idx = device_idx;
            gfx.queue_graphics_idx = graphics_idx;
            gfx.queue_present_idx = present_idx;

            {
                const auto& dev_props{ context.device_properties[gfx.device_idx] };
                CGE_LOG("[CGE] Selected: \"{}\"\n", dev_props.deviceName);
            }
            CGE_LOG("[CGE] VK DEVICE SELECTION\n");
        }
        {

        }
        (void)vsync;
    }

    void Renderer_VK::destroy_renderable()
    {
        this->renderable.reset();
    }
}

namespace cge
{
    inline static bool str_equal(const char* const a, const char* const b) noexcept
    {
        return std::strcmp(a, b) == 0;
    }

    inline static bool has_extensions(const std::span<const VkExtensionProperties> sup_exts, const std::span<const char* const> req_exts) noexcept
    {
        for (const char* const req : req_exts)
        {
            for (const VkExtensionProperties& sup : sup_exts)
            {
                if (str_equal(req, sup.extensionName)) goto found;
            }
            return false;
        found:
        }
        return true;
    }

    inline static bool has_layers(const std::span<const VkLayerProperties> sup_lyrs, const std::span<const char* const> req_lyrs) noexcept
    {
        for (const char* const req : req_lyrs)
        {
            for (const VkLayerProperties& sup : sup_lyrs)
            {
                if (str_equal(req, sup.layerName)) goto found;
            }
            return false;
        found:
        }
        return true;
    }

    std::uint64_t Renderer_VK::rank_device(Context& ctx, Renderable& gfx, std::uint32_t device_idx)
    {
        const VkPhysicalDeviceProperties& device_props{ ctx.device_properties[device_idx] };
        const std::span<const VkExtensionProperties> device_exts{ ctx.device_extensions[device_idx] };
        const std::span<const VkLayerProperties> device_lyrs{ ctx.device_layers[device_idx] };

        if (gfx.ds_format_count == 0) return 0;
        if (gfx.ds_mode_count == 0) return 0;
        if (!cge::has_extensions(device_exts, req_device_extensions)) return 0;
        if (!cge::has_layers(device_lyrs, req_device_layers)) return 0;

        std::uint64_t rank{ 1 };

        switch (device_props.deviceType)
        {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU  : rank += 50; break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: rank += 40; break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU   : rank += 30; break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU           : rank += 20; break;
        case VK_PHYSICAL_DEVICE_TYPE_OTHER         : rank += 10; break;
        default: break;
        }

        return rank;
    }

    std::uint64_t Renderer_VK::rank_device_graphics(Context& ctx, [[maybe_unused]] Renderable& gfx, std::uint32_t device_idx, std::uint32_t queue_idx)
    {
        const VkQueueFamilyProperties& qf_props{ ctx.device_queue_families[device_idx][queue_idx] };

        if ((qf_props.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) return 0;

        std::uint64_t rank{ 1 };

        rank += qf_props.queueCount;

        return rank;
    }

    std::uint64_t Renderer_VK::rank_device_present(Context& ctx, Renderable& gfx, std::uint32_t device_idx, std::uint32_t queue_idx)
    {
        const VkQueueFamilyProperties& qf_props{ ctx.device_queue_families[device_idx][queue_idx] };
        const VkBool32 supports_present{ gfx.ds_present_supports[device_idx][queue_idx] };

        if (!supports_present) return 0;

        std::uint64_t rank{ 1 };

        rank += qf_props.queueCount;

        return rank;
    }

}

namespace cge
{
    void Renderer_VK::target_window(EngineImpl& impl, wyn_window_t const window)
    {
        if (this->renderable.has_value()) destroy_renderable();

        if (window)
            create_renderable(wyn_window_t(window), impl.settings.vsync);
    }

    void Renderer_VK::render(EngineImpl& impl)
    {
        Renderable& gfx{ this->renderable.value() };
        (void)impl;
        (void)gfx;
    }
}

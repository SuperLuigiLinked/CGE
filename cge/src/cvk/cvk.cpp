/**
 * @file cge/cvk.cpp
 */

#include <cstring>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>

#include "cvk.hpp"

namespace cvk
{
#if defined(WYN_COCOA)
    extern void* create_metal_layer(void* ns_view);
#endif

    static void load_instance_functions(cvk::Context& ctx) noexcept;
    static void load_device_functions(cvk::Context& ctx, cvk::Renderable& gfx) noexcept;

    extern void create_context(cvk::Context& ctx) noexcept;
    extern void destroy_context(cvk::Context& ctx) noexcept;
    static void reinit_instance(cvk::Context& ctx) noexcept;
    static void deinit_instance(cvk::Context& ctx) noexcept;
#if defined(CGE_DEBUG)
    static void reinit_instance_properties(cvk::Context& ctx) noexcept;
    static void deinit_instance_properties(cvk::Context& ctx) noexcept;
#endif
    static void reinit_physical_devices(cvk::Context& ctx) noexcept;
    static void deinit_physical_devices(cvk::Context& ctx) noexcept;
    static void reinit_physical_properties(cvk::Context& ctx) noexcept;
    static void deinit_physical_properties(cvk::Context& ctx) noexcept;

#if defined(CGE_VALIDATE_VK)
    static void reinit_debug_messenger(cvk::Context& ctx) noexcept;
    static void deinit_debug_messenger(cvk::Context& ctx) noexcept;
    static constexpr const char* debug_msg_severity(VkDebugUtilsMessageSeverityFlagBitsEXT val) noexcept;
    static constexpr const char* debug_msg_type(VkDebugUtilsMessageTypeFlagsEXT val) noexcept;
    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT svrt, VkDebugUtilsMessageTypeFlagsEXT types, const VkDebugUtilsMessengerCallbackDataEXT* data, void* user) noexcept;
#endif

    extern void create_renderable(cvk::Context& ctx, cvk::Renderable& gfx, wyn_window_t window, bool vsync) noexcept;
    extern void destroy_renderable(cvk::Context& ctx, cvk::Renderable& gfx) noexcept;
    static void reinit_surface(cvk::Context& ctx, cvk::Renderable& gfx, wyn_window_t window);
    static void deinit_surface(cvk::Context& ctx, cvk::Renderable& gfx) noexcept;
    extern void update_surface_info(cvk::Context& ctx, cvk::Renderable& gfx, cvk::Offset device_idx) noexcept;
    static void reinit_device(cvk::Context& ctx, cvk::Renderable& gfx) noexcept;
    static void deinit_device(cvk::Context& ctx, cvk::Renderable& gfx) noexcept;
    static void reinit_buffers(cvk::Context& ctx, cvk::Renderable& gfx) noexcept;
    static void deinit_buffers(cvk::Context& ctx, cvk::Renderable& gfx) noexcept;
    static void reinit_cmdpool(cvk::Context& ctx, cvk::Renderable& gfx) noexcept;
    static void deinit_cmdpool(cvk::Context& ctx, cvk::Renderable& gfx) noexcept;
    static void reinit_shaders(cvk::Context& ctx, cvk::Renderable& gfx) noexcept;
    static void deinit_shaders(cvk::Context& ctx, cvk::Renderable& gfx) noexcept;
    static void reinit_pipelines(cvk::Context& ctx, cvk::Renderable& gfx) noexcept;
    static void deinit_pipelines(cvk::Context& ctx, cvk::Renderable& gfx) noexcept;
    extern void remake_swapchain(cvk::Context& ctx, cvk::Renderable& gfx, bool vsync) noexcept;
    static void deinit_swapchain(cvk::Context& ctx, cvk::Renderable& gfx) noexcept;

    static void reinit_atlases(cvk::Context& ctx, cvk::Renderable& gfx) noexcept;
    static void deinit_atlases(cvk::Context& ctx, cvk::Renderable& gfx) noexcept;
    static void destroy_atlas(cvk::Context& ctx, cvk::Renderable& gfx, cvk::Offset atlas_idx) noexcept;
    extern void upload_texture(cvk::Context& ctx, cvk::Renderable& gfx, cvk::Offset atlas_idx, cge::Texture tex) noexcept;
    static void create_atlas(cvk::Context& ctx, cvk::Renderable& gfx, cvk::Offset atlas_idx, cge::Texture tex) noexcept;
    static void transition_atlas(cvk::Context& ctx, cvk::Renderable& gfx, cvk::Offset atlas_idx, VkImageLayout old_layout, VkImageLayout new_layout) noexcept;
    static void stage_texture(cvk::Context& ctx, cvk::Renderable& gfx, cvk::Offset atlas_idx, cge::Texture tex) noexcept;
    static void update_descriptors(cvk::Context& ctx, cvk::Renderable& gfx, cvk::Offset atlas_idx) noexcept;
    static void single_commands(cvk::Context& ctx, cvk::Renderable& gfx, auto&& callback) noexcept;

    static void select_device(cvk::Context& ctx, cvk::Renderable& gfx) noexcept;
    static cvk::Ranking rank_device(const cvk::Context& ctx, const cvk::Renderable& gfx, cvk::Offset device_idx) noexcept;
    static cvk::Ranking rank_device_graphics(const cvk::Context& ctx, const cvk::Renderable& gfx, cvk::Offset device_idx, cvk::Offset queue_idx) noexcept;
    static cvk::Ranking rank_device_present(const cvk::Context& ctx, const cvk::Renderable& gfx, cvk::Offset device_idx, cvk::Offset queue_idx) noexcept;

    static bool str_equal(const char* a, const char* b) noexcept;
    static bool has_extensions(std::span<const VkExtensionProperties> sup_exts, std::span<const char* const> req_exts) noexcept;
    static bool has_layers(std::span<const VkLayerProperties> sup_lyrs, std::span<const char* const> req_lyrs) noexcept;
    static std::string load_txt(const char* filepath) noexcept;
    static void compile_spirv(cvk::Context& ctx, cvk::Renderable& gfx, VkShaderModule& module, const shaderc::Compiler& compiler, const shaderc::CompileOptions& options, const std::string& file_dir, const char* file_name, shaderc_shader_kind shader_kind) noexcept;
    static VkDeviceSize map_bytes(VkDeviceSize buffer_size, void* const buffer, VkDeviceSize& offs, std::span<const std::byte> bytes) noexcept;
    static cvk::Offset find_memtype(std::span<const VkMemoryType> mem_types, cvk::Offset alloc_type, cvk::Offset alloc_props) noexcept;
    static VkSurfaceFormatKHR ideal_format(std::span<const VkSurfaceFormatKHR> formats) noexcept;
    static VkPresentModeKHR ideal_present(std::span<const VkPresentModeKHR> modes, bool vsync) noexcept;
    static VkExtent2D ideal_resolution(VkExtent2D size, const VkSurfaceCapabilitiesKHR& caps) noexcept;

    extern VkResult render_frame(cvk::Context& ctx, cvk::Renderable& gfx, const cge::Primitives& prims) noexcept;
    static VkResult acquire_image(cvk::Renderable& gfx, cvk::Offset& acquired_idx, VkSemaphore signal_sem, std::span<const VkFence> wait_fences, std::span<const VkFence> reset_fences) noexcept;
    static VkResult record_commands(cvk::Renderable& gfx, cvk::Offset frame_idx, const cge::Primitives& prims) noexcept;
    static VkResult submit_commands(cvk::Renderable& gfx, cvk::Offset frame_idx, std::span<const VkSemaphore> wait_sems, std::span<const VkSemaphore> signal_sems, VkFence signal_fence) noexcept;
    static VkResult present_image(cvk::Renderable& gfx, cvk::Offset frame_idx, std::span<const VkSemaphore> wait_sems) noexcept;

}

namespace cvk
{
    void load_instance_functions(cvk::Context& ctx [[maybe_unused]]) noexcept
    {
    #if defined(CGE_VALIDATE_VK)
        CGE_ASSERT((CVK_LOAD_INSTANCE(ctx.instance, ctx.pfn, vkCreateDebugUtilsMessengerEXT)));
        CGE_ASSERT((CVK_LOAD_INSTANCE(ctx.instance, ctx.pfn, vkDestroyDebugUtilsMessengerEXT)));
    #endif
    }

    void load_device_functions(cvk::Context& ctx [[maybe_unused]], cvk::Renderable& gfx [[maybe_unused]]) noexcept
    {
    }
}

namespace cvk
{
    void create_context(cvk::Context& ctx) noexcept
    {
        CGE_LOG("[CGE] Initializing Vulkan Context...\n");
        cvk::reinit_instance(ctx);
        cvk::load_instance_functions(ctx);
    #if defined(CGE_DEBUG)
        cvk::reinit_instance_properties(ctx);
    #endif
    #if defined(CGE_VALIDATE_VK)
        cvk::reinit_debug_messenger(ctx);
    #endif
        cvk::reinit_physical_devices(ctx);
        cvk::reinit_physical_properties(ctx);
    }
    
    void destroy_context(cvk::Context& ctx) noexcept
    {
        cvk::deinit_physical_properties(ctx);
        cvk::deinit_physical_devices(ctx);
    #if defined(CGE_VALIDATE_VK)
        cvk::deinit_debug_messenger(ctx);
    #endif
    #if defined(CGE_DEBUG)
        cvk::deinit_instance_properties(ctx);
    #endif
        cvk::deinit_instance(ctx);
    }

    void reinit_instance(cvk::Context& ctx) noexcept
    {
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkApplicationInfo.html
        const VkApplicationInfo app_info{
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = {},
            .pApplicationName = {},
            .applicationVersion = {},
            .pEngineName = {},
            .engineVersion = {},
            .apiVersion = VK_API_VERSION_1_0,
        };
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkInstanceCreateInfo.html
        const VkInstanceCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = {},
        #if defined(__APPLE__)
            .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
        #else
            .flags = {},
        #endif
            .pApplicationInfo = &app_info,
            .enabledLayerCount = cvk::req_layers.size(),
            .ppEnabledLayerNames = cvk::req_layers.data(),
            .enabledExtensionCount = cvk::req_instance_extensions.size(),
            .ppEnabledExtensionNames = cvk::req_instance_extensions.data(),
        };
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCreateInstance.html
        const VkResult res_instance{ vkCreateInstance(&create_info, ctx.allocator, &ctx.instance) };
        CGE_ASSERT(res_instance == VK_SUCCESS);
    }

    void deinit_instance(cvk::Context& ctx) noexcept
    {
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroyInstance.html
        if (ctx.instance)
            vkDestroyInstance(ctx.instance, ctx.allocator);
    }

#if defined(CGE_DEBUG)
    void reinit_instance_properties(cvk::Context& ctx) noexcept
    {
        {
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkEnumerateInstanceExtensionProperties.html
            Offset count;
            const VkResult res_count{ vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr) };
            CGE_ASSERT(res_count == VK_SUCCESS);

            const bool res_resize{ soa::realloc(count, ctx.instance_ext_count, ctx.instance_ext_array) };
            CGE_ASSERT(res_resize);
            
            const VkResult res_props{ vkEnumerateInstanceExtensionProperties(nullptr, &ctx.instance_ext_count, ctx.instance_ext_array) };
            CGE_ASSERT(res_props == VK_SUCCESS);
        }
        {
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkEnumerateInstanceLayerProperties.html
            Offset count;
            const VkResult res_count{ vkEnumerateInstanceLayerProperties(&count, nullptr) };
            CGE_ASSERT(res_count == VK_SUCCESS);
            
            const bool res_resize{ soa::realloc(count, ctx.instance_lyr_count, ctx.instance_lyr_array) };
            CGE_ASSERT(res_resize);
            
            const VkResult res_props{ vkEnumerateInstanceLayerProperties(&ctx.instance_lyr_count, ctx.instance_lyr_array) };
            CGE_ASSERT(res_props == VK_SUCCESS);
        }
    }

    void deinit_instance_properties(cvk::Context& ctx) noexcept
    {
        soa::dealloc(ctx.instance_lyr_count, ctx.instance_lyr_array);
        soa::dealloc(ctx.instance_ext_count, ctx.instance_ext_array);
    }
#endif

    void reinit_physical_devices(cvk::Context& ctx) noexcept
    {
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkEnumeratePhysicalDevices.html
        Offset count;
        const VkResult res_count{ vkEnumeratePhysicalDevices(ctx.instance, &count, nullptr) };
        CGE_ASSERT(res_count == VK_SUCCESS);

        const bool res_resize{
            soa::realloc(
                count, ctx.device_count,
                ctx.devices,
                ctx.device_ext_count,
                ctx.device_lyr_count,
                ctx.device_fam_count,
                ctx.device_ext_array,
                ctx.device_lyr_array,
                ctx.device_fam_array,
                ctx.device_properties,
                ctx.device_features,
                ctx.device_memory
            )
        };
        CGE_ASSERT(res_resize);
        
        for (Offset idx{}; idx < ctx.device_count; ++idx)
        {
            ctx.devices[idx] = {};
            ctx.device_ext_count[idx] = {};
            ctx.device_lyr_count[idx] = {};
            ctx.device_fam_count[idx] = {};
            ctx.device_ext_array[idx] = {};
            ctx.device_lyr_array[idx] = {};
            ctx.device_fam_array[idx] = {};
        }
        const VkResult res_handles{ vkEnumeratePhysicalDevices(ctx.instance, &ctx.device_count, ctx.devices) };
        CGE_ASSERT(res_handles == VK_SUCCESS);

        CGE_ASSERT(ctx.device_count > 0);
    }

    void deinit_physical_devices(cvk::Context& ctx) noexcept
    {
        soa::dealloc(ctx.device_count, ctx.devices);
    }

    void reinit_physical_properties(cvk::Context& ctx) noexcept
    {
        for (Offset idx{}; idx < ctx.device_count; ++idx)
        {
            VkPhysicalDevice& handle{ ctx.devices[idx] };
            Offset& ext_count{ ctx.device_ext_count[idx] };
            Offset& lyr_count{ ctx.device_lyr_count[idx] };
            Offset& fam_count{ ctx.device_fam_count[idx] };
            VkExtensionProperties*& ext_array{ ctx.device_ext_array[idx] };
            VkLayerProperties*& lyr_array{ ctx.device_lyr_array[idx] };
            VkQueueFamilyProperties*& fam_array{ ctx.device_fam_array[idx] };
            VkPhysicalDeviceProperties& properties{ ctx.device_properties[idx] };
            VkPhysicalDeviceFeatures& features{ ctx.device_features[idx] };
            VkPhysicalDeviceMemoryProperties& memory{ ctx.device_memory[idx] };

            {
                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkEnumerateDeviceExtensionProperties.html
                Offset count;
                const VkResult res_count{ vkEnumerateDeviceExtensionProperties(handle, nullptr, &count, nullptr) };
                CGE_ASSERT(res_count == VK_SUCCESS);
                const bool res_resize{ soa::realloc(count, ext_count, ext_array) };
                CGE_ASSERT(res_resize);
                const VkResult res_props{ vkEnumerateDeviceExtensionProperties(handle, nullptr, &ext_count, ext_array) };
                CGE_ASSERT(res_props == VK_SUCCESS);
            }
            {
                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkEnumerateDeviceLayerProperties.html
                Offset count;
                const VkResult res_count{ vkEnumerateDeviceLayerProperties(handle, &count, nullptr) };
                CGE_ASSERT(res_count == VK_SUCCESS);
                const bool res_resize{ soa::realloc(count, lyr_count, lyr_array) };
                CGE_ASSERT(res_resize);
                const VkResult res_layers{ vkEnumerateDeviceLayerProperties(handle, &lyr_count, lyr_array) };
                CGE_ASSERT(res_layers == VK_SUCCESS);
            }
            {
                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetPhysicalDeviceQueueFamilyProperties.html
                Offset count;
                vkGetPhysicalDeviceQueueFamilyProperties(handle, &count, nullptr);
                const bool res_resize{ soa::realloc(count, fam_count, fam_array) };
                CGE_ASSERT(res_resize);
                vkGetPhysicalDeviceQueueFamilyProperties(handle, &fam_count, fam_array);
            }
            {
                //https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetPhysicalDeviceProperties.html
                vkGetPhysicalDeviceProperties(handle, &properties);

                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetPhysicalDeviceFeatures.html
                vkGetPhysicalDeviceFeatures(handle, &features);

                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetPhysicalDeviceMemoryProperties.html
                vkGetPhysicalDeviceMemoryProperties(handle, &memory);
            }
        }
    }

    void deinit_physical_properties(cvk::Context& ctx) noexcept
    {
        for (Offset idx{}; idx < ctx.device_count; ++idx)
        {
            soa::dealloc(ctx.device_fam_count[idx], ctx.device_fam_array[idx]);
            soa::dealloc(ctx.device_lyr_count[idx], ctx.device_lyr_array[idx]);
            soa::dealloc(ctx.device_ext_count[idx], ctx.device_ext_array[idx]);
        }
    }
}

#if defined(CGE_VALIDATE_VK)
namespace cvk
{
    void reinit_debug_messenger(cvk::Context& ctx) noexcept
    {
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDebugUtilsMessengerCreateInfoEXT.html
        const VkDebugUtilsMessengerCreateInfoEXT create_info{
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .pNext = {},
            .flags = {},
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
            .pfnUserCallback = cvk::debug_callback,
            .pUserData = {},
        };
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCreateDebugUtilsMessengerEXT.html
        const VkResult res_debug{ ctx.pfn.vkCreateDebugUtilsMessengerEXT(ctx.instance, &create_info, ctx.allocator, &ctx.messenger) };
        CGE_ASSERT(res_debug == VK_SUCCESS);
    }

    void deinit_debug_messenger(cvk::Context& ctx) noexcept
    {
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroyDebugUtilsMessengerEXT.html
        if (ctx.messenger)
            ctx.pfn.vkDestroyDebugUtilsMessengerEXT(ctx.instance, ctx.messenger, ctx.allocator);
    }

    constexpr const char* debug_msg_severity(const VkDebugUtilsMessageSeverityFlagBitsEXT val) noexcept
    {
        if (val & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT  ) return "ERROR";
        if (val & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) return "WARNING";
        if (val & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT   ) return "INFO";
        if (val & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) return "VERBOSE";
        return "";
    }

    constexpr const char* debug_msg_type(const VkDebugUtilsMessageTypeFlagsEXT val) noexcept
    {
        if (val & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT    ) return "GENERAL";
        if (val & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT ) return "VALIDATION";
        if (val & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) return "PERFORMANCE";
        return "";
    }

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/PFN_vkDebugUtilsMessengerCallbackEXT.html
    VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
        [[maybe_unused]] VkDebugUtilsMessageSeverityFlagBitsEXT svrt,
        [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT types,
        [[maybe_unused]] const VkDebugUtilsMessengerCallbackDataEXT* data,
        [[maybe_unused]] void* user
    ) noexcept
    {
        [[maybe_unused]] const char* const str_svrt{ cvk::debug_msg_severity(svrt) };
        [[maybe_unused]] const char* const str_type{ cvk::debug_msg_type(types) };
        [[maybe_unused]] const char* const str_id{ (data && data->pMessageIdName) ? data->pMessageIdName : "" };
        [[maybe_unused]] const char* const str_msg{ (data && data->pMessage) ? data->pMessage : "" };
        CGE_LOG("[VULKAN DEBUG - {} {}] {}\n{{\n\t{}\n}}\n", str_type, str_svrt, str_id, str_msg);
        return VK_FALSE;
    }
}
#endif

namespace cvk
{
    void create_renderable(Context& ctx, Renderable& gfx, wyn_window_t const window, bool const vsync) noexcept
    {
        CGE_LOG("[CGE] Initializing Vulkan Window...\n");
        cvk::reinit_surface(ctx, gfx, window);
        cvk::select_device(ctx, gfx);
        cvk::reinit_device(ctx, gfx);
        cvk::load_device_functions(ctx, gfx);
        cvk::reinit_buffers(ctx, gfx);
        cvk::reinit_cmdpool(ctx, gfx);
        cvk::reinit_shaders(ctx, gfx);
        cvk::reinit_pipelines(ctx, gfx);
        cvk::reinit_atlases(ctx, gfx);
        cvk::remake_swapchain(ctx, gfx, vsync);
    }

    void destroy_renderable(Context& ctx, Renderable& gfx) noexcept
    {
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDeviceWaitIdle.html
        const VkResult res_wait{ vkDeviceWaitIdle(gfx.device) };
        (void)res_wait;

        cvk::deinit_atlases(ctx, gfx);
        cvk::deinit_swapchain(ctx, gfx);
        cvk::deinit_pipelines(ctx, gfx);
        cvk::deinit_shaders(ctx, gfx);
        cvk::deinit_cmdpool(ctx, gfx);
        cvk::deinit_buffers(ctx, gfx);
        cvk::deinit_device(ctx, gfx);
        cvk::deinit_surface(ctx, gfx);
    }

    void reinit_surface(cvk::Context& ctx, cvk::Renderable& gfx, wyn_window_t window)
    {
        {
            gfx.window = static_cast<void*>(window);
            CGE_ASSERT(gfx.window);

            gfx.context = wyn_native_context(window);
            CGE_ASSERT(gfx.context);
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
            const VkResult res_surface{ vkCreateWin32SurfaceKHR(ctx.instance, &create_info, ctx.allocator, &gfx.surface) };
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
            const VkResult res_surface{ vkCreateXlibSurfaceKHR(ctx.instance, &create_info, ctx.allocator, &gfx.surface) };
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
            const VkResult res_surface{ vkCreateXcbSurfaceKHR(ctx.instance, &create_info, ctx.allocator, &gfx.surface) };
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
            const VkResult res_surface{ vkCreateWaylandSurfaceKHR(ctx.instance, &create_info, ctx.allocator, &gfx.surface) };
        #elif defined(WYN_COCOA)
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkMetalSurfaceCreateInfoEXT.html
            const VkMetalSurfaceCreateInfoEXT create_info{
                .sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT,
                .pNext = {},
                .flags = {},
                .pLayer = static_cast<const CAMetalLayer*>(cvk::create_metal_layer(gfx.context)),
            };
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCreateMetalSurfaceEXT.html
            const VkResult res_surface{ vkCreateMetalSurfaceEXT(ctx.instance, &create_info, ctx.allocator, &gfx.surface) };
        #else
            #error "Unimplemented"
        #endif
            CGE_ASSERT(res_surface == VK_SUCCESS);
        }
    }

    void deinit_surface(cvk::Context& ctx, cvk::Renderable& gfx) noexcept
    {
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroySurfaceKHR.html
        if (gfx.surface)
            vkDestroySurfaceKHR(ctx.instance, gfx.surface, ctx.allocator);
    }

    void update_surface_info(cvk::Context& ctx, cvk::Renderable& gfx, cvk::Offset device_idx) noexcept
    {
        VkPhysicalDevice& device{ ctx.devices[device_idx] };
        VkSurfaceKHR& surface{ gfx.surface };
        {
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetPhysicalDeviceSurfacePresentModesKHR.html
            Offset count;
            const VkResult res_count{ vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, nullptr) };
            CGE_ASSERT(res_count == VK_SUCCESS);
            const bool res_resize{ soa::realloc(count, gfx.ds_present_count, gfx.ds_present_array) };
            CGE_ASSERT(res_resize);
            const VkResult res_props{ vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &gfx.ds_present_count, gfx.ds_present_array) };
            CGE_ASSERT(res_props == VK_SUCCESS);
        }
        {
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetPhysicalDeviceSurfaceFormatsKHR.html
            Offset count;
            const VkResult res_count{ vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, nullptr) };
            CGE_ASSERT(res_count == VK_SUCCESS);
            const bool res_resize{ soa::realloc(count, gfx.ds_formats_count, gfx.ds_formats_array) };
            CGE_ASSERT(res_resize);
            const VkResult res_props{ vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &gfx.ds_formats_count, gfx.ds_formats_array) };
            CGE_ASSERT(res_props == VK_SUCCESS);
        }
        {
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetPhysicalDeviceSurfaceCapabilitiesKHR.html
            const VkResult res_caps{ vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &gfx.ds_capabilities) };
            CGE_ASSERT(res_caps == VK_SUCCESS);
        }
    }

    void reinit_device(cvk::Context& ctx, cvk::Renderable& gfx) noexcept
    {
        static constexpr std::array graphics_prios{ 1.0f };
        static constexpr std::array present_prios{ 1.0f };

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDeviceQueueCreateInfo.html
        const std::array queues_info{
            VkDeviceQueueCreateInfo{
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .pNext = {},
                .flags = {},
                .queueFamilyIndex = gfx.sel_graphics,
                .queueCount = graphics_prios.size(),
                .pQueuePriorities = graphics_prios.data(),
            },
            VkDeviceQueueCreateInfo{
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .pNext = {},
                .flags = {},
                .queueFamilyIndex = gfx.sel_present,
                .queueCount = present_prios.size(),
                .pQueuePriorities = present_prios.data(),
            },
        };
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceFeatures.html
        static constexpr VkPhysicalDeviceFeatures features{};

        const bool queues_unique{ gfx.sel_graphics != gfx.sel_present };
        const Offset queue_unique_count{ static_cast<Offset>(queues_unique ? 2 : 1) };
        //const Offset queue_concurrent_count{ static_cast<Offset>(queues_unique ? 2 : 0) };
        //const std::array queue_indices{ gfx.sel_graphics, gfx.sel_present };
        //const VkSharingMode queue_sharing{ queues_unique ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE };

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDeviceCreateInfo.html
        const VkDeviceCreateInfo device_info{
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = {},
            .flags = {},
            .queueCreateInfoCount = queue_unique_count,
            .pQueueCreateInfos = queues_info.data(),
            .enabledLayerCount = cvk::req_layers.size(),
            .ppEnabledLayerNames = cvk::req_layers.data(),
            .enabledExtensionCount = cvk::req_device_extensions.size(),
            .ppEnabledExtensionNames = cvk::req_device_extensions.data(),
            .pEnabledFeatures = &features,
        };

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCreateDevice.html
        const VkResult res_device{ vkCreateDevice(ctx.devices[gfx.sel_device], &device_info, ctx.allocator, &gfx.device) };
        CGE_ASSERT(res_device == VK_SUCCESS);

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetDeviceQueue.html
        vkGetDeviceQueue(gfx.device, gfx.sel_graphics, 0, &gfx.queue_graphics);
        vkGetDeviceQueue(gfx.device, gfx.sel_present, 0, &gfx.queue_present);
    }

    void deinit_device(cvk::Context& ctx [[maybe_unused]], cvk::Renderable& gfx) noexcept
    {
        soa::dealloc(gfx.ds_formats_count, gfx.ds_formats_array);
        soa::dealloc(gfx.ds_present_count, gfx.ds_present_array);

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroyDevice.html        
        if (gfx.device)
            vkDestroyDevice(gfx.device, ctx.allocator);
    }

    void reinit_buffers(cvk::Context& ctx, cvk::Renderable& gfx) noexcept
    {
        {
            constexpr VkDeviceSize MiB{ VkDeviceSize(1) << 20 };
            gfx.buffer_vtx_size = 32 * MiB;
            gfx.buffer_idx_size =  2 * MiB;
            gfx.buffer_stg_size = 16 * MiB;
            gfx.buffer_vtx_offs = 0;
            gfx.buffer_idx_offs = gfx.buffer_vtx_offs + gfx.buffer_vtx_size;
            gfx.buffer_stg_offs = gfx.buffer_idx_offs + gfx.buffer_idx_size;

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkBufferCreateInfo.html
            const VkBufferCreateInfo vtx_info{
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .pNext = {},
                .flags = {},
                .size = gfx.buffer_vtx_size,
                .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = {},
                .pQueueFamilyIndices = {},
            };
            const VkBufferCreateInfo idx_info{
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .pNext = {},
                .flags = {},
                .size = gfx.buffer_idx_size,
                .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = {},
                .pQueueFamilyIndices = {},
            };
            const VkBufferCreateInfo stg_info{
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .pNext = {},
                .flags = {},
                .size = gfx.buffer_stg_size,
                .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = {},
                .pQueueFamilyIndices = {},
            };
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCreateBuffer.html
            const VkResult res_vtx{ vkCreateBuffer(gfx.device, &vtx_info, ctx.allocator, &gfx.buffer_vtx) };
            CGE_ASSERT(res_vtx == VK_SUCCESS);
            const VkResult res_idx{ vkCreateBuffer(gfx.device, &idx_info, ctx.allocator, &gfx.buffer_idx) };
            CGE_ASSERT(res_idx == VK_SUCCESS);
            const VkResult res_stg{ vkCreateBuffer(gfx.device, &stg_info, ctx.allocator, &gfx.buffer_stg) };
            CGE_ASSERT(res_stg == VK_SUCCESS);

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetBufferMemoryRequirements.html
            vkGetBufferMemoryRequirements(gfx.device, gfx.buffer_vtx, &gfx.buffer_vtx_memreqs);
            vkGetBufferMemoryRequirements(gfx.device, gfx.buffer_idx, &gfx.buffer_idx_memreqs);
            vkGetBufferMemoryRequirements(gfx.device, gfx.buffer_stg, &gfx.buffer_stg_memreqs);
        }
        {
            const VkDeviceSize alloc_size{ gfx.buffer_vtx_memreqs.size + gfx.buffer_idx_memreqs.size + gfx.buffer_stg_memreqs.size };
            const Offset alloc_type{ gfx.buffer_vtx_memreqs.memoryTypeBits & gfx.buffer_idx_memreqs.memoryTypeBits & gfx.buffer_stg_memreqs.memoryTypeBits };
            constexpr VkMemoryPropertyFlags alloc_props{ VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };
            
            const VkPhysicalDeviceMemoryProperties& mem_props{ ctx.device_memory[gfx.sel_device] };
            const std::span<const VkMemoryType> mem_types{ mem_props.memoryTypes, mem_props.memoryTypeCount };
            const Offset mem_idx{ cvk::find_memtype(mem_types, alloc_type, alloc_props) };
            CGE_ASSERT(mem_idx != null_idx);

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkMemoryAllocateInfo.html
            const VkMemoryAllocateInfo alloc_info{
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .pNext = {},
                .allocationSize = alloc_size,
                .memoryTypeIndex = mem_idx,
            };
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkAllocateMemory.html
            const VkResult res_alloc{ vkAllocateMemory(gfx.device, &alloc_info, ctx.allocator, &gfx.buffer_memory) };
            CGE_ASSERT(res_alloc == VK_SUCCESS);

            gfx.buffer_capacity = alloc_size;
        }
        {
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkBindBufferMemory.html
            const VkResult res_vtx{ vkBindBufferMemory(gfx.device, gfx.buffer_vtx, gfx.buffer_memory, gfx.buffer_vtx_offs) };
            const VkResult res_idx{ vkBindBufferMemory(gfx.device, gfx.buffer_idx, gfx.buffer_memory, gfx.buffer_idx_offs) };
            const VkResult res_stg{ vkBindBufferMemory(gfx.device, gfx.buffer_stg, gfx.buffer_memory, gfx.buffer_stg_offs) };
            CGE_ASSERT(res_vtx == VK_SUCCESS);
            CGE_ASSERT(res_idx == VK_SUCCESS);
            CGE_ASSERT(res_stg == VK_SUCCESS);
        }
    }

    void deinit_buffers(cvk::Context& ctx [[maybe_unused]], cvk::Renderable& gfx) noexcept
    {
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroyBuffer.html
        if (gfx.buffer_stg)
            vkDestroyBuffer(gfx.device, gfx.buffer_stg, ctx.allocator);
        
        if (gfx.buffer_idx)
            vkDestroyBuffer(gfx.device, gfx.buffer_idx, ctx.allocator);
        
        if (gfx.buffer_vtx)
            vkDestroyBuffer(gfx.device, gfx.buffer_vtx, ctx.allocator);
        
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkFreeMemory.html
        if (gfx.buffer_memory)
            vkFreeMemory(gfx.device, gfx.buffer_memory, ctx.allocator);
    }

    void reinit_cmdpool(cvk::Context& ctx [[maybe_unused]], cvk::Renderable& gfx) noexcept
    {
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkCommandPoolCreateInfo.html
        const VkCommandPoolCreateInfo pool_info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = {},
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = gfx.sel_graphics,
        };
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCreateCommandPool.html
        const VkResult res_pool{ vkCreateCommandPool(gfx.device, &pool_info, ctx.allocator, &gfx.command_pool) };
        CGE_ASSERT(res_pool == VK_SUCCESS);
    }

    void deinit_cmdpool(cvk::Context& ctx [[maybe_unused]], cvk::Renderable& gfx) noexcept
    {
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroyCommandPool.html
        if (gfx.command_pool)
            vkDestroyCommandPool(gfx.device, gfx.command_pool, ctx.allocator);
    }

    void reinit_shaders(cvk::Context& ctx [[maybe_unused]], cvk::Renderable& gfx) noexcept
    {
        const shaderc::Compiler compiler{};
        const shaderc::CompileOptions options{};

        const std::string file_dir{ "shaders/glsl/" };
        cvk::compile_spirv(ctx, gfx, gfx.module_vertex, compiler, options, file_dir, "shader.vert", shaderc_shader_kind::shaderc_vertex_shader);
        cvk::compile_spirv(ctx, gfx, gfx.module_fragment, compiler, options, file_dir, "shader.frag", shaderc_shader_kind::shaderc_fragment_shader);
    }

    void deinit_shaders(cvk::Context& ctx [[maybe_unused]], cvk::Renderable& gfx) noexcept
    {
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroyShaderModule.html
        if (gfx.module_fragment)
            vkDestroyShaderModule(gfx.device, gfx.module_fragment, ctx.allocator);
        
        if (gfx.module_vertex)
            vkDestroyShaderModule(gfx.device, gfx.module_vertex, ctx.allocator);
    }

    void reinit_pipelines(cvk::Context& ctx [[maybe_unused]], cvk::Renderable& gfx) noexcept
    {
        // Render Pass
        {
            const VkSurfaceFormatKHR surface_format{ cvk::ideal_format({gfx.ds_formats_array, gfx.ds_formats_count}) };

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkAttachmentDescription.html
            const VkAttachmentDescription attachment_desc{
                .flags = {},
                .format = surface_format.format,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            };
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkAttachmentReference.html
            const VkAttachmentReference attachment_ref{
                .attachment = 0,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            };
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSubpassDescription.html
            const VkSubpassDescription subpass_desc{
                .flags = {},
                .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                .inputAttachmentCount = 0,
                .pInputAttachments = {},
                .colorAttachmentCount = 1,
                .pColorAttachments = &attachment_ref,
                .pResolveAttachments = {},
                .pDepthStencilAttachment = {},
                .preserveAttachmentCount = 0,
                .pPreserveAttachments = {},
            };
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSubpassDependency.html
            const VkSubpassDependency subpass_dep{
                .srcSubpass = VK_SUBPASS_EXTERNAL,
                .dstSubpass = 0,
                .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .srcAccessMask = {},
                .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                .dependencyFlags = {},
            };
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkRenderPassCreateInfo.html
            const VkRenderPassCreateInfo pass_info{
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                .pNext = {},
                .flags = {},
                .attachmentCount = 1,
                .pAttachments = &attachment_desc,
                .subpassCount = 1,
                .pSubpasses = &subpass_desc,
                .dependencyCount = 1,
                .pDependencies = &subpass_dep,
            };
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCreateRenderPass.html
            const VkResult res_pass{ vkCreateRenderPass(gfx.device, &pass_info, ctx.allocator, &gfx.render_pass) };
            CGE_ASSERT(res_pass == VK_SUCCESS);
        }

        // Pipeline Layout
        {
            constexpr Offset max_frames{ 1 };

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDescriptorSetLayoutBinding.html
            const VkDescriptorSetLayoutBinding sampler_binding{
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = {},
            };
            const std::array descriptor_bindings{ sampler_binding };

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDescriptorSetLayoutCreateInfo.html
            const VkDescriptorSetLayoutCreateInfo descriptor_layout_info{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .pNext = {},
                .flags = {},
                .bindingCount = descriptor_bindings.size(),
                .pBindings = descriptor_bindings.data(),
            };
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCreateDescriptorSetLayout.html
            const VkResult res_descset_layout{ vkCreateDescriptorSetLayout(gfx.device, &descriptor_layout_info, ctx.allocator, &gfx.descriptor_layout) };
            CGE_ASSERT(res_descset_layout == VK_SUCCESS);

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDescriptorPoolSize.html
            const VkDescriptorPoolSize sampler_pool_size{
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = max_frames,
            };
            const std::array descriptor_pool_sizes{ sampler_pool_size };

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDescriptorPoolCreateInfo.html
            const VkDescriptorPoolCreateInfo descriptor_pool_info{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                .pNext = {},
                .flags = {},
                .maxSets = max_frames,
                .poolSizeCount = descriptor_pool_sizes.size(),
                .pPoolSizes = descriptor_pool_sizes.data(),
            };
            const VkResult res_pool{ vkCreateDescriptorPool(gfx.device, &descriptor_pool_info, ctx.allocator, &gfx.descriptor_pool) };
            CGE_ASSERT(res_pool == VK_SUCCESS);

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDescriptorSetLayout.html
            const std::array set_layouts{ gfx.descriptor_layout };

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPushConstantRange.html
            const std::array<VkPushConstantRange, 0> pc_ranges{};
            
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDescriptorSetAllocateInfo.html
            const VkDescriptorSetAllocateInfo alloc_info{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .pNext = {},
                .descriptorPool = gfx.descriptor_pool,
                .descriptorSetCount = set_layouts.size(),
                .pSetLayouts = set_layouts.data(),
            };
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkAllocateDescriptorSets.html
            const VkResult res_alloc{ vkAllocateDescriptorSets(gfx.device, &alloc_info, &gfx.descriptor_set) };
            CGE_ASSERT(res_alloc == VK_SUCCESS);

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPipelineLayoutCreateInfo.html
            const VkPipelineLayoutCreateInfo pipeline_layout_info{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .pNext = {},
                .flags = {},
                .setLayoutCount = set_layouts.size(),
                .pSetLayouts = set_layouts.data(),
                .pushConstantRangeCount = pc_ranges.size(),
                .pPushConstantRanges = pc_ranges.data(),
            };
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCreatePipelineLayout.html
            const VkResult res_layout{ vkCreatePipelineLayout(gfx.device, &pipeline_layout_info, ctx.allocator, &gfx.pipeline_layout) };
            CGE_ASSERT(res_layout == VK_SUCCESS);
        }

        // Graphics Pipelines
        {
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDynamicState.html
            const std::array dynamic_states{
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR,
            };

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPipelineDynamicStateCreateInfo.html
            const VkPipelineDynamicStateCreateInfo dynamic_info{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                .pNext = {},
                .flags = {},
                .dynamicStateCount = dynamic_states.size(),
                .pDynamicStates = dynamic_states.data(),
            };

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPipelineShaderStageCreateInfo.html
            const std::array shader_stages{
                VkPipelineShaderStageCreateInfo{
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .pNext = {},
                    .flags = {},
                    .stage = VK_SHADER_STAGE_VERTEX_BIT,
                    .module = gfx.module_vertex,
                    .pName = shader_entry,
                    .pSpecializationInfo = {},
                },
                VkPipelineShaderStageCreateInfo{
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .pNext = {},
                    .flags = {},
                    .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .module = gfx.module_fragment,
                    .pName = shader_entry,
                    .pSpecializationInfo = {},
                },
            };

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkVertexInputBindingDescription.html
            constexpr VkVertexInputBindingDescription binding_vertex{
                .binding = 0,
                .stride = Offset(sizeof(cge::Vertex)),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
            };
            constexpr std::array bindings{ binding_vertex };

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkVertexInputAttributeDescription.html
            constexpr VkVertexInputAttributeDescription attribute_xyzw{
                .location = 0,
                .binding = binding_vertex.binding,
                .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                .offset = {},
            };
            constexpr VkVertexInputAttributeDescription attribute_uv{
                .location = 1,
                .binding = binding_vertex.binding,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = Offset(attribute_xyzw.offset + sizeof(cge::vec4)),
            };
            constexpr VkVertexInputAttributeDescription attribute_st{
                .location = 2,
                .binding = binding_vertex.binding,
                .format = VK_FORMAT_R32G32_UINT,
                .offset = Offset(attribute_uv.offset + sizeof(cge::vec2)),
            };
            constexpr std::array attributes{ attribute_xyzw, attribute_uv, attribute_st };

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPipelineVertexInputStateCreateInfo.html
            const VkPipelineVertexInputStateCreateInfo vertex_info{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                .pNext = {},
                .flags = {},
                .vertexBindingDescriptionCount = bindings.size(),
                .pVertexBindingDescriptions = bindings.data(),
                .vertexAttributeDescriptionCount = attributes.size(),
                .pVertexAttributeDescriptions = attributes.data(),
            };

            const wyn_size_t wyn_size{ wyn_window_size(gfx.window) };
            gfx.surface_extent = { .width = Offset(wyn_size.w), .height = Offset(wyn_size.h) };

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkExtent2D.html
            const VkExtent2D ideal_res{ cvk::ideal_resolution(gfx.surface_extent, gfx.ds_capabilities) };
            const VkExtent2D image_res{ .width = (ideal_res.width ? ideal_res.width : 1 ), .height = (ideal_res.height ? ideal_res.height : 1 ) };
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkViewport.html
            const VkViewport viewport{
                .x = 0.0f,
                .y = 0.0f,
                .width = float(image_res.width),
                .height = float(image_res.height),
                .minDepth = 0.0f,
                .maxDepth = 0.0f,
            };
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkRect2D.html
            const VkRect2D scissor{
                .offset = { 0, 0 },
                .extent = image_res,
            };
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPipelineViewportStateCreateInfo.html
            const VkPipelineViewportStateCreateInfo viewport_info{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                .pNext = {},
                .flags = {},
                .viewportCount = 1,
                .pViewports = &viewport,
                .scissorCount = 1,
                .pScissors = &scissor,
            };

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPipelineRasterizationStateCreateInfo.html
            const VkPipelineRasterizationStateCreateInfo rasterizer_info{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                .pNext = {},
                .flags = {},
                .depthClampEnable = VK_FALSE,
                .rasterizerDiscardEnable = VK_FALSE,
                .polygonMode = VK_POLYGON_MODE_FILL,
                .cullMode = VK_CULL_MODE_NONE,
                .frontFace = VK_FRONT_FACE_CLOCKWISE,
                .depthBiasEnable = VK_FALSE,
                .depthBiasConstantFactor = 0.0f,
                .depthBiasClamp = 0.0f,
                .depthBiasSlopeFactor = 0.0f,
                .lineWidth = 1.0f,
            };
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPipelineMultisampleStateCreateInfo.html
            const VkPipelineMultisampleStateCreateInfo multisample_info{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                .pNext = {},
                .flags = {},
                .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
                .sampleShadingEnable = VK_FALSE,
                .minSampleShading = {},
                .pSampleMask = {},
                .alphaToCoverageEnable = VK_FALSE,
                .alphaToOneEnable = VK_FALSE,
            };

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPipelineColorBlendAttachmentState.html
            const VkPipelineColorBlendAttachmentState blend_attachment{
                .blendEnable = VK_TRUE,
                .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .colorBlendOp = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                .alphaBlendOp = VK_BLEND_OP_ADD,
                .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
            };
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPipelineColorBlendStateCreateInfo.html
            const VkPipelineColorBlendStateCreateInfo blend_info{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                .pNext = {},
                .flags = {},
                .logicOpEnable = VK_FALSE,
                .logicOp = VK_LOGIC_OP_CLEAR,
                .attachmentCount = 1,
                .pAttachments = &blend_attachment,
                .blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f },
            };

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkGraphicsPipelineCreateInfo.html
            const VkGraphicsPipelineCreateInfo default_pipeline{
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                .pNext = {},
                .flags = {},
                .stageCount = shader_stages.size(),
                .pStages = shader_stages.data(),
                .pVertexInputState = &vertex_info,
                .pInputAssemblyState = {},
                .pTessellationState = {},
                .pViewportState = &viewport_info,
                .pRasterizationState = &rasterizer_info,
                .pMultisampleState = &multisample_info,
                .pDepthStencilState = {},
                .pColorBlendState = &blend_info,
                .pDynamicState = &dynamic_info,
                .layout = gfx.pipeline_layout,
                .renderPass = gfx.render_pass,
                .subpass = {},
                .basePipelineHandle = {},
                .basePipelineIndex = {},
            };

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPipelineInputAssemblyStateCreateInfo.html
            constexpr VkPipelineInputAssemblyStateCreateInfo assembly_point_list{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                .pNext = {},
                .flags = {},
                .topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
                .primitiveRestartEnable = VK_FALSE,
            };
            constexpr VkPipelineInputAssemblyStateCreateInfo assembly_line_list{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                .pNext = {},
                .flags = {},
                .topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
                .primitiveRestartEnable = VK_FALSE,
            };
            constexpr VkPipelineInputAssemblyStateCreateInfo assembly_triangle_list{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                .pNext = {},
                .flags = {},
                .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                .primitiveRestartEnable = VK_FALSE,
            };
            constexpr VkPipelineInputAssemblyStateCreateInfo assembly_line_strip{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                .pNext = {},
                .flags = {},
                .topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
                .primitiveRestartEnable = VK_TRUE,
            };
            constexpr VkPipelineInputAssemblyStateCreateInfo assembly_triangle_strip{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                .pNext = {},
                .flags = {},
                .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
                .primitiveRestartEnable = VK_TRUE,
            };
            constexpr VkPipelineInputAssemblyStateCreateInfo assembly_triangle_fan{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                .pNext = {},
                .flags = {},
            #ifdef __APPLE__
                .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                .primitiveRestartEnable = VK_FALSE,
            #else
                .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,
                .primitiveRestartEnable = VK_TRUE,
            #endif
            };

            VkGraphicsPipelineCreateInfo pipeline_point_list{ default_pipeline };
            pipeline_point_list.pInputAssemblyState = &assembly_point_list;
            VkGraphicsPipelineCreateInfo pipeline_line_list{ default_pipeline };
            pipeline_line_list.pInputAssemblyState = &assembly_line_list;
            VkGraphicsPipelineCreateInfo pipeline_triangle_list{ default_pipeline };
            pipeline_triangle_list.pInputAssemblyState = &assembly_triangle_list;
            VkGraphicsPipelineCreateInfo pipeline_line_strip{ default_pipeline };
            pipeline_line_strip.pInputAssemblyState = &assembly_line_strip;
            VkGraphicsPipelineCreateInfo pipeline_triangle_strip{ default_pipeline };
            pipeline_triangle_strip.pInputAssemblyState = &assembly_triangle_strip;
            VkGraphicsPipelineCreateInfo pipeline_triangle_fan{ default_pipeline };
            pipeline_triangle_fan.pInputAssemblyState = &assembly_triangle_fan;
            
            const std::array<VkGraphicsPipelineCreateInfo, num_pipelines> pipeline_infos{
                pipeline_triangle_fan,
                pipeline_triangle_strip,
                pipeline_triangle_list,
                pipeline_line_strip,
                pipeline_line_list,
                pipeline_point_list,
            };

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCreateGraphicsPipelines.html
            const VkResult res_graphics{ vkCreateGraphicsPipelines(gfx.device, nullptr, pipeline_infos.size(), pipeline_infos.data(), ctx.allocator, gfx.pipelines_graphics) };
            CGE_ASSERT(res_graphics == VK_SUCCESS);
        }
    }

    void deinit_pipelines(cvk::Context& ctx [[maybe_unused]], cvk::Renderable& gfx) noexcept
    {
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroyPipeline.html
        for (const VkPipeline pipeline : gfx.pipelines_graphics)
        {
            if (pipeline)
                vkDestroyPipeline(gfx.device, pipeline, ctx.allocator);
        }
        
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroyDescriptorPool.html
        if (gfx.descriptor_pool)
            vkDestroyDescriptorPool(gfx.device, gfx.descriptor_pool, ctx.allocator);
        
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroyDescriptorSetLayout.html
        if (gfx.descriptor_layout)
            vkDestroyDescriptorSetLayout(gfx.device, gfx.descriptor_layout, ctx.allocator);
        
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroyPipelineLayout.html
        if (gfx.pipeline_layout)
            vkDestroyPipelineLayout(gfx.device, gfx.pipeline_layout, ctx.allocator);
        
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroyRenderPass.html
        if (gfx.render_pass)
            vkDestroyRenderPass(gfx.device, gfx.render_pass, ctx.allocator);
    }

    void remake_swapchain(cvk::Context& ctx [[maybe_unused]], cvk::Renderable& gfx, const bool vsync) noexcept
    {
        const VkSwapchainKHR old_swapchain{ gfx.swapchain };

        const std::span<const VkSurfaceFormatKHR> ds_formats{ gfx.ds_formats_array, gfx.ds_formats_count };
        const std::span<const VkPresentModeKHR>   ds_present{ gfx.ds_present_array, gfx.ds_present_count };

        const VkExtent2D ideal_res{ cvk::ideal_resolution(gfx.surface_extent, gfx.ds_capabilities) };
        if ((ideal_res.width == 0) || (ideal_res.height == 0)) return;

        const VkSurfaceFormatKHR best_format{ cvk::ideal_format(ds_formats) };
        const VkPresentModeKHR best_present{ cvk::ideal_present(ds_present, vsync) };

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSurfaceCapabilitiesKHR.html#_description
        const Offset min_images{ gfx.ds_capabilities.minImageCount };
        const Offset max_images{ gfx.ds_capabilities.maxImageCount };
        const Offset req_images{ min_images + 1};
        const Offset num_images{ (max_images == 0) ? req_images : std::min(req_images, max_images) };

        const bool queues_unique{ gfx.sel_graphics != gfx.sel_present };
        const Offset queue_concurrent_count{ static_cast<Offset>(queues_unique ? 2 : 0) };
        const std::array queue_indices{ gfx.sel_graphics, gfx.sel_present };
        const Offset* queue_indices_ptr{ queues_unique ? queue_indices.data() : nullptr };
        const VkSharingMode queue_sharing{ queues_unique ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE };

        {
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSwapchainCreateInfoKHR.html#_description
            const VkSwapchainCreateInfoKHR swapchain_info{
                .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                .pNext = {},
                .flags = {},
                .surface = gfx.surface,
                .minImageCount = num_images,
                .imageFormat = best_format.format,
                .imageColorSpace = best_format.colorSpace,
                .imageExtent = ideal_res,
                .imageArrayLayers = 1,
                .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                .imageSharingMode = queue_sharing,
                .queueFamilyIndexCount = queue_concurrent_count,
                .pQueueFamilyIndices = queue_indices_ptr,
                .preTransform = gfx.ds_capabilities.currentTransform,
                .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                .presentMode = best_present,
                .clipped = VK_TRUE,
                .oldSwapchain = old_swapchain,
            };
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCreateSwapchainKHR.html
            const VkResult res_swapchain{ vkCreateSwapchainKHR(gfx.device, &swapchain_info, ctx.allocator, &gfx.swapchain) };
            CGE_ASSERT(res_swapchain == VK_SUCCESS);
        }
        if (old_swapchain != VK_NULL_HANDLE)
        {
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDeviceWaitIdle.html
            const VkResult res_wait{ vkDeviceWaitIdle(gfx.device) };
            CGE_ASSERT(res_wait == VK_SUCCESS);

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkFreeCommandBuffers.html
            vkFreeCommandBuffers(gfx.device, gfx.command_pool, Offset(gfx.frame_count), gfx.frame_commands);
            for (Offset idx{}; idx < gfx.frame_count; ++idx)
            {
                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroyFramebuffer.html
                vkDestroyFramebuffer(gfx.device, gfx.frame_buffer[idx], ctx.allocator);
                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroyImageView.html
                vkDestroyImageView(gfx.device, gfx.frame_view[idx], ctx.allocator);
                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroySemaphore.html
                vkDestroySemaphore(gfx.device, gfx.frame_sem_image[idx], ctx.allocator);
                vkDestroySemaphore(gfx.device, gfx.frame_sem_render[idx], ctx.allocator);
                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroyFence.html
                vkDestroyFence(gfx.device, gfx.frame_fence[idx], ctx.allocator);
            }

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroySwapchainKHR.html
            vkDestroySwapchainKHR(gfx.device, old_swapchain, ctx.allocator);
        }
        {
            {
                gfx.frame_idx = 0;

                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetSwapchainImagesKHR.html
                Offset count;
                const VkResult res_count{ vkGetSwapchainImagesKHR(gfx.device, gfx.swapchain, &count, nullptr) };
                CGE_ASSERT(res_count == VK_SUCCESS);
                const bool res_resize{
                    soa::realloc(
                        count, gfx.frame_count,
                        gfx.frame_image,
                        gfx.frame_view,
                        gfx.frame_buffer,
                        gfx.frame_commands,
                        gfx.frame_fence,
                        gfx.frame_sem_render,
                        gfx.frame_sem_image
                    )
                };
                CGE_ASSERT(res_resize);
                const VkResult res_images{ vkGetSwapchainImagesKHR(gfx.device, gfx.swapchain, &gfx.frame_count, gfx.frame_image) };
                CGE_ASSERT(res_images == VK_SUCCESS);
            }

            for (Offset idx{}; idx < gfx.frame_count; ++idx)
            {
                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkFenceCreateInfo.html
                constexpr VkFenceCreateInfo fence_info{
                    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                    .pNext = {},
                    .flags = VK_FENCE_CREATE_SIGNALED_BIT,
                };
                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSemaphoreCreateInfo.html
                constexpr VkSemaphoreCreateInfo sem_info{
                    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                    .pNext = {},
                    .flags = {},
                };
                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCreateFence.html
                const VkResult res_fence{ vkCreateFence(gfx.device, &fence_info, ctx.allocator, &gfx.frame_fence[idx]) };
                CGE_ASSERT(res_fence == VK_SUCCESS);
                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCreateSemaphore.html
                const VkResult res_sem_render{ vkCreateSemaphore(gfx.device, &sem_info, ctx.allocator, &gfx.frame_sem_render[idx]) };
                CGE_ASSERT(res_sem_render == VK_SUCCESS);
                const VkResult res_sem_image{ vkCreateSemaphore(gfx.device, &sem_info, ctx.allocator, &gfx.frame_sem_image[idx]) };
                CGE_ASSERT(res_sem_image == VK_SUCCESS);

                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImageViewCreateInfo.html
                const VkImageViewCreateInfo view_info{
                    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                    .pNext = {},
                    .flags = {},
                    .image = gfx.frame_image[idx],
                    .viewType = VK_IMAGE_VIEW_TYPE_2D,
                    .format = best_format.format,
                    .components = {},
                    .subresourceRange = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = {},
                        .levelCount = 1,
                        .baseArrayLayer = {},
                        .layerCount = 1,
                    },
                };
                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCreateImageView.html
                const VkResult res_view{ vkCreateImageView(gfx.device, &view_info, ctx.allocator, &gfx.frame_view[idx]) };
                CGE_ASSERT(res_view == VK_SUCCESS);

                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkFramebufferCreateInfo.html
                const VkFramebufferCreateInfo buffer_info{
                    .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                    .pNext = {},
                    .flags = {},
                    .renderPass = gfx.render_pass,
                    .attachmentCount = 1,
                    .pAttachments = &gfx.frame_view[idx],
                    .width = ideal_res.width,
                    .height = ideal_res.height,
                    .layers = 1,
                };
                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCreateFramebuffer.html
                const VkResult res_buffer{ vkCreateFramebuffer(gfx.device, &buffer_info, ctx.allocator, &gfx.frame_buffer[idx]) };
                CGE_ASSERT(res_buffer == VK_SUCCESS);
            }

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkCommandBufferAllocateInfo.html
            const VkCommandBufferAllocateInfo alloc_info{
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .pNext = {},
                .commandPool = gfx.command_pool,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = gfx.frame_count,
            };
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkAllocateCommandBuffers.html
            const VkResult res_alloc{ vkAllocateCommandBuffers(gfx.device, &alloc_info, gfx.frame_commands) };
            CGE_ASSERT(res_alloc == VK_SUCCESS);
        }
    }

    void deinit_swapchain(cvk::Context& ctx [[maybe_unused]], cvk::Renderable& gfx) noexcept
    {
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkFreeCommandBuffers.html
        if (gfx.frame_commands)
            vkFreeCommandBuffers(gfx.device, gfx.command_pool, gfx.frame_count, gfx.frame_commands);
        
        for (Offset idx{}; idx < gfx.frame_count; ++idx)
        {
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroyFramebuffer.html
            if (gfx.frame_buffer[idx])
                vkDestroyFramebuffer(gfx.device, gfx.frame_buffer[idx], ctx.allocator);
            
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroyImageView.html
            if (gfx.frame_view[idx])
                vkDestroyImageView(gfx.device, gfx.frame_view[idx], ctx.allocator);
            
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroySemaphore.html
            if (gfx.frame_sem_image[idx])
                vkDestroySemaphore(gfx.device, gfx.frame_sem_image[idx], ctx.allocator);
            
            if (gfx.frame_sem_render[idx])
                vkDestroySemaphore(gfx.device, gfx.frame_sem_render[idx], ctx.allocator);
            
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroyFence.html
            if (gfx.frame_fence[idx])
                vkDestroyFence(gfx.device, gfx.frame_fence[idx], ctx.allocator);
        }
        soa::dealloc(gfx.frame_count, gfx.frame_image);
        
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroySwapchainKHR.html
        if (gfx.swapchain)
            vkDestroySwapchainKHR(gfx.device, gfx.swapchain, ctx.allocator);
    }
}

namespace cvk
{
    void reinit_atlases(cvk::Context& ctx, cvk::Renderable& gfx) noexcept
    {
        if (gfx.atlas_count > 0)
        {
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDeviceWaitIdle.html
            const VkResult res_wait{ vkDeviceWaitIdle(gfx.device) };
            (void)res_wait;

            for (Offset idx{}; idx < gfx.atlas_count; ++idx)
            { 
                cvk::destroy_atlas(ctx, gfx, idx);
            }
        }

        {
            const Offset count{ 1 };
            const bool res_resize{
                soa::realloc(count, gfx.atlas_count,
                    gfx.atlas_image,
                    gfx.atlas_view,
                    gfx.atlas_sampler,
                    gfx.atlas_extent,
                    gfx.atlas_memory,
                    gfx.atlas_memreqs
                )
            };
            CGE_ASSERT(res_resize);

            for (Offset idx{}; idx < gfx.atlas_count; ++idx)
            {
                gfx.atlas_extent[idx] = {};
                gfx.atlas_image[idx] = {};
                gfx.atlas_view[idx] = {};
                gfx.atlas_sampler[idx] = {};
                gfx.atlas_memory[idx] = {};
            }
        }

        for (Offset idx{}; idx < gfx.atlas_count; ++idx)
        {
            cvk::upload_texture(ctx, gfx, idx, cvk::default_texture);
        }
    }

    void deinit_atlases(cvk::Context& ctx, cvk::Renderable& gfx) noexcept
    {
        for (Offset idx{}; idx < gfx.atlas_count; ++idx)
        {
            cvk::destroy_atlas(ctx, gfx, idx);
        }
        soa::dealloc(gfx.atlas_count, gfx.atlas_image);
    }

    void destroy_atlas(Context& ctx [[maybe_unused]], Renderable& gfx, const Offset atlas_idx) noexcept
    {
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroySampler.html
        if (gfx.atlas_sampler[atlas_idx])
            vkDestroySampler(gfx.device, gfx.atlas_sampler[atlas_idx], ctx.allocator);

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroyImageView.html
        if (gfx.atlas_view[atlas_idx])
            vkDestroyImageView(gfx.device, gfx.atlas_view[atlas_idx], ctx.allocator);

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroyImage.html
        if (gfx.atlas_image[atlas_idx])
            vkDestroyImage(gfx.device, gfx.atlas_image[atlas_idx], ctx.allocator);

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkFreeMemory.html
        if (gfx.atlas_memory[atlas_idx])
            vkFreeMemory(gfx.device, gfx.atlas_memory[atlas_idx], ctx.allocator);
    }

    void upload_texture(Context& ctx, Renderable& gfx, const Offset atlas_idx, cge::Texture texture) noexcept
    {
        CGE_ASSERT(atlas_idx < gfx.atlas_count);

        if (texture.empty()) texture = default_texture;

        cvk::destroy_atlas(ctx, gfx, atlas_idx);
        cvk::create_atlas(ctx, gfx, atlas_idx, texture);
        cvk::transition_atlas(ctx, gfx, atlas_idx, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        cvk::stage_texture(ctx, gfx, atlas_idx, texture);
        cvk::transition_atlas(ctx, gfx, atlas_idx, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        cvk::update_descriptors(ctx, gfx, atlas_idx);
    }

    void create_atlas(cvk::Context& ctx, cvk::Renderable& gfx, cvk::Offset atlas_idx, cge::Texture tex) noexcept
    {
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkExtent3D.html
        const VkExtent3D tex_extent{ .width = tex.width, .height = tex.height, .depth = 1 };

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkFormat.html
        const VkFormat tex_format{ VK_FORMAT_R8G8B8A8_UNORM };

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkComponentMapping.html
        const VkComponentMapping tex_remapping{
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY,
        }; 

        {
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImageCreateInfo.html
            const VkImageCreateInfo image_info{
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .pNext = {},
                .flags = {},
                .imageType = VK_IMAGE_TYPE_2D,
                .format = tex_format,
                .extent = tex_extent,
                .mipLevels = 1,
                .arrayLayers = 1,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = {},
                .pQueueFamilyIndices = {},
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            };
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCreateImage.html
            const VkResult res_image{ vkCreateImage(gfx.device, &image_info, ctx.allocator, &gfx.atlas_image[atlas_idx]) };
            CGE_ASSERT(res_image == VK_SUCCESS);

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetImageMemoryRequirements.html
            vkGetImageMemoryRequirements(gfx.device, gfx.atlas_image[atlas_idx], &gfx.atlas_memreqs[atlas_idx]);
        }
        {
            const VkPhysicalDeviceMemoryProperties& mem_props{ ctx.device_memory[gfx.sel_device] };
            const std::span<const VkMemoryType> mem_types{ mem_props.memoryTypes, mem_props.memoryTypeCount };
            const Offset mem_idx{ cvk::find_memtype(mem_types, gfx.atlas_memreqs[atlas_idx].memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) };
            CGE_ASSERT(mem_idx != null_idx);

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkMemoryAllocateInfo.html
            const VkMemoryAllocateInfo alloc_info{
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .pNext = {},
                .allocationSize = gfx.atlas_memreqs[atlas_idx].size,
                .memoryTypeIndex = mem_idx,
            };
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkAllocateMemory.html
            const VkResult res_alloc{ vkAllocateMemory(gfx.device, &alloc_info, ctx.allocator, &gfx.atlas_memory[atlas_idx]) };
            CGE_ASSERT(res_alloc == VK_SUCCESS);

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkBindImageMemory.html
            const VkResult res_bind{ vkBindImageMemory(gfx.device, gfx.atlas_image[atlas_idx], gfx.atlas_memory[atlas_idx], 0) };
            CGE_ASSERT(res_bind == VK_SUCCESS);
        }        
        {
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImageViewCreateInfo.html
            const VkImageViewCreateInfo view_info{
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext = {},
                .flags = {},
                .image = gfx.atlas_image[atlas_idx],
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = tex_format,
                .components = tex_remapping,
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            };
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCreateImageView.html
            const VkResult res_view{ vkCreateImageView(gfx.device, &view_info, ctx.allocator, &gfx.atlas_view[atlas_idx]) };
            CGE_ASSERT(res_view == VK_SUCCESS);
        }
        {
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSamplerCreateInfo.html
            const VkSamplerCreateInfo sampler_info{
                .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                .pNext = {},
                .flags = {},
                .magFilter = VK_FILTER_NEAREST,
                .minFilter = VK_FILTER_NEAREST,
                .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
                .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                .mipLodBias = 0.0f,
                .anisotropyEnable = VK_FALSE,
                .maxAnisotropy = 1.0f,
                .compareEnable = VK_FALSE,
                .compareOp = VK_COMPARE_OP_ALWAYS,
                .minLod = 0.0f,
                .maxLod = 0.0f,
                .borderColor = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK,
                .unnormalizedCoordinates = VK_FALSE,
            };
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCreateSampler.html
            const VkResult res_sampler{ vkCreateSampler(gfx.device, &sampler_info, ctx.allocator, &gfx.atlas_sampler[atlas_idx]) };
            CGE_ASSERT(res_sampler == VK_SUCCESS);
        }
    }

    void transition_atlas(cvk::Context& ctx [[maybe_unused]], cvk::Renderable& gfx, cvk::Offset atlas_idx, const VkImageLayout old_layout, const VkImageLayout new_layout) noexcept
    {
        VkAccessFlags src_access{};
        VkAccessFlags dst_access{};
        VkPipelineStageFlags src_stage{};
        VkPipelineStageFlags dst_stage{};

        if ((old_layout == VK_IMAGE_LAYOUT_UNDEFINED) && (new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL))
        {
            src_access = 0;
            dst_access = VK_ACCESS_TRANSFER_WRITE_BIT;
            src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if ((old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) && (new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL))
        {
            src_access = VK_ACCESS_TRANSFER_WRITE_BIT;
            dst_access = VK_ACCESS_SHADER_READ_BIT;
            src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else CGE_ASSERT(false);

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImageMemoryBarrier.html
        const VkImageMemoryBarrier barrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = {},
            .srcAccessMask = src_access,
            .dstAccessMask = dst_access,
            .oldLayout = old_layout,
            .newLayout = new_layout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = gfx.atlas_image[atlas_idx],
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };

        cvk::single_commands(ctx, gfx, [&](const VkCommandBuffer command_buffer){
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCmdPipelineBarrier.html
            vkCmdPipelineBarrier(command_buffer, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        });
    }

    void stage_texture(cvk::Context& ctx, cvk::Renderable& gfx, const cvk::Offset atlas_idx, const cge::Texture tex) noexcept
    {
        {
            const VkDeviceSize tex_size{ VkDeviceSize(tex.size()) };
            CGE_ASSERT(tex_size <= gfx.buffer_stg_size);

            const VkDeviceMemory buffer_memory{ gfx.buffer_memory };
            const VkDeviceSize buffer_offs{ gfx.buffer_stg_offs };
            const VkDeviceSize buffer_size{ gfx.buffer_stg_size };
            {
                void* buffer{};

                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkMapMemory.html
                const VkResult res_map{ vkMapMemory(gfx.device, buffer_memory, buffer_offs, buffer_size, 0, &buffer) };
                CGE_ASSERT(res_map == VK_SUCCESS);

                VkDeviceSize offset{};
                (void)cvk::map_bytes(buffer_size, buffer, offset, tex.as_bytes());

                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkUnmapMemory.html
                vkUnmapMemory(gfx.device, buffer_memory);
            }
        }
        {
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkBufferImageCopy.html
            const VkBufferImageCopy region{
                .bufferOffset = {},
                .bufferRowLength = {},
                .bufferImageHeight = {},
                .imageSubresource = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = 0,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
                .imageOffset = {},
                .imageExtent = {
                    .width = tex.width,
                    .height = tex.height,
                    .depth = 1,
                },
            };

            cvk::single_commands(ctx, gfx, [&](const VkCommandBuffer command_buffer){
                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCmdCopyBufferToImage.html
                vkCmdCopyBufferToImage(command_buffer, gfx.buffer_stg, gfx.atlas_image[atlas_idx], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
            });
        }
    }

    void update_descriptors(cvk::Context& ctx [[maybe_unused]], cvk::Renderable& gfx, cvk::Offset atlas_idx) noexcept
    {
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDescriptorImageInfo.html
        const VkDescriptorImageInfo image_info{
            .sampler = gfx.atlas_sampler[atlas_idx],
            .imageView = gfx.atlas_view[atlas_idx],
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkWriteDescriptorSet.html
        const VkWriteDescriptorSet sampler_write{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = {},
            .dstSet = gfx.descriptor_set,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &image_info,
            .pBufferInfo = {},
            .pTexelBufferView = {},
        };

        const std::array writes{ sampler_write };
        const std::array<VkCopyDescriptorSet, 0> copies{};

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkUpdateDescriptorSets.html
        vkUpdateDescriptorSets(gfx.device, Offset(writes.size()), writes.data(), Offset(copies.size()), copies.data());
    }

    void single_commands(cvk::Context& ctx [[maybe_unused]], cvk::Renderable& gfx, auto&& callback) noexcept
    {
        const VkCommandPool command_pool{ gfx.command_pool };
        const VkQueue queue{ gfx.queue_graphics };
        VkCommandBuffer command_buffer{};

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkCommandBufferAllocateInfo.html
        const VkCommandBufferAllocateInfo alloc_info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = {},
            .commandPool = command_pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkAllocateCommandBuffers.html
        const VkResult res_alloc{ vkAllocateCommandBuffers(gfx.device, &alloc_info, &command_buffer) };
        CGE_ASSERT(res_alloc == VK_SUCCESS);
        
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkCommandBufferBeginInfo.html
        const VkCommandBufferBeginInfo begin_info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = {},
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo = {},
        };
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkBeginCommandBuffer.html
        vkBeginCommandBuffer(command_buffer, &begin_info);

        try { callback(command_buffer); } catch (...) {}

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkEndCommandBuffer.html
        vkEndCommandBuffer(command_buffer);
        
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSubmitInfo.html
        const VkSubmitInfo submit_info{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = {},
            .waitSemaphoreCount = {},
            .pWaitSemaphores = {},
            .pWaitDstStageMask = {},
            .commandBufferCount = 1,
            .pCommandBuffers = &command_buffer,
            .signalSemaphoreCount = {},
            .pSignalSemaphores = {},
        };

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkQueueSubmit.html
        const VkResult res_submit{ vkQueueSubmit(queue, 1, &submit_info, nullptr) };
        CGE_ASSERT(res_submit == VK_SUCCESS);

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkQueueWaitIdle.html
        const VkResult res_wait{ vkQueueWaitIdle(queue) };
        CGE_ASSERT(res_wait == VK_SUCCESS);

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkFreeCommandBuffers.html
        vkFreeCommandBuffers(gfx.device, command_pool, 1, &command_buffer);
    }
}

namespace cvk
{
    void select_device(cvk::Context& ctx, cvk::Renderable& gfx) noexcept
    {
        Offset device_idx{};
        Offset graphics_idx{};
        Offset present_idx{};
        std::uint64_t device_rank{};
        std::uint64_t graphics_rank{};
        std::uint64_t present_rank{};

        const Offset device_count{ ctx.device_count };
        for (Offset di{}; di < device_count; ++di)
        {
            const uint64_t rank_d{ cvk::rank_device(ctx, gfx, di) };
            {
                [[maybe_unused]] const VkPhysicalDeviceProperties& dev_props{ ctx.device_properties[di] };
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

                const Offset fam_count{ ctx.device_fam_count[di] };
                for (Offset qfi{}; qfi < fam_count; ++qfi)
                {
                    const std::uint64_t rank_g{ cvk::rank_device_graphics(ctx, gfx, di, qfi) };
                    if (rank_g > graphics_rank)
                    {
                        graphics_idx = qfi;
                        graphics_rank = rank_g;
                    }

                    const std::uint64_t rank_p{ cvk::rank_device_present(ctx, gfx, di, qfi) };
                    if (rank_p > present_rank)
                    {
                        present_idx = qfi;
                        present_rank = rank_g;
                    }
                }
            }
        }

        gfx.sel_device = device_idx;
        gfx.sel_graphics = graphics_idx;
        gfx.sel_present = present_idx;

        {
            [[maybe_unused]] const VkPhysicalDeviceProperties& dev_props{ ctx.device_properties[gfx.sel_device] };
            CGE_LOG("[CGE] Selected: \"{}\"\n", dev_props.deviceName);
        }
        cvk::update_surface_info(ctx, gfx, gfx.sel_device);
    }

    cvk::Ranking rank_device(const cvk::Context& ctx, const cvk::Renderable& gfx, const cvk::Offset device_idx) noexcept
    {
        const VkPhysicalDevice& device{ ctx.devices[device_idx] };
        const VkSurfaceKHR& surface{ gfx.surface };
        const VkPhysicalDeviceProperties& device_props{ ctx.device_properties[device_idx] };
        const std::span<const VkExtensionProperties> device_exts{ ctx.device_ext_array[device_idx], ctx.device_ext_count[device_idx] };
        const std::span<const VkLayerProperties>     device_lyrs{ ctx.device_lyr_array[device_idx], ctx.device_lyr_count[device_idx] };

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetPhysicalDeviceSurfacePresentModesKHR.html
        Offset ds_present_count;
        const VkResult res_presents{ vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &ds_present_count, nullptr) };
        if (res_presents != VK_SUCCESS) return 0;
        if (ds_present_count == 0) return 0;

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetPhysicalDeviceSurfaceFormatsKHR.html
        Offset ds_formats_count;
        const VkResult res_formats{ vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &ds_formats_count, nullptr) };
        if (res_formats != VK_SUCCESS) return 0;
        if (ds_formats_count == 0) return 0;

        if (!cvk::has_extensions(device_exts, cvk::req_device_extensions)) return 0;
        if (!cvk::has_layers(device_lyrs, cvk::req_layers)) return 0;

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

    cvk::Ranking rank_device_graphics(const cvk::Context& ctx, const cvk::Renderable& gfx [[maybe_unused]], const cvk::Offset device_idx, const cvk::Offset queue_idx) noexcept
    {
        const VkQueueFamilyProperties& qf_props{ ctx.device_fam_array[device_idx][queue_idx] };

        if ((qf_props.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) return 0;

        std::uint64_t rank{ 1 };

        rank += qf_props.queueCount;

        return rank;
    }

    cvk::Ranking rank_device_present(const cvk::Context& ctx, const cvk::Renderable& gfx, const cvk::Offset device_idx, const cvk::Offset queue_idx) noexcept
    {
        const VkQueueFamilyProperties& qf_props{ ctx.device_fam_array[device_idx][queue_idx] };
        
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetPhysicalDeviceSurfaceSupportKHR.html
        VkBool32 supports_present;
        const VkResult res_support{ vkGetPhysicalDeviceSurfaceSupportKHR(ctx.devices[device_idx], queue_idx, gfx.surface, &supports_present) };
        if (res_support != VK_SUCCESS) return 0;
        if (!supports_present) return 0;

        std::uint64_t rank{ 1 };

        rank += qf_props.queueCount;

        return rank;
    }
}

namespace cvk
{
    bool str_equal(const char* const a, const char* const b) noexcept
    {
        return std::strcmp(a, b) == 0;
    }

    bool has_extensions(const std::span<const VkExtensionProperties> sup_exts, const std::span<const char* const> req_exts) noexcept
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

    bool has_layers(const std::span<const VkLayerProperties> sup_lyrs, const std::span<const char* const> req_lyrs) noexcept
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

    std::string load_txt(const char* const filepath) noexcept
    {
        std::string txt{};
        std::ifstream file{ filepath };
        std::getline(file, txt, '\0');
        return txt;
    }

    void compile_spirv(cvk::Context& ctx, cvk::Renderable& gfx, VkShaderModule& module, const shaderc::Compiler& compiler, const shaderc::CompileOptions& options, const std::string& file_dir, const char* const file_name, const shaderc_shader_kind shader_kind) noexcept
    {
        const std::string file_path{ file_dir + file_name };
        const std::string source{ cvk::load_txt(file_path.c_str()) };
        if (source.empty())
        {
            const std::string full_path{ std::filesystem::current_path().append(file_path).lexically_normal().string() };
            CGE_LOG("[CGE] Unable to load file: \"{}\"\n", full_path);
        }
        CGE_ASSERT(!source.empty());

        const shaderc::SpvCompilationResult res_compile{ compiler.CompileGlslToSpv(source.c_str(), source.size(), shader_kind, file_name, shader_entry, options) };
        const shaderc_compilation_status status{ res_compile.GetCompilationStatus() };
        if (status != shaderc_compilation_status_success)
        {
            [[maybe_unused]] const std::size_t num_errors{ res_compile.GetNumErrors() };
            [[maybe_unused]] const std::size_t num_warnings{ res_compile.GetNumWarnings() };
            CGE_LOG("[CGE] SPIR-V Compilation Error! ({} Errors, {} Warnings)\n  {}{}\n", num_errors, num_warnings, file_dir, res_compile.GetErrorMessage());
        }
        CGE_ASSERT(status == shaderc_compilation_status_success);
        
        const std::vector<Offset> code{ res_compile.cbegin(), res_compile.cend() };

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkShaderModuleCreateInfo.html
        const VkShaderModuleCreateInfo module_info{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext = {},
            .flags = {},
            .codeSize = code.size() * sizeof(Offset),
            .pCode = code.data(),
        };
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCreateShaderModule.html
        const VkResult res_module{ vkCreateShaderModule(gfx.device, &module_info, ctx.allocator, &module) };
        CGE_ASSERT(res_module == VK_SUCCESS);
    }

    VkDeviceSize map_bytes(const VkDeviceSize buffer_size, void* const buffer, VkDeviceSize& offs, const std::span<const std::byte> bytes) noexcept
    {
        const VkDeviceSize size{ static_cast<VkDeviceSize>(bytes.size()) };
        const VkDeviceSize next{ offs + size };
        CGE_ASSERT(offs <= buffer_size);
        CGE_ASSERT(size <= buffer_size);
        CGE_ASSERT(next <= buffer_size);
        
        std::memcpy(buffer, bytes.data(), bytes.size());

        const VkDeviceSize prev{ offs };
        offs = next;
        return prev;
    };

    Offset find_memtype(const std::span<const VkMemoryType> mem_types, const Offset alloc_type, const Offset alloc_props) noexcept
    {
        for (Offset idx{}; idx < mem_types.size(); ++idx)
        {
            const VkMemoryType& mem_type{ mem_types[idx] };
            
            const Offset this_bit{ Offset(1) << idx };
            const bool has_type{ (this_bit & alloc_type) != 0 };
            const bool has_props{ (mem_type.propertyFlags & alloc_props) == alloc_props };

            if (has_type && has_props) return idx;
        }
        return null_idx;
    }

    #define CGE_FIND_RETURN(find, range, ...) if (find(__VA_ARGS__) != range.end()) return { __VA_ARGS__ }

    VkSurfaceFormatKHR ideal_format(const std::span<const VkSurfaceFormatKHR> formats) noexcept
    {
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSurfaceFormatKHR.html
        const auto find_format = [&](const VkFormat format, const VkColorSpaceKHR colorspace) {
            return std::ranges::find_if(formats, [&](const VkSurfaceFormatKHR elem) { return (elem.format == format) && (elem.colorSpace == colorspace); });
        };
        CGE_FIND_RETURN(find_format, formats, VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR);
        CGE_ASSERT(false);
    }
    
    VkPresentModeKHR ideal_present(const std::span<const VkPresentModeKHR> modes, const bool vsync) noexcept
    {
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPresentModeKHR.html
        const auto find_mode = [&](const VkPresentModeKHR mode) {
            return std::ranges::find(modes, mode);
        };
        if (!vsync) CGE_FIND_RETURN(find_mode, modes, VK_PRESENT_MODE_IMMEDIATE_KHR);
        CGE_FIND_RETURN(find_mode, modes, VK_PRESENT_MODE_FIFO_RELAXED_KHR);
        CGE_FIND_RETURN(find_mode, modes, VK_PRESENT_MODE_FIFO_KHR);
        CGE_ASSERT(false);
    }

    VkExtent2D ideal_resolution(const VkExtent2D size, const VkSurfaceCapabilitiesKHR& caps) noexcept
    {
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSurfaceCapabilitiesKHR.html#_description
        constexpr Offset special_value{ 0xFFFFFFFF };
        
        if ((caps.currentExtent.width == special_value) && (caps.currentExtent.height == special_value)) return size;
        
        return {
            .width = std::clamp(size.width, caps.minImageExtent.width, caps.maxImageExtent.width),
            .height = std::clamp(size.height, caps.minImageExtent.height, caps.maxImageExtent.height),
        };
    }
}

namespace cvk
{
    VkResult render_frame(Context& ctx [[maybe_unused]], Renderable& gfx, const cge::Primitives& prims) noexcept
    {
        const Offset this_frame{ gfx.frame_idx };
        const Offset prev_frame{ (this_frame + (gfx.frame_count - 1)) % gfx.frame_count };

        const VkSemaphore image_acquired{ gfx.frame_sem_image[this_frame] };
        const VkSemaphore render_finished{ gfx.frame_sem_render[this_frame] };
        const VkFence this_available{ gfx.frame_fence[this_frame] };
        const VkFence prev_available{ gfx.frame_fence[prev_frame] };

        Offset image_idx;
        const VkResult res_acquire{ cvk::acquire_image(gfx, image_idx, image_acquired, std::array{ this_available, prev_available }, std::array{ this_available }) };
        if (res_acquire != VK_SUCCESS)
        {
            // CGE_LOG("[CGE] Render failed. (Could not acquire image)\n");
            return res_acquire;
        }
        gfx.frame_idx = (gfx.frame_idx + 1) % gfx.frame_count;

        if (image_idx != this_frame)
        {
            CGE_LOG("[CGE] Render failed. (Swapchain image mismatch)\n");
            CGE_ASSERT(image_idx == this_frame);
        }

        const VkResult res_record{ cvk::record_commands(gfx, image_idx, prims) };
        if (res_record != VK_SUCCESS)
        {
            // CGE_LOG("[CGE] Render failed. (Could not record commands)\n");
            return res_record;
        }

        const VkResult res_submit{ cvk::submit_commands(gfx, image_idx, std::array{ image_acquired }, std::array{ render_finished }, this_available) };
        if (res_submit != VK_SUCCESS)
        {
            // CGE_LOG("[CGE] Render failed. (Could not submit commands)\n");
            return res_submit;
        }

        const VkResult res_present{ cvk::present_image(gfx, image_idx, std::array{ render_finished }) };
        if (res_present != VK_SUCCESS)
        {
            // CGE_LOG("[CGE] Render failed. (Could not present image)\n");
            return res_present;
        }

        return VK_SUCCESS;
    }

    VkResult acquire_image(cvk::Renderable& gfx, cvk::Offset& acquired_idx, const VkSemaphore signal_sem, const std::span<const VkFence> wait_fences, const std::span<const VkFence> reset_fences) noexcept
    {
        constexpr std::uint64_t no_timeout{ std::uint64_t(~0) };

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkWaitForFences.html
        const VkResult res_wait{ vkWaitForFences(gfx.device, Offset(wait_fences.size()), wait_fences.data(), VK_TRUE, no_timeout) };
        if (res_wait != VK_SUCCESS) return res_wait;

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkAcquireNextImageKHR.html
        const VkResult res_acquire{ vkAcquireNextImageKHR(gfx.device, gfx.swapchain, no_timeout, signal_sem, VK_NULL_HANDLE, &acquired_idx) };
        if (res_acquire != VK_SUCCESS) return res_acquire;

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkResetFences.html
        const VkResult res_reset{ vkResetFences(gfx.device, Offset(reset_fences.size()), reset_fences.data()) };
        if (res_reset != VK_SUCCESS) return res_reset;

        return VK_SUCCESS;
    }

    VkResult record_commands(Renderable& gfx, const Offset frame_idx, const cge::Primitives& prims) noexcept
    {
        // ----------------------------------------------------------------

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkResetCommandBuffer.html
        const VkResult res_reset{ vkResetCommandBuffer(gfx.frame_commands[frame_idx], 0) };
        if (res_reset != VK_SUCCESS) return res_reset;

        // ----------------------------------------------------------------

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkExtent2D.html
        const VkExtent2D ideal_res{ cvk::ideal_resolution(gfx.surface_extent, gfx.ds_capabilities) };
        const VkExtent2D image_res{ .width = (ideal_res.width ? ideal_res.width : 1 ), .height = (ideal_res.height ? ideal_res.height : 1 ) };
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkViewport.html
        const VkViewport viewport{
            .x = 0.0f,
            .y = 0.0f,
            .width = float(image_res.width),
            .height = float(image_res.height),
            .minDepth = 0.0f,
            .maxDepth = 0.0f,
        };
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkRect2D.html
        const VkRect2D scissor{
            .offset = { 0, 0 },
            .extent = image_res,
        };
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkRect2D.html
        const VkRect2D image_rect{
            .offset = { 0, 0 },
            .extent = image_res,
        };

        const std::uint32_t clr_b{ (prims.background_clr) & 0xFF };
        const std::uint32_t clr_g{ (prims.background_clr >> 8) & 0xFF };
        const std::uint32_t clr_r{ (prims.background_clr >> 16) & 0xFF };
        const std::uint32_t clr_a{ (prims.background_clr >> 24) & 0xFF };

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkClearValue.html
        const VkClearValue clear_value{
            .color = {
                .float32 = {
                    float(clr_r) / 255.0f,
                    float(clr_g) / 255.0f,
                    float(clr_b) / 255.0f,
                    float(clr_a) / 255.0f,
                }
            }
        };
        
        // ======================= RENDERING  ORDER =======================
        /* Triangle Fans   */
        /* Triangle Strips */
        /* Triangles       */
        /* Line Strips     */
        /* Lines           */
        /* Points          */
        // ----------------------------------------------------------------

        using VertexSpan = std::span<const cge::Vertex>;
        using IndexSpan = std::span<const cge::Index>;
        using ByteSpan = std::span<const std::byte>;

        const std::array<VertexSpan, num_pipelines> vertices{
            /* Triangle Fans   */ VertexSpan{prims.triangle_fan_vtx},
            /* Triangle Strips */ VertexSpan{prims.triangle_strip_vtx},
            /* Triangles       */ VertexSpan{prims.triangle_vtx},
            /* Line Strips     */ VertexSpan{prims.line_strip_vtx},
            /* Lines           */ VertexSpan{prims.line_vtx},
            /* Points          */ VertexSpan{prims.point_vtx},
        };

        const std::array<IndexSpan, num_pipelines> indices{
        #ifdef __APPLE__
            /* Triangle Fans   */ IndexSpan{},
        #else
            /* Triangle Fans   */ IndexSpan{prims.triangle_fan_idx},
        #endif
            /* Triangle Strips */ IndexSpan{prims.triangle_strip_idx},
            /* Triangles       */ IndexSpan{},
            /* Line Strips     */ IndexSpan{prims.line_strip_idx},
            /* Lines           */ IndexSpan{},
            /* Points          */ IndexSpan{},
        };

        const std::array<VkDescriptorSet, num_pipelines> descriptor_sets{
            /* Triangle Fans   */ gfx.descriptor_set,
            /* Triangle Strips */ gfx.descriptor_set,
            /* Triangles       */ gfx.descriptor_set,
            /* Line Strips     */ gfx.descriptor_set,
            /* Lines           */ gfx.descriptor_set,
            /* Points          */ gfx.descriptor_set,
        };

        const std::array<VkPipeline, num_pipelines> pipeline_handles{
            /* Triangle Fans   */ gfx.pipelines_graphics[0],
            /* Triangle Strips */ gfx.pipelines_graphics[1],
            /* Triangles       */ gfx.pipelines_graphics[2],
            /* Line Strips     */ gfx.pipelines_graphics[3],
            /* Lines           */ gfx.pipelines_graphics[4],
            /* Points          */ gfx.pipelines_graphics[5],
        };

        const std::array<VkPipelineLayout, num_pipelines> pipeline_layouts{
            /* Triangle Fans   */ gfx.pipeline_layout,
            /* Triangle Strips */ gfx.pipeline_layout,
            /* Triangles       */ gfx.pipeline_layout,
            /* Line Strips     */ gfx.pipeline_layout,
            /* Lines           */ gfx.pipeline_layout,
            /* Points          */ gfx.pipeline_layout,
        };

        // ----------------------------------------------------------------

        const std::array<ByteSpan, num_pipelines> vtx_bytes{
            /* Triangle Fans   */ std::as_bytes(std::span{prims.triangle_fan_vtx}),
            /* Triangle Strips */ std::as_bytes(std::span{prims.triangle_strip_vtx}),
            /* Triangles       */ std::as_bytes(std::span{prims.triangle_vtx}),
            /* Line Strips     */ std::as_bytes(std::span{prims.line_strip_vtx}),
            /* Lines           */ std::as_bytes(std::span{prims.line_vtx}),
            /* Points          */ std::as_bytes(std::span{prims.point_vtx}),
        };
        const std::array<ByteSpan, num_pipelines> idx_bytes{
        #ifdef __APPLE__
            /* Triangle Fans   */ ByteSpan{},
        #else
            /* Triangle Fans   */ std::as_bytes(std::span{prims.triangle_fan_idx}),
        #endif
            /* Triangle Strips */ std::as_bytes(std::span{prims.triangle_strip_idx}),
            /* Triangles       */ ByteSpan{},
            /* Line Strips     */ std::as_bytes(std::span{prims.line_strip_idx}),
            /* Lines           */ ByteSpan{},
            /* Points          */ ByteSpan{},
        };

        std::array<VkDeviceSize, num_pipelines> vtx_offs{};
        std::array<VkDeviceSize, num_pipelines> idx_offs{};

        const VkDeviceMemory buffer_memory{ gfx.buffer_memory };
        const VkDeviceSize buffer_offs{ gfx.buffer_vtx_offs };
        const VkDeviceSize buffer_size{ gfx.buffer_vtx_size + gfx.buffer_idx_size };
        {
            void* buffer{};

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkMapMemory.html
            const VkResult res_map{ vkMapMemory(gfx.device, buffer_memory, buffer_offs, buffer_size, 0, &buffer) };
            if (res_map != VK_SUCCESS) return res_map;

            {
                VkDeviceSize offset{};

                offset = gfx.buffer_vtx_offs;
                for (std::size_t idx{}; idx < num_pipelines; ++idx)
                {
                    const VkDeviceSize rel_offs{ offset - gfx.buffer_vtx_offs };
                    const VkDeviceSize rel_size{ vtx_bytes[idx].size() };
                    const VkDeviceSize rel_end{ rel_offs + rel_size };
                    CGE_ASSERT(rel_end <= gfx.buffer_vtx_size);

                    vtx_offs[idx] = cvk::map_bytes(buffer_size, buffer, offset, vtx_bytes[idx]);
                }

                offset = gfx.buffer_idx_offs;
                for (std::size_t idx{}; idx < num_pipelines; ++idx)
                {
                    const VkDeviceSize rel_offs{ offset - gfx.buffer_idx_offs };
                    const VkDeviceSize rel_size{ idx_bytes[idx].size() };
                    const VkDeviceSize rel_end{ rel_offs + rel_size };
                    CGE_ASSERT(rel_end <= gfx.buffer_idx_size);

                    idx_offs[idx] = cvk::map_bytes(buffer_size, buffer, offset, idx_bytes[idx]);
                }
            }

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkUnmapMemory.html
            vkUnmapMemory(gfx.device, buffer_memory);
        }

        // ----------------------------------------------------------------

        const VkRenderPass render_pass{ gfx.render_pass };
        const VkCommandBuffer command_buffer{ gfx.frame_commands[frame_idx] };
        const VkFramebuffer frame_buffer{ gfx.frame_buffer[frame_idx] };

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkCommandBufferBeginInfo.html
        const VkCommandBufferBeginInfo begin_info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = {},
            .flags = {},
            .pInheritanceInfo = {},
        };
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkBeginCommandBuffer.html
        const VkResult res_begin{ vkBeginCommandBuffer(command_buffer, &begin_info) };
        if (res_begin != VK_SUCCESS) return res_begin;
        {
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkRenderPassBeginInfo.html
            const VkRenderPassBeginInfo pass_info{
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .pNext = {},
                .renderPass = render_pass,
                .framebuffer = frame_buffer,
                .renderArea = image_rect,
                .clearValueCount = 1,
                .pClearValues = &clear_value,
            };
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCmdEndRenderPass.html
            vkCmdBeginRenderPass(command_buffer, &pass_info, VK_SUBPASS_CONTENTS_INLINE);
            {
                for (std::size_t idx{}; idx < num_pipelines; ++idx)
                {
                    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_handles[idx]);
                    vkCmdSetViewport(command_buffer, 0, 1, &viewport);
                    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

                    const VkDeviceSize vtx_offset{ vtx_offs[idx] };
                    const VkDeviceSize idx_offset{ idx_offs[idx] - gfx.buffer_idx_offs };
                    const VkDescriptorSet desc_set{ descriptor_sets[idx] };
                    const VkPipelineLayout layout{ pipeline_layouts[idx] };
                    const Offset vtx_count{ Offset(vertices[idx].size()) };
                    const Offset idx_count{ Offset(indices[idx].size()) };
                    const bool has_idx{ indices[idx].data() != nullptr };

                    vkCmdBindVertexBuffers(command_buffer, 0, 1, &gfx.buffer_vtx, &vtx_offset);

                    if (has_idx)
                        vkCmdBindIndexBuffer(command_buffer, gfx.buffer_idx, idx_offset, VK_INDEX_TYPE_UINT16);

                    if (desc_set)
                        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &desc_set, 0, nullptr);

                    if (has_idx)
                        vkCmdDrawIndexed(command_buffer, idx_count, 1, 0, 0, 0);
                    else
                        vkCmdDraw(command_buffer, vtx_count, 1, 0, 0);
                }
            }
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCmdEndRenderPass.html
            vkCmdEndRenderPass(command_buffer);
        }
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkEndCommandBuffer.html
        const VkResult res_end{ vkEndCommandBuffer(command_buffer) };
        if (res_end != VK_SUCCESS) return res_end;

        // ----------------------------------------------------------------

        return VK_SUCCESS;
    }

    VkResult submit_commands(Renderable& gfx, const Offset frame_idx, const std::span<const VkSemaphore> wait_sems, const std::span<const VkSemaphore> signal_sems, const VkFence signal_fence) noexcept
    {
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPipelineStageFlags.html
        constexpr VkPipelineStageFlags stage_mask{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSubmitInfo.html
        const VkSubmitInfo submit_info{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = {},
            .waitSemaphoreCount = Offset(wait_sems.size()),
            .pWaitSemaphores = wait_sems.data(),
            .pWaitDstStageMask = &stage_mask,
            .commandBufferCount = 1,
            .pCommandBuffers = &gfx.frame_commands[frame_idx],
            .signalSemaphoreCount = Offset(signal_sems.size()),
            .pSignalSemaphores = signal_sems.data(),
        };
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkQueueSubmit.html
        const VkResult res_submit{ vkQueueSubmit(gfx.queue_graphics, 1, &submit_info, signal_fence) };
        return res_submit;
    }

    VkResult present_image(Renderable& gfx, const Offset frame_idx, const std::span<const VkSemaphore> wait_sems) noexcept
    {
        constexpr Offset num_swapchains{ 1 };
        const std::array<VkSwapchainKHR, num_swapchains> swapchains{ gfx.swapchain };
        const std::array<Offset, num_swapchains> indices{ frame_idx };
        std::array<VkResult, num_swapchains> results{};

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPresentInfoKHR.html
        const VkPresentInfoKHR present_info{
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext = {},
            .waitSemaphoreCount = Offset(wait_sems.size()),
            .pWaitSemaphores = wait_sems.data(),
            .swapchainCount = Offset(swapchains.size()),
            .pSwapchains = swapchains.data(),
            .pImageIndices = indices.data(),
            .pResults = results.data(),
        };
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkQueuePresentKHR.html
        const VkResult res_present{ vkQueuePresentKHR(gfx.queue_present, &present_info) };
        
        if (res_present == VK_SUBOPTIMAL_KHR)
        {
        #ifndef __APPLE__
            return res_present;
        #endif
        }
        else if (res_present != VK_SUCCESS)
        {
            return res_present;
        }

        return VK_SUCCESS;
    }
}

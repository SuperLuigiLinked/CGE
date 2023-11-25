/**
 * @file cge/cvk.hpp
 */

#pragma once

// #undef CGE_DEBUG
// #undef CGE_VALIDATE_VK

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
#if defined(__APPLE__)
    #include <vulkan/vulkan_beta.h>
#endif
#if defined(_WIN32)
    #define _CRT_SECURE_NO_WARNINGS
    #undef min
    #undef max
    #undef near
    #undef far
#endif
#include <shaderc/shaderc.hpp>

#include <cge.hpp>
#include "../engine.hpp"

// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetInstanceProcAddr.html
#define CVK_LOAD_INSTANCE(instance, var, func) var.func = reinterpret_cast<PFN_##func>(vkGetInstanceProcAddr(instance, #func))

// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetDeviceProcAddr.html
#define CVK_LOAD_DEVICE(device, var, func) var.func = reinterpret_cast<PFN_##func>(vkGetDeviceProcAddr(device, #func))

namespace cvk
{
    struct InstanceFunctions
    {
    #if defined(CGE_VALIDATE_VK)
        PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCreateDebugUtilsMessengerEXT.html
        PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroyDebugUtilsMessengerEXT.html
    #endif
    };

    struct DeviceFunctions
    {
    };
}

namespace cvk
{
    static inline constexpr std::array req_instance_extensions{
    #if defined(__APPLE__)
        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME, ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_portability_enumeration.html
    #endif
        VK_KHR_SURFACE_EXTENSION_NAME, ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_surface.html
    #if defined(WYN_WIN32)
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME, ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_win32_surface.html
    #elif defined(WYN_XLIB)
        VK_KHR_XLIB_SURFACE_EXTENSION_NAME, ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_xlib_surface.html
    #elif defined(WYN_XCB) || defined(WYN_X11)
        VK_KHR_XCB_SURFACE_EXTENSION_NAME, ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_xcb_surface.html
    #elif defined(WYN_WAYLAND)
        VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME, ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_wayland_surface.html
    #elif defined(WYN_COCOA)
        VK_EXT_METAL_SURFACE_EXTENSION_NAME, ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_metal_surface.html
    #endif
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_get_physical_device_properties2.html
        VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME, ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_get_surface_capabilities2.html
        VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME, ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_surface_maintenance1.html

    #if defined(CGE_VALIDATE_VK)
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME, ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_debug_utils.html
    #endif
    };

    static inline constexpr std::array req_device_extensions{
    #if defined(__APPLE__)
        VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME, ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_portability_subset.html
    #endif
        VK_KHR_SWAPCHAIN_EXTENSION_NAME, ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_swapchain.html
        VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME, ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_swapchain_maintenance1.html
    };

#if defined(CGE_VALIDATE_VK)    
    static inline constexpr std::array req_layers{
        "VK_LAYER_KHRONOS_validation", ///< https://vulkan.lunarg.com/doc/view/1.3.204.1/windows/khronos_validation_layer.html
    };
#else
    static inline constexpr std::array<const char*, 0> req_layers{};
#endif

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceFeatures.html
    static inline constexpr VkPhysicalDeviceFeatures req_features{};

}

namespace cvk
{
    using Offset = std::uint32_t;
    using Ranking = std::uint64_t;

    static inline constexpr cge::Color default_color{};
    static inline constexpr cge::Texture default_texture{ .width = 1, .height = 1, .data = &default_color };

    static inline constexpr decltype(auto) shader_entry{ "main" };
    static inline constexpr std::size_t num_pipelines{ 1 };

    static inline constexpr cvk::Offset null_idx{ ~cvk::Offset{} };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSurfaceCapabilitiesKHR.html#_description
    constexpr cvk::Offset special_value{ ~cvk::Offset{} };
}

namespace cvk
{
    struct Context
    {
        InstanceFunctions pfn;
        
        VkAllocationCallbacks* allocator;

        VkInstance             instance          ; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkInstance.html
    #if defined(CGE_DEBUG)
        Offset                 instance_ext_count;
        Offset                 instance_lyr_count;
        VkExtensionProperties* instance_ext_array; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkExtensionProperties.html
        VkLayerProperties*     instance_lyr_array; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkLayerProperties.html
    #endif
    #if defined(CGE_VALIDATE_VK)
        VkDebugUtilsMessengerEXT messenger; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDebugUtilsMessengerEXT.html
    #endif

        Offset device_count;
        VkPhysicalDevice*                 devices          ; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDevice.html
        Offset*                           device_ext_count ;
        Offset*                           device_lyr_count ;
        Offset*                           device_fam_count ;
        VkExtensionProperties**           device_ext_array ; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkExtensionProperties.html
        VkLayerProperties**               device_lyr_array ; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkLayerProperties.html
        VkQueueFamilyProperties**         device_fam_array ; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkQueueFamilyProperties.html
        VkPhysicalDeviceProperties*       device_properties; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceProperties
        VkPhysicalDeviceFeatures*         device_features  ; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceFeatures.html
        VkPhysicalDeviceMemoryProperties* device_memory    ; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceMemoryProperties.html
    };

    struct Renderable
    {
        DeviceFunctions pfn;

        void* window ; ///< HWND      | Window   | xcb_window_t      | wl_surface* | NSWindow*
        void* context; ///< HINSTANCE | Display* | xcb_connection_t* | wl_display* | NSView*

        VkSurfaceKHR surface; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSurfaceKHR.html
        VkExtent2D         surface_extent ; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkExtent2D.html
        VkSurfaceFormatKHR surface_format ; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSurfaceFormatKHR.html
        VkPresentModeKHR   surface_present; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPresentModeKHR.html

        Offset   sel_device  ;
        Offset   sel_graphics;
        Offset   sel_present ;
        VkDevice device        ; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDevice.html
        VkQueue  queue_graphics; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkQueue.html
        VkQueue  queue_present ; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkQueue.html

        Offset                   ds_present_count;
        Offset                   ds_formats_count;
        VkPresentModeKHR*        ds_present_array; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPresentModeKHR.html
        VkSurfaceFormatKHR*      ds_formats_array; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSurfaceFormatKHR.html
        VkSurfaceCapabilities2KHR              surface_caps; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSurfaceCapabilities2KHR.html
        VkSurfacePresentScalingCapabilitiesEXT scaling_caps; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSurfacePresentScalingCapabilitiesEXT.html

        VkBuffer             buffer_main    ; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkBuffer.html
        VkDeviceMemory       buffer_memory  ; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDeviceMemory.html
        VkDeviceSize         buffer_capacity; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDeviceSize.html
        VkMemoryRequirements buffer_memreqs ; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkMemoryRequirements.html

        VkCommandPool         command_pool          ; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkCommandPool.html

        VkSwapchainKHR   swapchain       ; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSwapchainKHR.html
        Offset           frame_idx       ;
        Offset           frame_count     ;
        VkImage*         frame_image     ; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImage.html
        VkImageView*     frame_view      ; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImageView.html
        VkFramebuffer*   frame_buffer    ; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkFramebuffer.html
        VkCommandBuffer* frame_commands  ; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkCommandBuffer.html
        VkFence*         frame_fence     ; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkFence.html
        VkSemaphore*     frame_sem_render; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSemaphore.html
        VkSemaphore*     frame_sem_image ; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSemaphore.html
        
        Offset atlas_count;
        VkImage*              atlas_image  ; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImage.html
        VkImageView*          atlas_view   ; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImageView.html
        VkSampler*            atlas_sampler; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSampler.html
        VkExtent2D*           atlas_extent ; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkExtent2D.html
        VkDeviceMemory*       atlas_memory ; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDeviceMemory.html
        VkMemoryRequirements* atlas_memreqs; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkMemoryRequirements

        VkShaderModule        module_vertex         ; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkShaderModule.html
        VkShaderModule        module_fragment       ; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkShaderModule.html
        VkRenderPass          render_pass           ; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkRenderPass.html
        VkDescriptorSetLayout descriptor_layout     ; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDescriptorSetLayout.html
        VkDescriptorPool      descriptor_pool       ; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDescriptorPool.html
        VkDescriptorSet       descriptor_set        ; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDescriptorSet.html
        VkPipelineLayout      pipeline_layout       ; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPipelineLayout.html
        VkPipeline pipelines_graphics[num_pipelines]; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPipeline.html

    };

    struct Vulkan
    {
        cvk::Context ctx;
        cvk::Renderable gfx;
    };
}

namespace cvk
{
    extern void create_context(cvk::Context& ctx) noexcept;
    extern void destroy_context(cvk::Context& ctx) noexcept;

    extern void create_renderable(cvk::Context& ctx, cvk::Renderable& gfx, wyn_window_t window, VkExtent2D size, bool vsync) noexcept;
    extern void destroy_renderable(cvk::Context& ctx, cvk::Renderable& gfx) noexcept;

    extern void upload_texture(cvk::Context& ctx, cvk::Renderable& gfx, Offset atlas_idx, cge::Texture tex) noexcept;

    extern VkResult render_frame(cvk::Context& ctx, cvk::Renderable& gfx, const cge::Scene& scene) noexcept;
    extern void remake_swapchain(cvk::Context& ctx, cvk::Renderable& gfx, VkExtent2D new_size, bool vsync) noexcept;
}

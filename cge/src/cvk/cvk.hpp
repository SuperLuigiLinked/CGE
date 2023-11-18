/**
 * @file cge/cvk.hpp
 */

#pragma once

#if !defined(NDEBUG)
    #define CGE_DEBUG_VK
#endif

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
#ifdef _WIN32
    #define _CRT_SECURE_NO_WARNINGS
    #undef min
    #undef max
    #undef near
    #undef far
#endif
#include <shaderc/shaderc.hpp>

#include "cge/cge.hpp"
#include "../impl.hpp"
#include "../debug.hpp"

#include <cstddef>
#include <cstdint>
#include <array>

namespace cvk
{
    static inline constexpr std::array req_instance_extensions{
    #ifdef __APPLE__
        "VK_KHR_portability_enumeration", ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_portability_enumeration.html
        "VK_KHR_get_physical_device_properties2", ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_get_physical_device_properties2.html
    #endif
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
    #ifdef __APPLE__
        "VK_KHR_portability_subset", ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_portability_subset.html
    #endif
        "VK_KHR_swapchain", ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_swapchain.html
    };
    
    static inline constexpr std::array req_instance_layers{
        "VK_LAYER_KHRONOS_validation", ///< https://vulkan.lunarg.com/doc/view/1.3.204.1/windows/khronos_validation_layer.html
    };
    
    static inline constexpr std::array req_device_layers{
        "VK_LAYER_KHRONOS_validation", ///< https://vulkan.lunarg.com/doc/view/1.3.204.1/windows/khronos_validation_layer.html
    };
}

namespace cvk
{
    using Index = std::uint32_t;

    static inline constexpr cge::Color default_color{};
    static inline constexpr cge::Texture default_texture{ .width = 1, .height = 1, .data = &default_color };

    static inline constexpr decltype(auto) shader_entry{ "main" };
    static inline constexpr std::size_t num_pipelines{ 6 };

    static inline constexpr Index null_idx{ Index(~0) };
}

namespace cvk
{
    struct Functions
    {
        PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCreateDebugUtilsMessengerEXT.html
        PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroyDebugUtilsMessengerEXT.html
    };
    
    struct Atlas
    {
        cge::Texture tex;

        VkImage image; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImage.html
        VkMemoryRequirements memreqs; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkMemoryRequirements

        VkDeviceMemory memory; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDeviceMemory.html

        VkImageView view; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImageView.html
        VkSampler sampler; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSampler.html
    };

    struct Context
    {
        Functions pfn;

        VkInstance instance; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkInstance.html
        Index instance_extension_count;
        Index instance_layer_count;
        std::vector<VkExtensionProperties> instance_extensions; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkExtensionProperties.html
        std::vector<VkLayerProperties> instance_layers; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkLayerProperties.html

    #if defined(CGE_DEBUG_VK)
        VkDebugUtilsMessengerEXT messenger; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDebugUtilsMessengerEXT.html
    #endif

        Index device_count;
        std::vector<VkPhysicalDevice> device_handles; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDevice.html
        std::vector<Index> device_extension_counts;
        std::vector<Index> device_layer_counts;
        std::vector<Index> device_queue_family_counts;
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
        Index device_idx;
        Index queue_graphics_idx;
        Index queue_present_idx;

        Index ds_mode_count;
        Index ds_format_count;
        std::vector<std::vector<VkBool32>> ds_present_supports; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkBool32.html
        std::vector<std::vector<VkPresentModeKHR>> ds_present_modes; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPresentModeKHR.html
        std::vector<std::vector<VkSurfaceFormatKHR>> ds_formats; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSurfaceFormatKHR.html
        std::vector<VkSurfaceCapabilitiesKHR> ds_capabilities; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSurfaceCapabilitiesKHR.html

        VkBuffer buffer_vtx; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkBuffer.html
        VkBuffer buffer_idx; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkBuffer.html
        VkBuffer buffer_stg; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkBuffer.html
        VkMemoryRequirements memreqs_vtx; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkMemoryRequirements.html
        VkMemoryRequirements memreqs_idx; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkMemoryRequirements.html
        VkMemoryRequirements memreqs_stg; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkMemoryRequirements.html
        VkDeviceSize buffer_vtx_offs; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDeviceSize.html
        VkDeviceSize buffer_idx_offs; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDeviceSize.html
        VkDeviceSize buffer_stg_offs; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDeviceSize.html
        VkDeviceSize buffer_vtx_size; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDeviceSize.html
        VkDeviceSize buffer_idx_size; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDeviceSize.html
        VkDeviceSize buffer_stg_size; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDeviceSize.html

        VkDeviceMemory buffer_memory; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDeviceMemory.html
        VkDeviceSize buffer_capacity; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDeviceSize.html

        VkCommandPool command_pool; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkCommandPool.html

        VkShaderModule module_vertex; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkShaderModule.html
        VkShaderModule module_fragment; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkShaderModule.html

        VkRenderPass render_pass; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkRenderPass.html
        VkDescriptorSetLayout descriptor_layout; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDescriptorSetLayout.html
        VkDescriptorPool descriptor_pool; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDescriptorPool.html
        VkDescriptorSet descriptor_set; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDescriptorSet.html
        VkPipelineLayout pipeline_layout; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPipelineLayout.html
        
        std::array<VkPipeline, num_pipelines> pipelines_graphics; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPipeline.html

        VkSwapchainKHR swapchain; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSwapchainKHR.html
        
        Index num_frames;
        Index frame_idx;
        std::vector<VkFence> frame_fence; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkFence.html
        std::vector<VkSemaphore> frame_sem_render; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSemaphore.html
        std::vector<VkSemaphore> frame_sem_image; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSemaphore.html
        std::vector<VkImage> frame_images; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImage.html
        std::vector<VkImageView> frame_views; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImageView.html
        std::vector<VkFramebuffer> frame_buffers; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkFramebuffer.html
        std::vector<VkCommandBuffer> frame_commands; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkCommandBuffer.html
    };
}

namespace cvk
{
    extern Context create_context();
    extern void destroy_context(Context& ctx);

    extern Renderable create_renderable(Context& ctx, wyn_window_t window, bool vsync);
    extern void destroy_renderable(Context& ctx, Renderable& gfx);

    extern Atlas create_atlas(Context& ctx, Renderable& gfx, cge::Texture texture);
    extern void destroy_atlas(Context& ctx, Renderable& gfx, Atlas& atlas);
    extern void upload_atlas(Context& ctx, Renderable& gfx, Atlas& atlas);

#if defined(WYN_COCOA)
    extern void* create_layer(void* window);
#endif

    extern bool render_frame(Renderable& gfx, const cge::Primitives& prims, const unsigned attempts);
    extern void update_surface_info(Context& ctx, Renderable& gfx);
    extern void remake_swapchain(Renderable& gfx, const bool vsync);
}

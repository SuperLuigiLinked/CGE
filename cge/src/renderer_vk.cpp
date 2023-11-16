/**
 * @file renderer_vk.cpp
 */

#ifdef _WIN32
    #define _CRT_SECURE_NO_WARNINGS
#endif

#include <cstring>

#include <fstream>
#include <filesystem>
#include <algorithm>
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
#ifdef _WIN32
    #undef min
    #undef max
    #undef near
    #undef far
#endif

#include <shaderc/shaderc.hpp>

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
    using IndexVK = std::uint32_t;

    class Renderer_VK final : public Renderer 
    {
    private:

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

        static inline constexpr RGBA default_color{};
        static inline constexpr Texture default_texture{ .width = 1, .height = 1, .data = &default_color };

        static inline constexpr decltype(auto) shader_entry{ "main" };
        static inline constexpr std::size_t num_pipelines{ 6 };

        static inline constexpr IndexVK null_idx{ IndexVK(~0) };

    private:

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
            IndexVK instance_extension_count;
            IndexVK instance_layer_count;
            std::vector<VkExtensionProperties> instance_extensions; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkExtensionProperties.html
            std::vector<VkLayerProperties> instance_layers; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkLayerProperties.html

        #if defined(CGE_DEBUG_VK)
            VkDebugUtilsMessengerEXT messenger; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDebugUtilsMessengerEXT.html
        #endif

            IndexVK device_count;
            std::vector<VkPhysicalDevice> device_handles; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDevice.html
            std::vector<IndexVK> device_extension_counts;
            std::vector<IndexVK> device_layer_counts;
            std::vector<IndexVK> device_queue_family_counts;
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
            IndexVK device_idx;
            IndexVK queue_graphics_idx;
            IndexVK queue_present_idx;

            IndexVK ds_mode_count;
            IndexVK ds_format_count;
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

            std::optional<Atlas> atlas;

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
            
            IndexVK num_frames;
            IndexVK frame_idx;
            std::vector<VkFence> frame_fence; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkFence.html
            std::vector<VkSemaphore> frame_sem_render; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSemaphore.html
            std::vector<VkSemaphore> frame_sem_image; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSemaphore.html
            std::vector<VkImage> frame_images; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImage.html
            std::vector<VkImageView> frame_views; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImageView.html
            std::vector<VkFramebuffer> frame_buffers; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkFramebuffer.html
            std::vector<VkCommandBuffer> frame_commands; ///< https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkCommandBuffer.html
        };

    private:

        Context context;
        std::optional<Renderable> renderable;

    private:
    
        static Context create_context();
        static void destroy_context(Context& ctx);

        static Renderable create_renderable(Context& ctx, wyn_window_t window, bool vsync);
        static void destroy_renderable(Context& ctx, Renderable& gfx);

        static Atlas create_atlas(Context& ctx, Renderable& gfx, Texture texture);
        static void destroy_atlas(Context& ctx, Renderable& gfx, Atlas& atlas);
        static void upload_atlas(Context& ctx, Renderable& gfx, Atlas& atlas);

        static std::uint64_t rank_device(Context& ctx, Renderable& gfx, IndexVK device_idx);
        static std::uint64_t rank_device_graphics(Context& ctx, Renderable& gfx, IndexVK device_idx, IndexVK queue_idx);
        static std::uint64_t rank_device_present(Context& ctx, Renderable& gfx, IndexVK device_idx, IndexVK queue_idx);

    private:

        static std::string load_txt(const char* const filepath) noexcept;
        static void compile_spirv(const VkDevice device, VkShaderModule& module, const shaderc::Compiler& compiler, const shaderc::CompileOptions& options, const std::string& file_dir, const char* const file_name, const shaderc_shader_kind shader_kind);

        static IndexVK vk_find_memtype(const std::span<const VkMemoryType>& mem_types, const IndexVK alloc_type, const IndexVK alloc_props) noexcept;
        static VkSurfaceFormatKHR vk_ideal_format(const std::span<const VkSurfaceFormatKHR> formats) noexcept;
        static VkPresentModeKHR vk_ideal_mode(const std::span<const VkPresentModeKHR> modes, const bool vsync) noexcept;
        static VkExtent2D vk_ideal_resolution(const VkExtent2D size, const VkSurfaceCapabilitiesKHR& caps) noexcept;

    public:

        Renderer_VK();
        ~Renderer_VK() final;

        void target_window(EngineImpl& impl, wyn_window_t window) final;
        void render(EngineImpl& impl) final;

    private:

        static bool render_frame(Renderable& gfx, const RenderSettings& settings);

        static IndexVK acquire_image(Renderable& gfx, VkSemaphore signal_sem, std::span<const VkFence> wait_fences, std::span<const VkFence> reset_fences);
        static void record_commands(Renderable& gfx, IndexVK frame_idx, const RenderSettings& settings);
        static bool submit_commands(Renderable& gfx, IndexVK frame_idx, std::span<const VkSemaphore> wait_sems, std::span<const VkSemaphore> signal_sems, VkFence signal_fence);
        static bool present_image(Renderable& gfx, IndexVK frame_idx, std::span<const VkSemaphore> wait_sems);

    };
}

namespace cge
{
    Renderer_VK::Renderer_VK()
        :
        context{},
        renderable{}
    {}

    Renderer_VK::~Renderer_VK()
    {
        if (this->renderable) this->destroy_renderable(this->context, *this->renderable);
        this->destroy_context(this->context);
    }

    extern Renderer* renderer_vk()
    {
        return new Renderer_VK();
    }
}


namespace cge
{
    std::string Renderer_VK::load_txt(const char* const filepath) noexcept
    {
        std::string txt{};
        std::ifstream file{ filepath };
        std::getline(file, txt, '\0');
        return txt;
    }

    void Renderer_VK::compile_spirv(const VkDevice device, VkShaderModule& module, const shaderc::Compiler& compiler, const shaderc::CompileOptions& options, const std::string& file_dir, const char* const file_name, const shaderc_shader_kind shader_kind)
    {
        const std::string file_path{ file_dir + file_name };
        const std::string source{ load_txt(file_path.c_str()) };
        if (source.empty())
        {
            const std::string full_path{ std::filesystem::current_path().append(file_path).lexically_normal().string() };
            CGE_LOG("[CGE] Unable to load file: \"{}\"\n", full_path);
        }
        CGE_ASSERT(!source.empty());

        const shaderc::SpvCompilationResult res_compile{ compiler.CompileGlslToSpv(source.c_str(), source.size(), shader_kind, file_name, shader_entry, options) };
        const shaderc_compilation_status status{ res_compile.GetCompilationStatus() };
        const std::size_t num_errors{ res_compile.GetNumErrors() };
        const std::size_t num_warnings{ res_compile.GetNumWarnings() };
        if (status != shaderc_compilation_status_success)
        {
            CGE_LOG("[CGE] SPIR-V Compilation Error! ({} Errors, {} Warnings)\n  {}{}\n", num_errors, num_warnings, file_dir, res_compile.GetErrorMessage());
        }
        CGE_ASSERT(status == shaderc_compilation_status_success);
        
        const std::vector<IndexVK> code{ res_compile.cbegin(), res_compile.cend() };

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkShaderModuleCreateInfo.html
        const VkShaderModuleCreateInfo module_info{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext = {},
            .flags = {},
            .codeSize = code.size() * sizeof(IndexVK),
            .pCode = code.data(),
        };
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCreateShaderModule.html
        const VkResult res_module{ vkCreateShaderModule(device, &module_info, nullptr, &module) };
        CGE_ASSERT(res_module == VK_SUCCESS);
    }

    static inline VkDeviceSize vk_map_bytes(const VkDeviceSize buffer_size, void* const buffer, VkDeviceSize& offs, const std::span<const std::byte> bytes)
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
}

namespace cge
{
    IndexVK Renderer_VK::vk_find_memtype(const std::span<const VkMemoryType>& mem_types, const IndexVK alloc_type, const IndexVK alloc_props) noexcept
    {
        for (IndexVK idx{}; idx < mem_types.size(); ++idx)
        {
            const VkMemoryType& mem_type{ mem_types[idx] };
            
            const IndexVK this_bit{ IndexVK(1) << idx };
            const bool has_type{ (this_bit & alloc_type) != 0 };
            const bool has_props{ (mem_type.propertyFlags & alloc_props) == alloc_props };

            if (has_type && has_props) return idx;
        }
        return null_idx;
    }

    #define CGE_FIND_RETURN(find, range, ...) if (find(__VA_ARGS__) != range.end()) return { __VA_ARGS__ }

    VkSurfaceFormatKHR Renderer_VK::vk_ideal_format(const std::span<const VkSurfaceFormatKHR> formats) noexcept
    {
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSurfaceFormatKHR.html
        const auto find_format = [&](const VkFormat format, const VkColorSpaceKHR colorspace) {
            return std::ranges::find_if(formats, [&](const VkSurfaceFormatKHR elem) { return (elem.format == format) && (elem.colorSpace == colorspace); });
        };
        CGE_FIND_RETURN(find_format, formats, VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR);
        CGE_ASSERT(false);
    }
    
    VkPresentModeKHR Renderer_VK::vk_ideal_mode(const std::span<const VkPresentModeKHR> modes, const bool vsync) noexcept
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

    VkExtent2D Renderer_VK::vk_ideal_resolution(const VkExtent2D size, const VkSurfaceCapabilitiesKHR& caps) noexcept
    {
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSurfaceCapabilitiesKHR.html#_description
        constexpr IndexVK special_value{ 0xFFFFFFFF };

        if ((caps.currentExtent.width == special_value) && (caps.currentExtent.height == special_value)) return size;
        
        return {
            .width = std::clamp(size.width, caps.minImageExtent.width, caps.maxImageExtent.width),
            .height = std::clamp(size.height, caps.minImageExtent.height, caps.maxImageExtent.height),
        };
    }
}

namespace cge
{
    static constexpr const char* vk_msg_severity(const VkDebugUtilsMessageSeverityFlagBitsEXT val) noexcept
    {
        switch (val)
        {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT  : return "ERROR";
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: return "WARNING";
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT   : return "INFO";
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: return "VERBOSE";
                                                     default: return "???";
        }
    }

    static constexpr const char* vk_msg_type(const VkDebugUtilsMessageTypeFlagsEXT val) noexcept
    {
        switch (val)
        {
        case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT    : return "GENERAL";
        case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT : return "VALIDATION";
        case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: return "PERFORMANCE";
                                                     default: return "???";
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
        const char* const str_id{ (data && data->pMessageIdName) ? data->pMessageIdName : "" };
        const char* const str_msg{ (data && data->pMessage) ? data->pMessage : "" };
        CGE_LOG("[VULKAN DEBUG - {} {}] {}\n{{\n\t{}\n}}\n", str_type, str_svrt, str_id, str_msg);
        return VK_FALSE;
    }
}

namespace cge
{
    Renderer_VK::Context Renderer_VK::create_context()
    {
        CGE_LOG("[CGE] Initializing Vulkan Context...\n");
        Context ctx{};
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
            #if defined(__APPLE__)
                .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
            #else
                .flags = {},
            #endif
                .pApplicationInfo = &app_info,
                .enabledLayerCount = IndexVK(req_instance_layers.size()),
                .ppEnabledLayerNames = req_instance_layers.data(),
                .enabledExtensionCount = IndexVK(req_instance_extensions.size()),
                .ppEnabledExtensionNames = req_instance_extensions.data(),
            };
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCreateInstance.html
            const VkResult res_instance{ vkCreateInstance(&create_info, nullptr, &ctx.instance) };
            CGE_ASSERT(res_instance == VK_SUCCESS);

            CGE_LOG("[CGE] VK INSTANCE\n");
        }
        {
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkEnumerateInstanceExtensionProperties.html
            const VkResult res_extension_count{ vkEnumerateInstanceExtensionProperties(nullptr, &ctx.instance_extension_count, nullptr) };
            CGE_ASSERT(res_extension_count == VK_SUCCESS);
            ctx.instance_extensions.resize(ctx.instance_extension_count);
            const VkResult res_extensions{ vkEnumerateInstanceExtensionProperties(nullptr, &ctx.instance_extension_count, ctx.instance_extensions.data()) };
            CGE_ASSERT(res_extensions == VK_SUCCESS);

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkEnumerateInstanceLayerProperties.html
            const VkResult res_layer_count{ vkEnumerateInstanceLayerProperties(&ctx.instance_layer_count, nullptr) };
            CGE_ASSERT(res_layer_count == VK_SUCCESS);
            ctx.instance_layers.resize(ctx.instance_layer_count);
            const VkResult res_layers{ vkEnumerateInstanceLayerProperties(&ctx.instance_layer_count, ctx.instance_layers.data()) };
            CGE_ASSERT(res_layers == VK_SUCCESS);

            CGE_LOG("[CGE] VK INSTANCE PROPERTIES\n");
        }
        {
            CGE_ASSERT((CGE_LOAD_INSTANCE(ctx.instance, ctx.pfn, vkCreateDebugUtilsMessengerEXT)));
            CGE_ASSERT((CGE_LOAD_INSTANCE(ctx.instance, ctx.pfn, vkDestroyDebugUtilsMessengerEXT)));

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
            const VkResult res_debug{ ctx.pfn.vkCreateDebugUtilsMessengerEXT(ctx.instance, &create_info, nullptr, &ctx.messenger) };
            CGE_ASSERT(res_debug == VK_SUCCESS);

            CGE_LOG("[CGE] VK DEBUG\n");
        }
        #endif
        {
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkEnumeratePhysicalDevices.html
            const VkResult res_count{ vkEnumeratePhysicalDevices(ctx.instance, &ctx.device_count, nullptr) };
            CGE_ASSERT(res_count == VK_SUCCESS);
            ctx.device_handles.resize(ctx.device_count);
            ctx.device_extension_counts.resize(ctx.device_count);
            ctx.device_extensions.resize(ctx.device_count);
            ctx.device_layer_counts.resize(ctx.device_count);
            ctx.device_layers.resize(ctx.device_count);
            ctx.device_properties.resize(ctx.device_count);
            ctx.device_features.resize(ctx.device_count);
            ctx.device_memory.resize(ctx.device_count);
            ctx.device_queue_family_counts.resize(ctx.device_count);
            ctx.device_queue_families.resize(ctx.device_count);
            const VkResult res_handles{ vkEnumeratePhysicalDevices(ctx.instance, &ctx.device_count, ctx.device_handles.data()) };
            CGE_ASSERT(res_handles == VK_SUCCESS);
            
            CGE_LOG("[CGE] VK PHYSICAL DEVICES\n");
        }
        {
            CGE_ASSERT(ctx.device_count > 0);

            for (IndexVK idx{}; idx < ctx.device_count; ++idx)
            {
                VkPhysicalDevice& handle{ ctx.device_handles[idx] };
                IndexVK& extension_count{ ctx.device_extension_counts[idx] };
                IndexVK& layer_count{ ctx.device_layer_counts[idx] };
                IndexVK& queue_family_count{ ctx.device_queue_family_counts[idx] };
                std::vector<VkExtensionProperties>& extensions{ ctx.device_extensions[idx] };
                std::vector<VkLayerProperties>& layers{ ctx.device_layers[idx] };
                std::vector<VkQueueFamilyProperties>& queue_families{ ctx.device_queue_families[idx] };
                VkPhysicalDeviceProperties& properties{ ctx.device_properties[idx] };
                VkPhysicalDeviceFeatures& features{ ctx.device_features[idx] };
                VkPhysicalDeviceMemoryProperties& memory{ ctx.device_memory[idx] };

                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkEnumerateDeviceExtensionProperties.html
                const VkResult res_extension_count{ vkEnumerateDeviceExtensionProperties(handle, nullptr, &extension_count, nullptr) };
                CGE_ASSERT(res_extension_count == VK_SUCCESS);
                extensions.resize(extension_count);
                const VkResult res_extensions{ vkEnumerateDeviceExtensionProperties(handle, nullptr, &extension_count, extensions.data()) };
                CGE_ASSERT(res_extensions == VK_SUCCESS);

                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkEnumerateDeviceLayerProperties.html
                const VkResult res_layer_count{ vkEnumerateDeviceLayerProperties(handle, &layer_count, nullptr) };
                CGE_ASSERT(res_layer_count == VK_SUCCESS);
                layers.resize(layer_count);
                const VkResult res_layers{ vkEnumerateDeviceLayerProperties(handle, &layer_count, layers.data()) };
                CGE_ASSERT(res_layers == VK_SUCCESS);

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
        return ctx;
    }

    void Renderer_VK::destroy_context(Context& ctx)
    {
    #if defined(CGE_DEBUG_VK)
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroyDebugUtilsMessengerEXT.html
        ctx.pfn.vkDestroyDebugUtilsMessengerEXT(ctx.instance, ctx.messenger, nullptr);
    #endif

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroyInstance.html
        vkDestroyInstance(ctx.instance, nullptr);
    }
}

namespace cge
{
    Renderer_VK::Renderable Renderer_VK::create_renderable(Context& ctx, wyn_window_t const window, bool const vsync [[maybe_unused]])
    {
        CGE_LOG("[CGE] Initializing Vulkan Window...\n");
        Renderable gfx{};
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
            const VkResult res_surface{ vkCreateWin32SurfaceKHR(ctx.instance, &create_info, nullptr, &gfx.surface_handle) };
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
            const VkResult res_surface{ vkCreateXlibSurfaceKHR(ctx.instance, &create_info, nullptr, &gfx.surface_handle) };
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
            const VkResult res_surface{ vkCreateXcbSurfaceKHR(ctx.instance, &create_info, nullptr, &gfx.surface_handle) };
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
            const VkResult res_surface{ vkCreateWaylandSurfaceKHR(ctx.instance, &create_info, nullptr, &gfx.surface_handle) };
        #elif defined(WYN_COCOA)
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkMetalSurfaceCreateInfoEXT.html
            const VkMetalSurfaceCreateInfoEXT create_info{
                .sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT,
                .pNext = {},
                .flags = {},
                .pLayer = static_cast<const CAMetalLayer*>(cge::mvk::create_layer(gfx.window)),
            };
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCreateMetalSurfaceEXT.html
            const VkResult res_surface{ vkCreateMetalSurfaceEXT(ctx.instance, &create_info, nullptr, &gfx.surface_handle) };
        #else
            #error "Unimplemented"
        #endif
            CGE_ASSERT(res_surface == VK_SUCCESS);

            const wyn_size_t size{ wyn_window_size(gfx.window) };
            gfx.surface_size = { .width = uint32_t(size.w), .height = uint32_t(size.h) };

            CGE_LOG("[CGE] VK SURFACE\n");
        }
        {
            const IndexVK device_count{  ctx.device_count };
            gfx.ds_present_supports.resize(device_count);
            gfx.ds_present_modes.resize(device_count);
            gfx.ds_formats.resize(device_count);
            gfx.ds_capabilities.resize(device_count);
            
            for (IndexVK di{}; di < device_count; ++di)
            {
                VkPhysicalDevice& device{ ctx.device_handles[di] };
                VkSurfaceKHR& surface{ gfx.surface_handle };
                std::vector<VkBool32>& present_supports{ gfx.ds_present_supports[di] };
                std::vector<VkPresentModeKHR>& present_modes{ gfx.ds_present_modes[di] };
                std::vector<VkSurfaceFormatKHR>& formats{ gfx.ds_formats[di] };
                VkSurfaceCapabilitiesKHR& capabilities{ gfx.ds_capabilities[di] };
                IndexVK& mode_count{ gfx.ds_mode_count };
                IndexVK& format_count{ gfx.ds_format_count };

                const IndexVK queue_count{ ctx.device_queue_family_counts[di] };
                present_supports.resize(queue_count);

                for (IndexVK qi{}; qi < queue_count; ++qi)
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
                CGE_ASSERT(res_modes == VK_SUCCESS);
                
                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetPhysicalDeviceSurfaceFormatsKHR.html
                const VkResult res_format_count{ vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr) };
                CGE_ASSERT(res_format_count == VK_SUCCESS);
                formats.resize(format_count);
                const VkResult res_formats{ vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, formats.data()) };
                CGE_ASSERT(res_formats == VK_SUCCESS);

                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetPhysicalDeviceSurfaceCapabilitiesKHR.html
                const VkResult res_caps{ vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &capabilities) };
                CGE_ASSERT(res_caps == VK_SUCCESS);
            }

            CGE_LOG("[CGE] VK DEVICE SURFACE\n");
        }
        {
            IndexVK device_idx{};
            IndexVK graphics_idx{};
            IndexVK present_idx{};
            std::uint64_t device_rank{};
            std::uint64_t graphics_rank{};
            std::uint64_t present_rank{};

            const IndexVK device_count{ ctx.device_count };
            for (IndexVK di{}; di < device_count; ++di)
            {
                const uint64_t rank_d{ rank_device(ctx, gfx, di) };
                {
                    const auto& dev_props{ ctx.device_properties[di] };
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

                    const IndexVK queue_count{ ctx.device_queue_family_counts[di] };
                    for (IndexVK qi{}; qi < queue_count; ++qi)
                    {
                        const std::uint64_t rank_g{ rank_device_graphics(ctx, gfx, di, qi) };
                        if (rank_g > graphics_rank)
                        {
                            graphics_idx = qi;
                            graphics_rank = rank_g;
                        }

                        const std::uint64_t rank_p{ rank_device_present(ctx, gfx, di, qi) };
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
                const auto& dev_props{ ctx.device_properties[gfx.device_idx] };
                CGE_LOG("[CGE] Selected: \"{}\"\n", dev_props.deviceName);
            }
            CGE_LOG("[CGE] VK DEVICE SELECTION\n");
        }
        {
            static constexpr std::array graphics_prios{ 1.0f };
            static constexpr std::array present_prios{ 1.0f };

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDeviceQueueCreateInfo.html
            const std::array queues_info{
                VkDeviceQueueCreateInfo{
                    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                    .pNext = {},
                    .flags = {},
                    .queueFamilyIndex = gfx.queue_graphics_idx,
                    .queueCount = graphics_prios.size(),
                    .pQueuePriorities = graphics_prios.data(),
                },
                VkDeviceQueueCreateInfo{
                    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                    .pNext = {},
                    .flags = {},
                    .queueFamilyIndex = gfx.queue_present_idx,
                    .queueCount = present_prios.size(),
                    .pQueuePriorities = present_prios.data(),
                },
            };
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceFeatures.html
            static constexpr VkPhysicalDeviceFeatures features{};

            const bool queues_unique{ gfx.queue_graphics_idx != gfx.queue_present_idx };
            const IndexVK queue_unique_count{ static_cast<IndexVK>(queues_unique ? 2 : 1) };
            //const IndexVK queue_concurrent_count{ static_cast<IndexVK>(queues_unique ? 2 : 0) };
            //const std::array queue_indices{ gfx.queue_graphics_idx, gfx.queue_present_idx };
            //const VkSharingMode queue_sharing{ queues_unique ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE };

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDeviceCreateInfo.html
            const VkDeviceCreateInfo device_info{
                .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                .pNext = {},
                .flags = {},
                .queueCreateInfoCount = queue_unique_count,
                .pQueueCreateInfos = queues_info.data(),
                .enabledLayerCount = req_device_layers.size(),
                .ppEnabledLayerNames = req_device_layers.data(),
                .enabledExtensionCount = req_device_extensions.size(),
                .ppEnabledExtensionNames = req_device_extensions.data(),
                .pEnabledFeatures = &features,
            };

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCreateDevice.html
            const VkResult res_device{ vkCreateDevice(ctx.device_handles[gfx.device_idx], &device_info, nullptr, &gfx.device) };
            CGE_ASSERT(res_device == VK_SUCCESS);

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetDeviceQueue.html
            vkGetDeviceQueue(gfx.device, gfx.queue_graphics_idx, 0, &gfx.queue_graphics);
            vkGetDeviceQueue(gfx.device, gfx.queue_present_idx, 0, &gfx.queue_present);

            CGE_LOG("[CGE] VK LOGICAL DEVICE\n");
        }
        {
            constexpr VkDeviceSize MiB{ VkDeviceSize(1) << 20 };
            gfx.buffer_vtx_size = 500 * MiB;
            gfx.buffer_idx_size =   1 * MiB;
            gfx.buffer_stg_size = 500 * MiB;
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
            const VkResult res_vtx{ vkCreateBuffer(gfx.device, &vtx_info, nullptr, &gfx.buffer_vtx) };
            const VkResult res_idx{ vkCreateBuffer(gfx.device, &idx_info, nullptr, &gfx.buffer_idx) };
            const VkResult res_stg{ vkCreateBuffer(gfx.device, &stg_info, nullptr, &gfx.buffer_stg) };
            CGE_ASSERT(res_vtx == VK_SUCCESS);
            CGE_ASSERT(res_idx == VK_SUCCESS);
            CGE_ASSERT(res_stg == VK_SUCCESS);

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetBufferMemoryRequirements.html
            vkGetBufferMemoryRequirements(gfx.device, gfx.buffer_vtx, &gfx.memreqs_vtx);
            vkGetBufferMemoryRequirements(gfx.device, gfx.buffer_idx, &gfx.memreqs_idx);
            vkGetBufferMemoryRequirements(gfx.device, gfx.buffer_stg, &gfx.memreqs_stg);
        
            CGE_LOG("[CGE] VK BUFFERS\n");
        }
        {
            const VkDeviceSize alloc_size{ gfx.memreqs_vtx.size + gfx.memreqs_idx.size + gfx.memreqs_stg.size };
            const IndexVK alloc_type{ gfx.memreqs_vtx.memoryTypeBits & gfx.memreqs_idx.memoryTypeBits & gfx.memreqs_stg.memoryTypeBits };
            constexpr VkMemoryPropertyFlags alloc_props{ VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };
            
            const VkPhysicalDeviceMemoryProperties& mem_props{ ctx.device_memory[gfx.device_idx] };
            const std::span<const VkMemoryType> mem_types{ mem_props.memoryTypes, mem_props.memoryTypeCount };
            const IndexVK mem_idx{ vk_find_memtype(mem_types, alloc_type, alloc_props) };
            CGE_ASSERT(mem_idx != null_idx);

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkMemoryAllocateInfo.html
            const VkMemoryAllocateInfo alloc_info{
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .pNext = {},
                .allocationSize = alloc_size,
                .memoryTypeIndex = mem_idx,
            };
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkAllocateMemory.html
            const VkResult res_alloc{ vkAllocateMemory(gfx.device, &alloc_info, nullptr, &gfx.buffer_memory) };
            CGE_ASSERT(res_alloc == VK_SUCCESS);

            gfx.buffer_capacity = alloc_size;

            CGE_LOG("[CGE] VK MEMORY\n");
        }
        {
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkBindBufferMemory.html
            const VkResult res_vtx{ vkBindBufferMemory(gfx.device, gfx.buffer_vtx, gfx.buffer_memory, gfx.buffer_vtx_offs) };
            const VkResult res_idx{ vkBindBufferMemory(gfx.device, gfx.buffer_idx, gfx.buffer_memory, gfx.buffer_idx_offs) };
            const VkResult res_stg{ vkBindBufferMemory(gfx.device, gfx.buffer_stg, gfx.buffer_memory, gfx.buffer_stg_offs) };
            CGE_ASSERT(res_vtx == VK_SUCCESS);
            CGE_ASSERT(res_idx == VK_SUCCESS);
            CGE_ASSERT(res_stg == VK_SUCCESS);

            CGE_LOG("[CGE] VK BIND BUFFERS\n");
        }
        {
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkCommandPoolCreateInfo.html
            const VkCommandPoolCreateInfo pool_info{
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .pNext = {},
                .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                .queueFamilyIndex = gfx.queue_graphics_idx,
            };
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCreateCommandPool.html
            const VkResult res_pool{ vkCreateCommandPool(gfx.device, &pool_info, nullptr, &gfx.command_pool) };
            CGE_ASSERT(res_pool == VK_SUCCESS);

            CGE_LOG("[CGE] VK COMMAND POOL\n");
        }
        {
            const shaderc::Compiler compiler{};
            const shaderc::CompileOptions options{};

            const std::string file_dir{ "shaders/glsl/" };
            compile_spirv(gfx.device, gfx.module_vertex, compiler, options, file_dir, "shader.vert", shaderc_shader_kind::shaderc_vertex_shader);
            compile_spirv(gfx.device, gfx.module_fragment, compiler, options, file_dir, "shader.frag", shaderc_shader_kind::shaderc_fragment_shader);

            CGE_LOG("[CGE] VK SHADERS\n");
        }
        {
            const VkSurfaceFormatKHR surface_format{ vk_ideal_format(gfx.ds_formats[gfx.device_idx]) };

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
            const VkResult res_pass{ vkCreateRenderPass(gfx.device, &pass_info, nullptr, &gfx.render_pass) };
            CGE_ASSERT(res_pass == VK_SUCCESS);

            CGE_LOG("[CGE] VK RENDER PASS\n");
        }
        {
            constexpr IndexVK max_frames{ 1 };

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
            const VkResult res_descset_layout{ vkCreateDescriptorSetLayout(gfx.device, &descriptor_layout_info, nullptr, &gfx.descriptor_layout) };
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
            const VkResult res_pool{ vkCreateDescriptorPool(gfx.device, &descriptor_pool_info, nullptr, &gfx.descriptor_pool) };
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
            const VkResult res_layout{ vkCreatePipelineLayout(gfx.device, &pipeline_layout_info, nullptr, &gfx.pipeline_layout) };
            CGE_ASSERT(res_layout == VK_SUCCESS);

            CGE_LOG("[CGE] VK PIPELINE LAYOUT\n");
        }
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
                .stride = IndexVK(sizeof(cge::Vertex)),
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
                .offset = IndexVK(attribute_xyzw.offset + sizeof(cge::vec4)),
            };
            constexpr VkVertexInputAttributeDescription attribute_st{
                .location = 2,
                .binding = binding_vertex.binding,
                .format = VK_FORMAT_R32G32_UINT,
                .offset = IndexVK(attribute_uv.offset + sizeof(cge::vec2)),
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

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkExtent2D.html
            const VkExtent2D ideal_res{ vk_ideal_resolution(gfx.surface_size, gfx.ds_capabilities[gfx.device_idx]) };

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkViewport.html
            const VkViewport viewport{
                .x = 0.0f,
                .y = 0.0f,
                .width = float(ideal_res.width),
                .height = float(ideal_res.height),
                .minDepth = 0.0f,
                .maxDepth = 0.0f,
            };
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkRect2D.html
            const VkRect2D scissor{
                .offset = { 0, 0 },
                .extent = ideal_res,
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
            const VkResult res_graphics{ vkCreateGraphicsPipelines(gfx.device, nullptr, pipeline_infos.size(), pipeline_infos.data(), nullptr, gfx.pipelines_graphics.data()) };
            CGE_ASSERT(res_graphics == VK_SUCCESS);

            CGE_LOG("[CGE] VK GRAPHICS PIPELINES\n");
        }
        {
            const VkSurfaceCapabilitiesKHR& ds_caps{ gfx.ds_capabilities[gfx.device_idx] };
            const std::span<const VkSurfaceFormatKHR> ds_formats{ gfx.ds_formats[gfx.device_idx] };
            const std::span<const VkPresentModeKHR> ds_modes{ gfx.ds_present_modes[gfx.device_idx] };

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSurfaceCapabilitiesKHR.html#_description
            const IndexVK min_images{ ds_caps.minImageCount };
            const IndexVK max_images{ ds_caps.maxImageCount };
            const IndexVK req_images{ min_images + 1};
            const IndexVK num_images{ (max_images == 0) ? req_images : std::min(req_images, max_images) };

            const VkSurfaceFormatKHR best_format{ vk_ideal_format(ds_formats) };
            const VkPresentModeKHR best_present{ vk_ideal_mode(ds_modes, vsync) };
            const VkExtent2D best_res{ vk_ideal_resolution(gfx.surface_size, ds_caps) };

            const bool queues_unique{ gfx.queue_graphics_idx != gfx.queue_present_idx };
            const IndexVK queue_concurrent_count{ static_cast<IndexVK>(queues_unique ? 2 : 0) };
            const std::array queue_indices{ gfx.queue_graphics_idx, gfx.queue_present_idx };
            const IndexVK* queue_indices_ptr{ queues_unique ? queue_indices.data() : nullptr };
            const VkSharingMode queue_sharing{ queues_unique ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE };

            {
                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSwapchainCreateInfoKHR.html#_description
                const VkSwapchainCreateInfoKHR swapchain_info{
                    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                    .pNext = {},
                    .flags = {},
                    .surface = gfx.surface_handle,
                    .minImageCount = num_images,
                    .imageFormat = best_format.format,
                    .imageColorSpace = best_format.colorSpace,
                    .imageExtent = best_res,
                    .imageArrayLayers = 1,
                    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                    .imageSharingMode = queue_sharing,
                    .queueFamilyIndexCount = queue_concurrent_count,
                    .pQueueFamilyIndices = queue_indices_ptr,
                    .preTransform = ds_caps.currentTransform,
                    .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                    .presentMode = best_present,
                    .clipped = VK_TRUE,
                    .oldSwapchain = gfx.swapchain,
                };
                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCreateSwapchainKHR.html
                const VkResult res_swapchain{ vkCreateSwapchainKHR(gfx.device, &swapchain_info, nullptr, &gfx.swapchain) };
                CGE_ASSERT(res_swapchain == VK_SUCCESS);

                CGE_LOG("[CGE] VK SWAPCHAIN\n");
            }
            {
                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetSwapchainImagesKHR.html
                const VkResult res_image_count{ vkGetSwapchainImagesKHR(gfx.device, gfx.swapchain, &gfx.num_frames, nullptr) };
                CGE_ASSERT(res_image_count == VK_SUCCESS);
                gfx.frame_idx = 0;
                gfx.frame_fence.resize(gfx.num_frames);
                gfx.frame_sem_render.resize(gfx.num_frames);
                gfx.frame_sem_image.resize(gfx.num_frames);
                gfx.frame_images.resize(gfx.num_frames);
                gfx.frame_views.resize(gfx.num_frames);
                gfx.frame_buffers.resize(gfx.num_frames);
                gfx.frame_commands.resize(gfx.num_frames);
                const VkResult res_images{ vkGetSwapchainImagesKHR(gfx.device, gfx.swapchain, &gfx.num_frames, gfx.frame_images.data()) };
                CGE_ASSERT(res_images == VK_SUCCESS);

                for (IndexVK idx{}; idx < gfx.num_frames; ++idx)
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
                    const VkResult res_fence{ vkCreateFence(gfx.device, &fence_info, nullptr, &gfx.frame_fence[idx]) };
                    CGE_ASSERT(res_fence == VK_SUCCESS);
                    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCreateSemaphore.html
                    const VkResult res_sem_render{ vkCreateSemaphore(gfx.device, &sem_info, nullptr, &gfx.frame_sem_render[idx]) };
                    CGE_ASSERT(res_sem_render == VK_SUCCESS);
                    const VkResult res_sem_image{ vkCreateSemaphore(gfx.device, &sem_info, nullptr, &gfx.frame_sem_image[idx]) };
                    CGE_ASSERT(res_sem_image == VK_SUCCESS);

                    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImageViewCreateInfo.html
                    const VkImageViewCreateInfo view_info{
                        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                        .pNext = {},
                        .flags = {},
                        .image = gfx.frame_images[idx],
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
                    const VkResult res_view{ vkCreateImageView(gfx.device, &view_info, nullptr, &gfx.frame_views[idx]) };
                    CGE_ASSERT(res_view == VK_SUCCESS);

                    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkFramebufferCreateInfo.html
                    const VkFramebufferCreateInfo buffer_info{
                        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                        .pNext = {},
                        .flags = {},
                        .renderPass = gfx.render_pass,
                        .attachmentCount = 1,
                        .pAttachments = &gfx.frame_views[idx],
                        .width = best_res.width,
                        .height = best_res.height,
                        .layers = 1,
                    };
                    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCreateFramebuffer.html
                    const VkResult res_buffer{ vkCreateFramebuffer(gfx.device, &buffer_info, nullptr, &gfx.frame_buffers[idx]) };
                    CGE_ASSERT(res_buffer == VK_SUCCESS);
                }

                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkCommandBufferAllocateInfo.html
                const VkCommandBufferAllocateInfo alloc_info{
                    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                    .pNext = {},
                    .commandPool = gfx.command_pool,
                    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                    .commandBufferCount = gfx.num_frames,
                };
                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkAllocateCommandBuffers.html
                const VkResult res_alloc{ vkAllocateCommandBuffers(gfx.device, &alloc_info, gfx.frame_commands.data()) };
                CGE_ASSERT(res_alloc == VK_SUCCESS);

                CGE_LOG("[CGE] VK SWAPCHAIN FRAMES\n");
            }
        }
        return gfx;
    }

    void Renderer_VK::destroy_renderable(Context& ctx, Renderable& gfx)
    {
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDeviceWaitIdle.html
        const VkResult res_wait{ vkDeviceWaitIdle(gfx.device) };
        (void)res_wait;

        if (gfx.atlas) destroy_atlas(ctx, gfx, *gfx.atlas);

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroyFramebuffer.html
        for (const VkFramebuffer buffer : gfx.frame_buffers) vkDestroyFramebuffer(gfx.device, buffer, nullptr);
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroyImageView.html
        for (const VkImageView view : gfx.frame_views) vkDestroyImageView(gfx.device, view, nullptr);

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroySemaphore.html
        for (const VkSemaphore sem : gfx.frame_sem_image) vkDestroySemaphore(gfx.device, sem, nullptr);
        for (const VkSemaphore sem : gfx.frame_sem_render) vkDestroySemaphore(gfx.device, sem, nullptr);
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroyFence.html
        for (const VkFence fence : gfx.frame_fence) vkDestroyFence(gfx.device, fence, nullptr);

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroySwapchainKHR.html
        vkDestroySwapchainKHR(gfx.device, gfx.swapchain, nullptr);

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroyPipeline.html
        for (const VkPipeline pipeline : gfx.pipelines_graphics) vkDestroyPipeline(gfx.device, pipeline, nullptr);

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroyDescriptorPool.html
        vkDestroyDescriptorPool(gfx.device, gfx.descriptor_pool, nullptr);

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroyDescriptorSetLayout.html
        vkDestroyDescriptorSetLayout(gfx.device, gfx.descriptor_layout, nullptr);

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroyPipelineLayout.html
        vkDestroyPipelineLayout(gfx.device, gfx.pipeline_layout, nullptr);

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroyRenderPass.html
        vkDestroyRenderPass(gfx.device, gfx.render_pass, nullptr);

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroyShaderModule.html
        vkDestroyShaderModule(gfx.device, gfx.module_fragment, nullptr);
        vkDestroyShaderModule(gfx.device, gfx.module_vertex, nullptr);

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroyCommandPool.html
        vkDestroyCommandPool(gfx.device, gfx.command_pool, nullptr);

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroyBuffer.html
        vkDestroyBuffer(gfx.device, gfx.buffer_stg, nullptr);
        vkDestroyBuffer(gfx.device, gfx.buffer_idx, nullptr);
        vkDestroyBuffer(gfx.device, gfx.buffer_vtx, nullptr);

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkFreeMemory.html
        vkFreeMemory(gfx.device, gfx.buffer_memory, nullptr);

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroySurfaceKHR.html
        vkDestroySurfaceKHR(ctx.instance, gfx.surface_handle, nullptr);

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroyDevice.html        
        vkDestroyDevice(gfx.device, nullptr);
    }
}

namespace cge
{
    Renderer_VK::Atlas Renderer_VK::create_atlas(Context& ctx, Renderable& gfx, Texture const texture)
    {
        Atlas atlas{ .tex = texture.empty() ? default_texture : texture };

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkExtent3D.html
        const VkExtent3D tex_extent{ .width = atlas.tex.width, .height = atlas.tex.height, .depth = 1 };

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
            const VkResult res_image{ vkCreateImage(gfx.device, &image_info, nullptr, &atlas.image) };
            CGE_ASSERT(res_image == VK_SUCCESS);

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetImageMemoryRequirements.html
            vkGetImageMemoryRequirements(gfx.device, atlas.image, &atlas.memreqs);
        }
        {
            const VkPhysicalDeviceMemoryProperties& mem_props{ ctx.device_memory[gfx.device_idx] };
            const std::span<const VkMemoryType> mem_types{ mem_props.memoryTypes, mem_props.memoryTypeCount };
            const IndexVK mem_idx{ vk_find_memtype(mem_types, atlas.memreqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) };
            CGE_ASSERT(mem_idx != null_idx);

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkMemoryAllocateInfo.html
            const VkMemoryAllocateInfo alloc_info{
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .pNext = {},
                .allocationSize = atlas.memreqs.size,
                .memoryTypeIndex = mem_idx,
            };
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkAllocateMemory.html
            const VkResult res_alloc{ vkAllocateMemory(gfx.device, &alloc_info, nullptr, &atlas.memory) };
            CGE_ASSERT(res_alloc == VK_SUCCESS);

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkBindImageMemory.html
            const VkResult res_bind{ vkBindImageMemory(gfx.device, atlas.image, atlas.memory, 0) };
            CGE_ASSERT(res_bind == VK_SUCCESS);
        }        
        {
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImageViewCreateInfo.html
            const VkImageViewCreateInfo view_info{
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext = {},
                .flags = {},
                .image = atlas.image,
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
            const VkResult res_view{ vkCreateImageView(gfx.device, &view_info, nullptr, &atlas.view) };
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
            const VkResult res_sampler{ vkCreateSampler(gfx.device, &sampler_info, nullptr, &atlas.sampler) };
            CGE_ASSERT(res_sampler == VK_SUCCESS);
        }
        return atlas;
    }

    void Renderer_VK::destroy_atlas(Context& ctx, Renderable& gfx, Atlas& atlas)
    {
        (void)ctx;

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroySampler.html
        vkDestroySampler(gfx.device, atlas.sampler, nullptr);

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroyImageView.html
        vkDestroyImageView(gfx.device, atlas.view, nullptr);

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroyImage.html
        vkDestroyImage(gfx.device, atlas.image, nullptr);

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkFreeMemory.html
        vkFreeMemory(gfx.device, atlas.memory, nullptr);
    }

    void Renderer_VK::upload_atlas(Context& ctx, Renderable& gfx, Atlas& atlas)
    {
        (void)ctx;
        
        const auto single_commands = [&](auto&& callback) noexcept
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
        };

        const auto transition_atlas = [&](const VkImageLayout old_layout, const VkImageLayout new_layout)
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
                .image = atlas.image,
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            };

            single_commands([&](const VkCommandBuffer command_buffer){
                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCmdPipelineBarrier.html
                vkCmdPipelineBarrier(command_buffer, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
            });
        };

        const auto stage_texture = [&]()
        {
            const VkDeviceSize tex_size{ VkDeviceSize(atlas.tex.size()) };
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
                (void)vk_map_bytes(buffer_size, buffer, offset, atlas.tex.as_bytes());

                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkUnmapMemory.html
                vkUnmapMemory(gfx.device, buffer_memory);
            }
        };

        const auto transfer_staged = [&]()
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
                    .width = atlas.tex.width,
                    .height = atlas.tex.height,
                    .depth = 1
                },
            };

            single_commands([&](const VkCommandBuffer command_buffer){
                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCmdCopyBufferToImage.html
                vkCmdCopyBufferToImage(command_buffer, gfx.buffer_stg, atlas.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
            });
        };

        const auto update_descriptors = [&]()
        {
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDescriptorImageInfo.html
            const VkDescriptorImageInfo image_info{
                .sampler = atlas.sampler,
                .imageView = atlas.view,
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
            vkUpdateDescriptorSets(gfx.device, IndexVK(writes.size()), writes.data(), IndexVK(copies.size()), copies.data());
        };

        transition_atlas(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        stage_texture();
        transfer_staged();
        transition_atlas(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        update_descriptors();
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

    std::uint64_t Renderer_VK::rank_device(Context& ctx, Renderable& gfx, IndexVK device_idx)
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

    std::uint64_t Renderer_VK::rank_device_graphics(Context& ctx, [[maybe_unused]] Renderable& gfx, IndexVK device_idx, IndexVK queue_idx)
    {
        const VkQueueFamilyProperties& qf_props{ ctx.device_queue_families[device_idx][queue_idx] };

        if ((qf_props.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) return 0;

        std::uint64_t rank{ 1 };

        rank += qf_props.queueCount;

        return rank;
    }

    std::uint64_t Renderer_VK::rank_device_present(Context& ctx, Renderable& gfx, IndexVK device_idx, IndexVK queue_idx)
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
        if (this->context.instance == nullptr)
        {
            this->context = this->create_context();
        }

        if (this->renderable.has_value())
        {
            destroy_renderable(this->context, *this->renderable);
            this->renderable.reset();
        }

        if (window)
        {
            Renderable& gfx{ this->renderable.emplace(create_renderable(this->context, wyn_window_t(window), impl.settings.vsync)) };
            
            Atlas& atlas{ gfx.atlas.emplace(create_atlas(this->context, gfx, default_texture)) };
            CGE_LOG("[CGE] VK ATLAS CREATE\n");
            
            upload_atlas(this->context, gfx, atlas);
            CGE_LOG("[CGE] VK ATLAS UPLOAD\n");
        }
    }

    void Renderer_VK::render(EngineImpl& impl)
    {
        Renderable& gfx{ this->renderable.value() };
        const bool res_render{ render_frame(gfx, impl.gfx_settings) };
        CGE_ASSERT(res_render);
        gfx.frame_idx = (gfx.frame_idx + 1) % gfx.num_frames;
    }
}

namespace cge
{
    bool Renderer_VK::render_frame(Renderable& gfx, const RenderSettings& settings)
    {
        const IndexVK this_frame{ gfx.frame_idx };
        const IndexVK prev_frame{ (this_frame + (gfx.num_frames - 1)) % gfx.num_frames };

        const VkSemaphore image_acquired{ gfx.frame_sem_image[this_frame] };
        const VkSemaphore render_finished{ gfx.frame_sem_render[this_frame] };
        const VkFence this_available{ gfx.frame_fence[this_frame] };
        const VkFence prev_available{ gfx.frame_fence[prev_frame] };

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkQueuePresentKHR.html

        const IndexVK image_idx{ acquire_image(gfx, image_acquired, std::array{ this_available, prev_available }, std::array{ this_available }) };
        if (image_idx == null_idx) return false;
        CGE_ASSERT(image_idx == this_frame);

        record_commands(gfx, image_idx, settings);
        if (!submit_commands(gfx, image_idx, std::array{ image_acquired }, std::array{ render_finished }, this_available)) return false;
        if (!present_image(gfx, image_idx, std::array{ render_finished })) return false;

        return true;
    }

    IndexVK Renderer_VK::acquire_image(Renderable& gfx, const VkSemaphore signal_sem, const std::span<const VkFence> wait_fences, const std::span<const VkFence> reset_fences)
    {
        constexpr std::uint64_t no_timeout{ std::uint64_t(~0) };

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkWaitForFences.html
        const VkResult res_wait{ vkWaitForFences(gfx.device, IndexVK(wait_fences.size()), wait_fences.data(), VK_TRUE, no_timeout) };
        CGE_ASSERT(res_wait == VK_SUCCESS);

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkAcquireNextImageKHR.html
        IndexVK image_idx{};
        const VkResult res_acquire{ vkAcquireNextImageKHR(gfx.device, gfx.swapchain, no_timeout, signal_sem, VK_NULL_HANDLE, &image_idx) };
        
        if (res_acquire == VK_ERROR_OUT_OF_DATE_KHR) return null_idx;

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkResetFences.html
        const VkResult res_reset{ vkResetFences(gfx.device, IndexVK(reset_fences.size()), reset_fences.data()) };
        CGE_ASSERT(res_reset == VK_SUCCESS);

        return image_idx;
    }

    void Renderer_VK::record_commands(Renderable& gfx, const IndexVK frame_idx, const RenderSettings& settings)
    {
        // ----------------------------------------------------------------

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkResetCommandBuffer.html
        const VkResult res_reset{ vkResetCommandBuffer(gfx.frame_commands[frame_idx], 0) };
        CGE_ASSERT(res_reset == VK_SUCCESS);

        // ----------------------------------------------------------------

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkExtent2D.html
        const VkExtent2D resolution{ vk_ideal_resolution(gfx.surface_size, gfx.ds_capabilities[gfx.device_idx]) };
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkViewport.html
        const VkViewport viewport{
            .x = 0.0f,
            .y = 0.0f,
            .width = float(resolution.width),
            .height = float(resolution.height),
            .minDepth = 0.0f,
            .maxDepth = 0.0f,
        };
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkRect2D.html
        const VkRect2D scissor{
            .offset = { 0, 0 },
            .extent = resolution,
        };
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkRect2D.html
        const VkRect2D image_rect{
            .offset = { 0, 0 },
            .extent = vk_ideal_resolution(gfx.surface_size, gfx.ds_capabilities[gfx.device_idx])
        };

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkClearValue.html
        const vec4 backcolor{ settings.backcolor.as_vec() };
        const VkClearValue clear_value{
            .color = {
                .float32 = {
                    backcolor.x,
                    backcolor.y,
                    backcolor.z,
                    backcolor.w,
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
            /* Triangle Fans   */ VertexSpan{settings.triangle_fan_vtx},
            /* Triangle Strips */ VertexSpan{settings.triangle_strip_vtx},
            /* Triangles       */ VertexSpan{settings.triangle_vtx},
            /* Line Strips     */ VertexSpan{settings.line_strip_vtx},
            /* Lines           */ VertexSpan{settings.line_vtx},
            /* Points          */ VertexSpan{settings.point_vtx},
        };

        const std::array<IndexSpan, num_pipelines> indices{
        #ifdef __APPLE__
            /* Triangle Fans   */ IndexSpan{},
        #else
            /* Triangle Fans   */ IndexSpan{settings.triangle_fan_idx},
        #endif
            /* Triangle Strips */ IndexSpan{settings.triangle_strip_idx},
            /* Triangles       */ IndexSpan{},
            /* Line Strips     */ IndexSpan{settings.line_strip_idx},
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
            /* Triangle Fans   */ std::as_bytes(std::span{settings.triangle_fan_vtx}),
            /* Triangle Strips */ std::as_bytes(std::span{settings.triangle_strip_vtx}),
            /* Triangles       */ std::as_bytes(std::span{settings.triangle_vtx}),
            /* Line Strips     */ std::as_bytes(std::span{settings.line_strip_vtx}),
            /* Lines           */ std::as_bytes(std::span{settings.line_vtx}),
            /* Points          */ std::as_bytes(std::span{settings.point_vtx}),
        };
        const std::array<ByteSpan, num_pipelines> idx_bytes{
        #ifdef __APPLE__
            /* Triangle Fans   */ ByteSpan{},
        #else
            /* Triangle Fans   */ std::as_bytes(std::span{settings.triangle_fan_idx}),
        #endif
            /* Triangle Strips */ std::as_bytes(std::span{settings.triangle_strip_idx}),
            /* Triangles       */ ByteSpan{},
            /* Line Strips     */ std::as_bytes(std::span{settings.line_strip_idx}),
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
            CGE_ASSERT(res_map == VK_SUCCESS);

            {
                VkDeviceSize offset{};

                offset = gfx.buffer_vtx_offs;
                for (std::size_t idx{}; idx < num_pipelines; ++idx)
                {
                    const VkDeviceSize rel_offs{ offset - gfx.buffer_vtx_offs };
                    const VkDeviceSize rel_size{ vtx_bytes[idx].size() };
                    const VkDeviceSize rel_end{ rel_offs + rel_size };
                    CGE_ASSERT(rel_end <= gfx.buffer_vtx_size);

                    vtx_offs[idx] = vk_map_bytes(buffer_size, buffer, offset, vtx_bytes[idx]);
                }

                offset = gfx.buffer_idx_offs;
                for (std::size_t idx{}; idx < num_pipelines; ++idx)
                {
                    const VkDeviceSize rel_offs{ offset - gfx.buffer_idx_offs };
                    const VkDeviceSize rel_size{ idx_bytes[idx].size() };
                    const VkDeviceSize rel_end{ rel_offs + rel_size };
                    CGE_ASSERT(rel_end <= gfx.buffer_idx_size);

                    idx_offs[idx] = vk_map_bytes(buffer_size, buffer, offset, idx_bytes[idx]);
                }
            }

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkUnmapMemory.html
            vkUnmapMemory(gfx.device, buffer_memory);
        }

        // ----------------------------------------------------------------

        const VkRenderPass render_pass{ gfx.render_pass };
        const VkCommandBuffer command_buffer{ gfx.frame_commands[frame_idx] };
        const VkFramebuffer frame_buffer{ gfx.frame_buffers[frame_idx] };

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkCommandBufferBeginInfo.html
        const VkCommandBufferBeginInfo begin_info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = {},
            .flags = {},
            .pInheritanceInfo = {},
        };
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkBeginCommandBuffer.html
        const VkResult res_begin{ vkBeginCommandBuffer(command_buffer, &begin_info) };
        CGE_ASSERT(res_begin == VK_SUCCESS);
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
                    const IndexVK vtx_count{ IndexVK(vertices[idx].size()) };
                    const IndexVK idx_count{ IndexVK(indices[idx].size()) };
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
        CGE_ASSERT(res_end == VK_SUCCESS);

        // ----------------------------------------------------------------
    }

    bool Renderer_VK::submit_commands(Renderable& gfx, const IndexVK frame_idx, const std::span<const VkSemaphore> wait_sems, const std::span<const VkSemaphore> signal_sems, const VkFence signal_fence)
    {
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPipelineStageFlags.html
        constexpr VkPipelineStageFlags stage_mask{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSubmitInfo.html
        const VkSubmitInfo submit_info{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = {},
            .waitSemaphoreCount = IndexVK(wait_sems.size()),
            .pWaitSemaphores = wait_sems.data(),
            .pWaitDstStageMask = &stage_mask,
            .commandBufferCount = 1,
            .pCommandBuffers = &gfx.frame_commands[frame_idx],
            .signalSemaphoreCount = IndexVK(signal_sems.size()),
            .pSignalSemaphores = signal_sems.data(),
        };
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkQueueSubmit.html
        const VkResult res_submit{ vkQueueSubmit(gfx.queue_graphics, 1, &submit_info, signal_fence) };
        
        if (res_submit == VK_ERROR_OUT_OF_DATE_KHR) return false;

        return true;
    }

    bool Renderer_VK::present_image(Renderable& gfx, const IndexVK frame_idx, const std::span<const VkSemaphore> wait_sems)
    {
        constexpr IndexVK num_swapchains{ 1 };
        const std::array<VkSwapchainKHR, num_swapchains> swapchains{ gfx.swapchain };
        const std::array<IndexVK, num_swapchains> indices{ frame_idx };
        std::array<VkResult, num_swapchains> results{};

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPresentInfoKHR.html
        const VkPresentInfoKHR present_info{
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext = {},
            .waitSemaphoreCount = IndexVK(wait_sems.size()),
            .pWaitSemaphores = wait_sems.data(),
            .swapchainCount = IndexVK(swapchains.size()),
            .pSwapchains = swapchains.data(),
            .pImageIndices = indices.data(),
            .pResults = results.data(),
        };
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkQueuePresentKHR.html
        const VkResult res_present{ vkQueuePresentKHR(gfx.queue_present, &present_info) };
        
        if (res_present == VK_ERROR_OUT_OF_DATE_KHR) return false;
        if (res_present == VK_SUBOPTIMAL_KHR) CGE_LOG("[CGE] SUBOPTIMAL-PRESENT\n");

        return true;
    }
}

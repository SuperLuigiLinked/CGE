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
    class Renderer_VK final : public Renderer 
    {
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
        :
        context{},
        renderable{}
    {
        this->context = this->create_context();
    }

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
    static std::string load_txt(const char* const filepath) noexcept
    {
        std::string txt{};
        std::ifstream file{ filepath };
        std::getline(file, txt, '\0');
        return txt;
    }

    static void compile_spirv(const VkDevice device, VkShaderModule& module, const shaderc::Compiler& compiler, const shaderc::CompileOptions& options, const std::string& file_dir, const char* const file_name, const shaderc_shader_kind shader_kind)
    {
        const std::string file_path{ file_dir + file_name };
        const std::string source{ cge::load_txt(file_path.c_str()) };
        if (source.empty())
        {
            const std::string full_path{ std::filesystem::current_path().append(file_path).lexically_normal().string() };
            CGE_LOG("[CGE] Unable to load file: \"{}\"\n", full_path);
        }
        CGE_ASSERT(!source.empty());

        const shaderc::SpvCompilationResult res_compile{ compiler.CompileGlslToSpv(source.c_str(), source.size(), shader_kind, file_name, options) };
        const shaderc_compilation_status status{ res_compile.GetCompilationStatus() };
        const std::size_t num_errors{ res_compile.GetNumErrors() };
        const std::size_t num_warnings{ res_compile.GetNumWarnings() };
        if (status != shaderc_compilation_status_success)
        {
            CGE_LOG("[CGE] SPIR-V Compilation Error! ({} Errors, {} Warnings)\n  {}{}\n", num_errors, num_warnings, file_dir, res_compile.GetErrorMessage());
        }
        CGE_ASSERT(status == shaderc_compilation_status_success);
        
        const std::vector<std::uint32_t> code{ res_compile.cbegin(), res_compile.cend() };

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkShaderModuleCreateInfo.html
        const VkShaderModuleCreateInfo module_info{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext = {},
            .flags = {},
            .codeSize = code.size() * sizeof(std::uint32_t),
            .pCode = code.data(),
        };
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCreateShaderModule.html
        const VkResult res_module{ vkCreateShaderModule(device, &module_info, nullptr, &module) };
        CGE_ASSERT(res_module == VK_SUCCESS);
    }
}

namespace cge
{
    static std::uint32_t vk_find_memtype(const std::span<const VkMemoryType>& mem_types, const std::uint32_t alloc_type, const std::uint32_t alloc_props) noexcept
    {
        for (std::uint32_t idx{}; idx < mem_types.size(); ++idx)
        {
            const VkMemoryType& mem_type{ mem_types[idx] };
            
            const std::uint32_t this_bit{ std::uint32_t(1) << idx };
            const bool has_type{ (this_bit & alloc_type) != 0 };
            const bool has_props{ (mem_type.propertyFlags & alloc_props) == alloc_props };

            if (has_type && has_props) return idx;
        }
        return std::uint32_t(-1);
    }

    #define CGE_FIND_RETURN(find, range, ...) if (find(__VA_ARGS__) != range.end()) return { __VA_ARGS__ }

    static VkSurfaceFormatKHR vk_ideal_format(const std::span<const VkSurfaceFormatKHR> formats) noexcept
    {
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSurfaceFormatKHR.html
        const auto find_format = [&](const VkFormat format, const VkColorSpaceKHR colorspace) {
            return std::ranges::find_if(formats, [&](const VkSurfaceFormatKHR elem) { return (elem.format == format) && (elem.colorSpace == colorspace); });
        };
        CGE_FIND_RETURN(find_format, formats, VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR);
        CGE_ASSERT(false);
    }

    static VkPresentModeKHR vk_ideal_mode [[maybe_unused]] (const std::span<const VkPresentModeKHR> modes, const bool vsync) noexcept
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

    static VkExtent2D vk_ideal_resolution [[maybe_unused]] (const VkExtent2D size, const VkSurfaceCapabilitiesKHR& caps) noexcept
    {
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSurfaceCapabilitiesKHR.html#_description
        constexpr std::uint32_t special_value{ 0xFFFFFFFF };

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
                .enabledLayerCount = std::uint32_t(req_instance_layers.size()),
                .ppEnabledLayerNames = req_instance_layers.data(),
                .enabledExtensionCount = std::uint32_t(req_instance_extensions.size()),
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
            CGE_ASSERT((res_extensions == VK_SUCCESS) || (res_extensions == VK_INCOMPLETE));

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkEnumerateInstanceLayerProperties.html
            const VkResult res_layer_count{ vkEnumerateInstanceLayerProperties(&ctx.instance_layer_count, nullptr) };
            CGE_ASSERT(res_layer_count == VK_SUCCESS);
            ctx.instance_layers.resize(ctx.instance_layer_count);
            const VkResult res_layers{ vkEnumerateInstanceLayerProperties(&ctx.instance_layer_count, ctx.instance_layers.data()) };
            CGE_ASSERT((res_layers == VK_SUCCESS) || (res_layers == VK_INCOMPLETE));

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
            CGE_ASSERT((res_handles == VK_SUCCESS) || (res_handles == VK_INCOMPLETE));
            
            CGE_LOG("[CGE] VK PHYSICAL DEVICES\n");
        }
        {
            CGE_ASSERT(ctx.device_count > 0);

            for (std::uint32_t idx{}; idx < ctx.device_count; ++idx)
            {
                VkPhysicalDevice& handle{ ctx.device_handles[idx] };
                std::uint32_t& extension_count{ ctx.device_extension_counts[idx] };
                std::uint32_t& layer_count{ ctx.device_layer_counts[idx] };
                std::uint32_t& queue_family_count{ ctx.device_queue_family_counts[idx] };
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
            const std::uint32_t device_count{  ctx.device_count };
            gfx.ds_present_supports.resize(device_count);
            gfx.ds_present_modes.resize(device_count);
            gfx.ds_formats.resize(device_count);
            gfx.ds_capabilities.resize(device_count);
            
            for (std::uint32_t di{}; di < device_count; ++di)
            {
                VkPhysicalDevice& device{ ctx.device_handles[di] };
                VkSurfaceKHR& surface{ gfx.surface_handle };
                std::vector<VkBool32>& present_supports{ gfx.ds_present_supports[di] };
                std::vector<VkPresentModeKHR>& present_modes{ gfx.ds_present_modes[di] };
                std::vector<VkSurfaceFormatKHR>& formats{ gfx.ds_formats[di] };
                VkSurfaceCapabilitiesKHR& capabilities{ gfx.ds_capabilities[di] };
                std::uint32_t& mode_count{ gfx.ds_mode_count };
                std::uint32_t& format_count{ gfx.ds_format_count };

                const std::uint32_t queue_count{ ctx.device_queue_family_counts[di] };
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

            const std::uint32_t device_count{ ctx.device_count };
            for (std::uint32_t di{}; di < device_count; ++di)
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

                    const std::uint32_t queue_count{ ctx.device_queue_family_counts[di] };
                    for (std::uint32_t qi{}; qi < queue_count; ++qi)
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
            const std::uint32_t queue_unique_count{ static_cast<std::uint32_t>(queues_unique ? 2 : 1) };
            //const std::uint32_t queue_concurrent_count{ static_cast<std::uint32_t>(queues_unique ? 2 : 0) };
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
            gfx.atlas.emplace(create_atlas(ctx, gfx, default_texture));

            CGE_LOG("[CGE] VK TEXTURE ATLAS\n");
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
            const std::uint32_t alloc_type{ gfx.memreqs_vtx.memoryTypeBits & gfx.memreqs_idx.memoryTypeBits & gfx.memreqs_stg.memoryTypeBits };
            constexpr VkMemoryPropertyFlags alloc_props{ VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };
            
            const VkPhysicalDeviceMemoryProperties& mem_props{ ctx.device_memory[gfx.device_idx] };
            const std::span<const VkMemoryType> mem_types{ mem_props.memoryTypes, mem_props.memoryTypeCount };
            const std::uint32_t mem_idx{ cge::vk_find_memtype(mem_types, alloc_type, alloc_props) };
            CGE_ASSERT(mem_idx != std::uint32_t(-1));

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
            const VkSurfaceFormatKHR surface_format{ cge::vk_ideal_format(gfx.ds_formats[gfx.device_idx]) };

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
            const shaderc::Compiler compiler{};
            const shaderc::CompileOptions options{};

            const std::string file_dir{ "shaders/glsl/" };
            cge::compile_spirv(gfx.device, gfx.module_vertex, compiler, options, file_dir, "shader.vert", shaderc_shader_kind::shaderc_vertex_shader);
            cge::compile_spirv(gfx.device, gfx.module_fragment, compiler, options, file_dir, "shader.frag", shaderc_shader_kind::shaderc_fragment_shader);

            CGE_LOG("[CGE] VK SHADERS\n");
        }
        return gfx;
    }

    void Renderer_VK::destroy_renderable(Context& ctx, Renderable& gfx)
    {
        if (gfx.atlas) destroy_atlas(ctx, gfx, *gfx.atlas);

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroyShaderModule.html
        vkDestroyShaderModule(gfx.device, gfx.module_fragment, nullptr);
        vkDestroyShaderModule(gfx.device, gfx.module_vertex, nullptr);

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroyRenderPass.html
        vkDestroyRenderPass(gfx.device, gfx.render_pass, nullptr);

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

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDeviceWaitIdle.html
        const VkResult res_wait{ vkDeviceWaitIdle(gfx.device) };
        (void)res_wait;

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkDestroyDevice.html        
        vkDestroyDevice(gfx.device, nullptr);
    }
}

namespace cge
{
    Renderer_VK::Atlas Renderer_VK::create_atlas(Context& ctx, Renderable& gfx, Texture const texture)
    {
        Atlas atlas{ .tex = (!texture.width || !texture.height || !texture.data) ? default_texture : texture };

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
            const std::uint32_t mem_idx{ cge::vk_find_memtype(mem_types, atlas.memreqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) };
            CGE_ASSERT(mem_idx != std::uint32_t(-1));

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
        if (this->renderable.has_value())
        {
            destroy_renderable(this->context, *this->renderable);
            this->renderable.reset();
        }

        if (window)
        {
            this->renderable.emplace(create_renderable(this->context, wyn_window_t(window), impl.settings.vsync));
        }
    }

    void Renderer_VK::render(EngineImpl& impl)
    {
        Renderable& gfx{ this->renderable.value() };
        (void)impl;
        (void)gfx;
    }
}

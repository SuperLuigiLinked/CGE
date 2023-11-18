# @file cge/CMakeLists.txt

# ================================================================================================================================

set(CGE_DEBUG ON CACHE BOOL "Enables internal debug logging.")
set(CGE_VALIDATE_VK ON CACHE BOOL "Enables Vulkan Validation.")

# ================================================================================================================================

add_library(cge)
add_library(cge::cge ALIAS cge)

target_link_libraries(cge PRIVATE wyn::wyn wyn::wyt Vulkan::Vulkan Vulkan::shaderc_combined)
if (LINUX)
    target_link_libraries(cge PRIVATE Vulkan::glslang Vulkan::SPIRV-Tools "/lib/libSPIRV-Tools-opt.so")
endif()

if (CGE_DEBUG)
    target_link_libraries(cge PRIVATE fmt::fmt)
    target_compile_definitions(cge PRIVATE "CGE_DEBUG")
endif()

if (CGE_VALIDATE_VK)
    target_compile_definitions(cge PRIVATE "CGE_VALIDATE_VK")
endif()

target_include_directories(cge PUBLIC "include/")

target_sources(cge
    PUBLIC
        "include/cge.hpp"
        
    PRIVATE
        "src/engine.hpp"
        "src/engine.cpp"
        "src/renderer_vk.cpp"
    
    PRIVATE
        "src/cvk/cvk.hpp"
        "src/cvk/cvk.cpp"

    PRIVATE
        "src/shaders/glsl/shader.vert"
        "src/shaders/glsl/shader.frag"
)
if (APPLE)
    target_sources(cge PRIVATE "src/cvk/cvk.mm")
    target_link_libraries(cge PRIVATE "-framework QuartzCore")
endif()

target_compile_features(cge PRIVATE cxx_std_23)

target_compile_options(cge
    PRIVATE
        "-Weverything"
        "-Wno-c++98-compat"
        "-Wno-c++98-compat-pedantic"
        "-Wno-pre-c++20-compat-pedantic"
        "-Wno-padded"
        "-Wno-switch-enum"
        "-Wno-braced-scalar-init"
        "-Wno-unsafe-buffer-usage"
        "-Wno-cast-function-type"
        "-Wno-cast-function-type-strict"
        "-Wno-unused-macros"
)

# ================================================================================================================================

add_custom_command(
    TARGET cge POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders" "${CMAKE_BINARY_DIR}/examples/shaders"
)

# ================================================================================================================================

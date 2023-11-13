# @file cge/CMakeLists.txt

# ================================================================================================================================

set(CGE_DEBUG ON CACHE BOOL "Enables internal debug logging.")

# ================================================================================================================================

add_library(cge)
add_library(cge::cge ALIAS cge)

target_link_libraries(cge PRIVATE wyn::wyn wyn::wyt Vulkan::Vulkan)

if (CGE_DEBUG)
    target_link_libraries(cge PRIVATE fmt::fmt)
    target_compile_definitions(cge PRIVATE "CGE_DEBUG")
endif()

target_include_directories(cge PUBLIC "include/")

target_sources(cge
    PUBLIC
        "include/cge/cge.hpp"
        "include/cge/engine.hpp"
        "include/cge/settings.hpp"
        "include/cge/types.hpp"
        "include/cge/vec.hpp"
 
    PRIVATE
        "src/engine.cpp"
        "src/events.cpp"
        "src/game.cpp"
        "src/renderer_vk.cpp"
        "src/debug.hpp"
        "src/impl.hpp"
        "src/renderer.hpp"
)

target_compile_features(cge PRIVATE cxx_std_23)

target_compile_options(cge
    PRIVATE
        "-Weverything"
        "-Wno-c++98-compat"
        "-Wno-c++98-compat-pedantic"
        "-Wno-pre-c++20-compat-pedantic"
        "-Wno-padded"
        "-Wno-switch-enum"
        "-Wno-cast-function-type-strict"
)

# ================================================================================================================================

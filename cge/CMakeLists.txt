# @file cge/CMakeLists.txt

# ================================================================================================================================

include(FetchContent)

FetchContent_Declare(WYN
    GIT_REPOSITORY "https://github.com/SuperLuigiLinked/Wyn"
    GIT_TAG "main"
)
FetchContent_MakeAvailable(WYN)

# ================================================================================================================================

add_library(cge)
add_library(cge::cge ALIAS cge)

target_link_libraries(cge PRIVATE wyn::wyn wyn::wyt)

target_include_directories(cge PUBLIC "include/")

target_sources(cge
    PUBLIC
        "include/cge/cge.hpp"
        "include/cge/engine.hpp"
        "include/cge/settings.hpp"
        "include/cge/types.hpp"
        "include/cge/vec.hpp"
)

target_sources(cge
    PRIVATE
        "src/engine.cpp"
        "src/events.cpp"
        "src/render.cpp"
        "src/update.cpp"
        "src/impl.hpp"
)

target_compile_features(cge PRIVATE cxx_std_23)

target_compile_options(cge
    PRIVATE
        "-Weverything"
        "-Wno-c++98-compat"
        "-Wno-c++98-compat-pedantic"
        "-Wno-pre-c++20-compat-pedantic"
        "-Wno-padded"
)

# ================================================================================================================================

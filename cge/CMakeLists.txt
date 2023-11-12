# @file cge/CMakeLists.txt

# ================================================================================================================================

add_library(cge)
add_library(cge::cge ALIAS cge)

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
)

target_compile_features(cge PRIVATE cxx_std_23)

target_compile_options(cge
    PRIVATE
        "-Weverything"
        "-Wno-c++98-compat"
)

# ================================================================================================================================

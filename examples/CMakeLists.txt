# @file examples/CMakeLists.txt

# ================================================================================================================================

include(FetchContent)

FetchContent_Declare(FMT
    GIT_REPOSITORY "https://github.com/fmtlib/fmt"
    GIT_TAG "10.1.1"
)
FetchContent_MakeAvailable(FMT)

# ================================================================================================================================

add_executable(cge_demo1)
add_executable(cge::demo1 ALIAS cge_demo1)

target_sources(cge_demo1 PRIVATE "demo1/main.cpp")
target_link_libraries(cge_demo1 cge::cge fmt::fmt)

target_compile_features(cge_demo1 PRIVATE cxx_std_23)

target_compile_options(cge_demo1
    PRIVATE
        "-Weverything"
        "-Wno-c++98-compat"
        "-Wno-c++98-compat-pedantic"
        "-Wno-pre-c++20-compat-pedantic"
        "-Wno-padded"
        "-Wno-missing-field-initializers"
)

# ================================================================================================================================

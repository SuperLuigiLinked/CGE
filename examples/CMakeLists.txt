# @file examples/CMakeLists.txt


# ================================================================================================================================

add_executable(cge_example)
add_executable(cge::example ALIAS cge_example)

target_sources(cge_example PRIVATE "example/main.cpp")
target_link_libraries(cge_example cge::cge fmt::fmt)

target_compile_features(cge_example PRIVATE cxx_std_23)

target_compile_options(cge_example
    PRIVATE
        "-Weverything"
        "-Wno-c++98-compat"
        "-Wno-c++98-compat-pedantic"
        "-Wno-pre-c++20-compat-pedantic"
        "-Wno-padded"
        "-Wno-missing-field-initializers"
)

# ================================================================================================================================

# @file CMakeLists.txt

# ================================================================================================================================

cmake_minimum_required(VERSION 3.25)
project(CGE)

# ================================================================================================================================

include(FetchContent)

FetchContent_Declare(FMT
    GIT_REPOSITORY "https://github.com/fmtlib/fmt"
    GIT_TAG "10.1.1"
)
FetchContent_MakeAvailable(FMT)

FetchContent_Declare(WYN
    GIT_REPOSITORY "https://github.com/SuperLuigiLinked/Wyn"
    GIT_TAG "main"
)
FetchContent_MakeAvailable(WYN)

find_package(Vulkan REQUIRED)

# ================================================================================================================================

add_subdirectory(cge)
add_subdirectory(examples)

# ================================================================================================================================

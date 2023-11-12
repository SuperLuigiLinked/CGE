# @file CMakeLists.txt

# ================================================================================================================================

cmake_minimum_required(VERSION 3.25)
project(CGE)

# ================================================================================================================================

include(FetchContent)

FetchContent_Declare(WYN
    GIT_REPOSITORY "https://github.com/SuperLuigiLinked/Wyn"
    GIT_TAG "main"
)
FetchContent_MakeAvailable(WYN)
FetchContent_GetProperties(WYN SOURCE_DIR WYN_SOURCE_DIR)

# ================================================================================================================================

add_subdirectory(cge)
add_subdirectory(examples)

# ================================================================================================================================

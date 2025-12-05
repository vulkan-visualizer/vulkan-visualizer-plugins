cmake_minimum_required(VERSION 3.28)
project(vulkan_visualizer_plugins VERSION 1.1.0 LANGUAGES CXX)

# ============================================================================
# C++ Configuration
# ============================================================================
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)


# ============================================================================
# Dependencies
# ============================================================================
include(FetchContent)
FetchContent_Declare(imgui_src
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.92.2b-docking
        GIT_SHALLOW TRUE
)
FetchContent_Declare(stb
        GIT_REPOSITORY https://github.com/nothings/stb.git
        GIT_TAG master
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui_src stb)
set(_IMGUI_SOURCES
        ${imgui_src_SOURCE_DIR}/imgui.cpp
        ${imgui_src_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_src_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_src_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_src_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_src_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
        ${imgui_src_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)
set(_STB_IMPL "${CMAKE_CURRENT_BINARY_DIR}/stb_impl.cpp")
file(WRITE "${_STB_IMPL}" [[
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4100 4127 4189 4244 4365 4514 4820 4996)
#endif
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
]])
FetchContent_Declare(vulkan-visualizer
        GIT_REPOSITORY https://github.com/vulkan-visualizer/vulkan-visualizer.git
        GIT_TAG master
        GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(imgui_src stb vulkan-visualizer)


# ============================================================================
# Library Target
# ============================================================================
add_library(vk_vis_plugins)
add_library(vulkan_visualizer::plugins ALIAS vk_vis_plugins)
target_sources(vk_vis_plugins
        PRIVATE
        src/vk.plugins.camera.cpp
        src/vk.plugins.renderer.cpp
        src/vk.plugins.ui.cpp
        ${_IMGUI_SOURCES}
        ${_STB_IMPL}
        PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES
        module/vk.plugins.camera.ixx
        module/vk.plugins.renderer.ixx
        module/vk.plugins.ui.ixx
)
target_link_libraries(vk_vis_plugins PRIVATE vulkan_visualizer::vk_vis)
target_include_directories(vk_vis_plugins PRIVATE
        $<BUILD_INTERFACE:${imgui_src_SOURCE_DIR}>
        $<BUILD_INTERFACE:${stb_SOURCE_DIR}>
)


# ============================================================================
# Test
# ============================================================================
enable_testing()

add_executable(test_camera test/test_camera.cpp)
target_link_libraries(test_camera PRIVATE vulkan_visualizer::plugins)
add_test(NAME run_test_camera COMMAND test_camera)
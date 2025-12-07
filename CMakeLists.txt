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
FetchContent_Declare(vulkan-visualizer
        GIT_REPOSITORY https://github.com/vulkan-visualizer/vulkan-visualizer.git
        GIT_TAG master
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(vulkan-visualizer)

# ============================================================================
# Library Target
# ============================================================================
add_library(vk_vis_plugins)
add_library(vulkan_visualizer::plugins ALIAS vk_vis_plugins)
target_sources(vk_vis_plugins
        PRIVATE
        src/vk.plugins.viewport.cpp
        PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES
        module/vk.plugins.viewport.ixx
)
target_link_libraries(vk_vis_plugins PRIVATE vulkan_visualizer::vk_vis)


# ============================================================================
# Test
# ============================================================================
function(compile_all_glsl_shaders)
    cmake_parse_arguments(ARG "" "SOURCE_ROOT" "" ${ARGN})

    if (NOT ARG_SOURCE_ROOT)
        message(FATAL_ERROR "compile_all_glsl_shaders: SOURCE_ROOT is required")
    endif()

    find_program(GLSLC_EXECUTABLE glslc REQUIRED)
    message(STATUS "Using glslc: ${GLSLC_EXECUTABLE}")

    # Find shader directories but ignore _deps
    file(GLOB_RECURSE SHADER_DIRS
            LIST_DIRECTORIES true
            RELATIVE "${ARG_SOURCE_ROOT}"
            "${ARG_SOURCE_ROOT}/*/shader"
            "${ARG_SOURCE_ROOT}/*/shaders"
            "${ARG_SOURCE_ROOT}/shader"
            "${ARG_SOURCE_ROOT}/shaders"
    )

    set(SPIRV_OUTPUTS)

    foreach(dir IN LISTS SHADER_DIRS)
        set(ABS_DIR "${ARG_SOURCE_ROOT}/${dir}")

        file(GLOB SHADER_SOURCES
                RELATIVE "${ARG_SOURCE_ROOT}"
                "${ABS_DIR}/*.vert"
                "${ABS_DIR}/*.frag"
                "${ABS_DIR}/*.comp"
                "${ABS_DIR}/*.geom"
                "${ABS_DIR}/*.tesc"
                "${ABS_DIR}/*.tese"
        )

        foreach(src IN LISTS SHADER_SOURCES)
            set(ABS_SRC "${ARG_SOURCE_ROOT}/${src}")
            set(SPV "${CMAKE_BINARY_DIR}/${src}.spv")

            get_filename_component(SPV_DIR "${SPV}" DIRECTORY)
            file(MAKE_DIRECTORY "${SPV_DIR}")

            add_custom_command(
                    OUTPUT "${SPV}"
                    COMMAND "${GLSLC_EXECUTABLE}"
                    --target-env=vulkan1.3
                    -O
                    -c
                    "${ABS_SRC}"
                    -o "${SPV}"
                    DEPENDS "${ABS_SRC}"
                    COMMENT "Compiling GLSL ${src}"
                    VERBATIM
            )

            list(APPEND SPIRV_OUTPUTS "${SPV}")
        endforeach()
    endforeach()

    if (SPIRV_OUTPUTS)
        add_custom_target(shaders DEPENDS ${SPIRV_OUTPUTS})
        message(STATUS "Shader build target created: shaders")
    endif()
endfunction()

compile_all_glsl_shaders(
        SOURCE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/test
)

enable_testing()

file(GLOB_RECURSE TEST_SOURCES
        CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/test/*.cpp"
)

foreach (test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)

    add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name} PRIVATE vulkan_visualizer::plugins)
    add_dependencies(${test_name} shaders)

    add_test(NAME run_${test_name} COMMAND ${test_name})

    if (WIN32)
        add_custom_command(
                TARGET ${test_name}
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:${test_name}>
                $<TARGET_FILE_DIR:${test_name}>
                COMMAND_EXPAND_LISTS
        )
    endif ()
endforeach ()
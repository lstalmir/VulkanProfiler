# CMakeList.txt : CMake project for Vulkan traits, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project (profiler_shaders)

find_package (PythonInterp 3.4 REQUIRED)
find_package (PythonLibs 3.4 REQUIRED)

set (compiled_shaders_header ${CMAKE_CURRENT_BINARY_DIR}/profiler_overlay_draw_stats_shaders.generated.h)

set (shaders
    ${CMAKE_CURRENT_SOURCE_DIR}/profiler_overlay_draw_stats.frag.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/profiler_overlay_draw_stats.vert.glsl
    )

configure_file (
    ${CMAKE_CURRENT_SOURCE_DIR}/generated.py.in
    generated.py)

add_custom_target (profiler_shaders ALL
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/compile_shaders.py ${CMAKE_CURRENT_BINARY_DIR}
    BYPRODUCTS ${compiled_shaders_header}
    COMMENT "Compiling shaders")

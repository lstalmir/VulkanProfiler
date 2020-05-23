# CMakeList.txt : CMake project for profiler layer, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project (profiler_output)

set (headers
    "profiler_output_console.h"
    "profiler_output_overlay.h"
    )

set (sources
    "profiler_output_console.cpp"
    "profiler_output_overlay.cpp"
    )

set (imgui
    "imgui/imgui.h"
    "imgui/imgui.cpp"
    "imgui/imgui_demo.cpp"
    "imgui/imgui_draw.cpp"
    "imgui/imgui_widgets.cpp"
    "imgui/examples/imgui_impl_win32.h"
    "imgui/examples/imgui_impl_win32.cpp"
    "imgui_impl_vulkan_layer.h"
    "imgui_impl_vulkan_layer.cpp"
    )

set (imgui_widgets
    "imgui_widgets/imgui_histogram_ex.h"
    "imgui_widgets/imgui_histogram_ex.cpp"
    )
    
# Link intermediate static library
add_library (profiler_output
    ${sources}
    ${headers}
    ${imgui}
    ${imgui_widgets})

# Set include path
target_include_directories (profiler_output
    PRIVATE ${VULKAN_HEADERS_INCLUDE_DIR}
    PRIVATE ${VULKAN_LOADER_INCLUDE_DIR}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../vulkan_traits
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/shaders
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/resources
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/imgui)

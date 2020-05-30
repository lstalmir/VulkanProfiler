# CMakeList.txt : CMake project for profiler layer, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project (profiler_overlay)

set (headers
    "profiler_overlay.h"
    )

set (sources
    "profiler_overlay.cpp"
    )

set (imgui
    "imgui/imgui.h"
    "imgui/imgui.cpp"
    "imgui/imgui_demo.cpp"
    "imgui/imgui_draw.cpp"
    "imgui/imgui_widgets.cpp"
    "imgui_impl_vulkan_layer.h"
    "imgui_impl_vulkan_layer.cpp"
    )

# Platform-dependent implementation
if (WIN32)
set (imgui_platform
    "imgui/examples/imgui_impl_win32.h"
    "imgui/examples/imgui_impl_win32.cpp"
    )
elseif (X11_FOUND)
set (imgui_platform
    "imgui_impl_xlib.h"
    "imgui_impl_xlib.cpp"
    )
endif ()

set (imgui_widgets
    "imgui_widgets/imgui_histogram_ex.h"
    "imgui_widgets/imgui_histogram_ex.cpp"
    )
    
# Link intermediate static library
add_library (profiler_overlay
    ${sources}
    ${headers}
    ${imgui}
    ${imgui_platform}
    ${imgui_widgets})

# Set include path
target_include_directories (profiler_overlay
    PRIVATE ${VULKAN_HEADERS_INCLUDE_DIR}
    PRIVATE ${VULKAN_LOADER_INCLUDE_DIR}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../VkLayer_profiler_layer
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../vulkan_traits
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/shaders
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/resources
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/imgui)

# Set include paths and link window libraries in linux builds
if (UNIX)

    if (Wayland_FOUND)
        target_link_libraries (profiler_overlay ${Wayland_LIBRARIES})
        target_include_directories (profiler_overlay
            PRIVATE ${Wayland_INCLUDE_DIRS})
    endif () # Wayland_FOUND

    if (XCB_FOUND)
        target_link_libraries (profiler_overlay ${XCB_LIBRARIES})
        target_include_directories (profiler_overlay
            PRIVATE ${XCB_INCLUDE_DIRS})
    endif () # XCB_FOUND

    if (X11_FOUND)
        target_link_libraries (profiler_overlay ${X11_LIBRARIES} )
        target_include_directories (profiler_overlay
            PRIVATE ${X11_INCLUDE_DIR})
    endif () # X11_FOUND
    if (X11_Xrandr_FOUND)
        target_link_libraries (profiler_overlay ${X11_Xrandr_LIB})
        target_include_directories (profiler_overlay
            PRIVATE ${X11_Xrandr_INCLUDE_PATH})
    endif () # X11_Xrandr_FOUND

endif () # UNIX

# Copyright (c) 2019-2021 Lukasz Stalmirski
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required (VERSION 3.8...3.31)

project (profiler_overlay)

set (headers
    "profiler_overlay.h"
    "profiler_overlay_resources.h"
    "profiler_overlay_settings.h"
    "profiler_overlay_shader_view.h"
    "lang/en_us.h"
    "lang/pl_pl.h"
    )

set (sources
    "profiler_overlay.cpp"
    "profiler_overlay_resources.cpp"
    "profiler_overlay_settings.cpp"
    "profiler_overlay_shader_view.cpp"
    )

set (imgui
    "imgui_impl_vulkan_layer.h"
    "imgui_impl_vulkan_layer.cpp"
    "imgui_window.h"
    )

# Platform-dependent implementation
if (WIN32)
set (imgui_platform
    "imgui_impl_win32.h"
    "imgui_impl_win32.cpp"
    )
elseif (UNIX)
if (X11_FOUND)
set (imgui_platform_x11
    "imgui_impl_xlib.h"
    "imgui_impl_xlib.cpp")
endif ()
if (XCB_FOUND)
set (imgui_platform_xcb
    "imgui_impl_xcb.h"
    "imgui_impl_xcb.cpp")
endif ()
if (Wayland_FOUND)
set (imgui_platform_wayland
    "imgui_impl_wayland.h"
    "imgui_impl_wayland.cpp")
endif ()
set (imgui_platform
    "imgui_impl_xkb.h"
    "imgui_impl_xkb.cpp"
    ${imgui_platform_x11}
    ${imgui_platform_xcb}
    #${imgui_platform_wayland}
    )
endif ()

set (imgui_widgets
    "imgui_widgets/imgui_breakdown_ex.h"
    "imgui_widgets/imgui_breakdown_ex.cpp"
    "imgui_widgets/imgui_histogram_ex.h"
    "imgui_widgets/imgui_histogram_ex.cpp"
    "imgui_widgets/imgui_table_ex.h"
    "imgui_widgets/imgui_table_ex.cpp"
    "imgui_widgets/imgui_ex.h"
    "imgui_widgets/imgui_ex.cpp"
    )

# Generate assets file
add_custom_command (
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/profiler_overlay_assets.h"
    COMMAND "${Python3_EXECUTABLE}"
        "${PROFILER_SCRIPTS_DIR}/gen_assets.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/profiler_overlay_assets.json"
        --output "${CMAKE_CURRENT_BINARY_DIR}/profiler_overlay_assets.h"
        --group OverlayAssets
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/profiler_overlay_assets.json"
    DEPENDS "${PROFILER_SCRIPTS_DIR}/gen_assets.py")

add_custom_target (profiler_overlay_assets
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/profiler_overlay_assets.h")

# Link intermediate static library
add_library (profiler_overlay
    ${sources}
    ${headers}
    ${imgui}
    ${imgui_platform}
    ${imgui_widgets})

target_link_libraries (profiler_overlay
    PUBLIC profiler_common
    PRIVATE imgui
    PRIVATE imgui-text-editor
    PRIVATE imgui-file-dialog
    PRIVATE stb
    PRIVATE profiler
    PRIVATE profiler_helpers)

add_dependencies (profiler_overlay profiler_overlay_assets)

# Set include paths and link window libraries in linux builds
if (UNIX)

    target_link_libraries (profiler_overlay PRIVATE xkbcommon)

    if (Wayland_FOUND)
        target_link_libraries (profiler_overlay PRIVATE ${Wayland_LIBRARIES})
        target_include_directories (profiler_overlay PRIVATE ${Wayland_INCLUDE_DIRS})
    endif () # Wayland_FOUND

    if (XCB_FOUND)
        target_link_libraries (profiler_overlay PRIVATE ${XCB_LIBRARIES})
        target_include_directories (profiler_overlay PRIVATE ${XCB_INCLUDE_DIRS})
    endif () # XCB_FOUND

    if (X11_FOUND)
        target_link_libraries (profiler_overlay PRIVATE ${X11_LIBRARIES} )
        target_include_directories (profiler_overlay PRIVATE ${X11_INCLUDE_DIR})
    endif () # X11_FOUND

endif () # UNIX

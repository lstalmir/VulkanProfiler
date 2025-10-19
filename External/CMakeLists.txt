# Copyright (c) 2023-2025 Lukasz Stalmirski
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

set (CMAKE_FOLDER External)

# Disable all warnings in this directory
if (MSVC)
    add_compile_options (/W0)
else ()
    add_compile_options (-w)
endif ()

# fmtlib/fmt
add_subdirectory (fmt)

# google/farmhash
add_library (farmhash STATIC
    farmhash/src/farmhash.h
    farmhash/src/farmhash.cc)

if (MSVC)
    # Disable __builtin_expect in MSVC builds
    target_compile_definitions (farmhash
        PUBLIC FARMHASH_NO_BUILTIN_EXPECT)
endif ()

target_include_directories (farmhash
    PUBLIC farmhash/src)

target_compile_definitions (farmhash
    PUBLIC NAMESPACE_FOR_HASH_FUNCTIONS=Farmhash)

# google/googletest
set (gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set (INSTALL_GTEST OFF CACHE BOOL "" FORCE)
add_subdirectory (googletest)

# intel/metrics-discovery
add_library (metrics-discovery INTERFACE)
target_include_directories (metrics-discovery
    INTERFACE metrics-discovery/inc/common/instrumentation/api)

# KhronosGroup/Vulkan-Headers
add_subdirectory (Vulkan-Headers)
set (VULKAN_HEADERS_INSTALL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Vulkan-Headers" PARENT_SCOPE)

# KhronosGroup/Vulkan-Utility-Libraries
add_subdirectory (Vulkan-Utility-Libraries)

# KhronosGroup/SPIRV-Headers
add_subdirectory (SPIRV-Headers)

# KhronosGroup/SPIRV-Tools
add_subdirectory (SPIRV-Tools)

# KhronosGroup/SPIRV-Cross
add_subdirectory (SPIRV-Cross)

# SPIRV-Specification
add_subdirectory (SPIRV-Specification)

# GPUOpen/VulkanMemoryAllocator
add_subdirectory (VulkanMemoryAllocator)

# nlohmann/json
set (JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory (json)

# ocornut/imgui
set (imgui_backends
    imgui/backends/imgui_impl_vulkan.h
    imgui/backends/imgui_impl_vulkan.cpp)

add_library (imgui STATIC
    imgui/imgui.h
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/misc/cpp/imgui_stdlib.h
    imgui/misc/cpp/imgui_stdlib.cpp
    ${imgui_backends})

target_include_directories (imgui
    PUBLIC imgui
    PUBLIC imgui/backends
    PUBLIC imgui/misc/cpp)

target_compile_definitions (imgui
    PUBLIC IMGUI_USER_CONFIG="${CMAKE_CURRENT_SOURCE_DIR}/imgui_config.h"
    PRIVATE VK_NO_PROTOTYPES=1)

target_link_libraries (imgui
    PUBLIC Vulkan-Headers)

# lstalmir/imgui-text-editor
add_library (imgui-text-editor STATIC
    imgui-text-editor/TextEditor.h
    imgui-text-editor/TextEditor.cpp)

target_include_directories (imgui-text-editor
    PUBLIC imgui-text-editor)

target_link_libraries (imgui-text-editor
    PUBLIC imgui)

# aiekick/ImGuiFileDialog
add_library (imgui-file-dialog STATIC
    imgui-file-dialog/ImGuiFileDialog.h
    imgui-file-dialog/ImGuiFileDialog.cpp
    imgui-file-dialog/ImGuiFileDialogConfig.h)

target_include_directories (imgui-file-dialog
    PUBLIC imgui-file-dialog)

target_link_libraries (imgui-file-dialog
    PUBLIC imgui)

# epezent/implot
add_library (implot STATIC
    implot/implot.h
    implot/implot.cpp
    implot/implot_internal.h
    implot/implot_items.cpp)

target_include_directories (implot
    PUBLIC implot)

target_link_libraries (implot
    PUBLIC imgui)

# nothings/stb
add_library (stb INTERFACE)
target_include_directories (stb
    INTERFACE stb)

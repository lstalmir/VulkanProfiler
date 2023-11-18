# Copyright (c) 2023 Lukasz Stalmirski
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

cmake_minimum_required (VERSION 3.8)

set (CMAKE_FOLDER External)

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

# KhronosGroup/SPIRV-Headers
add_subdirectory (SPIRV-Headers)

# KhronosGroup/SPIRV-Tools
add_subdirectory (SPIRV-Tools)

# nlohmann/json
set (JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory (json)

# ocornut/imgui
set (imgui_backends
    imgui/backends/imgui_impl_vulkan.h
    imgui/backends/imgui_impl_vulkan.cpp)

if (WIN32)
set (imgui_backends ${imgui_backends}
    imgui/backends/imgui_impl_win32.h
    imgui/backends/imgui_impl_win32.cpp)
endif ()

add_library (imgui STATIC
    imgui/imgui.h
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    ${imgui_backends})

target_include_directories (imgui
    PUBLIC imgui
    PUBLIC imgui/backends)

target_link_libraries (imgui
    PUBLIC Vulkan-Headers)

# BalazsJako/ImGuiColorTextEdit
add_library (imguicolortextedit STATIC
    ImGuiColorTextEdit/TextEditor.h
    ImGuiColorTextEdit/TextEditor.cpp)

target_link_libraries (imguicolortextedit
    PUBLIC imgui)

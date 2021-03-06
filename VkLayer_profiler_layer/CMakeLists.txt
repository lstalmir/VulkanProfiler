﻿# Copyright (c) 2019-2021 Lukasz Stalmirski
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

find_package (Python3 REQUIRED)

set (PROFILER_LAYER_VER_MAJOR 0)
set (PROFILER_LAYER_VER_MINOR 1)
set (PROFILER_LAYER_VER_PATCH 0)
set (PROFILER_LAYER_VER_IMPLEMENTATION 1)
set (PROFILER_LAYER_NAME VK_LAYER_profiler)
set (PROFILER_LAYER_PROJECTNAME VkLayer_profiler_layer)
set (PROFILER_LAYER_COMPANYNAME )
set (PROFILER_LAYER_FILEDESCRIPTION )
set (PROFILER_LAYER_LEGALCOPYRIGHT )
set (PROFILER_LAYER_PRODUCTNAME "Vulkan(R) Profiling Layer")

project (${PROFILER_LAYER_PROJECTNAME})

if (WIN32)

    add_definitions (-DVK_USE_PLATFORM_WIN32_KHR)
    add_definitions (-DNOMINMAX)

elseif (UNIX)

    add_compile_options (-fPIC)

    if (X11_FOUND)
        add_definitions (-DVK_USE_PLATFORM_XLIB_KHR)
    endif ()

    if (XCB_FOUND)
        add_definitions (-DVK_USE_PLATFORM_XCB_KHR)
    endif ()

    if (Wayland_FOUND)
        add_definitions (-DVK_USE_PLATFORM_WAYLAND_KHR)
    endif ()

endif ()

# Generate dispatch tables
add_custom_command (
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/vk_dispatch_tables.h
    COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen_dispatch_tables.py
        ${VULKAN_HEADERS_INSTALL_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS Vulkan-Headers
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen_dispatch_tables.py)

add_custom_target (profiler_pre_build
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/vk_dispatch_tables.h)

# Generated files
include_directories (${CMAKE_CURRENT_BINARY_DIR})

# Utilities
include_directories (utils)

# Include implementation of the layer
add_subdirectory (profiler)
add_subdirectory (profiler_ext)
add_subdirectory (profiler_helpers)
add_subdirectory (profiler_overlay)
add_subdirectory (profiler_trace)

add_dependencies (profiler profiler_pre_build)
add_dependencies (profiler_helpers profiler_pre_build)
add_dependencies (profiler_ext profiler_pre_build)
add_dependencies (profiler_overlay profiler_pre_build)
add_dependencies (profiler_trace profiler_pre_build)

# Enable tests
if (BUILD_TESTS)
    add_subdirectory (profiler_tests)
endif ()

set (resource
    ${CMAKE_CURRENT_BINARY_DIR}/${PROFILER_LAYER_PROJECTNAME}.rc
    )

set (generated
    ${CMAKE_CURRENT_BINARY_DIR}/${PROFILER_LAYER_PROJECTNAME}.generated.h
    )

set (json
    ${CMAKE_CURRENT_BINARY_DIR}/${PROFILER_LAYER_PROJECTNAME}.json
    )

set (helper_functions
    "profiler_layer_functions/Dispatch.h"
    "profiler_layer_functions/Helpers.h"
    )

set (core_functions
    "profiler_layer_functions/core/VkCommandBuffer_functions.cpp"
    "profiler_layer_functions/core/VkCommandBuffer_functions.h"
    "profiler_layer_functions/core/VkDevice_functions.cpp"
    "profiler_layer_functions/core/VkDevice_functions.h"
    "profiler_layer_functions/core/VkDevice_functions_base.cpp"
    "profiler_layer_functions/core/VkDevice_functions_base.h"
    "profiler_layer_functions/core/VkInstance_functions.cpp"
    "profiler_layer_functions/core/VkInstance_functions.h"
    "profiler_layer_functions/core/VkInstance_functions_base.cpp"
    "profiler_layer_functions/core/VkInstance_functions_base.h"
    "profiler_layer_functions/core/VkLoader_functions.cpp"
    "profiler_layer_functions/core/VkLoader_functions.h"
    "profiler_layer_functions/core/VkPhysicalDevice_functions.cpp"
    "profiler_layer_functions/core/VkPhysicalDevice_functions.h"
    "profiler_layer_functions/core/VkQueue_functions.cpp"
    "profiler_layer_functions/core/VkQueue_functions.h"
    )

set (extension_functions
    "profiler_layer_functions/extensions/VkCreateRenderPass2Khr_functions.cpp"
    "profiler_layer_functions/extensions/VkCreateRenderPass2Khr_functions.h"
    "profiler_layer_functions/extensions/VkDebugMarkerExt_functions.cpp"
    "profiler_layer_functions/extensions/VkDebugMarkerExt_functions.h"
    "profiler_layer_functions/extensions/VkDebugUtilsExt_functions.cpp"
    "profiler_layer_functions/extensions/VkDebugUtilsExt_functions.h"
    "profiler_layer_functions/extensions/VkDrawIndirectCountAmd_functions.cpp"
    "profiler_layer_functions/extensions/VkDrawIndirectCountAmd_functions.h"
    "profiler_layer_functions/extensions/VkDrawIndirectCountKhr_functions.cpp"
    "profiler_layer_functions/extensions/VkDrawIndirectCountKhr_functions.h"
    "profiler_layer_functions/extensions/VkSurfaceKhr_functions.cpp"
    "profiler_layer_functions/extensions/VkSurfaceKhr_functions.h"
    "profiler_layer_functions/extensions/VkSwapchainKhr_functions.cpp"
    "profiler_layer_functions/extensions/VkSwapchainKhr_functions.h"
    "profiler_layer_functions/extensions/VkWaylandSurfaceKhr_functions.cpp"
    "profiler_layer_functions/extensions/VkWaylandSurfaceKhr_functions.h"
    "profiler_layer_functions/extensions/VkWin32SurfaceKhr_functions.cpp"
    "profiler_layer_functions/extensions/VkWin32SurfaceKhr_functions.h"
    "profiler_layer_functions/extensions/VkXcbSurfaceKhr_functions.cpp"
    "profiler_layer_functions/extensions/VkXcbSurfaceKhr_functions.h"
    "profiler_layer_functions/extensions/VkXlibSurfaceKhr_functions.cpp"
    "profiler_layer_functions/extensions/VkXlibSurfaceKhr_functions.h"
    )

# Configure resources file
configure_file (
    ${CMAKE_CURRENT_SOURCE_DIR}/${PROFILER_LAYER_PROJECTNAME}.rc.in
    ${resource}
    @ONLY)

configure_file (
    ${CMAKE_CURRENT_SOURCE_DIR}/${PROFILER_LAYER_PROJECTNAME}.generated.h.in
    ${generated}
    @ONLY)

if (WIN32)
    # WA: Windows uses backslash as directory separator, which needs to be
    # escaped twice (once here and once in the json, thus 4 backslashes)
    set (THIS_DIR ".\\\\")
else ()
    set (THIS_DIR "./")
endif ()

configure_file (
    ${CMAKE_CURRENT_SOURCE_DIR}/${PROFILER_LAYER_PROJECTNAME}.json.in
    ${json}_configured
    @ONLY)

add_library (${PROFILER_LAYER_PROJECTNAME}_lib STATIC
    ${CMAKE_CURRENT_BINARY_DIR}/vk_dispatch_tables.h
    ${helper_functions}
    ${core_functions}
    ${extension_functions}
    ${utils}
    ${generated})

# Add sources to this project's executable.
add_library (${PROFILER_LAYER_PROJECTNAME} SHARED
    "VkLayer_profiler_layer.cpp"
    "VkLayer_profiler_layer.def"
    ${resource})

file (GENERATE
    OUTPUT ${json}
    INPUT ${json}_configured)

# Link against implementation
target_link_libraries (${PROFILER_LAYER_PROJECTNAME}_lib
    PUBLIC profiler
    PUBLIC profiler_helpers
    PUBLIC profiler_ext
    PUBLIC profiler_overlay
    PUBLIC profiler_trace)

target_link_libraries (${PROFILER_LAYER_PROJECTNAME} PRIVATE ${PROFILER_LAYER_PROJECTNAME}_lib)

# Set include path
target_include_directories (${PROFILER_LAYER_PROJECTNAME}_lib
    PUBLIC ${VULKAN_HEADERS_INCLUDE_DIR}
    PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/profiler_layer_functions/core
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/profiler_layer_functions/extensions
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/utils/ctrie/include)

# Install target
install (TARGETS ${PROFILER_LAYER_PROJECTNAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

install (FILES ${json}
    DESTINATION ${CMAKE_INSTALL_BINDIR})

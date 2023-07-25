# Copyright (c) 2019-2023 Lukasz Stalmirski
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

set_property (GLOBAL PROPERTY USE_FOLDERS ON)

project (VulkanProfiler CXX)
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option (BUILD_TESTS "Build tests." ON)

# Build third-party dependencies.
add_subdirectory (External EXCLUDE_FROM_ALL)

# Find Vulkan SDK and glslangValidator
# Linux:    install libvulkan-dev and glslang-tools
# Windows:  install Vulkan SDK
find_package (Vulkan)
find_program (glslangvalidator glslangValidator)

if (NOT Vulkan_FOUND OR NOT glslangvalidator)
    message (WARNING "Vulkan SDK and/or glslangValidator not found. Tests will be disabled.")
    set (BUILD_TESTS OFF CACHE BOOL "" FORCE)
endif ()

# Detect available libraries
if (UNIX)
    # ECM is required on Linux to find Wayland and XCB
    find_package (ECM REQUIRED NO_MODULE)
    set (CMAKE_MODULE_PATH ${ECM_FIND_MODULE_DIR})

    #find_package (Wayland)
    find_package (XCB REQUIRED COMPONENTS XCB SHAPE)
    find_package (X11 REQUIRED)

    # Export symbols explicitly
    set (CMAKE_CXX_VISIBILITY_PRESET hidden)
    set (CMAKE_VISIBILITY_INLINES_HIDDEN 1)
endif ()

# Include sub-projects
add_subdirectory (VkLayer_profiler_layer)

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

cmake_minimum_required (VERSION 3.8...3.31)

project (profiler_tests)

# Find Vulkan SDK and glslangValidator
# Linux:    install libvulkan-dev and glslang-tools
# Windows:  install Vulkan SDK
find_package (Vulkan)
find_program (glslangvalidator glslangValidator)

if (NOT Vulkan_FOUND OR NOT glslangvalidator)
    message (WARNING "Vulkan SDK and/or glslangValidator not found. Tests will be disabled.")
    set (BUILD_TESTS OFF CACHE BOOL "" FORCE)
endif ()

if (BUILD_TESTS)
    add_subdirectory (shaders)

    set (tests
        "profiler_command_buffer_tests.cpp"
        "profiler_extensions_tests.cpp"
        "profiler_memory_tests.cpp"
        "profiler_testing_common.h"
        "profiler_vulkan_simple_triangle.h"
        "profiler_vulkan_state.h"
        )

    add_executable (profiler_tests
        ${tests}
        )

    add_dependencies (profiler_tests profiler_tests_shaders)
    add_dependencies (profiler_tests VkLayer_profiler_layer)

    target_link_libraries (profiler_tests
        PRIVATE ${Vulkan_LIBRARIES}
        PRIVATE gtest_main
        PRIVATE profiler
        )

    target_include_directories (profiler_tests
        PRIVATE "${CMAKE_CURRENT_BINARY_DIR}"
        )

    install (TARGETS profiler_tests)
endif (BUILD_TESTS)

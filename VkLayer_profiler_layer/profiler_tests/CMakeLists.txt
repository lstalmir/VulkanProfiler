# Copyright (c) 2020 Lukasz Stalmirski
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

project (profiler_tests)

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set (gtest_force_shared_crt ON CACHE BOOL "" FORCE)

add_subdirectory (googletest)
add_subdirectory (shaders)

set (tests
    "profiler_command_buffer_tests.cpp"
    "profiler_extensions_tests.cpp"
    "profiler_memory_tests.cpp"
    )

add_executable (profiler_tests
    ${tests}
    )
    
add_dependencies (profiler_tests profiler_tests_shaders)

target_link_libraries (profiler_tests
    PRIVATE ${Vulkan_LIBRARIES}
    PRIVATE gtest_main
    PRIVATE profiler
    PRIVATE VkLayer_profiler_layer_lib
    )

target_link_directories (profiler_tests
    PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/../profiler"
    )

target_include_directories (profiler_tests
    PRIVATE "${CMAKE_CURRENT_BINARY_DIR}"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/.."
    PRIVATE "${VULKAN_HEADERS_INCLUDE_DIR}"
    PRIVATE "${VULKAN_LOADER_INCLUDE_DIR}"
    )

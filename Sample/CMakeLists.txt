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

cmake_minimum_required (VERSION 3.7)

project (Sample)

add_subdirectory (SDL)
add_subdirectory (shaders)

set (headers
    "Args.h"
    "Device.h"
    "Image.h"
    "SwapChain.h"
    )

set (sources
    "Args.cpp"
    "Device.cpp"
    "Image.cpp"
    "Sample.cpp"
    "SwapChain.cpp"
    )

add_executable (Sample
    ${headers}
    ${sources})

target_include_directories (Sample
    PRIVATE ${Vulkan_INCLUDE_DIRS}
    PRIVATE ${VK_EXT_PROFILER_INCLUDE_DIR}
    PRIVATE "${CMAKE_CURRENT_LIST_DIR}/glm/glm"
    PRIVATE "${CMAKE_CURRENT_LIST_DIR}/SDL/include"
    )

add_dependencies (Sample SDL2main)
add_dependencies (Sample Sample-shaders)

target_link_libraries (Sample
    PRIVATE ${Vulkan_LIBRARIES}
    PRIVATE SDL2-static
    PRIVATE SDL2main
    )

install (TARGETS Sample
    RUNTIME DESTINATION bin
    DESTINATION ${CMAKE_OUTPUT_BINDIR})

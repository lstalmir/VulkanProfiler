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

project (profiler_ext)

set (headers
    "VkProfilerEXT.h"
    )

set (sources
    "VkProfilerEXT.cpp"
    )

# Link intermediate static library
add_library (profiler_ext
    ${sources}
    ${headers})

target_link_libraries (profiler_ext
    PRIVATE profiler)

# Set include path
target_include_directories (profiler_ext
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../profiler_layer_functions/core"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../profiler_layer_functions/extensions")

# Install headers
install (TARGETS profiler_ext
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")

install (FILES ${headers}
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

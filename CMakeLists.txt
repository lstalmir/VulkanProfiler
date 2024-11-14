# Copyright (c) 2019-2024 Lukasz Stalmirski
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

# Platform-specific configuration and dependencies.
include (CMake/platform.cmake NO_POLICY_SCOPE)

project (VulkanProfiler CXX)
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option (BUILD_TESTS "Build tests." ON)
option (BUILD_SPIRV_DOCS "Build SPIR-V documentation into the disassembly view." ON)
set    (SPIRV_DOCS_URL "https://registry.khronos.org/SPIR-V/specs/unified1/SPIRV.html"
            CACHE STRING "URL of the SPIR-V documentation page.")

# Build third-party dependencies.
add_subdirectory (External EXCLUDE_FROM_ALL)

find_package (Python3 REQUIRED)
string (CONFIGURE [[
    execute_process (COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_LIST_DIR}/VkLayer_profiler_layer/scripts/gen_licenses.py"
        --license "Vulkan(R) Profiling Layer" "${CMAKE_CURRENT_LIST_DIR}/LICENSE.md"
        --externaldir "${CMAKE_CURRENT_LIST_DIR}/External"
        --output "${CMAKE_BINARY_DIR}/LICENSES")
    ]] PROFILER_GENERATE_LICENSES_CODE)

install (CODE ${PROFILER_GENERATE_LICENSES_CODE})
install (FILES "${CMAKE_BINARY_DIR}/LICENSES" DESTINATION .)

# Include sub-projects
add_subdirectory (VkLayer_profiler_layer)

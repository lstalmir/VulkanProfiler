# Copyright (c) 2019-2025 Lukasz Stalmirski
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

project (profiler_helpers)

# Generate string serialization and deserialization functions
add_custom_command (
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/profiler_string_serializer.generated.h"
    COMMAND "${Python3_EXECUTABLE}"
        "${PROFILER_SCRIPTS_DIR}/gen_names.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/profiler_string_serializer.in"
        --output "${CMAKE_CURRENT_BINARY_DIR}/profiler_string_serializer.generated.h"
        --registry "${VULKAN_HEADERS_INSTALL_DIR}/${CMAKE_INSTALL_DATADIR}/vulkan/registry/vk.xml"
    DEPENDS "${PROFILER_SCRIPTS_DIR}/gen_names.py"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/profiler_string_serializer.in"
    DEPENDS "${VULKAN_HEADERS_INSTALL_DIR}/${CMAKE_INSTALL_DATADIR}/vulkan/registry/vk.xml")
list (APPEND PROFILER_HELPERS_CODEGEN_FILES "${CMAKE_CURRENT_BINARY_DIR}/profiler_string_serializer.generated.h")

add_custom_target (profiler_helpers_codegen
    DEPENDS ${PROFILER_HELPERS_CODEGEN_FILES})

set (headers
    "profiler_csv_helpers.h"
    "profiler_string_serializer.h"
    "profiler_memory_comparator.h"
    "profiler_metrics_set_file.h"
    "profiler_time_helpers.h"
    )

set (sources
    "profiler_csv_helpers.cpp"
    "profiler_string_serializer.cpp"
    "profiler_memory_comparator.cpp"
    "profiler_metrics_set_file.cpp"
    )

# Link intermediate static library
add_library (profiler_helpers
    ${sources}
    ${headers})

target_link_libraries (profiler_helpers
    PUBLIC profiler_common)

add_dependencies (profiler_helpers profiler_helpers_codegen)
target_include_directories (profiler_helpers
    PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")

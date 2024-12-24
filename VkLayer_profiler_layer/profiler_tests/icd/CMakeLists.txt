# Copyright (c) 2024 Lukasz Stalmirski
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

project (profiler_test_icd)

# Use profiler's version info
set (PROFILER_TEST_ICD_VER_MAJOR ${PROFILER_LAYER_VER_MAJOR})
set (PROFILER_TEST_ICD_VER_MINOR ${PROFILER_LAYER_VER_MINOR})
set (PROFILER_TEST_ICD_VER_BUILD ${PROFILER_LAYER_VER_BUILD})
set (PROFILER_TEST_ICD_VER_PATCH ${PROFILER_LAYER_VER_PATCH})
set (PROFILER_TEST_ICD_VER_IMPLEMENTATION ${PROFILER_LAYER_VER_IMPLEMENTATION})
set (PROFILER_TEST_ICD_NAME VK_ICD_PROFILER_mock_icd)
set (PROFILER_TEST_ICD_API_VERSION ${PROFILER_LAYER_API_VERSION})
set (PROFILER_TEST_ICD_PROJECTNAME VkIcd_profiler_mock_icd)
set (PROFILER_TEST_ICD_COMPANYNAME )
set (PROFILER_TEST_ICD_FILEDESCRIPTION "Mock ICD used for testing Vulkan(R) Profiling Layer.")
set (PROFILER_TEST_ICD_LEGALCOPYRIGHT )
set (PROFILER_TEST_ICD_PRODUCTNAME "Vulkan(R) Profiler Mock ICD")

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set (PROFILER_TEST_ICD_ARCH 64)
else ()
    set (PROFILER_TEST_ICD_ARCH 32)
endif ()

# Generate mock ICD
add_custom_command (
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/profiler_test_icd_base.h"
    COMMAND "${Python3_EXECUTABLE}"
        "${PROFILER_SCRIPTS_DIR}/gen_icd.py"
        --vk_xml="${VULKAN_HEADERS_INSTALL_DIR}/registry/vk.xml"
        --output="${CMAKE_CURRENT_BINARY_DIR}/profiler_test_icd_base.h"
    DEPENDS "${VULKAN_HEADERS_INSTALL_DIR}/registry/vk.xml"
    DEPENDS "${PROFILER_SCRIPTS_DIR}/gen_icd.py")
list (APPEND PROFILER_TEST_ICD_CODEGEN_FILES "${CMAKE_CURRENT_BINARY_DIR}/profiler_test_icd_base.h")

# Configure JSON file
configure_file (profiler_test_icd.json.in "${CMAKE_CURRENT_BINARY_DIR}/profiler_test_icd.json_configured" @ONLY)
configure_file (profiler_test_icd.rc.in   "${CMAKE_CURRENT_BINARY_DIR}/profiler_test_icd.rc" @ONLY)

file (GENERATE
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/profiler_test_icd.json"
    INPUT "${CMAKE_CURRENT_BINARY_DIR}/profiler_test_icd.json_configured")

add_custom_target (profiler_test_icd_codegen
    DEPENDS ${PROFILER_TEST_ICD_CODEGEN_FILES})

add_library (profiler_test_icd SHARED
    "${CMAKE_CURRENT_BINARY_DIR}/profiler_test_icd.rc"
    profiler_test_icd.def
    profiler_test_command_buffer.h
    profiler_test_command_buffer.cpp
    profiler_test_device.h
    profiler_test_device.cpp
    profiler_test_icd.h
    profiler_test_icd.cpp
    profiler_test_icd_helpers.h
    profiler_test_instance.h
    profiler_test_instance.cpp
    profiler_test_physical_device.h
    profiler_test_physical_device.cpp
    profiler_test_query_pool.h
    profiler_test_queue.h
    profiler_test_queue.cpp)

add_dependencies (profiler_test_icd profiler_test_icd_codegen)

target_include_directories (profiler_test_icd
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
    PUBLIC "${CMAKE_CURRENT_BINARY_DIR}"
    PUBLIC "${VULKAN_HEADERS_INSTALL_DIR}/include")

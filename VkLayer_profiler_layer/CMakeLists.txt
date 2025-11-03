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

find_package (Python3 REQUIRED)

set (PROFILER_LAYER_VER_MAJOR 1)
set (PROFILER_LAYER_VER_MINOR 0)
set (PROFILER_LAYER_VER_BUILD 9999)
set (PROFILER_LAYER_VER_PATCH 0)
set (PROFILER_LAYER_VER_IMPLEMENTATION 1)
set (PROFILER_LAYER_NAME VK_LAYER_PROFILER_unified)
set (PROFILER_LAYER_API_VERSION "1.1.106")
set (PROFILER_LAYER_PROJECTNAME VkLayer_profiler_layer)
set (PROFILER_LAYER_COMPANYNAME )
set (PROFILER_LAYER_FILEDESCRIPTION "Simple Vulkan(R) Profiling Layer")
set (PROFILER_LAYER_LEGALCOPYRIGHT )
set (PROFILER_LAYER_PRODUCTNAME "Vulkan(R) Profiling Layer")

# Common directories
set (PROFILER_SCRIPTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/scripts")

# Generate version info
execute_process (
    COMMAND "${Python3_EXECUTABLE}" "${PROFILER_SCRIPTS_DIR}/gen_version.py" "${CMAKE_BINARY_DIR}/profiler_version.cmake"
    RESULT_VARIABLE PROFILER_GEN_VERSION_INFO_RESULT)

if (PROFILER_GEN_VERSION_INFO_RESULT STREQUAL "0")
    include ("${CMAKE_BINARY_DIR}/profiler_version.cmake")
endif ()

# Get Vulkan headers version
function (get_vulkan_api_version)
    set (vulkan_ver_major 1)
    set (vulkan_ver_minor 1)
    set (vulkan_ver_headers 106)

    set (vulkan_core_path "${VULKAN_HEADERS_INSTALL_DIR}/include/vulkan/vulkan_core.h")
    if (EXISTS ${vulkan_core_path})
        file (READ ${vulkan_core_path} ver)
        if (ver MATCHES "#define[ ]+VK_HEADER_VERSION_COMPLETE[ ]+VK_MAKE_API_VERSION\\([ ]*[0-9]+,[ ]*([0-9]+),[ ]*([0-9]+),[ ]*VK_HEADER_VERSION[ ]*\\)")
            set (vulkan_ver_major ${CMAKE_MATCH_1})
            set (vulkan_ver_minor ${CMAKE_MATCH_2})
        endif ()
        if (ver MATCHES "#define[ ]+VK_HEADER_VERSION[ ]+([0-9]+)")
            set (vulkan_ver_headers ${CMAKE_MATCH_1})
        endif ()
    endif ()

    set (PROFILER_LAYER_API_VERSION "${vulkan_ver_major}.${vulkan_ver_minor}.${vulkan_ver_headers}" PARENT_SCOPE)
endfunction ()
get_vulkan_api_version ()

project (${PROFILER_LAYER_PROJECTNAME})
message ("-- Profiler version: ${PROFILER_LAYER_VER_MAJOR}.${PROFILER_LAYER_VER_MINOR}.${PROFILER_LAYER_VER_BUILD}.${PROFILER_LAYER_VER_PATCH}")
message ("-- Vulkan version: ${PROFILER_LAYER_API_VERSION}")

# Generate layer manifest file
set (json
    "${CMAKE_CURRENT_BINARY_DIR}/${PROFILER_LAYER_PROJECTNAME}.json"
    )

if (WIN32)
    # WA: Windows uses backslash as directory separator, which needs to be
    # escaped twice (once here and once in the json, thus 4 backslashes)
    set (THIS_DIR ".\\\\")
else ()
    set (THIS_DIR "./")
endif ()

configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/${PROFILER_LAYER_PROJECTNAME}.json.in"
    "${json}_configured"
    @ONLY)

file (GENERATE
    OUTPUT "${json}"
    INPUT "${json}_configured")

add_custom_command (
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/vk_layer_config.h"
    COMMAND "${Python3_EXECUTABLE}"
        "${PROFILER_SCRIPTS_DIR}/gen_config.py"
        "${json}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/vk_layer_config.h"
    DEPENDS "${json}"
    DEPENDS "${PROFILER_SCRIPTS_DIR}/gen_config.py")
list (APPEND PROFILER_CODEGEN_FILES "${CMAKE_CURRENT_BINARY_DIR}/vk_layer_config.h")

# Generate dispatch tables
add_custom_command (
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/vk_dispatch_tables.h"
    COMMAND "${Python3_EXECUTABLE}"
        "${PROFILER_SCRIPTS_DIR}/gen_dispatch_tables.py"
        "${VULKAN_HEADERS_INSTALL_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS "${VULKAN_HEADERS_INSTALL_DIR}/registry/vk.xml"
    DEPENDS "${PROFILER_SCRIPTS_DIR}/gen_dispatch_tables.py")
list (APPEND PROFILER_CODEGEN_FILES "${CMAKE_CURRENT_BINARY_DIR}/vk_dispatch_tables.h")

if (BUILD_SPIRV_DOCS)
    # Generate SPIR-V documentation
    add_custom_command (
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/spv_docs.h"
            "${CMAKE_SOURCE_DIR}/External/SPIRV-Specification/LICENSE"
        COMMAND "${Python3_EXECUTABLE}"
            "${PROFILER_SCRIPTS_DIR}/gen_spirv_docs.py"
            --json "${CMAKE_SOURCE_DIR}/External/SPIRV-Headers/include/spirv/unified1/spirv.json"
            --grammar "${CMAKE_SOURCE_DIR}/External/SPIRV-Headers/include/spirv/unified1/spirv.core.grammar.json"
            --spec "${CMAKE_SOURCE_DIR}/External/SPIRV-Specification/SPIRV.html"
            --out-license "${CMAKE_SOURCE_DIR}/External/SPIRV-Specification/LICENSE"
            --out-header "${CMAKE_CURRENT_BINARY_DIR}/spv_docs.h"
        DEPENDS "${CMAKE_SOURCE_DIR}/External/SPIRV-Headers/include/spirv/unified1/spirv.json"
        DEPENDS "${CMAKE_SOURCE_DIR}/External/SPIRV-Headers/include/spirv/unified1/spirv.core.grammar.json"
        DEPENDS "${CMAKE_SOURCE_DIR}/External/SPIRV-Specification/SPIRV.html"
        DEPENDS "${PROFILER_SCRIPTS_DIR}/gen_spirv_docs.py")
    list (APPEND PROFILER_CODEGEN_FILES "${CMAKE_CURRENT_BINARY_DIR}/spv_docs.h")
endif ()

# Generate licenses file
set (PROFILER_LICENSES_FILE "${CMAKE_BINARY_DIR}/LICENSES.rst")
add_custom_command (
    OUTPUT "${PROFILER_LICENSES_FILE}"
    COMMAND "${Python3_EXECUTABLE}"
        "${PROFILER_SCRIPTS_DIR}/gen_licenses.py"
        --license "Vulkan(R) Profiling Layer" "${CMAKE_SOURCE_DIR}/LICENSE.md"
        --externaldir "${CMAKE_SOURCE_DIR}/External"
        --output "${PROFILER_LICENSES_FILE}"
    VERBATIM
    DEPENDS "${CMAKE_SOURCE_DIR}/External/SPIRV-Specification/LICENSE"
    DEPENDS "${PROFILER_SCRIPTS_DIR}/gen_licenses.py")
list (APPEND PROFILER_CODEGEN_FILES ${PROFILER_LICENSES_FILE})

add_custom_target (profiler_codegen
    DEPENDS ${PROFILER_CODEGEN_FILES})

add_library (profiler_common INTERFACE)
target_link_libraries (profiler_common
    INTERFACE Vulkan-Headers
    INTERFACE VulkanUtilityHeaders
    INTERFACE VulkanLayerSettings
    INTERFACE VulkanMemoryAllocator
    INTERFACE SPIRV-Headers
    INTERFACE SPIRV-Tools
    INTERFACE spirv-cross-c
    INTERFACE farmhash
    INTERFACE fmt
    INTERFACE nlohmann_json)
target_include_directories (profiler_common
    INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}"
    INTERFACE "${CMAKE_CURRENT_BINARY_DIR}"
    INTERFACE utils)
target_compile_definitions (profiler_common INTERFACE
    $<$<CONFIG:Debug>:PROFILER_CONFIG_DEBUG>
    $<$<CONFIG:RelWithDebInfo>:PROFILER_CONFIG_RELWITHDEBINFO>
    $<$<CONFIG:Release>:PROFILER_CONFIG_RELEASE>
    $<$<CONFIG:MinSizeRel>:PROFILER_CONFIG_MINSIZEREL>)
add_dependencies (profiler_common profiler_codegen)

if (PROFILER_ENABLE_TIP)
    target_compile_definitions (profiler_common INTERFACE
        PROFILER_ENABLE_TIP=1)
endif ()

if (BUILD_SPIRV_DOCS)
    target_compile_definitions (profiler_common INTERFACE
        PROFILER_BUILD_SPIRV_DOCS
        PROFILER_SPIRV_DOCS_URL="${SPIRV_DOCS_URL}")
endif ()

# Include implementation of the layer
add_subdirectory (profiler)
add_subdirectory (profiler_ext)
add_subdirectory (profiler_helpers)
add_subdirectory (profiler_overlay)
add_subdirectory (profiler_trace)

# Enable tests
if (BUILD_TESTS)
    add_subdirectory (profiler_tests)
endif ()

set (resource
    "${CMAKE_CURRENT_BINARY_DIR}/${PROFILER_LAYER_PROJECTNAME}.rc"
    )

set (generated
    "${CMAKE_CURRENT_BINARY_DIR}/${PROFILER_LAYER_PROJECTNAME}.generated.h"
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
    "profiler_layer_functions/extensions/VkAccelerationStructureKhr_functions.cpp"
    "profiler_layer_functions/extensions/VkAccelerationStructureKhr_functions.h"
    "profiler_layer_functions/extensions/VkAndroidSurfaceKhr_functions.cpp"
    "profiler_layer_functions/extensions/VkAndroidSurfaceKhr_functions.h"
    "profiler_layer_functions/extensions/VkBindMemory2Khr_functions.cpp"
    "profiler_layer_functions/extensions/VkBindMemory2Khr_functions.h"
    "profiler_layer_functions/extensions/VkCopyCommands2Khr_functions.cpp"
    "profiler_layer_functions/extensions/VkCopyCommands2Khr_functions.h"
    "profiler_layer_functions/extensions/VkCreateRenderPass2Khr_functions.cpp"
    "profiler_layer_functions/extensions/VkCreateRenderPass2Khr_functions.h"
    "profiler_layer_functions/extensions/VkDebugMarkerExt_functions.cpp"
    "profiler_layer_functions/extensions/VkDebugMarkerExt_functions.h"
    "profiler_layer_functions/extensions/VkDebugUtilsExt_functions.cpp"
    "profiler_layer_functions/extensions/VkDebugUtilsExt_functions.h"
    "profiler_layer_functions/extensions/VkDeferredHostOperationsKhr_functions.cpp"
    "profiler_layer_functions/extensions/VkDeferredHostOperationsKhr_functions.h"
    "profiler_layer_functions/extensions/VkDrawIndirectCountAmd_functions.cpp"
    "profiler_layer_functions/extensions/VkDrawIndirectCountAmd_functions.h"
    "profiler_layer_functions/extensions/VkDrawIndirectCountKhr_functions.cpp"
    "profiler_layer_functions/extensions/VkDrawIndirectCountKhr_functions.h"
    "profiler_layer_functions/extensions/VkDynamicRenderingKhr_functions.cpp"
    "profiler_layer_functions/extensions/VkDynamicRenderingKhr_functions.h"
    "profiler_layer_functions/extensions/VkMeshShaderExt_functions.cpp"
    "profiler_layer_functions/extensions/VkMeshShaderExt_functions.h"
    "profiler_layer_functions/extensions/VkMeshShaderNv_functions.cpp"
    "profiler_layer_functions/extensions/VkMeshShaderNv_functions.h"
    "profiler_layer_functions/extensions/VkOpacityMicromapExt_functions.cpp"
    "profiler_layer_functions/extensions/VkOpacityMicromapExt_functions.h"
    "profiler_layer_functions/extensions/VkPipelineExecutablePropertiesKhr_functions.h"
    "profiler_layer_functions/extensions/VkRayTracingMaintenance1Khr_functions.cpp"
    "profiler_layer_functions/extensions/VkRayTracingMaintenance1Khr_functions.h"
    "profiler_layer_functions/extensions/VkRayTracingPipelineKhr_functions.cpp"
    "profiler_layer_functions/extensions/VkRayTracingPipelineKhr_functions.h"
    "profiler_layer_functions/extensions/VkShaderObjectExt_functions.cpp"
    "profiler_layer_functions/extensions/VkShaderObjectExt_functions.h"
    "profiler_layer_functions/extensions/VkSurfaceKhr_functions.cpp"
    "profiler_layer_functions/extensions/VkSurfaceKhr_functions.h"
    "profiler_layer_functions/extensions/VkSwapchainKhr_functions.cpp"
    "profiler_layer_functions/extensions/VkSwapchainKhr_functions.h"
    "profiler_layer_functions/extensions/VkSynchronization2Khr_functions.cpp"
    "profiler_layer_functions/extensions/VkSynchronization2Khr_functions.h"
    "profiler_layer_functions/extensions/VkToolingInfoExt_functions.cpp"
    "profiler_layer_functions/extensions/VkToolingInfoExt_functions.h"
    "profiler_layer_functions/extensions/VkWaylandSurfaceKhr_functions.cpp"
    "profiler_layer_functions/extensions/VkWaylandSurfaceKhr_functions.h"
    "profiler_layer_functions/extensions/VkWin32SurfaceKhr_functions.cpp"
    "profiler_layer_functions/extensions/VkWin32SurfaceKhr_functions.h"
    "profiler_layer_functions/extensions/VkXcbSurfaceKhr_functions.cpp"
    "profiler_layer_functions/extensions/VkXcbSurfaceKhr_functions.h"
    "profiler_layer_functions/extensions/VkXlibSurfaceKhr_functions.cpp"
    "profiler_layer_functions/extensions/VkXlibSurfaceKhr_functions.h"
    )

set (objects
    "profiler_layer_objects/profiler_layer_frontend.cpp"
    "profiler_layer_objects/profiler_layer_frontend.h"
    "profiler_layer_objects/VkDevice_object.h"
    "profiler_layer_objects/VkInstance_object.h"
    "profiler_layer_objects/VkObject.h"
    "profiler_layer_objects/VkObject_Types.inl"
    "profiler_layer_objects/VkPhysicalDevice_object.h"
    "profiler_layer_objects/VkQueue_object.h"
    "profiler_layer_objects/VkSurfaceKhr_object.h"
    "profiler_layer_objects/VkSwapchainKhr_object.h"
    )

source_group ("Functions" FILES ${helper_functions})
source_group ("Functions/Core" FILES ${core_functions})
source_group ("Functions/Extensions" FILES ${extension_functions})
source_group ("Objects" FILES ${objects})

# Configure resources file
configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/${PROFILER_LAYER_PROJECTNAME}.rc.in"
    ${resource}
    @ONLY)

configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/${PROFILER_LAYER_PROJECTNAME}.generated.h.in"
    ${generated}
    @ONLY)

add_library (${PROFILER_LAYER_PROJECTNAME}_lib STATIC
    "${CMAKE_CURRENT_BINARY_DIR}/vk_dispatch_tables.h"
    ${helper_functions}
    ${core_functions}
    ${extension_functions}
    ${objects}
    ${generated})

# Add sources to this project's executable.
add_library (${PROFILER_LAYER_PROJECTNAME} SHARED
    "VkLayer_profiler_layer.cpp"
    "VkLayer_profiler_layer.def"
    ${resource})

if (UNIX AND NOT APPLE)
    target_link_options(${PROFILER_LAYER_PROJECTNAME}
        PRIVATE LINKER:--version-script=${CMAKE_CURRENT_SOURCE_DIR}/VkLayer_profiler_layer.map)
endif ()

# Link against implementation
target_link_libraries (${PROFILER_LAYER_PROJECTNAME}_lib
    PUBLIC profiler
    PUBLIC profiler_helpers
    PUBLIC profiler_ext
    PUBLIC profiler_overlay
    PUBLIC profiler_trace)

target_link_libraries (${PROFILER_LAYER_PROJECTNAME}
    PRIVATE ${PROFILER_LAYER_PROJECTNAME}_lib)

# Set include path
target_include_directories (${PROFILER_LAYER_PROJECTNAME}_lib
    PUBLIC "${VULKAN_HEADERS_INCLUDE_DIR}"
    PUBLIC "${CMAKE_CURRENT_BINARY_DIR}"
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/profiler_layer_functions/core"
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/profiler_layer_functions/extensions")

# Install target
set (PROFILER_INSTALL_BINDIR "${CMAKE_INSTALL_BINDIR}")
set (PROFILER_INSTALL_LIBDIR "${CMAKE_INSTALL_LIBDIR}")

if (ANDROID)
    # Put the layer in the appropriate target ABI folder
    set (PROFILER_INSTALL_BINDIR "${CMAKE_INSTALL_BINDIR}/${CMAKE_ANDROID_ARCH_ABI}")
    set (PROFILER_INSTALL_LIBDIR "${CMAKE_INSTALL_LIBDIR}/${CMAKE_ANDROID_ARCH_ABI}")
endif ()

install (TARGETS ${PROFILER_LAYER_PROJECTNAME}
    RUNTIME DESTINATION "${PROFILER_INSTALL_BINDIR}"
    LIBRARY DESTINATION "${PROFILER_INSTALL_LIBDIR}")

install (FILES "${PROFILER_LICENSES_FILE}" DESTINATION .)

# Copy JSON to the path where DLL/SO is installed
if (WIN32)
    install (FILES ${json} DESTINATION "${PROFILER_INSTALL_BINDIR}")
# Android doesn't use JSON files for layer discovery
elseif (NOT ANDROID)
    install (FILES ${json} DESTINATION "${PROFILER_INSTALL_LIBDIR}")
endif ()

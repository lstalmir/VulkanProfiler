# CMakeList.txt : CMake project for VulkanProfiler, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project (VkLayer_profiler_layer)

# Include implementation of the layer
add_subdirectory (profiler_layer)

set (headers
    "VkLayer_profiler_layer.h"
    )

set (sources
    "VkLayer_profiler_layer.cpp"
    )

# Set include path
include_directories (${VULKAN_HEADERS_INCLUDE_DIR})

# Add sources to this project's executable.
add_library (VkLayer_profiler_layer SHARED ${sources} ${headers})

# Link against implementation
add_dependencies (VkLayer_profiler_layer profiler_layer)

target_include_directories (VkLayer_profiler_layer PRIVATE "${CMAKE_CURRENT_LIST_DIR}/profiling_layer")

install (TARGETS VkLayer_profiler_layer
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

# Export the layer metadata file
install (FILES "VkLayer_profiler_layer.json"
    DESTINATION bin ${CMAKE_INSTALL_LIBDIR})

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

# Add sources to this project's executable.
add_library (VkLayer_profiler_layer SHARED ${sources} ${headers})

# Link against implementation
target_link_libraries (VkLayer_profiler_layer profiler_layer)

# Set include path
target_include_directories (VkLayer_profiler_layer
    PRIVATE ${VULKAN_HEADERS_INCLUDE_DIR})

install (TARGETS VkLayer_profiler_layer
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

# Export the layer metadata file
install (FILES "VkLayer_profiler_layer.json"
    DESTINATION ${CMAKE_INSTALL_BINDIR})

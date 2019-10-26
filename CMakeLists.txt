# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("VulkanProfiler")

# Build with C++17
set (CMAKE_CXX_STANDARD 17)

add_subdirectory ("Sample")

set (VULKAN_HEADERS_INSTALL_DIR
    "${CMAKE_CURRENT_LIST_DIR}/Vulkan-Headers"
    )

set (VULKAN_HEADERS_INCLUDE_DIR
    "${CMAKE_CURRENT_LIST_DIR}/Vulkan-Headers/include"
    "${CMAKE_CURRENT_LIST_DIR}/Vulkan-Headers/include/vulkan"
    )

# Build loader
add_subdirectory (Vulkan-Headers)
add_subdirectory (Vulkan-Loader)

set (VULKAN_LOADER_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/Vulkan-Loader/loader/generated")

# Include sub-projects
add_subdirectory ("VkLayer_profiler_layer")

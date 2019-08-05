# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("VulkanProfiler")

add_subdirectory ("Sample")

set (VULKAN_HEADERS_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/Vulkan-Headers/include")
add_compile_definitions (VK_NO_PROTOTYPES)

# Include sub-projects
add_subdirectory ("VkLayer_profiler_layer")

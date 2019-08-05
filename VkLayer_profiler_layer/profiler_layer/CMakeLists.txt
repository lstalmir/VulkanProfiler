# CMakeList.txt : CMake project for profiler layer, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project (profiler_layer)

set (headers
    "profiler.h"
    "VkInstance_dispatch.h"
    "VkInstance_functions.h"
    )

set (sources
    "profiler.cpp"
    "VkInstance_dispatch.cpp"
    "VkInstance_functions.cpp"
    )
    
# Link intermediate static library
add_library (profiler_layer ${sources} ${headers})

# Set include path
target_include_directories (profiler_layer
    PRIVATE ${VULKAN_HEADERS_INCLUDE_DIR})

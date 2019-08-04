# CMakeList.txt : CMake project for profiler layer, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project (profiler_layer)

set (headers
    "profiler.h"
    "instance_functions.h"
    )

set (sources
    "profiler.cpp"
    "instance_functions.cpp"
    )
    
# Set include path
include_directories (${VULKAN_HEADERS_INCLUDE_DIR})

# Link intermediate static library
add_library (profiler_layer ${sources} ${headers})

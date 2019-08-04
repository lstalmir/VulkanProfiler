# CMakeList.txt : CMake project for profiler layer, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project (profiler_layer)

set (headers
    "profiler.h"
    )

set (sources
    "profiler.cpp"
    )

# Link intermediate static library
add_library (profiler_layer ${sources} ${headers})

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
add_library (profiler_layer
    ${sources}
    ${headers})

# Set include path
target_include_directories (profiler_layer
    PRIVATE ${VULKAN_HEADERS_INCLUDE_DIR}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

# Link vulkan-1
if (CMAKE_SIZEOF_VOID_P EQUAL 8) # 64-bit
    target_link_directories (profiler_layer INTERFACE $ENV{VK_SDK_PATH}/Lib)
else ()
    target_link_directories (profiler_layer INTERFACE $ENV{VK_SDK_PATH}/Lib32)
endif ()

target_link_libraries (profiler_layer vulkan-1)

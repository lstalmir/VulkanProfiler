# CMakeList.txt : CMake project for profiler layer, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project (profiler_layer)

# Compile shaders
add_subdirectory (shaders)

# Embed resources
add_subdirectory (resources)

set (headers
    "profiler.h"
    "profiler_allocator.h"
    "profiler_command_buffer.h"
    "profiler_console_output.h"
    "profiler_counters.h"
    "profiler_debug_utils.h"
    "profiler_frame_stats.h"
    "profiler_helpers.h"
    "profiler_mode.h"
    "profiler_resources.h"
    "profiler_shader_tuple.h"
    "profiler_shaders.h"
    )

set (sources
    "profiler.cpp"
    "profiler_command_buffer.cpp"
    "profiler_console_output.cpp"
    "profiler_debug_utils.cpp"
    )
    
# Link intermediate static library
add_library (profiler_layer
    ${sources}
    ${headers})

add_dependencies (profiler_layer profiler_shaders)
add_dependencies (profiler_layer profiler_resources)
add_dependencies (profiler_layer vulkan_traits)

# Set include path
target_include_directories (profiler_layer
    PRIVATE ${VULKAN_HEADERS_INCLUDE_DIR}
    PRIVATE ${VULKAN_LOADER_INCLUDE_DIR}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../vulkan_traits
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/shaders
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/resources)

# Link vulkan-1
if (CMAKE_SIZEOF_VOID_P EQUAL 8) # 64-bit
    target_link_directories (profiler_layer INTERFACE $ENV{VK_SDK_PATH}/Lib)
else ()
    target_link_directories (profiler_layer INTERFACE $ENV{VK_SDK_PATH}/Lib32)
endif ()

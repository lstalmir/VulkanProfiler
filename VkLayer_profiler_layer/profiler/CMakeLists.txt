# CMakeList.txt : CMake project for profiler layer, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project (profiler_layer)

set (headers
    "profiler.h"
    "profiler_allocator.h"
    "profiler_command_buffer.h"
    "profiler_counters.h"
    "profiler_data_aggregator.h"
    "profiler_debug_utils.h"
    "profiler_drawcall.h"
    "profiler_frame_stats.h"
    "profiler_helpers.h"
    "profiler_pipeline.h"
    "profiler_resources.h"
    "profiler_shader.h"
    )

set (sources
    "profiler.cpp"
    "profiler_command_buffer.cpp"
    "profiler_data_aggregator.cpp"
    "profiler_debug_utils.cpp"
    # Windows
    "profiler_helpers_windows.cpp"
    )

set (farmhash
    "farmhash/src/farmhash.h"
    "farmhash/src/farmhash.cc"
    )

# Link intermediate static library
add_library (profiler_layer
    ${sources}
    ${headers}
    ${farmhash})

target_compile_definitions (profiler_layer
    PRIVATE -DNAMESPACE_FOR_HASH_FUNCTIONS=Profiler::Hash
    )

if (MSVC)
    # Disable __builtin_expect in MSVC builds
    target_compile_definitions (profiler_layer
        PRIVATE -DFARMHASH_NO_BUILTIN_EXPECT)
endif ()

# Set include path
target_include_directories (profiler_layer
    PRIVATE ${VULKAN_HEADERS_INCLUDE_DIR}
    PRIVATE ${VULKAN_LOADER_INCLUDE_DIR}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../vulkan_traits
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/shaders
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/resources)

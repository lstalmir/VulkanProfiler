# CMakeList.txt : CMake project for profiler layer, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project (profiler)

set (headers
    "profiler.h"
    "profiler_command_buffer.h"
    "profiler_counters.h"
    "profiler_data.h"
    "profiler_data_aggregator.h"
    "profiler_helpers.h"
    "profiler_resources.h"
    "profiler_shader.h"
    "profiler_stat_comparators.h"
    )

set (sources
    "profiler.cpp"
    "profiler_command_buffer.cpp"
    "profiler_data_aggregator.cpp"
    # Windows
    "profiler_helpers_windows.cpp"
    # Linux
    "profiler_helpers_linux.cpp"
    )

set (farmhash
    "farmhash/src/farmhash.h"
    "farmhash/src/farmhash.cc"
    )

set (intel
    "intel/profiler_metrics_api.h"
    "intel/profiler_metrics_api.cpp"
    )

# Link intermediate static library
add_library (profiler
    ${sources}
    ${headers}
    ${farmhash}
    ${intel})

target_compile_definitions (profiler
    PRIVATE -DNAMESPACE_FOR_HASH_FUNCTIONS=Profiler::Hash
    )

if (MSVC)
    # Disable __builtin_expect in MSVC builds
    target_compile_definitions (profiler
        PRIVATE -DFARMHASH_NO_BUILTIN_EXPECT)
endif ()

# Set include path
target_include_directories (profiler
    PRIVATE ${VULKAN_HEADERS_INCLUDE_DIR}
    PRIVATE ${VULKAN_LOADER_INCLUDE_DIR}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../vulkan_traits
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/shaders
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/resources)

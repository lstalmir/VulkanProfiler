cmake_minimum_required (VERSION 3.8)

project (profiler_ult)

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set (gtest_force_shared_crt ON CACHE BOOL "" FORCE)

add_subdirectory (googletest)
add_subdirectory (shaders)

set (tests
    "profiler_command_buffer_ult.cpp"
    "profiler_extensions_ult.cpp"
    )

add_executable (profiler_ult
    ${tests}
    )
    
add_dependencies (profiler_ult profiler_ult_shaders)

target_link_libraries (profiler_ult
    PRIVATE ${Vulkan_LIBRARIES}
    PRIVATE gtest_main
    PRIVATE profiler
    PRIVATE VkLayer_profiler_layer_lib
    )

target_link_directories (profiler_ult
    PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/../profiler"
    )

target_include_directories (profiler_ult
    PRIVATE "${CMAKE_CURRENT_BINARY_DIR}"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/.."
    PRIVATE "${VULKAN_HEADERS_INCLUDE_DIR}"
    PRIVATE "${VULKAN_LOADER_INCLUDE_DIR}"
    )

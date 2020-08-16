# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project (VulkanProfiler CXX)

# Options
option (BUILD_SAMPLE "Build Sample project." ON)
option (BUILD_ULT "Build ULT project." ON)

# Find Vulkan SDK and glslangValidator
# Linux:    install libvulkan-dev and glslang-tools
# Windows:  install Vulkan SDK
find_package (Vulkan)
find_program (glslangvalidator glslangValidator)

if (NOT Vulkan_FOUND OR NOT glslangvalidator)
    message (WARNING "Vulkan SDK and/or glslangValidator not found. Samples and tests will be disabled.")
    set (BUILD_SAMPLE OFF CACHE BOOL "" FORCE)
    set (BUILD_ULT OFF CACHE BOOL "" FORCE)
endif ()

# Detect available libraries
if (UNIX)
    find_package (Wayland)
    find_package (XCB)
    find_package (X11)

    if (NOT Wayland_FOUND)
        set (BUILD_WSI_WAYLAND_SUPPORT OFF CACHE BOOL "" FORCE)
    endif ()

    if (NOT XCB_FOUND)
        set (BUILD_WSI_XCB_SUPPORT OFF CACHE BOOL "" FORCE)
    endif ()

    if (NOT X11_FOUND)
        set (BUILD_WSI_XLIB_SUPPORT OFF CACHE BOOL "" FORCE)
    endif ()

    if (NOT Wayland_FOUND AND NOT XCB_FOUND AND NOT X11_FOUND)
        message (FATAL_ERROR "No window library found")
    endif ()

endif ()

# Build with C++17
set (CMAKE_CXX_STANDARD 17)

set (VK_EXT_PROFILER_INCLUDE_DIR
    "${CMAKE_CURRENT_LIST_DIR}/VkLayer_profiler_layer/profiler_ext"
    )

if (BUILD_SAMPLE)
    add_subdirectory (Sample)
endif ()

set (VULKAN_HEADERS_INSTALL_DIR
    "${CMAKE_CURRENT_LIST_DIR}/Vulkan-Headers"
    )

set (VULKAN_HEADERS_INCLUDE_DIR
    "${CMAKE_CURRENT_LIST_DIR}/Vulkan-Headers/include"
    )

# Build loader
add_subdirectory (Vulkan-Headers)
add_subdirectory (Vulkan-Loader)

set (VULKAN_LOADER_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/Vulkan-Loader/loader/generated")

# Include sub-projects
add_subdirectory (VkLayer_profiler_layer)

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
    # ECM is required on Linux to find Wayland and XCB
    find_package (ECM REQUIRED NO_MODULE)
    set (CMAKE_MODULE_PATH ${ECM_FIND_MODULE_DIR})

    #find_package (Wayland)
    find_package (XCB REQUIRED COMPONENTS XCB SHAPE)
    find_package (X11 REQUIRED)
endif ()

# Build with C++17
set (CMAKE_CXX_STANDARD 17)

set (VK_EXT_PROFILER_INCLUDE_DIR
    "${CMAKE_CURRENT_LIST_DIR}/VkLayer_profiler_layer/profiler_ext"
    )

set (VULKAN_HEADERS_INSTALL_DIR
    "${CMAKE_CURRENT_LIST_DIR}/Vulkan-Headers"
    )

set (VULKAN_HEADERS_INCLUDE_DIR
    "${CMAKE_CURRENT_LIST_DIR}/Vulkan-Headers/include"
    )

# Include sub-projects
add_subdirectory (Vulkan-Headers)
add_subdirectory (VkLayer_profiler_layer)

if (BUILD_SAMPLE)
    add_subdirectory (Sample)
endif ()

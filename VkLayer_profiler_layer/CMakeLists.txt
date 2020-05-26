# CMakeList.txt : CMake project for VulkanProfiler, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

set (PROFILER_LAYER_VER_MAJOR 0)
set (PROFILER_LAYER_VER_MINOR 1)
set (PROFILER_LAYER_VER_PATCH 0)
set (PROFILER_LAYER_VER_IMPLEMENTATION 1)
set (PROFILER_LAYER_NAME VK_LAYER_profiler)
set (PROFILER_LAYER_PROJECTNAME VkLayer_profiler_layer)
set (PROFILER_LAYER_COMPANYNAME )
set (PROFILER_LAYER_FILEDESCRIPTION )
set (PROFILER_LAYER_LEGALCOPYRIGHT )
set (PROFILER_LAYER_PRODUCTNAME "Vulkan(R) Profiling Layer")

project (${PROFILER_LAYER_PROJECTNAME})

if (WIN32)

    add_definitions (-DVK_USE_PLATFORM_WIN32_KHR)

elseif (UNIX)

    add_compile_options (-fPIC)

    find_package (Wayland)
    if (Wayland_FOUND)
        add_definitions (-DVK_USE_PLATFORM_WAYLAND_KHR)
    endif ()

    find_package (XCB)
    if (XCB_FOUND)
        add_definitions (-DVK_USE_PLATFORM_XCB_KHR)
    endif ()

    find_package (X11)
    if (X11_FOUND)
        add_definitions (-DVK_USE_PLATFORM_XLIB_KHR)
    endif ()
    if (X11_Xrandr_FOUND)
        add_definitions (-DVK_USE_PLATFORM_XLIB_XRANDR_EXT)
    endif ()

    if (NOT Wayland_FOUND AND
        NOT XCB_FOUND AND
        NOT X11_FOUND)
        message (FATAL_ERROR "No window library found")
    endif ()

endif ()

# Include implementation of the layer
add_subdirectory (profiler)
add_subdirectory (profiler_ext)
add_subdirectory (profiler_output)

set (resource
    ${CMAKE_CURRENT_BINARY_DIR}/${PROFILER_LAYER_PROJECTNAME}.rc
    )

set (generated
    ${CMAKE_CURRENT_BINARY_DIR}/${PROFILER_LAYER_PROJECTNAME}.generated.h
    )

set (headers
    "profiler_layer_functions/Dispatch.h"
    "profiler_layer_functions/Helpers.h"
    "profiler_layer_functions/VkCommandBuffer_functions.h"
    "profiler_layer_functions/VkDevice_functions.h"
    "profiler_layer_functions/VkDevice_functions_base.h"
    "profiler_layer_functions/VkInstance_functions.h"
    "profiler_layer_functions/VkLoader_functions.h"
    "profiler_layer_functions/VkQueue_functions.h"
    )

set (sources
    "profiler_layer_functions/VkCommandBuffer_functions.cpp"
    "profiler_layer_functions/VkDevice_functions.cpp"
    "profiler_layer_functions/VkDevice_functions_base.cpp"
    "profiler_layer_functions/VkInstance_functions.cpp"
    "profiler_layer_functions/VkLoader_functions.cpp"
    "profiler_layer_functions/VkQueue_functions.cpp"
    "VkLayer_profiler_layer.cpp"
    )
    
# Configure resources file
configure_file (
    ${CMAKE_CURRENT_SOURCE_DIR}/${PROFILER_LAYER_PROJECTNAME}.rc.in
    ${resource}
    @ONLY)

configure_file (
    ${CMAKE_CURRENT_SOURCE_DIR}/${PROFILER_LAYER_PROJECTNAME}.generated.h.in
    ${generated}
    @ONLY)

# Add sources to this project's executable.
add_library (${PROFILER_LAYER_PROJECTNAME} SHARED
    ${sources}
    ${headers}
    ${generated}
    ${resource})

# Link against implementation
target_link_libraries (${PROFILER_LAYER_PROJECTNAME} profiler_layer)
target_link_libraries (${PROFILER_LAYER_PROJECTNAME} profiler_ext)
target_link_libraries (${PROFILER_LAYER_PROJECTNAME} profiler_output)

# Set include path
target_include_directories (${PROFILER_LAYER_PROJECTNAME}
    PRIVATE ${VULKAN_HEADERS_INCLUDE_DIR}
    PRIVATE ${VULKAN_LOADER_INCLUDE_DIR}
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Install target
install (TARGETS ${PROFILER_LAYER_PROJECTNAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

# Export the layer metadata file
install (FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/${PROFILER_LAYER_PROJECTNAME}.json
    DESTINATION ${CMAKE_INSTALL_BINDIR})

add_custom_command (TARGET ${PROFILER_LAYER_PROJECTNAME}
    POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/${PROFILER_LAYER_PROJECTNAME}.json
    ${CMAKE_CURRENT_BINARY_DIR}/${PROFILER_LAYER_PROJECTNAME}.json)
    
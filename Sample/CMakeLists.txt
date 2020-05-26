# CMakeList.txt : CMake project for Sample, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.7)

project (Sample)

add_subdirectory (SDL)
add_subdirectory (shaders)

set (headers
    "Device.h"
    "Image.h"
    "SwapChain.h"
    )

set (sources
    "Device.cpp"
    "Image.cpp"
    "Sample.cpp"
    "SwapChain.cpp"
    )

add_executable (Sample
    ${headers}
    ${sources})

target_include_directories (Sample
    PRIVATE $ENV{VK_SDK_PATH}/Include
    PRIVATE "${CMAKE_CURRENT_LIST_DIR}/glm/glm"
    PRIVATE "${CMAKE_CURRENT_LIST_DIR}/SDL/include"
    )

add_dependencies (Sample SDL2main)
add_dependencies (Sample Sample-shaders)

function (TARGET_LINK_DIRECTORIES target)
    if (${CMAKE_VERSION} VERSION_LESS "3.15.0")
        link_directories (${ARGN})
    else ()
        target_link_directories (${target} PRIVATE ${ARGN})
    endif ()
endfunction ()

if (${CMAKE_SIZEOF_VOID_P} EQUAL 8) # 64-bit
    TARGET_LINK_DIRECTORIES (Sample $ENV{VK_SDK_PATH}/Lib)
else ()
    TARGET_LINK_DIRECTORIES (Sample $ENV{VK_SDK_PATH}/Lib32)
endif ()

target_link_libraries (Sample
    PRIVATE vulkan-1
    PRIVATE SDL2-static
    PRIVATE SDL2main
    )

install (TARGETS Sample DESTINATION ${CMAKE_OUTPUT_BINDIR})

# CMakeList.txt : CMake project for Sample, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project (Sample)

add_subdirectory (SDL)

set (sources
    "Sample.cpp"
    )

add_executable (Sample ${sources})

target_include_directories (Sample
    PRIVATE "$ENV{VK_SDK_PATH}/Include"
    PRIVATE "${CMAKE_CURRENT_LIST_DIR}/glm/glm"
    PRIVATE "${CMAKE_CURRENT_LIST_DIR}/SDL/include"
    )

add_dependencies (Sample SDL2main)

target_link_directories (Sample
    PRIVATE "$ENV{VK_SDK_PATH}/Lib"
    )

target_link_libraries (Sample
    PRIVATE vulkan-1
    PRIVATE SDL2-static
    PRIVATE SDL2main
    )

install (TARGETS Sample DESTINATION ${CMAKE_OUTPUT_BINDIR})

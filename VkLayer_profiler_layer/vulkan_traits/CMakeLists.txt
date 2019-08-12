# CMakeList.txt : CMake project for Vulkan traits, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project (vulkan_traits)

find_package (PythonInterp 3.4 REQUIRED)
find_package (PythonLibs 3.4 REQUIRED)

set (vulkan_traits_header ${CMAKE_CURRENT_BINARY_DIR}/vulkan_traits.generated.h)

configure_file (
    ${CMAKE_CURRENT_SOURCE_DIR}/generated.py.in
    generated.py)

add_custom_target (vulkan_traits ALL
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_traits.py ${CMAKE_CURRENT_BINARY_DIR}
    BYPRODUCTS ${vulkan_traits_header}
    COMMENT "Generating Vulkan traits")

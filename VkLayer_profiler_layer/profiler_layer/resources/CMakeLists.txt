# CMakeList.txt : CMake project for profiler layer, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project (profiler_resources)

find_package (PythonInterp 3.4 REQUIRED)
find_package (PythonLibs 3.4 REQUIRED)

set (embedded_resources_header ${CMAKE_CURRENT_BINARY_DIR}/profiler_resources.generated.h)

set (resources
    ${CMAKE_CURRENT_SOURCE_DIR}/profiler_font_glyphs.dds
    )
    
configure_file (
    ${CMAKE_CURRENT_SOURCE_DIR}/generated.py.in
    generated.py)

add_custom_target (profiler_resources ALL
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/embed_resources.py ${CMAKE_CURRENT_BINARY_DIR}
    BYPRODUCTS ${embedded_resources_header}
    COMMENT "Embedding resources")

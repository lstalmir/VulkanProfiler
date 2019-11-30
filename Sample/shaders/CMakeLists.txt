# CMakeList.txt : CMake project for Sample, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project (Sample-shaders)

find_program (glslangvalidator glslangvalidator)
if (NOT glslangvalidator)
    message (FATAL_ERROR "glslangvalidator not found")
else ()
    message ("-- Using glslangvalidator: ${glslangvalidator}")
endif ()

set (sources
    "FragmentShader.frag.hlsl"
    "VertexShader.vert.hlsl"
    )

set (spirvs
    )

foreach (source ${sources})
    set (in ${CMAKE_CURRENT_SOURCE_DIR}/${source})
    set (out ${CMAKE_CURRENT_BINARY_DIR}/../${source}.spv)
    add_custom_command (
        OUTPUT ${out}
        COMMAND ${glslangvalidator} ${in} -o ${out} -V -e main
        DEPENDS ${in}
        COMMENT ${source}
        )
    set (spirvs ${spirvs} ${out})
endforeach ()

add_custom_target (Sample-shaders ALL
    DEPENDS ${spirvs})

install (FILES
    ${spirvs}
    DESTINATION BIN ${CMAKE_OUTPUT_BINDIR})

cmake_minimum_required (VERSION 3.8)

project (profiler_ult_shaders)

set (sources
    "simple_triangle.vert.hlsl"
    "simple_triangle.frag.hlsl"
    )
    
set (spirvs
    )

foreach (source ${sources})
    set (in ${CMAKE_CURRENT_SOURCE_DIR}/${source})
    set (out ${CMAKE_CURRENT_BINARY_DIR}/${source}.h)
    string (REPLACE "." "_" var_name ${source})
    add_custom_command (
        OUTPUT ${out}
        COMMAND ${glslangvalidator} ${in} -o ${out} -V -D -e main --vn ${var_name}
        DEPENDS ${in}
        COMMENT ${source}
        )
    set (spirvs ${spirvs} ${out})
endforeach ()

add_custom_target (profiler_ult_shaders ALL
    DEPENDS ${spirvs})

install (FILES
    ${spirvs}
    DESTINATION BIN ${CMAKE_OUTPUT_BINDIR})

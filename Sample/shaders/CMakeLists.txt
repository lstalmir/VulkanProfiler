# Copyright (c) 2020 Lukasz Stalmirski
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required (VERSION 3.8)

project (Sample-shaders)

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

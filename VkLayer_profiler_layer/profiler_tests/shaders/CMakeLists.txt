# Copyright (c) 2019-2025 Lukasz Stalmirski
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

cmake_minimum_required (VERSION 3.8...3.31)

project (profiler_tests_shaders)

set (sources
    "simple_triangle.vert.glsl"
    "simple_triangle.frag.glsl"
    "simple_triangle_rt.rgen.glsl"
    "simple_triangle_rt.rchit.glsl"
    "simple_triangle_rt.rmiss.glsl"
    )

set (spirvs
    )

foreach (source ${sources})
    set (in ${CMAKE_CURRENT_SOURCE_DIR}/${source})
    set (out ${CMAKE_CURRENT_BINARY_DIR}/${source}.h)
    string (REPLACE "." "_" var_name ${source})
    add_custom_command (
        OUTPUT ${out}
        COMMAND ${glslangvalidator} ${in} -o ${out} -V -e main --vn ${var_name} --target-env spirv1.4
        DEPENDS ${in}
        COMMENT ${source}
        )
    set (spirvs ${spirvs} ${out})
endforeach ()

add_custom_target (profiler_tests_shaders ALL
    DEPENDS ${spirvs})

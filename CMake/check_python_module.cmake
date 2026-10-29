# Copyright (c) 2026 Lukasz Stalmirski
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

function (check_python_module MODULE_NAME)
    set (OPTIONS REQUIRED)
    set (ONE_VALUE_ARGS)
    set (MULTI_VALUE_ARGS NAMES)
    cmake_parse_arguments (arg
        "${OPTIONS}"
        "${ONE_VALUE_ARGS}"
        "${MULTI_VALUE_ARGS}"
        ${ARGN})

    if (NOT arg_NAMES)
        set (arg_NAMES ${MODULE_NAME})
    endif ()

    foreach (MOD IN LISTS arg_NAMES)
        execute_process (
            COMMAND "${Python3_EXECUTABLE}" -c "import ${MOD}"
            RESULT_VARIABLE EXIT_CODE
            OUTPUT_QUIET
            ERROR_QUIET)

        if (${EXIT_CODE} EQUAL 0)
            set (${MODULE_NAME}_FOUND 1)
            set (${MODULE_NAME}_FOUND 1 PARENT_SCOPE)
            message ("-- Found Python module ${MODULE_NAME}: ${MOD}")
            break ()
        endif ()
    endforeach ()
    
    if (NOT ${MODULE_NAME}_FOUND AND arg_REQUIRED)
        message (FATAL_ERROR "Required Python module ${MODULE_NAME} not found.")
    endif ()
endfunction ()

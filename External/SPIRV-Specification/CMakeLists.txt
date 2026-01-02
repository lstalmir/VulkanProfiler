# Copyright (c) 2024-2026 Lukasz Stalmirski
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

set (SPIRV_SPEC_FILE "${CMAKE_CURRENT_LIST_DIR}/SPIRV.html")
set (SPIRV_SPEC_LICENSE "${CMAKE_CURRENT_LIST_DIR}/LICENSE")

function (spirv_spec_download)
    set (SPIRV_DOWNLOAD_FILE "${SPIRV_SPEC_FILE}~")

    message ("Downloading SPIR-V documentation from ${SPIRV_DOCS_URL}")
    file (DOWNLOAD "${SPIRV_DOCS_URL}" "${SPIRV_DOWNLOAD_FILE}"
          INACTIVITY_TIMEOUT 3
          STATUS SPIRV_DOCS_DOWNLOAD_STATUS)

    list (GET SPIRV_DOCS_DOWNLOAD_STATUS 0 SPIRV_DOCS_DOWNLOAD_STATUS_CODE)
    list (GET SPIRV_DOCS_DOWNLOAD_STATUS 1 SPIRV_DOCS_DOWNLOAD_STATUS_MSG)

    if (SPIRV_DOCS_DOWNLOAD_STATUS_CODE EQUAL 0)
        if (EXISTS "${SPIRV_SPEC_FILE}")
            # Compare files to avoid rebuilding if the specification hasn't changed.
            execute_process (COMMAND ${CMAKE_COMMAND} -E compare_files
                    "${SPIRV_SPEC_FILE}"
                    "${SPIRV_DOWNLOAD_FILE}"
                RESULT_VARIABLE COMPARE_RESULT)

            if (COMPARE_RESULT EQUAL 1)
                file (RENAME "${SPIRV_DOWNLOAD_FILE}" "${SPIRV_SPEC_FILE}")
            endif ()
        else ()
            file (RENAME "${SPIRV_DOWNLOAD_FILE}" "${SPIRV_SPEC_FILE}")
        endif ()
    else ()
        message (WARNING "${SPIRV_DOCS_DOWNLOAD_STATUS_CODE}: ${SPIRV_DOCS_DOWNLOAD_STATUS_MSG}")
    endif ()

    file (REMOVE "${SPIRV_DOWNLOAD_FILE}")
endfunction ()

if (BUILD_SPIRV_DOCS)
    # Check if the specification has been recently downloaded.
    file (TIMESTAMP "${SPIRV_SPEC_FILE}" SPEC_DOWNLOAD_TIME "%s" UTC)
    string (TIMESTAMP CURRENT_TIME "%s" UTC)

    if (CURRENT_TIME AND SPEC_DOWNLOAD_TIME)
        math (EXPR TIME_SINCE_DOWNLOAD "${CURRENT_TIME} - ${SPEC_DOWNLOAD_TIME}")
        if (TIME_SINCE_DOWNLOAD GREATER 86400)
            spirv_spec_download()
        endif ()
    else ()
        spirv_spec_download()
    endif ()

    if (NOT EXISTS "${SPIRV_SPEC_FILE}")
        message (WARNING "SPIRV.html not found. SPIR-V documentation will be disabled in this build.")
        set (BUILD_SPIRV_DOCS OFF CACHE BOOL "" FORCE)
    endif ()
endif ()

if (NOT BUILD_SPIRV_DOCS)
    file (REMOVE "${SPIRV_SPEC_FILE}")
    file (REMOVE "${SPIRV_SPEC_LICENSE}")
endif ()

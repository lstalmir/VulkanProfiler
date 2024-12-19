# Copyright (c) 2024 Lukasz Stalmirski
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

if (BUILD_SPIRV_DOCS)
    message ("Downloading SPIR-V documentation from ${SPIRV_DOCS_URL}")
    file (DOWNLOAD "${SPIRV_DOCS_URL}" "${CMAKE_CURRENT_LIST_DIR}/SPIRV.html"
          STATUS SPIRV_DOCS_DOWNLOAD_STATUS)

    list (GET SPIRV_DOCS_DOWNLOAD_STATUS 0 SPIRV_DOCS_DOWNLOAD_STATUS_CODE)
    list (GET SPIRV_DOCS_DOWNLOAD_STATUS 1 SPIRV_DOCS_DOWNLOAD_STATUS_MSG)

    if (NOT SPIRV_DOCS_DOWNLOAD_STATUS_CODE EQUAL 0)
        message ("Error ${SPIRV_DOCS_DOWNLOAD_STATUS_CODE}: ${SPIRV_DOCS_DOWNLOAD_STATUS_MSG}")
        message ("SPIR-V documentation will be disabled in this build")
        set (BUILD_SPIRV_DOCS OFF CACHE BOOL "" FORCE)
    endif ()
endif ()

if (NOT BUILD_SPIRV_DOCS)
    file (REMOVE "${CMAKE_CURRENT_LIST_DIR}/SPIRV.html")
    file (REMOVE "${CMAKE_CURRENT_LIST_DIR}/LICENSE")
endif ()

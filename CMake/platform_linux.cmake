# Copyright (c) 2023-2025 Lukasz Stalmirski
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

find_package (X11)

if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.16.0)
    # ECM is required on Linux to find Wayland and XCB.
    find_package (ECM NO_MODULE)
    if (ECM_FOUND)
        set (CMAKE_MODULE_PATH ${ECM_FIND_MODULE_DIR})
        find_package (XCB COMPONENTS XCB SHAPE)
        #find_package (Wayland)
    endif ()
endif ()

if (X11_FOUND AND X11_Xshape_FOUND)
    message ("-- Profiler platform enabled: Xlib")
    set (PROFILER_PLATFORM_XLIB_FOUND 1)
    set (PROFILER_PLATFORM_FOUND 1)
    add_definitions (-DVK_USE_PLATFORM_XLIB_KHR)
endif ()

if (XCB_FOUND AND XCB_SHAPE_FOUND)
    message ("-- Profiler platform enabled: XCB")
    set (PROFILER_PLATFORM_XCB_FOUND 1)
    set (PROFILER_PLATFORM_FOUND 1)
    add_definitions (-DVK_USE_PLATFORM_XCB_KHR)
endif ()

if (Wayland_FOUND)
    message ("-- Profiler platform enabled: Wayland")
    set (PROFILER_PLATFORM_WAYLAND_FOUND 1)
    set (PROFILER_PLATFORM_FOUND 1)
    add_definitions (-DVK_USE_PLATFORM_WAYLAND_KHR)
endif ()

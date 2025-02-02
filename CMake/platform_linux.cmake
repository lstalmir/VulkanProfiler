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

if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.16.0)
    # ECM is required on Linux to find Wayland and XCB.
    find_package (ECM NO_MODULE)
    if (ECM_FOUND)
        set (CMAKE_MODULE_PATH ${ECM_FIND_MODULE_DIR})

        #find_package (Wayland)
        #if (Wayland_FOUND)
        #    set (PROFILER_PLATFORM_FOUND 1)
        #endif ()

        find_package (XCB)
        if (XCB_FOUND)
            set (PROFILER_PLATFORM_FOUND 1)
        endif ()
    endif ()
endif ()

# If either Wayland or XCB was found, X11 is optional.
find_package (X11)
if (X11_FOUND)
    set (PROFILER_PLATFORM_FOUND 1)
endif ()

# Enable Vulkan platforms for each SDK found.
if (X11_FOUND)
    add_definitions (-DVK_USE_PLATFORM_XLIB_KHR)
endif ()
if (XCB_FOUND)
    add_definitions (-DVK_USE_PLATFORM_XCB_KHR)
endif ()
if (Wayland_FOUND)
    add_definitions (-DVK_USE_PLATFORM_WAYLAND_KHR)
endif ()

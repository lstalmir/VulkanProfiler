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

cmake_minimum_required (VERSION 3.8...3.31)

include (GNUInstallDirs)

set (vkprofiler_gui_sources
    vkprofiler_gui_main.cpp
    vkprofiler_gui.h
    vkprofiler_gui.cpp
    vkprofiler_gui_backend.h
    vkprofiler_gui_backend.cpp)

add_executable (vkprofiler_gui WIN32
    ${vkprofiler_gui_sources})

target_link_libraries (vkprofiler_gui
    PRIVATE profiler_common
    PRIVATE profiler_overlay
    PRIVATE profiler_trace
    PRIVATE SDL3::SDL3-static)

# Install target
install (TARGETS vkprofiler_gui
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

# Copyright (c) 2019-2023 Lukasz Stalmirski
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

project (profiler)

set (headers
    "profiler.h"
    "profiler_allocator.h"
    "profiler_command_buffer.h"
    "profiler_command_buffer_query_pool.h"
    "profiler_command_pool.h"
    "profiler_config.h"
    "profiler_counters.h"
    "profiler_data.h"
    "profiler_data_aggregator.h"
    "profiler_helpers.h"
    "profiler_memory_manager.h"
    "profiler_query_pool.h"
    "profiler_resources.h"
    "profiler_shader.h"
    "profiler_stat_comparators.h"
    "profiler_sync.h"
    )

set (sources
    "profiler.cpp"
    "profiler_allocator.cpp"
    "profiler_command_buffer.cpp"
    "profiler_command_buffer_query_pool.cpp"
    "profiler_command_pool.cpp"
    "profiler_config.cpp"
    "profiler_data.cpp"
    "profiler_data_aggregator.cpp"
    "profiler_memory_manager.cpp"
    "profiler_query_pool.cpp"
    "profiler_shader.cpp"
    "profiler_sync.cpp"
    # Windows
    "profiler_counters_windows.inl"
    "profiler_helpers_windows.cpp"
    # Linux
    "profiler_counters_linux.inl"
    "profiler_helpers_linux.cpp"
    )

set (intel
    "intel/profiler_metrics_api.h"
    "intel/profiler_metrics_api.cpp"
    )

# Link intermediate static library
add_library (profiler
    ${sources}
    ${headers}
    ${intel})

target_link_libraries (profiler
    PUBLIC profiler_common
    PUBLIC metrics-discovery)

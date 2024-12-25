// Copyright (c) 2024 Lukasz Stalmirski
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once
#include <vulkan/vulkan.h>

#ifndef VK_EXT_profiler_testing
#define VK_EXT_profiler_testing 1
#define VK_EXT_PROFILER_TESTING_SPEC_VERSION 1
#define VK_EXT_PROFILER_TESTING_EXTENSION_NAME "VK_EXT_profiler_testing"

namespace Profiler
{
    class DeviceProfiler;
    class ProfilerOverlayOutput;
}

typedef VKAPI_ATTR void( VKAPI_CALL* PFN_vkGetDeviceProfilerEXT )( VkDevice device, Profiler::DeviceProfiler** ppProfiler );
typedef VKAPI_ATTR void( VKAPI_CALL* PFN_vkGetDeviceProfilerOverlayEXT )( VkDevice device, Profiler::ProfilerOverlayOutput** ppOverlay );

#ifndef VK_NO_PROTOTYPES
VKAPI_ATTR void VKAPI_CALL vkGetDeviceProfilerEXT(
    VkDevice device,
    Profiler::DeviceProfiler** ppProfiler );

VKAPI_ATTR void VKAPI_CALL vkGetDeviceProfilerOverlayEXT(
    VkDevice device,
    Profiler::ProfilerOverlayOutput** ppOverlay );
#endif

#endif // VK_EXT_profiler_testing

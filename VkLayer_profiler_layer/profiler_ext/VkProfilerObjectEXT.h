// Copyright (c) 2025 Lukasz Stalmirski
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

#ifndef VK_EXT_profiler_object
#define VK_EXT_profiler_object 1
#define VK_EXT_PROFILER_OBJECT_SPEC_VERSION 1
#define VK_EXT_PROFILER_OBJECT_EXTENSION_NAME "VK_EXT_profiler_object"

VK_DEFINE_NON_DISPATCHABLE_HANDLE( VkProfilerEXT );
VK_DEFINE_NON_DISPATCHABLE_HANDLE( VkProfilerOverlayEXT );

typedef void( VKAPI_PTR* PFN_vkGetProfilerEXT )(VkDevice, VkProfilerEXT*);
typedef void( VKAPI_PTR* PFN_vkGetProfilerOverlayEXT )(VkDevice, VkProfilerOverlayEXT*);

#ifndef VK_NO_PROTOTYPES
VKAPI_ATTR void VKAPI_CALL vkGetProfilerEXT(
    VkDevice device,
    VkProfilerEXT* pProfiler );

VKAPI_ATTR void VKAPI_CALL vkGetProfilerOverlayEXT(
    VkDevice device,
    VkProfilerOverlayEXT* pOverlay );
#endif // VK_NO_PROTOTYPES
#endif // VK_EXT_profiler_object
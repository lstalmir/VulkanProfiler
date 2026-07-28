// Copyright (c) 2026 Lukasz Stalmirski
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

#ifdef __cplusplus
extern "C" {
#endif

typedef enum VkProfilerAccelerationStructureTypeFlagBitsEXT
{
    VK_PROFILER_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_BIT_EXT = 0x00000001,
    VK_PROFILER_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_BIT_EXT = 0x00000002,
    VK_PROFILER_ACCELERATION_STRUCTURE_TYPE_GENERIC_BIT_EXT = 0x00000004,
    VK_PROFILER_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_BIT_EXT = 0x00000008,
    VK_PROFILER_ACCELERATION_STRUCTURE_TYPE_MAX_ENUM_BIT_EXT = 0x7FFFFFFF
} VkProfilerAccelerationStructureTypeFlagBitsEXT;
typedef VkFlags VkProfilerAccelerationStructureTypeFlagsEXT;

inline VkAccelerationStructureTypeKHR vkProfilerGetAccelerationStructureTypeFromFlagsEXT(
    VkProfilerAccelerationStructureTypeFlagsEXT flags )
{
    switch( flags )
    {
    case VK_PROFILER_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_BIT_EXT:
        return VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

    case VK_PROFILER_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_BIT_EXT:
        return VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

    case VK_PROFILER_ACCELERATION_STRUCTURE_TYPE_GENERIC_BIT_EXT:
        return VK_ACCELERATION_STRUCTURE_TYPE_GENERIC_KHR;

    case VK_PROFILER_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_BIT_EXT:
        return VK_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_KHR;

    default:
        return VK_ACCELERATION_STRUCTURE_TYPE_MAX_ENUM_KHR;
    }
}

inline VkProfilerAccelerationStructureTypeFlagsEXT vkProfilerGetAccelerationStructureFlagsFromTypeEXT(
    VkAccelerationStructureTypeKHR type )
{
    switch( type )
    {
    case VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR:
        return VK_PROFILER_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_BIT_EXT;

    case VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR:
        return VK_PROFILER_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_BIT_EXT;

    case VK_ACCELERATION_STRUCTURE_TYPE_GENERIC_KHR:
        return VK_PROFILER_ACCELERATION_STRUCTURE_TYPE_GENERIC_BIT_EXT;

    case VK_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_KHR:
        return VK_PROFILER_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_BIT_EXT;

    default:
        return 0;
    }
}

#ifdef __cplusplus
} // extern "C"
#endif

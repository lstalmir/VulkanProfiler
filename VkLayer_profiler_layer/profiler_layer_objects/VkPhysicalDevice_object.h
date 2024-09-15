// Copyright (c) 2019-2021 Lukasz Stalmirski
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
#include "vk_dispatch_tables.h"
#include <vector>

namespace Profiler
{
    enum class VkPhysicalDevice_Vendor_ID
    {
        eUnknown = 0,
        eAMD = 0x1002,
        eARM = 0x13B3,
        eINTEL = 0x8086,
        eNV = 0x10DE,
        eQualcomm = 0x5143
    };

    struct VkPhysicalDevice_Object
    {
        VkPhysicalDevice Handle = VK_NULL_HANDLE;

        VkPhysicalDeviceProperties Properties;
        VkPhysicalDeviceMemoryProperties MemoryProperties;

        VkPhysicalDevice_Vendor_ID VendorID;

        std::vector<VkQueueFamilyProperties> QueueFamilyProperties;

        inline uint32_t FindGraphicsQueueFamilyIndex() const
        {
            return FindQueueFamilyIndex(
                VK_QUEUE_GRAPHICS_BIT );
        }

        inline uint32_t FindComputeQueueFamilyIndex() const
        {
            return FindQueueFamilyIndex(
                VK_QUEUE_COMPUTE_BIT,
                VK_QUEUE_GRAPHICS_BIT );
        }

        inline uint32_t FindTransferQueueFamilyIndex() const
        {
            return FindQueueFamilyIndex(
                VK_QUEUE_TRANSFER_BIT,
                VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT );
        }

        inline uint32_t FindQueueFamilyIndex( VkQueueFlags includeBits, VkQueueFlags excludeBits = 0 ) const
        {
            for( uint32_t i = 0; i < QueueFamilyProperties.size(); ++i )
            {
                if( ( ( QueueFamilyProperties[i].queueFlags & includeBits ) == includeBits ) &&
                    ( ( QueueFamilyProperties[i].queueFlags & excludeBits ) == 0 ) )
                {
                    return i;
                }
            }
            return UINT32_MAX;
        }
    };
}

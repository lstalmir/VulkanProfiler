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
#include "profiler_data.h"
#include "utils/lockable_unordered_map.h"
#include <vulkan/vulkan.h>

namespace Profiler
{
    struct VkDevice_Object;

    /***********************************************************************************\

    Class:
        DeviceProfilerMemoryTracker

    Description:
        Tracks device memory allocations and bindings.

    \***********************************************************************************/
    class DeviceProfilerMemoryTracker
    {
    public:
        DeviceProfilerMemoryTracker();

        VkResult Initialize( VkDevice_Object* pDevice );
        void Destroy();

        void RegisterAllocation( VkDeviceMemory memory, const VkMemoryAllocateInfo* pAllocateInfo );
        void UnregisterAllocation( VkDeviceMemory memory );

        void RegisterBuffer( VkBuffer buffer, const VkBufferCreateInfo* pCreateInfo );
        void UnregisterBuffer( VkBuffer buffer );
        void BindBufferMemory( VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize offset );

        void RegisterImage( VkImage image, const VkImageCreateInfo* pCreateInfo );
        void UnregisterImage( VkImage image );
        void BindImageMemory( VkImage image, VkDeviceMemory memory, VkDeviceSize offset );

        std::pair<VkBuffer, DeviceProfilerBufferMemoryData> GetBufferAtAddress( VkDeviceAddress address, VkBufferUsageFlags requiredUsage ) const;

        DeviceProfilerMemoryData GetMemoryData() const;

    private:
        VkDevice_Object* m_pDevice;

        std::shared_mutex mutable m_AggregatedDataMutex;
        uint64_t m_TotalAllocationSize;
        uint64_t m_TotalAllocationCount;
        std::vector<DeviceProfilerMemoryHeapData> m_Heaps;
        std::vector<DeviceProfilerMemoryTypeData> m_Types;

        ConcurrentMap<VkDeviceMemory, DeviceProfilerDeviceMemoryData> m_Allocations;
        ConcurrentMap<VkBuffer, DeviceProfilerBufferMemoryData> m_Buffers;
        ConcurrentMap<VkImage, DeviceProfilerImageMemoryData> m_Images;

        PFN_vkGetBufferDeviceAddress m_pfnGetBufferDeviceAddress;

        void ResetMemoryData();

        VkDeviceAddress GetBufferDeviceAddress( VkBuffer buffer, VkBufferUsageFlags usage ) const;
    };
}

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
#include "profiler_layer_objects/VkObject.h"
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

        void RegisterAllocation( VkDeviceMemoryHandle memory, const VkMemoryAllocateInfo* pAllocateInfo );
        void UnregisterAllocation( VkDeviceMemoryHandle memory );

        void RegisterBuffer( VkBufferHandle buffer, const VkBufferCreateInfo* pCreateInfo );
        void UnregisterBuffer( VkBufferHandle buffer );
        void BindBufferMemory( VkBufferHandle buffer, VkDeviceMemoryHandle memory, VkDeviceSize offset );
        void BindSparseBufferMemory( VkBufferHandle buffer, VkDeviceSize bufferOffset, VkDeviceMemoryHandle memory, VkDeviceSize memoryOffset, VkDeviceSize size, VkSparseMemoryBindFlags flags );

        void RegisterImage( VkImageHandle image, const VkImageCreateInfo* pCreateInfo );
        void UnregisterImage( VkImageHandle image );
        void BindImageMemory( VkImageHandle image, VkDeviceMemoryHandle memory, VkDeviceSize offset );
        void BindSparseImageMemory( VkImageHandle image, VkDeviceSize imageOffset, VkDeviceMemoryHandle memory, VkDeviceSize memoryOffset, VkDeviceSize size, VkSparseMemoryBindFlags flags );
        void BindSparseImageMemory( VkImageHandle image, VkImageSubresource subresource, VkOffset3D offset, VkExtent3D extent, VkDeviceMemoryHandle memory, VkDeviceSize memoryOffset, VkSparseMemoryBindFlags flags );

        void RegisterAccelerationStructure( VkAccelerationStructureKHRHandle accelerationStructure, VkBufferHandle buffer, const VkAccelerationStructureCreateInfoKHR* pCreateInfo );
        void UnregisterAccelerationStructure( VkAccelerationStructureKHRHandle accelerationStructure );

        void RegisterMicromap( VkMicromapEXTHandle micromap, VkBufferHandle buffer, const VkMicromapCreateInfoEXT* pCreateInfo );
        void UnregisterMicromap( VkMicromapEXTHandle micromap );

        DeviceProfilerMemoryData GetMemoryData() const;

    private:
        VkDevice_Object* m_pDevice;

        PFN_vkGetPhysicalDeviceMemoryProperties2 m_pfnGetPhysicalDeviceMemoryProperties2;

        bool m_MemoryBudgetEnabled;

        std::shared_mutex mutable m_AggregatedDataMutex;
        uint64_t m_TotalAllocationSize;
        uint64_t m_TotalAllocationCount;
        std::vector<DeviceProfilerMemoryHeapData> m_Heaps;
        std::vector<DeviceProfilerMemoryTypeData> m_Types;

        ConcurrentMap<VkDeviceMemoryHandle, DeviceProfilerDeviceMemoryData> m_Allocations;
        ConcurrentMap<VkBufferHandle, DeviceProfilerBufferMemoryData> m_Buffers;
        ConcurrentMap<VkImageHandle, DeviceProfilerImageMemoryData> m_Images;
        ConcurrentMap<VkAccelerationStructureKHRHandle, DeviceProfilerAccelerationStructureMemoryData> m_AccelerationStructures;
        ConcurrentMap<VkMicromapEXTHandle, DeviceProfilerMicromapMemoryData> m_Micromaps;

        std::shared_mutex mutable m_MemoryBindingMutex;

        void ResetMemoryData();
    };
}

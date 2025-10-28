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

        void RegisterAllocation( VkObjectHandle<VkDeviceMemory> memory, const VkMemoryAllocateInfo* pAllocateInfo );
        void UnregisterAllocation( VkObjectHandle<VkDeviceMemory> memory );

        void RegisterBuffer( VkObjectHandle<VkBuffer> buffer, const VkBufferCreateInfo* pCreateInfo );
        void UnregisterBuffer( VkObjectHandle<VkBuffer> buffer );
        void BindBufferMemory( VkObjectHandle<VkBuffer> buffer, VkObjectHandle<VkDeviceMemory> memory, VkDeviceSize offset );
        void BindSparseBufferMemory( VkObjectHandle<VkBuffer> buffer, VkDeviceSize bufferOffset, VkObjectHandle<VkDeviceMemory> memory, VkDeviceSize memoryOffset, VkDeviceSize size, VkSparseMemoryBindFlags flags );

        void RegisterImage( VkObjectHandle<VkImage> image, const VkImageCreateInfo* pCreateInfo );
        void UnregisterImage( VkObjectHandle<VkImage> image );
        void BindImageMemory( VkObjectHandle<VkImage> image, VkObjectHandle<VkDeviceMemory> memory, VkDeviceSize offset );
        void BindSparseImageMemory( VkObjectHandle<VkImage> image, VkDeviceSize imageOffset, VkObjectHandle<VkDeviceMemory> memory, VkDeviceSize memoryOffset, VkDeviceSize size, VkSparseMemoryBindFlags flags );
        void BindSparseImageMemory( VkObjectHandle<VkImage> image, VkImageSubresource subresource, VkOffset3D offset, VkExtent3D extent, VkObjectHandle<VkDeviceMemory> memory, VkDeviceSize memoryOffset, VkSparseMemoryBindFlags flags );

        void RegisterAccelerationStructure( VkObjectHandle<VkAccelerationStructureKHR> accelerationStructure, VkObjectHandle<VkBuffer> buffer, const VkAccelerationStructureCreateInfoKHR* pCreateInfo );
        void UnregisterAccelerationStructure( VkObjectHandle<VkAccelerationStructureKHR> accelerationStructure );

        void RegisterMicromap( VkObjectHandle<VkMicromapEXT> micromap, VkObjectHandle<VkBuffer> buffer, const VkMicromapCreateInfoEXT* pCreateInfo );
        void UnregisterMicromap( VkObjectHandle<VkMicromapEXT> micromap );

        std::pair<VkBuffer, DeviceProfilerBufferMemoryData> GetBufferAtAddress( VkDeviceAddress address, VkBufferUsageFlags requiredUsage ) const;

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

        ConcurrentMap<VkObjectHandle<VkDeviceMemory>, DeviceProfilerDeviceMemoryData> m_Allocations;
        ConcurrentMap<VkObjectHandle<VkBuffer>, DeviceProfilerBufferMemoryData> m_Buffers;
        ConcurrentMap<VkObjectHandle<VkImage>, DeviceProfilerImageMemoryData> m_Images;
        ConcurrentMap<VkObjectHandle<VkAccelerationStructureKHR>, DeviceProfilerAccelerationStructureMemoryData> m_AccelerationStructures;
        ConcurrentMap<VkObjectHandle<VkMicromapEXT>, DeviceProfilerMicromapMemoryData> m_Micromaps;

        std::shared_mutex mutable m_MemoryBindingMutex;

        PFN_vkGetBufferDeviceAddress m_pfnGetBufferDeviceAddress;

        void ResetMemoryData();

        VkDeviceAddress GetBufferDeviceAddress( VkBuffer buffer, VkBufferUsageFlags usage ) const;
    };
}

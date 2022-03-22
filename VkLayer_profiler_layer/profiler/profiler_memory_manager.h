// Copyright (c) 2022 Lukasz Stalmirski
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
#include <vulkan/vk_layer.h>
#include <list>
#include <vector>
#include <mutex>

namespace Profiler
{
    struct VkDevice_Object;

    struct DeviceProfilerMemoryPool
    {
        VkDeviceMemory m_DeviceMemory;
        VkDeviceSize m_Size;
        VkDeviceSize m_FreeSize;
        uint32_t m_MemoryTypeIndex;
        VkMemoryPropertyFlags m_Flags;
        void* m_pMappedMemory;
        std::vector<bool> m_Allocations;
    };

    struct DeviceProfilerMemoryAllocation
    {
        DeviceProfilerMemoryPool* m_pPool;
        VkDeviceSize m_Offset;
        VkDeviceSize m_Size;
        void* m_pMappedMemory;
    };

    class DeviceProfilerMemoryManager
    {
    public:
        DeviceProfilerMemoryManager();

        VkResult Initialize( VkDevice_Object* pDevice );
        void Destroy();

        VkResult AllocateMemory(
            const VkMemoryRequirements& memoryRequirements,
            VkMemoryPropertyFlags requiredFlags,
            DeviceProfilerMemoryAllocation* pAllocation );

        void FreeMemory(
            DeviceProfilerMemoryAllocation* pAllocation );

    private:
        VkDevice_Object* m_pDevice;

        std::mutex m_MemoryAllocationMutex;

        VkDeviceSize m_DefaultMemoryPoolSize;
        VkDeviceSize m_DefaultMemoryBlockSize;

        VkPhysicalDeviceMemoryProperties m_DeviceMemoryProperties;

        std::list<DeviceProfilerMemoryPool> m_MemoryPools;

        VkResult AllocatePool(
            VkDeviceSize size,
            uint32_t requiredTypeBits,
            VkMemoryPropertyFlags requiredFlags,
            DeviceProfilerMemoryPool** ppPool );
    };
}

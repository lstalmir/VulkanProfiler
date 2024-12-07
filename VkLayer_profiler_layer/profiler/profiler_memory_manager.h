// Copyright (c) 2022-2024 Lukasz Stalmirski
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
#define VK_NO_PROTOTYPES
#include <vulkan/vk_layer.h>
#include <vk_mem_alloc.h>
#include <mutex>
#include <unordered_map>

namespace Profiler
{
    struct VkDevice_Object;

    /***********************************************************************************\

    Class:
        DeviceProfilerMemoryManager

    Description:
        Manages GPU resources and allocations created internally by the profiling layer.

    \***********************************************************************************/
    class DeviceProfilerMemoryManager
    {
    public:
        DeviceProfilerMemoryManager();

        VkResult Initialize( VkDevice_Object* pDevice );
        void Destroy();

        VkResult AllocateBuffer(
            const VkBufferCreateInfo& bufferCreateInfo,
            const VmaAllocationCreateInfo& allocationCreateInfo,
            VkBuffer* pBuffer,
            VmaAllocation* pAllocation,
            VmaAllocationInfo* pAllocationInfo = nullptr );

        void FreeBuffer(
            VkBuffer buffer,
            VmaAllocation allocation );

        VkResult AllocateImage(
            const VkImageCreateInfo& imageCreateInfo,
            const VmaAllocationCreateInfo& allocationCreateInfo,
            VkImage* pImage,
            VmaAllocation* pAllocation,
            VmaAllocationInfo* pAllocationInfo = nullptr );

        void FreeImage(
            VkImage image,
            VmaAllocation allocation );

        VkResult Flush(
            VmaAllocation allocation,
            VkDeviceSize offset = 0,
            VkDeviceSize size = VK_WHOLE_SIZE );

        VkResult Invalidate(
            VmaAllocation allocation,
            VkDeviceSize offset = 0,
            VkDeviceSize size = VK_WHOLE_SIZE );

    private:
        VkDevice_Object* m_pDevice;
        VmaAllocator m_Allocator;

        std::mutex m_AllocationMutex;
        std::unordered_map<VkBuffer, VmaAllocation> m_BufferAllocations;
        std::unordered_map<VkImage, VmaAllocation> m_ImageAllocations;
    };
}

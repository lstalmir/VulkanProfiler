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

#include "profiler_memory_manager.h"
#include "profiler_layer_objects/VkDevice_object.h"

#include <assert.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace Profiler
{
    /***********************************************************************************\

    Function:
        DeviceProfilerMemoryManager

    Description:
        Constructor.

    \***********************************************************************************/
    DeviceProfilerMemoryManager::DeviceProfilerMemoryManager()
        : m_pDevice( nullptr )
        , m_Allocator( VK_NULL_HANDLE )
        , m_AllocationMutex()
        , m_BufferAllocations()
        , m_ImageAllocations()
    {
    }

    /***********************************************************************************\

    Function:
        Initialize

    Description:
        Initializes the memory manager and creates the allocator.

    \***********************************************************************************/
    VkResult DeviceProfilerMemoryManager::Initialize( VkDevice_Object* pDevice )
    {
        assert( !m_pDevice );
        m_pDevice = pDevice;

        // Create memory allocator.
        assert( !m_Allocator );
        VmaAllocatorCreateInfo allocatorCreateInfo = {};
        allocatorCreateInfo.physicalDevice = m_pDevice->pPhysicalDevice->Handle;
        allocatorCreateInfo.device = m_pDevice->Handle;
        allocatorCreateInfo.instance = m_pDevice->pInstance->Handle;

        uint32_t apiVersion = m_pDevice->pInstance->ApplicationInfo.apiVersion;
        if( apiVersion == 0 )
        {
            apiVersion = VK_API_VERSION_1_0;
        }
        allocatorCreateInfo.vulkanApiVersion = std::min( apiVersion, m_pDevice->pPhysicalDevice->Properties.apiVersion );

        VmaVulkanFunctions functions = {};
        functions.vkGetInstanceProcAddr = pDevice->pInstance->Callbacks.GetInstanceProcAddr;
        functions.vkGetDeviceProcAddr = pDevice->Callbacks.GetDeviceProcAddr;
        allocatorCreateInfo.pVulkanFunctions = &functions;

        VkResult result = vmaCreateAllocator( &allocatorCreateInfo, &m_Allocator );
        if( result != VK_SUCCESS )
        {
            m_pDevice = nullptr;
            m_Allocator = VK_NULL_HANDLE;
            return result;
        }

        assert( m_Allocator );
        return result;
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:
        Frees all allocations and destroys the allocator.

    \***********************************************************************************/
    void DeviceProfilerMemoryManager::Destroy()
    {
        // Free all device memory allocations.
        std::scoped_lock lk( m_AllocationMutex );

        for( auto [buffer, allocation] : m_BufferAllocations )
        {
            vmaDestroyBuffer( m_Allocator, buffer, allocation );
        }

        for( auto [image, allocation] : m_ImageAllocations )
        {
            vmaDestroyImage( m_Allocator, image, allocation );
        }

        // Destroy the allocator.
        vmaDestroyAllocator( m_Allocator );

        // Invalidate pointers.
        m_pDevice = nullptr;
        m_Allocator = VK_NULL_HANDLE;
        m_BufferAllocations.clear();
        m_ImageAllocations.clear();
    }

    /***********************************************************************************\

    Function:
        AllocateBuffer

    Description:
        Creates a buffer object and automaticall binds memory to it.

    \***********************************************************************************/
    VkResult DeviceProfilerMemoryManager::AllocateBuffer(
        const VkBufferCreateInfo& bufferCreateInfo,
        const VmaAllocationCreateInfo& allocationCreateInfo,
        VkBuffer* pBuffer,
        VmaAllocation* pAllocation,
        VmaAllocationInfo* pAllocationInfo )
    {
        std::scoped_lock lk( m_AllocationMutex );
        return vmaCreateBuffer( m_Allocator,
            &bufferCreateInfo,
            &allocationCreateInfo,
            pBuffer,
            pAllocation,
            pAllocationInfo );
    }

    /***********************************************************************************\

    Function:
        FreeBuffer

    Description:
        Destroys the buffer and frees its memory.

    \***********************************************************************************/
    void DeviceProfilerMemoryManager::FreeBuffer(
        VkBuffer buffer,
        VmaAllocation allocation )
    {
        std::scoped_lock lk( m_AllocationMutex );
        vmaDestroyBuffer( m_Allocator, buffer, allocation );
    }

    /***********************************************************************************\

    Function:
        AllocateImage

    Description:
        Creates an image object and automaticall binds memory to it.

    \***********************************************************************************/
    VkResult DeviceProfilerMemoryManager::AllocateImage(
        const VkImageCreateInfo& imageCreateInfo,
        const VmaAllocationCreateInfo& allocationCreateInfo,
        VkImage* pImage,
        VmaAllocation* pAllocation,
        VmaAllocationInfo* pAllocationInfo )
    {
        std::scoped_lock lk( m_AllocationMutex );
        return vmaCreateImage( m_Allocator,
            &imageCreateInfo,
            &allocationCreateInfo,
            pImage,
            pAllocation,
            pAllocationInfo );
    }

    /***********************************************************************************\

    Function:
        FreeImage

    Description:
        Destroys the buffer and frees its memory.

    \***********************************************************************************/
    void DeviceProfilerMemoryManager::FreeImage(
        VkImage image,
        VmaAllocation allocation )
    {
        std::scoped_lock lk( m_AllocationMutex );
        vmaDestroyImage( m_Allocator, image, allocation );
    }

    /***********************************************************************************\

    Function:
        Flush

    Description:
        Flushes the memory of the allocation to make it visible to the device.
        Has effect only if the memory type used for the allocation is not HOST_COHERENT.

    \***********************************************************************************/
    VkResult DeviceProfilerMemoryManager::Flush(
        VmaAllocation allocation,
        VkDeviceSize offset,
        VkDeviceSize size )
    {
        std::scoped_lock lk( m_AllocationMutex );
        return vmaFlushAllocation( m_Allocator, allocation, offset, size );
    }

    /***********************************************************************************\

    Function:
        Invalidate

    Description:
        Invalidates the memory of the allocation to make it visible to the host.
        Has effect only if the memory type used for the allocation is not HOST_COHERENT.

    \***********************************************************************************/
    VkResult DeviceProfilerMemoryManager::Invalidate(
        VmaAllocation allocation,
        VkDeviceSize offset,
        VkDeviceSize size )
    {
        std::scoped_lock lk( m_AllocationMutex );
        return vmaInvalidateAllocation( m_Allocator, allocation, offset, size );
    }
}

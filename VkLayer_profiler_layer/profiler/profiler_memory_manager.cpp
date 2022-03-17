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

#include "profiler_memory_manager.h"
#include "profiler_layer_objects/VkDevice_object.h"

#include <assert.h>

namespace Profiler
{
    DeviceProfilerMemoryManager::DeviceProfilerMemoryManager()
        : m_pDevice( nullptr )
        , m_MemoryAllocationMutex()
        , m_DefaultMemoryPoolSize( 16 * 1024 * 1024 )
        , m_DefaultMemoryBlockSize( 4 * 1024 )
        , m_DeviceMemoryProperties()
        , m_MemoryPools()
    {
    }

    VkResult DeviceProfilerMemoryManager::Initialize( VkDevice_Object* pDevice )
    {
        assert( !m_pDevice );
        m_pDevice = pDevice;

        // Get device memory properties.
        m_DeviceMemoryProperties = m_pDevice->pPhysicalDevice->MemoryProperties;

        return VK_SUCCESS;
    }

    void DeviceProfilerMemoryManager::Destroy()
    {
        // Free all device memory allocations.
        for( auto& memoryPool : m_MemoryPools )
        {
            m_pDevice->Callbacks.FreeMemory(
                m_pDevice->Handle,
                memoryPool.m_DeviceMemory,
                nullptr );
        }

        m_MemoryPools.clear();

        // Invalidate pointer to the device object.
        m_pDevice = nullptr;
    }

    VkResult DeviceProfilerMemoryManager::AllocateMemory(
        const VkMemoryRequirements& memoryRequirements,
        VkMemoryPropertyFlags requiredFlags,
        DeviceProfilerMemoryAllocation* pAllocation )
    {
        std::scoped_lock lk( m_MemoryAllocationMutex );

        VkResult result = VK_ERROR_FRAGMENTED_POOL;

        // Compute number of blocks that have to be allocated.
        const size_t requiredBlockCount = (memoryRequirements.size + m_DefaultMemoryBlockSize - 1) / m_DefaultMemoryBlockSize; 

        // Try to suballocate from an existing pool.
        for( auto& memoryPool : m_MemoryPools )
        {
            if( (memoryPool.m_FreeSize >= memoryRequirements.size) &&
                ((memoryPool.m_Flags & requiredFlags) == requiredFlags) &&
                ((memoryRequirements.memoryTypeBits & (1U << memoryPool.m_MemoryTypeIndex)) != 0) )
            {
                size_t firstFreeBlockIndex = 0;
                size_t freeBlockCount = 0;

                const size_t memoryPoolBlockCount = memoryPool.m_Allocations.size();
                for( size_t i = 0; i < memoryPoolBlockCount; ++i )
                {
                    // Skip allocated blocks.
                    while( (i < memoryPoolBlockCount) &&
                           (memoryPool.m_Allocations[i]) )
                        ++i;

                    // Find first free block with the required alignment.
                    while( (i < memoryPoolBlockCount) &&
                            !(memoryPool.m_Allocations[i]) &&
                            (((i * m_DefaultMemoryBlockSize) & (memoryRequirements.alignment - 1)) != 0) )
                        ++i;

                    firstFreeBlockIndex = i;
                    freeBlockCount = 0;

                    // Count free blocks.
                    while( (i < memoryPoolBlockCount) &&
                           (freeBlockCount < requiredBlockCount) &&
                           !(memoryPool.m_Allocations[i]) )
                        ++i, ++freeBlockCount;

                    if( freeBlockCount == requiredBlockCount )
                        break;
                }

                // Check if memory can be suballocated.
                if( freeBlockCount == requiredBlockCount )
                {
                    // Mark blocks as allocated.
                    const size_t lastBlockIndex = firstFreeBlockIndex + freeBlockCount;
                    for( size_t i = firstFreeBlockIndex; i < lastBlockIndex; ++i )
                    {
                        memoryPool.m_Allocations[ i ] = true;
                    }

                    // Create allocation struct.
                    pAllocation->m_pPool = &memoryPool;
                    pAllocation->m_Offset = firstFreeBlockIndex * m_DefaultMemoryBlockSize;
                    pAllocation->m_Size = freeBlockCount * m_DefaultMemoryBlockSize;
                    pAllocation->m_pMappedMemory = nullptr;

                    if( memoryPool.m_pMappedMemory != nullptr )
                    {
                        pAllocation->m_pMappedMemory =
                            reinterpret_cast<std::byte*>( memoryPool.m_pMappedMemory ) + pAllocation->m_Offset;
                    }

                    result = VK_SUCCESS;
                }
            }
        }

        if( result != VK_SUCCESS )
        {
            // Create new memory pool.
            DeviceProfilerMemoryPool* pMemoryPool = nullptr;
            result = AllocatePool(
                std::max( memoryRequirements.size, m_DefaultMemoryPoolSize ),
                memoryRequirements.memoryTypeBits,
                requiredFlags,
                &pMemoryPool );

            if( result == VK_SUCCESS )
            {
                // Mark blocks as allocated.
                for( size_t i = 0; i < requiredBlockCount; ++i )
                {
                    pMemoryPool->m_Allocations[ i ] = true;
                }

                // Create allocation struct.
                pAllocation->m_pPool = pMemoryPool;
                pAllocation->m_Offset = 0;
                pAllocation->m_Size = requiredBlockCount * m_DefaultMemoryBlockSize;

                if( pAllocation->m_pPool->m_pMappedMemory != nullptr )
                {
                    pAllocation->m_pMappedMemory =
                        reinterpret_cast<std::byte*>( pMemoryPool->m_pMappedMemory ) + pAllocation->m_Offset;
                }
            }
        }

        if( result == VK_SUCCESS )
        {
            pAllocation->m_pPool->m_FreeSize -= pAllocation->m_Size;
        }

        return result;
    }

    void DeviceProfilerMemoryManager::FreeMemory(
        DeviceProfilerMemoryAllocation* pAllocation )
    {
        std::scoped_lock lk( m_MemoryAllocationMutex );

        auto* pMemoryPool = pAllocation->m_pPool;

        const size_t firstBlockIndex = pAllocation->m_Offset / m_DefaultMemoryBlockSize;
        const size_t blockCount = pAllocation->m_Size / m_DefaultMemoryBlockSize;

        // Mark blocks as free.
        const size_t lastBlockIndex = firstBlockIndex + blockCount;
        for( size_t i = firstBlockIndex; i < lastBlockIndex; ++i )
        {
            pMemoryPool->m_Allocations[ i ] = false;
        }

        pMemoryPool->m_FreeSize += pAllocation->m_Size;
    }

    VkResult DeviceProfilerMemoryManager::AllocatePool(
        VkDeviceSize size,
        uint32_t requiredTypeBits,
        VkMemoryPropertyFlags requiredFlags,
        DeviceProfilerMemoryPool** ppPool )
    {
        VkResult result = VK_SUCCESS;
        try
        {
            const size_t blockCount = ((size + m_DefaultMemoryBlockSize - 1) / m_DefaultMemoryBlockSize);

            auto* pMemoryPool = &m_MemoryPools.emplace_back();
            pMemoryPool->m_Allocations.resize( blockCount );
            pMemoryPool->m_Size = blockCount * m_DefaultMemoryBlockSize;
            pMemoryPool->m_FreeSize = pMemoryPool->m_Size;

            // Find suitable memory type index.
            pMemoryPool->m_MemoryTypeIndex = UINT32_MAX;
            for( uint32_t i = 0; i < m_DeviceMemoryProperties.memoryTypeCount; ++i )
            {
                const auto& memoryProperties = m_DeviceMemoryProperties.memoryTypes[ i ];
                if( ((requiredTypeBits & (1U << i)) != 0) &&
                    ((memoryProperties.propertyFlags & requiredFlags) == requiredFlags) )
                {
                    pMemoryPool->m_MemoryTypeIndex = i;
                    pMemoryPool->m_Flags = memoryProperties.propertyFlags;
                    break;
                }
            }

            if( pMemoryPool->m_MemoryTypeIndex == UINT32_MAX )
            {
                // Required memory type not found.
                result = VK_ERROR_OUT_OF_DEVICE_MEMORY;
            }

            if( result == VK_SUCCESS )
            {
                // Allocate device memory.
                VkMemoryAllocateInfo memoryAllocateInfo = {};
                memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                memoryAllocateInfo.allocationSize = pMemoryPool->m_Size;
                memoryAllocateInfo.memoryTypeIndex = pMemoryPool->m_MemoryTypeIndex;

                result = m_pDevice->Callbacks.AllocateMemory(
                    m_pDevice->Handle,
                    &memoryAllocateInfo,
                    nullptr,
                    &pMemoryPool->m_DeviceMemory );
            }

            if( (result == VK_SUCCESS) &&
                ((pMemoryPool->m_Flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) )
            {
                // Map host-visible memory.
                result = m_pDevice->Callbacks.MapMemory(
                    m_pDevice->Handle,
                    pMemoryPool->m_DeviceMemory, 0,
                    pMemoryPool->m_Size, 0,
                    &pMemoryPool->m_pMappedMemory );
            }

            if( result == VK_SUCCESS )
            {
                *ppPool = pMemoryPool;
            }
        }
        catch( std::bad_alloc& )
        {
            result = VK_ERROR_OUT_OF_HOST_MEMORY;
        }
        catch( ... )
        {
            result = VK_ERROR_UNKNOWN;
        }

        return result;
    }
}

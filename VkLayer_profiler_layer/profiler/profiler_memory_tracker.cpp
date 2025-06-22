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

#include "profiler_memory_tracker.h"
#include "profiler_layer_objects/VkDevice_object.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        DeviceProfilerMemoryTracker

    Description:
        Constructor.

    \***********************************************************************************/
    DeviceProfilerMemoryTracker::DeviceProfilerMemoryTracker()
        : m_pDevice( nullptr )
        , m_AggregatedDataMutex()
        , m_TotalAllocationSize( 0 )
        , m_TotalAllocationCount( 0 )
        , m_Heaps()
        , m_Types()
        , m_Allocations()
        , m_Buffers()
        , m_Images()
    {
    }

    /***********************************************************************************\

    Function:
        Initialize

    Description:

    \***********************************************************************************/
    VkResult DeviceProfilerMemoryTracker::Initialize( VkDevice_Object* pDevice )
    {
        m_pDevice = pDevice;

        const VkPhysicalDeviceMemoryProperties& memoryProperties =
            m_pDevice->pPhysicalDevice->MemoryProperties;
        m_Heaps.resize( memoryProperties.memoryHeapCount );
        m_Types.resize( memoryProperties.memoryTypeCount );

        ResetMemoryData();
        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:

    \***********************************************************************************/
    void DeviceProfilerMemoryTracker::Destroy()
    {
        m_pDevice = nullptr;
        m_Heaps.clear();
        m_Types.clear();

        ResetMemoryData();
    }

    /***********************************************************************************\

    Function:
        RegisterAllocation

    Description:

    \***********************************************************************************/
    void DeviceProfilerMemoryTracker::RegisterAllocation( VkDeviceMemory memory, const VkMemoryAllocateInfo* pAllocateInfo )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        const VkPhysicalDeviceMemoryProperties& memoryProperties =
            m_pDevice->pPhysicalDevice->MemoryProperties;

        DeviceProfilerDeviceMemoryData data = {};
        data.m_Size = pAllocateInfo->allocationSize;
        data.m_TypeIndex = pAllocateInfo->memoryTypeIndex;
        data.m_HeapIndex = memoryProperties.memoryTypes[ data.m_TypeIndex ].heapIndex;

        m_Allocations.insert( memory, data );

        std::scoped_lock lk( m_AggregatedDataMutex );

        m_Heaps[ data.m_HeapIndex ].m_AllocationCount++;
        m_Heaps[ data.m_HeapIndex ].m_AllocationSize += data.m_Size;

        m_Types[ data.m_TypeIndex ].m_AllocationCount++;
        m_Types[ data.m_TypeIndex ].m_AllocationSize += data.m_Size;

        m_TotalAllocationCount++;
        m_TotalAllocationSize += data.m_Size;
    }

    /***********************************************************************************\

    Function:
        UnregisterAllocation

    Description:

    \***********************************************************************************/
    void DeviceProfilerMemoryTracker::UnregisterAllocation( VkDeviceMemory memory )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        DeviceProfilerDeviceMemoryData data;
        if( m_Allocations.find( memory, &data ) )
        {
            std::scoped_lock lk( m_AggregatedDataMutex );

            m_Heaps[ data.m_HeapIndex ].m_AllocationCount--;
            m_Heaps[ data.m_HeapIndex ].m_AllocationSize -= data.m_Size;

            m_Types[ data.m_TypeIndex ].m_AllocationCount--;
            m_Types[ data.m_TypeIndex ].m_AllocationSize -= data.m_Size;

            m_TotalAllocationCount--;
            m_TotalAllocationSize -= data.m_Size;
        }

        m_Allocations.remove( memory );
    }

    /***********************************************************************************\

    Function:
        RegisterBuffer

    Description:
        Register new buffer resource to track its memory usage.

    \***********************************************************************************/
    void DeviceProfilerMemoryTracker::RegisterBuffer( VkBuffer buffer, const VkBufferCreateInfo* pCreateInfo )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        DeviceProfilerBufferMemoryData data = {};
        data.m_BufferSize = pCreateInfo->size;
        data.m_BufferFlags = pCreateInfo->flags;
        data.m_BufferUsage = pCreateInfo->usage;

        m_pDevice->Callbacks.GetBufferMemoryRequirements(
            m_pDevice->Handle,
            buffer,
            &data.m_MemoryRequirements );

        m_Buffers.insert( buffer, data );
    }

    /***********************************************************************************\

    Function:
        UnregisterBuffer

    Description:

    \***********************************************************************************/
    void DeviceProfilerMemoryTracker::UnregisterBuffer( VkBuffer buffer )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );
        m_Buffers.remove( buffer );
    }

    /***********************************************************************************\

    Function:
        BindBufferMemory

    Description:

    \***********************************************************************************/
    void DeviceProfilerMemoryTracker::BindBufferMemory( VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize offset )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        std::shared_lock lk( m_Buffers );

        auto it = m_Buffers.unsafe_find( buffer );
        if( it != m_Buffers.end() )
        {
            DeviceProfilerBufferMemoryBindingData binding;
            binding.m_Memory = memory;
            binding.m_MemoryOffset = offset;
            binding.m_BufferOffset = 0;
            binding.m_Size = it->second.m_BufferSize;

            // Only one binding at a time is allowed using this API.
            it->second.m_MemoryBindings = binding;
        }
    }

    /***********************************************************************************\

    Function:
        BindBufferMemory

    Description:

    \***********************************************************************************/
    void DeviceProfilerMemoryTracker::BindBufferMemory( VkBuffer buffer, uint32_t bindCount, const VkSparseMemoryBind* pBinds )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        std::shared_lock lk( m_Buffers );

        auto it = m_Buffers.unsafe_find( buffer );
        if( it != m_Buffers.end() )
        {
            if( !std::holds_alternative<std::vector<DeviceProfilerBufferMemoryBindingData>>( it->second.m_MemoryBindings ) )
            {
                // Create a vector to hold multiple bindings.
                it->second.m_MemoryBindings = std::vector<DeviceProfilerBufferMemoryBindingData>();
            }

            std::vector<DeviceProfilerBufferMemoryBindingData>& bindings =
                std::get<std::vector<DeviceProfilerBufferMemoryBindingData>>( it->second.m_MemoryBindings );
            bindings.reserve( bindings.size() + bindCount );

            for( uint32_t i = 0; i < bindCount; ++i )
            {
                DeviceProfilerBufferMemoryBindingData& binding = bindings.emplace_back();
                binding.m_Memory = pBinds[i].memory;
                binding.m_MemoryOffset = pBinds[i].memoryOffset;
                binding.m_BufferOffset = pBinds[i].resourceOffset;
                binding.m_Size = pBinds[i].size;
            }
        }
    }

    /***********************************************************************************\

    Function:
        RegisterImage

    Description:
        Register new buffer resource to track its memory usage.

    \***********************************************************************************/
    void DeviceProfilerMemoryTracker::RegisterImage( VkImage image, const VkImageCreateInfo* pCreateInfo )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        DeviceProfilerImageMemoryData data = {};
        data.m_ImageExtent = pCreateInfo->extent;
        data.m_ImageFormat = pCreateInfo->format;
        data.m_ImageType = pCreateInfo->imageType;
        data.m_ImageUsage = pCreateInfo->usage;
        data.m_ImageTiling = pCreateInfo->tiling;
        data.m_Memory = VK_NULL_HANDLE;
        data.m_MemoryOffset = 0;

        m_pDevice->Callbacks.GetImageMemoryRequirements(
            m_pDevice->Handle,
            image,
            &data.m_MemoryRequirements );

        m_Images.insert( image, data );
    }

    /***********************************************************************************\

    Function:
        UnregisterImage

    Description:

    \***********************************************************************************/
    void DeviceProfilerMemoryTracker::UnregisterImage( VkImage image )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );
        m_Images.remove( image );
    }

    /***********************************************************************************\

    Function:
        BindImageMemory

    Description:

    \***********************************************************************************/
    void DeviceProfilerMemoryTracker::BindImageMemory( VkImage image, VkDeviceMemory memory, VkDeviceSize offset )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        std::shared_lock lk( m_Images );

        auto it = m_Images.unsafe_find( image );
        if( it != m_Images.end() )
        {
            it->second.m_Memory = memory;
            it->second.m_MemoryOffset = offset;
        }
    }

    /***********************************************************************************\

    Function:
        GetMemoryData

    Description:

    \***********************************************************************************/
    DeviceProfilerMemoryData DeviceProfilerMemoryTracker::GetMemoryData() const
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        DeviceProfilerMemoryData data;
        data.m_Allocations = m_Allocations.to_unordered_map();
        data.m_Buffers = m_Buffers.to_unordered_map();
        data.m_Images = m_Images.to_unordered_map();

        std::shared_lock lk( m_AggregatedDataMutex );
        data.m_TotalAllocationSize = m_TotalAllocationSize;
        data.m_TotalAllocationCount = m_TotalAllocationCount;
        data.m_Heaps = m_Heaps;
        data.m_Types = m_Types;

        return data;
    }

    /***********************************************************************************\

    Function:
        ResetMemoryData

    Description:

    \***********************************************************************************/
    void DeviceProfilerMemoryTracker::ResetMemoryData()
    {
        m_TotalAllocationSize = 0;
        m_TotalAllocationCount = 0;
        memset( m_Heaps.data(), 0, m_Heaps.size() * sizeof( DeviceProfilerMemoryHeapData ) );
        memset( m_Types.data(), 0, m_Types.size() * sizeof( DeviceProfilerMemoryTypeData ) );
        m_Allocations.clear();
        m_Buffers.clear();
        m_Images.clear();
    }
}

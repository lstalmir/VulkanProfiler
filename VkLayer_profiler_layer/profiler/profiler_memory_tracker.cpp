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
    void DeviceProfilerMemoryTracker::RegisterAllocation( VkObjectHandle<VkDeviceMemory> memory, const VkMemoryAllocateInfo* pAllocateInfo )
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
    void DeviceProfilerMemoryTracker::UnregisterAllocation( VkObjectHandle<VkDeviceMemory> memory )
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
    void DeviceProfilerMemoryTracker::RegisterBuffer( VkObjectHandle<VkBuffer> buffer, const VkBufferCreateInfo* pCreateInfo )
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
    void DeviceProfilerMemoryTracker::UnregisterBuffer( VkObjectHandle<VkBuffer> buffer )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );
        m_Buffers.remove( buffer );
    }

    /***********************************************************************************\

    Function:
        BindBufferMemory

    Description:

    \***********************************************************************************/
    void DeviceProfilerMemoryTracker::BindBufferMemory( VkObjectHandle<VkBuffer> buffer, VkObjectHandle<VkDeviceMemory> memory, VkDeviceSize offset )
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
        BindSparseBufferMemory

    Description:

    \***********************************************************************************/
    void DeviceProfilerMemoryTracker::BindSparseBufferMemory( VkObjectHandle<VkBuffer> buffer, VkDeviceSize bufferOffset, VkObjectHandle<VkDeviceMemory> memory, VkDeviceSize memoryOffset, VkDeviceSize size, VkSparseMemoryBindFlags flags )
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

            if( memory.m_Handle != VK_NULL_HANDLE )
            {
                // New memory binding of the buffer region.
                DeviceProfilerBufferMemoryBindingData& binding = bindings.emplace_back();
                binding.m_Memory = memory;
                binding.m_MemoryOffset = memoryOffset;
                binding.m_BufferOffset = bufferOffset;
                binding.m_Size = size;
            }
            else
            {
                // If memory is null, the resource region is unbound.
                // Remove all bindings that entirely cover the range and update the partially-unbound regions.
                const VkDeviceSize startUnbindOffset = bufferOffset;
                const VkDeviceSize endUnbindOffset = bufferOffset + size;

                for( auto binding = bindings.begin(); binding != bindings.end(); )
                {
                    const VkDeviceSize startBindingOffset = binding->m_BufferOffset;
                    const VkDeviceSize endBindingOffset = binding->m_BufferOffset + binding->m_Size;

                    if( ( startUnbindOffset <= startBindingOffset ) && ( endUnbindOffset >= endBindingOffset ) )
                    {
                        // Binding entirely covered by the unbound range, remove it.
                        binding = bindings.erase( binding );
                    }
                    else if( ( startUnbindOffset > startBindingOffset ) && ( endUnbindOffset < endBindingOffset ) )
                    {
                        // Buffer partially-unbound in the middle.
                        DeviceProfilerBufferMemoryBindingData newBinding = *binding;
                        newBinding.m_Size = startUnbindOffset - startBindingOffset;
                        binding->m_BufferOffset = endUnbindOffset;
                        binding->m_MemoryOffset += newBinding.m_Size + size;
                        binding->m_Size -= newBinding.m_Size + size;
                        binding = bindings.insert( binding, newBinding );
                        binding++;
                    }
                    else if( ( startUnbindOffset <= startBindingOffset ) && ( endUnbindOffset > startBindingOffset ) )
                    {
                        // Buffer partially-unbound at the start.
                        binding->m_BufferOffset = endUnbindOffset;
                        binding->m_MemoryOffset += endUnbindOffset - startBindingOffset;
                        binding->m_Size -= endUnbindOffset - startBindingOffset;
                        binding++;
                    }
                    else if( ( startUnbindOffset < endBindingOffset ) && ( endUnbindOffset >= endBindingOffset ) )
                    {
                        // Buffer partially-unbound at the end.
                        binding->m_Size -= endBindingOffset - startUnbindOffset;
                        binding++;
                    }
                    else
                    {
                        // Binding not affected.
                        binding++;
                    }
                }
            }
        }
    }

    /***********************************************************************************\

    Function:
        RegisterImage

    Description:
        Register new buffer resource to track its memory usage.

    \***********************************************************************************/
    void DeviceProfilerMemoryTracker::RegisterImage( VkObjectHandle<VkImage> image, const VkImageCreateInfo* pCreateInfo )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        DeviceProfilerImageMemoryData data = {};
        data.m_ImageExtent = pCreateInfo->extent;
        data.m_ImageFormat = pCreateInfo->format;
        data.m_ImageType = pCreateInfo->imageType;
        data.m_ImageFlags = pCreateInfo->flags;
        data.m_ImageUsage = pCreateInfo->usage;
        data.m_ImageTiling = pCreateInfo->tiling;
        data.m_ImageMipLevels = pCreateInfo->mipLevels;
        data.m_ImageArrayLayers = pCreateInfo->arrayLayers;

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
    void DeviceProfilerMemoryTracker::UnregisterImage( VkObjectHandle<VkImage> image )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );
        m_Images.remove( image );
    }

    /***********************************************************************************\

    Function:
        BindImageMemory

    Description:

    \***********************************************************************************/
    void DeviceProfilerMemoryTracker::BindImageMemory( VkObjectHandle<VkImage> image, VkObjectHandle<VkDeviceMemory> memory, VkDeviceSize offset )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        std::shared_lock lk( m_Images );

        auto it = m_Images.unsafe_find( image );
        if( it != m_Images.end() )
        {
            DeviceProfilerImageMemoryBindingData binding;
            binding.m_Memory = memory;
            binding.m_MemoryOffset = offset;
            binding.m_Size = it->second.m_MemoryRequirements.size;
            binding.m_ImageSubresource.aspectMask = GetFormatAllAspectFlags( it->second.m_ImageFormat );
            binding.m_ImageSubresource.mipLevel = VK_REMAINING_MIP_LEVELS;
            binding.m_ImageSubresource.arrayLayer = VK_REMAINING_ARRAY_LAYERS;
            binding.m_ImageOffset = {};
            binding.m_ImageExtent = it->second.m_ImageExtent;

            // Only one binding at a time is allowed using this API.
            it->second.m_MemoryBindings = binding;
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

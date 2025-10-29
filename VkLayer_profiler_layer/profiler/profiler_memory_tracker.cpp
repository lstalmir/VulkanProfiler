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
        , m_pfnGetPhysicalDeviceMemoryProperties2( nullptr )
        , m_MemoryBudgetEnabled( false )
        , m_AggregatedDataMutex()
        , m_TotalAllocationSize( 0 )
        , m_TotalAllocationCount( 0 )
        , m_Heaps()
        , m_Types()
        , m_Allocations()
        , m_Buffers()
        , m_Images()
        , m_AccelerationStructures()
        , m_Micromaps()
        , m_pfnGetBufferDeviceAddress( nullptr )
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

        // Resolve function pointers.
        if( m_pDevice->pInstance->Callbacks.GetPhysicalDeviceMemoryProperties2 )
        {
            m_pfnGetPhysicalDeviceMemoryProperties2 = m_pDevice->pInstance->Callbacks.GetPhysicalDeviceMemoryProperties2;
        }
        else if( m_pDevice->pInstance->Callbacks.GetPhysicalDeviceMemoryProperties2KHR &&
                 m_pDevice->pInstance->EnabledExtensions.count( VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME ) )
        {
            m_pfnGetPhysicalDeviceMemoryProperties2 = m_pDevice->pInstance->Callbacks.GetPhysicalDeviceMemoryProperties2KHR;
        }

        // Enable available extensions.
        if( m_pfnGetPhysicalDeviceMemoryProperties2 )
        {
            m_MemoryBudgetEnabled = m_pDevice->EnabledExtensions.count( VK_EXT_MEMORY_BUDGET_EXTENSION_NAME );
        }

        // Preallocate memory data structures.
        const VkPhysicalDeviceMemoryProperties& memoryProperties =
            m_pDevice->pPhysicalDevice->MemoryProperties;
        m_Heaps.resize( memoryProperties.memoryHeapCount );
        m_Types.resize( memoryProperties.memoryTypeCount );

        // Use core variant of the function if available.
        m_pfnGetBufferDeviceAddress = m_pDevice->Callbacks.GetBufferDeviceAddress;

        if( !m_pfnGetBufferDeviceAddress &&
            m_pDevice->EnabledExtensions.count( VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME ) )
        {
            // Use KHR variant if core is not available.
            m_pfnGetBufferDeviceAddress = m_pDevice->Callbacks.GetBufferDeviceAddressKHR;
        }

        if( !m_pfnGetBufferDeviceAddress &&
            m_pDevice->EnabledExtensions.count( VK_EXT_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME ) )
        {
            // Use EXT variant if KHR is not available.
            m_pfnGetBufferDeviceAddress = m_pDevice->Callbacks.GetBufferDeviceAddressEXT;
        }

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

        if( pCreateInfo->flags & VK_BUFFER_CREATE_SPARSE_BINDING_BIT )
        {
            // Get virtual address of the buffer if sparse binding is enabled.
            data.m_BufferAddress = GetBufferDeviceAddress( buffer, pCreateInfo->usage );
        }

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
            DeviceProfilerBufferMemoryData& bufferData = it->second;
            lk.unlock();

            // Memory bindings must be synchronized with the data collection thread.
            // Assuming that the application won't bind memory to the same buffer from multiple threads at the same time.
            std::shared_lock bindingLock( m_MemoryBindingMutex );

            DeviceProfilerBufferMemoryBindingData binding;
            binding.m_Memory = memory;
            binding.m_MemoryOffset = offset;
            binding.m_BufferOffset = 0;
            binding.m_Size = bufferData.m_BufferSize;

            if( !( bufferData.m_BufferFlags & VK_BUFFER_CREATE_SPARSE_BINDING_BIT ) )
            {
                // Non-sparse buffers have address assigned upon memory binding.
                bufferData.m_BufferAddress = GetBufferDeviceAddress( buffer, bufferData.m_BufferUsage );
            }

            // Only one binding at a time is allowed using this API.
            bufferData.m_MemoryBindings = binding;
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
            DeviceProfilerBufferMemoryData& bufferData = it->second;
            lk.unlock();

            // Memory bindings must be synchronized with the data collection thread.
            // Assuming that the application won't bind memory to the same buffer from multiple threads at the same time.
            std::shared_lock bindingLock( m_MemoryBindingMutex );

            if( !std::holds_alternative<std::vector<DeviceProfilerBufferMemoryBindingData>>( bufferData.m_MemoryBindings ) )
            {
                // Create a vector to hold multiple bindings.
                bufferData.m_MemoryBindings = std::vector<DeviceProfilerBufferMemoryBindingData>();
            }

            std::vector<DeviceProfilerBufferMemoryBindingData>& bindings =
                std::get<std::vector<DeviceProfilerBufferMemoryBindingData>>( bufferData.m_MemoryBindings );

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

        if( data.m_ImageFlags & VK_IMAGE_CREATE_SPARSE_BINDING_BIT )
        {
            uint32_t sparseMemoryRequirementCount = 0;
            m_pDevice->Callbacks.GetImageSparseMemoryRequirements(
                m_pDevice->Handle,
                image,
                &sparseMemoryRequirementCount,
                nullptr );

            data.m_SparseMemoryRequirements.resize( sparseMemoryRequirementCount );
            m_pDevice->Callbacks.GetImageSparseMemoryRequirements(
                m_pDevice->Handle,
                image,
                &sparseMemoryRequirementCount,
                data.m_SparseMemoryRequirements.data() );
        }

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
            DeviceProfilerImageMemoryData& imageData = it->second;
            lk.unlock();

            // Memory bindings must be synchronized with the data collection thread.
            // Assuming that the application won't bind memory to the same image from multiple threads at the same time.
            std::shared_lock bindingLock( m_MemoryBindingMutex );

            DeviceProfilerImageMemoryBindingData binding;
            binding.m_Type = DeviceProfilerImageMemoryBindingType::eOpaque;
            binding.m_Opaque.m_Memory = memory;
            binding.m_Opaque.m_MemoryOffset = offset;
            binding.m_Opaque.m_ImageOffset = 0;
            binding.m_Opaque.m_Size = imageData.m_MemoryRequirements.size;

            // Only one binding at a time is allowed using this API.
            imageData.m_MemoryBindings = binding;
        }
    }

    /***********************************************************************************\

    Function:
        BindSparseImageMemory

    Description:

    \***********************************************************************************/
    void DeviceProfilerMemoryTracker::BindSparseImageMemory( VkObjectHandle<VkImage> image, VkDeviceSize imageOffset, VkObjectHandle<VkDeviceMemory> memory, VkDeviceSize memoryOffset, VkDeviceSize size, VkSparseMemoryBindFlags flags )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        std::shared_lock lk( m_Images );

        auto it = m_Images.unsafe_find( image );
        if( it != m_Images.end() )
        {
            DeviceProfilerImageMemoryData& imageData = it->second;
            lk.unlock();

            // Memory bindings must be synchronized with the data collection thread.
            // Assuming that the application won't bind memory to the same image from multiple threads at the same time.
            std::shared_lock bindingLock( m_MemoryBindingMutex );

            if( !std::holds_alternative<std::vector<DeviceProfilerImageMemoryBindingData>>( imageData.m_MemoryBindings ) )
            {
                // Create a vector to hold multiple bindings.
                imageData.m_MemoryBindings = std::vector<DeviceProfilerImageMemoryBindingData>();
            }

            std::vector<DeviceProfilerImageMemoryBindingData>& bindings =
                std::get<std::vector<DeviceProfilerImageMemoryBindingData>>( imageData.m_MemoryBindings );

            if( memory.m_Handle != VK_NULL_HANDLE )
            {
                // New memory binding of the buffer region.
                DeviceProfilerImageMemoryBindingData& binding = bindings.emplace_back();
                binding.m_Type = DeviceProfilerImageMemoryBindingType::eOpaque;
                binding.m_Opaque.m_Memory = memory;
                binding.m_Opaque.m_MemoryOffset = memoryOffset;
                binding.m_Opaque.m_ImageOffset = imageOffset;
                binding.m_Opaque.m_Size = size;
            }
            else
            {
                // Memory unbinds not supported yet.
            }
        }
    }

    /***********************************************************************************\

    Function:
        BindSparseImageMemory

    Description:

    \***********************************************************************************/
    void DeviceProfilerMemoryTracker::BindSparseImageMemory( VkObjectHandle<VkImage> image, VkImageSubresource subresource, VkOffset3D offset, VkExtent3D extent, VkObjectHandle<VkDeviceMemory> memory, VkDeviceSize memoryOffset, VkSparseMemoryBindFlags flags )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        std::shared_lock lk( m_Images );

        auto it = m_Images.unsafe_find( image );
        if( it != m_Images.end() )
        {
            DeviceProfilerImageMemoryData& imageData = it->second;
            lk.unlock();

            // Memory bindings must be synchronized with the data collection thread.
            // Assuming that the application won't bind memory to the same image from multiple threads at the same time.
            std::shared_lock bindingLock( m_MemoryBindingMutex );

            if( !std::holds_alternative<std::vector<DeviceProfilerImageMemoryBindingData>>( imageData.m_MemoryBindings ) )
            {
                // Create a vector to hold multiple bindings.
                imageData.m_MemoryBindings = std::vector<DeviceProfilerImageMemoryBindingData>();
            }

            std::vector<DeviceProfilerImageMemoryBindingData>& bindings =
                std::get<std::vector<DeviceProfilerImageMemoryBindingData>>( imageData.m_MemoryBindings );

            // Remove bindings that are entirely overlapped by the new binding.
            for( auto it = bindings.begin(); it != bindings.end(); )
            {
                if( ( it->m_Type == DeviceProfilerImageMemoryBindingType::eBlock ) &&
                    ( it->m_Block.m_ImageSubresource.aspectMask == subresource.aspectMask ) &&
                    ( it->m_Block.m_ImageSubresource.arrayLayer == subresource.arrayLayer ) &&
                    ( it->m_Block.m_ImageSubresource.mipLevel == subresource.mipLevel ) &&
                    ( it->m_Block.m_ImageOffset.x >= offset.x ) &&
                    ( it->m_Block.m_ImageOffset.y >= offset.y ) &&
                    ( it->m_Block.m_ImageOffset.z >= offset.z ) &&
                    ( it->m_Block.m_ImageOffset.x + it->m_Block.m_ImageExtent.width ) <= ( offset.x + extent.width ) &&
                    ( it->m_Block.m_ImageOffset.y + it->m_Block.m_ImageExtent.height ) <= ( offset.y + extent.height ) &&
                    ( it->m_Block.m_ImageOffset.z + it->m_Block.m_ImageExtent.depth ) <= ( offset.z + extent.depth ) )
                {
                    // Binding entirely covered by the new one, remove it.
                    it = bindings.erase( it );
                }
                else
                {
                    it = std::next( it );
                }
            }

            if( memory.m_Handle != VK_NULL_HANDLE )
            {
                // New memory binding of the buffer region.
                DeviceProfilerImageMemoryBindingData& binding = bindings.emplace_back();
                binding.m_Type = DeviceProfilerImageMemoryBindingType::eBlock;
                binding.m_Block.m_Memory = memory;
                binding.m_Block.m_MemoryOffset = memoryOffset;
                binding.m_Block.m_ImageSubresource = subresource;
                binding.m_Block.m_ImageOffset = offset;
                binding.m_Block.m_ImageExtent = extent;
            }
        }
    }

    /***********************************************************************************\

    Function:
        RegisterAccelerationStructure

    Description:
        Register new acceleration structure resource to track its memory usage.

    \***********************************************************************************/
    void DeviceProfilerMemoryTracker::RegisterAccelerationStructure( VkObjectHandle<VkAccelerationStructureKHR> accelerationStructure, VkObjectHandle<VkBuffer> buffer, const VkAccelerationStructureCreateInfoKHR* pCreateInfo )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        DeviceProfilerAccelerationStructureMemoryData data = {};
        data.m_Type = pCreateInfo->type;
        data.m_Flags = pCreateInfo->createFlags;
        data.m_Buffer = buffer;
        data.m_Offset = pCreateInfo->offset;
        data.m_Size = pCreateInfo->size;

        m_AccelerationStructures.insert( accelerationStructure, data );
    }

    /***********************************************************************************\

    Function:
        UnregisterAccelerationStructure

    Description:

    \***********************************************************************************/
    void DeviceProfilerMemoryTracker::UnregisterAccelerationStructure( VkObjectHandle<VkAccelerationStructureKHR> accelerationStructure )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );
        m_AccelerationStructures.remove( accelerationStructure );
    }

    /***********************************************************************************\

    Function:
        RegisterMicromap

    Description:
        Register new opacity micromap resource to track its memory usage.

    \***********************************************************************************/
    void DeviceProfilerMemoryTracker::RegisterMicromap( VkObjectHandle<VkMicromapEXT> micromap, VkObjectHandle<VkBuffer> buffer, const VkMicromapCreateInfoEXT* pCreateInfo )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        DeviceProfilerMicromapMemoryData data = {};
        data.m_Type = pCreateInfo->type;
        data.m_Flags = pCreateInfo->createFlags;
        data.m_Buffer = buffer;
        data.m_Offset = pCreateInfo->offset;
        data.m_Size = pCreateInfo->size;

        m_Micromaps.insert( micromap, data );
    }

    /***********************************************************************************\

    Function:
        UnregisterMicromap

    Description:

    \***********************************************************************************/
    void DeviceProfilerMemoryTracker::UnregisterMicromap( VkObjectHandle<VkMicromapEXT> micromap )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );
        m_Micromaps.remove( micromap );
    }

    /***********************************************************************************\

    Function:
        GetBufferAtAddress

    Description:

    \***********************************************************************************/
    std::pair<VkBuffer, DeviceProfilerBufferMemoryData> DeviceProfilerMemoryTracker::GetBufferAtAddress( VkDeviceAddress address, VkBufferUsageFlags requiredUsage ) const
    {
        TipGuard tip( m_pDevice->TIP, __func__ );
        std::shared_lock lk( m_Buffers );

        for( const auto& [buffer, data] : m_Buffers )
        {
            if( ( data.m_BufferAddress != 0 ) &&
                ( ( data.m_BufferUsage & requiredUsage ) == requiredUsage ) &&
                ( address >= data.m_BufferAddress ) &&
                ( address < data.m_BufferAddress + data.m_BufferSize ) )
            {
                return { buffer, data };
            }
        }

        return { VK_NULL_HANDLE, {} };
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

        std::unique_lock bindingLock( m_MemoryBindingMutex );
        data.m_Allocations = m_Allocations.to_unordered_map();
        data.m_Buffers = m_Buffers.to_unordered_map();
        data.m_Images = m_Images.to_unordered_map();
        data.m_AccelerationStructures = m_AccelerationStructures.to_unordered_map();
        data.m_Micromaps = m_Micromaps.to_unordered_map();
        bindingLock.unlock();

        std::unique_lock dataLock( m_AggregatedDataMutex );
        data.m_TotalAllocationSize = m_TotalAllocationSize;
        data.m_TotalAllocationCount = m_TotalAllocationCount;
        data.m_Heaps = m_Heaps;
        data.m_Types = m_Types;
        dataLock.unlock();

        // Get available memory budget.
        VkPhysicalDeviceMemoryBudgetPropertiesEXT memoryBudget = {};
        memoryBudget.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;

        if( m_MemoryBudgetEnabled )
        {
            // Query current memory budget using VK_EXT_memory_budget extension.
            VkPhysicalDeviceMemoryProperties2 memoryProperties = {};
            memoryProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
            memoryProperties.pNext = &memoryBudget;

            m_pfnGetPhysicalDeviceMemoryProperties2( m_pDevice->pPhysicalDevice->Handle, &memoryProperties );
        }
        else
        {
            // Memory budget extension not available, use total heap sizes.
            const uint32_t memoryHeapCount = m_pDevice->pPhysicalDevice->MemoryProperties.memoryHeapCount;
            const VkMemoryHeap* pMemoryHeaps = m_pDevice->pPhysicalDevice->MemoryProperties.memoryHeaps;

            for( uint32_t i = 0; i < memoryHeapCount; i++ )
            {
                memoryBudget.heapBudget[i] = pMemoryHeaps[i].size;
            }
        }

        const size_t heapCount = m_Heaps.size();
        for( size_t i = 0; i < heapCount; ++i )
        {
            data.m_Heaps[i].m_BudgetSize = memoryBudget.heapBudget[i];
        }

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
        m_AccelerationStructures.clear();
        m_Micromaps.clear();
    }

    /***********************************************************************************\

    Function:
        GetBufferDeviceAddress

    Description:

    \***********************************************************************************/
    VkDeviceAddress DeviceProfilerMemoryTracker::GetBufferDeviceAddress( VkBuffer buffer, VkBufferUsageFlags usage ) const
    {
        // Check if extension is available.
        if( !m_pfnGetBufferDeviceAddress )
        {
            return 0;
        }

        // Save addresses of shader binding table buffers to read shader group handles.
        if( usage & VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR )
        {
            VkBufferDeviceAddressInfo deviceAddressInfo = {};
            deviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
            deviceAddressInfo.buffer = buffer;

            VkDeviceAddress bufferAddress = m_pfnGetBufferDeviceAddress(
                m_pDevice->Handle,
                &deviceAddressInfo );

            assert( bufferAddress );
            return bufferAddress;
        }

        return 0;
    }
}

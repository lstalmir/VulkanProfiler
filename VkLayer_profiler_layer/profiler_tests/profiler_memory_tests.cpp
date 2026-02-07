// Copyright (c) 2019-2021 Lukasz Stalmirski
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

#include "profiler_testing_common.h"

namespace Profiler
{
    class DeviceProfilerMemoryULT : public ProfilerBaseULT
    {
    protected:
        struct SparseBindingFeature : VulkanFeature
        {
            SparseBindingFeature()
                : VulkanFeature( "sparseBinding", std::string(), false )
            {
            }

            inline bool CheckSupport( const VkPhysicalDeviceFeatures2* pFeatures ) const override
            {
                return pFeatures->features.sparseBinding &&
                       pFeatures->features.sparseResidencyBuffer;
            }

            inline void Configure( VkPhysicalDeviceFeatures2* pFeatures ) override
            {
                pFeatures->features.sparseBinding = true;
                pFeatures->features.sparseResidencyBuffer = true;
            }
        } sparseBindingFeature;

        VkPhysicalDeviceMemoryProperties MemoryProperties = {};

        inline void SetUp() override
        {
            ProfilerBaseULT::SetUp();
            MemoryProperties = Vk->PhysicalDeviceMemoryProperties;
        }

        inline void SetUpVulkan( VulkanState::CreateInfo& createInfo ) override
        {
            ProfilerBaseULT::SetUpVulkan( createInfo );
            createInfo.DeviceFeatures.push_back( &sparseBindingFeature );
        }

        inline uint32_t FindMemoryType( VkMemoryPropertyFlags properties ) const
        {
            for( uint32_t i = 0; i < MemoryProperties.memoryTypeCount; ++i )
            {
                if( ( MemoryProperties.memoryTypes[i].propertyFlags & properties ) == properties )
                {
                    return i;
                }
            }
            return -1;
        }

        inline uint32_t FindMemoryType( VkMemoryPropertyFlags properties, uint32_t memoryTypeBits ) const
        {
            for( uint32_t i = 0; i < MemoryProperties.memoryTypeCount; ++i )
            {
                if( ( memoryTypeBits & ( 1U << i ) ) &&
                    ( MemoryProperties.memoryTypes[i].propertyFlags & properties ) == properties )
                {
                    return i;
                }
            }
            return -1;
        }

        inline uint32_t GetMemoryTypeHeapIndex( uint32_t memoryTypeIndex ) const
        {
            return MemoryProperties.memoryTypes[ memoryTypeIndex ].heapIndex;
        }

        inline VkResult CreateSparseBufferResource(
            VkDeviceSize bufferSize,
            VkBuffer* pBuffer,
            VkDeviceMemory* pMemory,
            VkMemoryRequirements* pMemoryRequirements,
            bool bindMemory = true )
        {
            VkResult result = VK_SUCCESS;
            VkQueue queue = Vk->GetQueue( VK_QUEUE_SPARSE_BINDING_BIT );

            // Create sparse buffer.
            if( result == VK_SUCCESS )
            {
                VkBufferCreateInfo bufferCreateInfo = {};
                bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                bufferCreateInfo.flags = VK_BUFFER_CREATE_SPARSE_BINDING_BIT | VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT;
                bufferCreateInfo.size = bufferSize;
                bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

                result = vkCreateBuffer( Vk->Device, &bufferCreateInfo, nullptr, pBuffer );
            }

            // Get memory requirements for the allocation and binding.
            if( result == VK_SUCCESS )
            {
                vkGetBufferMemoryRequirements( Vk->Device, *pBuffer, pMemoryRequirements );
            }

            // Allocate memory for sparse binding.
            if( result == VK_SUCCESS )
            {
                VkMemoryAllocateInfo allocateInfo = {};
                allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                allocateInfo.memoryTypeIndex = FindMemoryType( VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, pMemoryRequirements->memoryTypeBits );
                allocateInfo.allocationSize = pMemoryRequirements->size;

                result = vkAllocateMemory( Vk->Device, &allocateInfo, nullptr, pMemory );
            }

            if( bindMemory )
            {
                // Bind sparse buffer to memory.
                if( result == VK_SUCCESS )
                {
                    VkSparseMemoryBind sparseMemoryBind = {};
                    sparseMemoryBind.resourceOffset = 0;
                    sparseMemoryBind.size = pMemoryRequirements->size;
                    sparseMemoryBind.memory = *pMemory;
                    sparseMemoryBind.memoryOffset = 0;

                    result = BindSparseBufferResource( *pBuffer, sparseMemoryBind );
                }
            }

            return result;
        }

        inline VkResult BindSparseBufferResource( VkBuffer buffer, const VkSparseMemoryBind& bind )
        {
            return BindSparseBufferResource( buffer, std::vector<VkSparseMemoryBind>( 1, bind ) );
        }

        inline VkResult BindSparseBufferResource( VkBuffer buffer, const std::vector<VkSparseMemoryBind>& binds )
        {
            VkResult result = VK_SUCCESS;
            VkQueue queue = Vk->GetQueue( VK_QUEUE_SPARSE_BINDING_BIT );

            // Bind sparse buffer to memory.
            if( result == VK_SUCCESS )
            {
                VkSparseBufferMemoryBindInfo sparseBufferMemoryBindInfo = {};
                sparseBufferMemoryBindInfo.buffer = buffer;
                sparseBufferMemoryBindInfo.bindCount = binds.size();
                sparseBufferMemoryBindInfo.pBinds = binds.data();

                VkBindSparseInfo bindSparseInfo = {};
                bindSparseInfo.sType = VK_STRUCTURE_TYPE_BIND_SPARSE_INFO;
                bindSparseInfo.bufferBindCount = 1;
                bindSparseInfo.pBufferBinds = &sparseBufferMemoryBindInfo;

                result = vkQueueBindSparse( queue, 1, &bindSparseInfo, VK_NULL_HANDLE );
            }

            // Wait for the binding to complete.
            if( result == VK_SUCCESS )
            {
                result = vkQueueWaitIdle( queue );
            }

            return result;
        }
    };

    TEST_F( DeviceProfilerMemoryULT, AllocateMemory )
    {
        static constexpr size_t TEST_ALLOCATION_SIZE = 4096; // 4kB

        VkDeviceMemory deviceMemory = {};

        uint32_t deviceLocalMemoryTypeIndex = FindMemoryType( VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
        uint32_t deviceLocalMemoryHeapIndex = GetMemoryTypeHeapIndex( deviceLocalMemoryTypeIndex );

        { // Allocate memory
            VkMemoryAllocateInfo allocateInfo = {};
            allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocateInfo.memoryTypeIndex = deviceLocalMemoryTypeIndex;
            allocateInfo.allocationSize = TEST_ALLOCATION_SIZE;
            ASSERT_EQ( VK_SUCCESS, vkAllocateMemory( Vk->Device, &allocateInfo, nullptr, &deviceMemory ) );
        }

        { // Collect and post-process data
            Prof->FinishFrame();

            std::shared_ptr<DeviceProfilerFrameData> pData = Prof->GetData();
            const DeviceProfilerFrameData& data = *pData;
            ASSERT_EQ( MemoryProperties.memoryHeapCount, data.m_Memory.m_Heaps.size() );
            ASSERT_EQ( MemoryProperties.memoryTypeCount, data.m_Memory.m_Types.size() );

            // Verify that allocation has been registered by the profiling layer
            EXPECT_EQ( TEST_ALLOCATION_SIZE, data.m_Memory.m_TotalAllocationSize );
            EXPECT_EQ( 1, data.m_Memory.m_TotalAllocationCount );
            // Verify memory heap
            EXPECT_EQ( TEST_ALLOCATION_SIZE, data.m_Memory.m_Heaps[ deviceLocalMemoryHeapIndex ].m_AllocationSize );
            EXPECT_EQ( 1, data.m_Memory.m_Heaps[ deviceLocalMemoryHeapIndex ].m_AllocationCount );
            // Verify memory type
            EXPECT_EQ( TEST_ALLOCATION_SIZE, data.m_Memory.m_Types[ deviceLocalMemoryTypeIndex ].m_AllocationSize );
            EXPECT_EQ( 1, data.m_Memory.m_Types[ deviceLocalMemoryTypeIndex ].m_AllocationCount );

            // Other heaps should be 0
            for( uint32_t i = 0; i < MemoryProperties.memoryHeapCount; ++i )
            {
                if( i == deviceLocalMemoryHeapIndex )
                    continue;

                EXPECT_EQ( 0, data.m_Memory.m_Heaps[ i ].m_AllocationSize ) << "Unexpected allocations on heap " << i;
                EXPECT_EQ( 0, data.m_Memory.m_Heaps[ i ].m_AllocationCount ) << "Unexpected allocations on heap " << i;
            }

            // Other types should be 0
            for( uint32_t i = 0; i < MemoryProperties.memoryTypeCount; ++i )
            {
                if( i == deviceLocalMemoryTypeIndex )
                    continue;

                EXPECT_EQ( 0, data.m_Memory.m_Types[ i ].m_AllocationSize ) << "Unexpected allocations of type " << i;
                EXPECT_EQ( 0, data.m_Memory.m_Types[ i ].m_AllocationCount ) << "Unexpected allocations of type " << i;
            }
        }
    }

    TEST_F( DeviceProfilerMemoryULT, TryAllocateOutOfDeviceMemory )
    {
        static constexpr size_t TEST_ALLOCATION_SIZE = -1;

        VkDeviceMemory deviceMemory = {};

        uint32_t deviceLocalMemoryTypeIndex = FindMemoryType( VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
        uint32_t deviceLocalMemoryHeapIndex = GetMemoryTypeHeapIndex( deviceLocalMemoryTypeIndex );

        { // Allocate memory
            VkMemoryAllocateInfo allocateInfo = {};
            allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocateInfo.memoryTypeIndex = deviceLocalMemoryTypeIndex;
            allocateInfo.allocationSize = TEST_ALLOCATION_SIZE;
            ASSERT_EQ( VK_ERROR_OUT_OF_DEVICE_MEMORY, vkAllocateMemory( Vk->Device, &allocateInfo, nullptr, &deviceMemory ) );
        }

        { // Collect and post-process data
            Prof->FinishFrame();

            std::shared_ptr<DeviceProfilerFrameData> pData = Prof->GetData();
            const DeviceProfilerFrameData& data = *pData;
            ASSERT_EQ( MemoryProperties.memoryHeapCount, data.m_Memory.m_Heaps.size() );
            ASSERT_EQ( MemoryProperties.memoryTypeCount, data.m_Memory.m_Types.size() );

            // Verify that allocation has not been registered by the profiling layer
            EXPECT_EQ( 0, data.m_Memory.m_TotalAllocationSize );
            EXPECT_EQ( 0, data.m_Memory.m_TotalAllocationCount );

            // All heaps should be 0
            for( uint32_t i = 0; i < MemoryProperties.memoryHeapCount; ++i )
            {
                EXPECT_EQ( 0, data.m_Memory.m_Heaps[ i ].m_AllocationSize ) << "Unexpected allocations on heap " << i;
                EXPECT_EQ( 0, data.m_Memory.m_Heaps[ i ].m_AllocationCount ) << "Unexpected allocations on heap " << i;
            }

            // All types should be 0
            for( uint32_t i = 0; i < MemoryProperties.memoryTypeCount; ++i )
            {
                EXPECT_EQ( 0, data.m_Memory.m_Types[ i ].m_AllocationSize ) << "Unexpected allocations of type " << i;
                EXPECT_EQ( 0, data.m_Memory.m_Types[ i ].m_AllocationCount ) << "Unexpected allocations of type " << i;
            }
        }
    }

    TEST_F( DeviceProfilerMemoryULT, AllocateMultiple )
    {
        static constexpr size_t TEST_ALLOCATION_SIZE = 4096; // 4kB

        VkDeviceMemory deviceMemory[ 2 ] = {};

        uint32_t deviceLocalMemoryTypeIndex = FindMemoryType( VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
        uint32_t deviceLocalMemoryHeapIndex = GetMemoryTypeHeapIndex( deviceLocalMemoryTypeIndex );
        
        { // Allocate device local memory
            VkMemoryAllocateInfo allocateInfo = {};
            allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocateInfo.memoryTypeIndex = deviceLocalMemoryTypeIndex;
            allocateInfo.allocationSize = TEST_ALLOCATION_SIZE;
            ASSERT_EQ( VK_SUCCESS, vkAllocateMemory( Vk->Device, &allocateInfo, nullptr, &deviceMemory[ 0 ] ) );
            ASSERT_EQ( VK_SUCCESS, vkAllocateMemory( Vk->Device, &allocateInfo, nullptr, &deviceMemory[ 1 ] ) );
        }

        { // Collect and post-process data
            Prof->FinishFrame();

            std::shared_ptr<DeviceProfilerFrameData> pData = Prof->GetData();
            const DeviceProfilerFrameData& data = *pData;
            ASSERT_EQ( MemoryProperties.memoryHeapCount, data.m_Memory.m_Heaps.size() );
            ASSERT_EQ( MemoryProperties.memoryTypeCount, data.m_Memory.m_Types.size() );

            // Verify that allocation has not been registered by the profiling layer
            EXPECT_EQ( 2 * TEST_ALLOCATION_SIZE, data.m_Memory.m_TotalAllocationSize );
            EXPECT_EQ( 2, data.m_Memory.m_TotalAllocationCount );
            // Verify memory heap
            EXPECT_EQ( 2 * TEST_ALLOCATION_SIZE, data.m_Memory.m_Heaps[ deviceLocalMemoryHeapIndex ].m_AllocationSize );
            EXPECT_EQ( 2, data.m_Memory.m_Heaps[ deviceLocalMemoryHeapIndex ].m_AllocationCount );
            // Verify memory type
            EXPECT_EQ( 2 * TEST_ALLOCATION_SIZE, data.m_Memory.m_Types[ deviceLocalMemoryTypeIndex ].m_AllocationSize );
            EXPECT_EQ( 2, data.m_Memory.m_Types[ deviceLocalMemoryTypeIndex ].m_AllocationCount );

            // Other heaps should be 0
            for( uint32_t i = 0; i < MemoryProperties.memoryHeapCount; ++i )
            {
                if( i == deviceLocalMemoryHeapIndex )
                    continue;

                EXPECT_EQ( 0, data.m_Memory.m_Heaps[ i ].m_AllocationSize ) << "Unexpected allocations on heap " << i;
                EXPECT_EQ( 0, data.m_Memory.m_Heaps[ i ].m_AllocationCount ) << "Unexpected allocations on heap " << i;
            }

            // Other types should be 0
            for( uint32_t i = 0; i < MemoryProperties.memoryTypeCount; ++i )
            {
                if( i == deviceLocalMemoryTypeIndex )
                    continue;

                EXPECT_EQ( 0, data.m_Memory.m_Types[ i ].m_AllocationSize ) << "Unexpected allocations of type " << i;
                EXPECT_EQ( 0, data.m_Memory.m_Types[ i ].m_AllocationCount ) << "Unexpected allocations of type " << i;
            }
        }
    }

    TEST_F( DeviceProfilerMemoryULT, FreeMemory )
    {
        static constexpr size_t TEST_ALLOCATION_SIZE = 4096; // 4kB

        VkDeviceMemory deviceMemory[ 3 ] = {};

        uint32_t deviceLocalMemoryTypeIndex = FindMemoryType( VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
        uint32_t deviceLocalMemoryHeapIndex = GetMemoryTypeHeapIndex( deviceLocalMemoryTypeIndex );

        { // Allocate memory
            VkMemoryAllocateInfo allocateInfo = {};
            allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocateInfo.memoryTypeIndex = deviceLocalMemoryTypeIndex;
            allocateInfo.allocationSize = TEST_ALLOCATION_SIZE;
            ASSERT_EQ( VK_SUCCESS, vkAllocateMemory( Vk->Device, &allocateInfo, nullptr, &deviceMemory[ 0 ] ) );
            ASSERT_EQ( VK_SUCCESS, vkAllocateMemory( Vk->Device, &allocateInfo, nullptr, &deviceMemory[ 1 ] ) );
            ASSERT_EQ( VK_SUCCESS, vkAllocateMemory( Vk->Device, &allocateInfo, nullptr, &deviceMemory[ 2 ] ) );
        }

        { // Free memory
            vkFreeMemory( Vk->Device, deviceMemory[ 1 ], nullptr );
        }

        { // Collect and post-process data
            Prof->FinishFrame();

            std::shared_ptr<DeviceProfilerFrameData> pData = Prof->GetData();
            const DeviceProfilerFrameData& data = *pData;
            ASSERT_EQ( MemoryProperties.memoryHeapCount, data.m_Memory.m_Heaps.size() );
            ASSERT_EQ( MemoryProperties.memoryTypeCount, data.m_Memory.m_Types.size() );

            // Verify that allocation has been registered by the profiling layer
            EXPECT_EQ( 2 * TEST_ALLOCATION_SIZE, data.m_Memory.m_TotalAllocationSize );
            EXPECT_EQ( 2, data.m_Memory.m_TotalAllocationCount );
            // Verify memory heap
            EXPECT_EQ( 2 * TEST_ALLOCATION_SIZE, data.m_Memory.m_Heaps[ deviceLocalMemoryHeapIndex ].m_AllocationSize );
            EXPECT_EQ( 2, data.m_Memory.m_Heaps[ deviceLocalMemoryHeapIndex ].m_AllocationCount );
            // Verify memory type
            EXPECT_EQ( 2 * TEST_ALLOCATION_SIZE, data.m_Memory.m_Types[ deviceLocalMemoryTypeIndex ].m_AllocationSize );
            EXPECT_EQ( 2, data.m_Memory.m_Types[ deviceLocalMemoryTypeIndex ].m_AllocationCount );

            // Other heaps should be 0
            for( uint32_t i = 0; i < MemoryProperties.memoryHeapCount; ++i )
            {
                if( i == deviceLocalMemoryHeapIndex )
                    continue;

                EXPECT_EQ( 0, data.m_Memory.m_Heaps[ i ].m_AllocationSize ) << "Unexpected allocations on heap " << i;
                EXPECT_EQ( 0, data.m_Memory.m_Heaps[ i ].m_AllocationCount ) << "Unexpected allocations on heap " << i;
            }

            // Other types should be 0
            for( uint32_t i = 0; i < MemoryProperties.memoryTypeCount; ++i )
            {
                if( i == deviceLocalMemoryTypeIndex )
                    continue;

                EXPECT_EQ( 0, data.m_Memory.m_Types[ i ].m_AllocationSize ) << "Unexpected allocations of type " << i;
                EXPECT_EQ( 0, data.m_Memory.m_Types[ i ].m_AllocationCount ) << "Unexpected allocations of type " << i;
            }
        }
    }

    TEST_F( DeviceProfilerMemoryULT, MultipleFramePersistence )
    {
        static constexpr size_t TEST_ALLOCATION_SIZE = 4096; // 4kB

        VkDeviceMemory deviceMemory[ 3 ] = {};

        uint32_t deviceLocalMemoryTypeIndex = FindMemoryType( VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
        uint32_t deviceLocalMemoryHeapIndex = GetMemoryTypeHeapIndex( deviceLocalMemoryTypeIndex );

        { // Allocate memory
            VkMemoryAllocateInfo allocateInfo = {};
            allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocateInfo.memoryTypeIndex = deviceLocalMemoryTypeIndex;
            allocateInfo.allocationSize = TEST_ALLOCATION_SIZE;
            Prof->FinishFrame();
            ASSERT_EQ( VK_SUCCESS, vkAllocateMemory( Vk->Device, &allocateInfo, nullptr, &deviceMemory[ 0 ] ) );
            Prof->FinishFrame();
            ASSERT_EQ( VK_SUCCESS, vkAllocateMemory( Vk->Device, &allocateInfo, nullptr, &deviceMemory[ 1 ] ) );
            Prof->FinishFrame();
            ASSERT_EQ( VK_SUCCESS, vkAllocateMemory( Vk->Device, &allocateInfo, nullptr, &deviceMemory[ 2 ] ) );
            Prof->FinishFrame();
        }

        { // Collect and post-process data
            Prof->FinishFrame();

            std::shared_ptr<DeviceProfilerFrameData> pData = Prof->GetData();
            const DeviceProfilerFrameData& data = *pData;
            ASSERT_EQ( MemoryProperties.memoryHeapCount, data.m_Memory.m_Heaps.size() );
            ASSERT_EQ( MemoryProperties.memoryTypeCount, data.m_Memory.m_Types.size() );

            // Verify that allocation has been registered by the profiling layer
            EXPECT_EQ( 3 * TEST_ALLOCATION_SIZE, data.m_Memory.m_TotalAllocationSize );
            EXPECT_EQ( 3, data.m_Memory.m_TotalAllocationCount );
            // Verify memory heap
            EXPECT_EQ( 3 * TEST_ALLOCATION_SIZE, data.m_Memory.m_Heaps[ deviceLocalMemoryHeapIndex ].m_AllocationSize );
            EXPECT_EQ( 3, data.m_Memory.m_Heaps[ deviceLocalMemoryHeapIndex ].m_AllocationCount );
            // Verify memory type
            EXPECT_EQ( 3 * TEST_ALLOCATION_SIZE, data.m_Memory.m_Types[ deviceLocalMemoryTypeIndex ].m_AllocationSize );
            EXPECT_EQ( 3, data.m_Memory.m_Types[ deviceLocalMemoryTypeIndex ].m_AllocationCount );
            
            // Other heaps should be 0
            for( uint32_t i = 0; i < MemoryProperties.memoryHeapCount; ++i )
            {
                if( i == deviceLocalMemoryHeapIndex )
                    continue;

                EXPECT_EQ( 0, data.m_Memory.m_Heaps[ i ].m_AllocationSize ) << "Unexpected allocations on heap " << i;
                EXPECT_EQ( 0, data.m_Memory.m_Heaps[ i ].m_AllocationCount ) << "Unexpected allocations on heap " << i;
            }

            // Other types should be 0
            for( uint32_t i = 0; i < MemoryProperties.memoryTypeCount; ++i )
            {
                if( i == deviceLocalMemoryTypeIndex )
                    continue;

                EXPECT_EQ( 0, data.m_Memory.m_Types[ i ].m_AllocationSize ) << "Unexpected allocations of type " << i;
                EXPECT_EQ( 0, data.m_Memory.m_Types[ i ].m_AllocationCount ) << "Unexpected allocations of type " << i;
            }
        }
    }

    /***********************************************************************************\

    Test:
        SparseBinding_Simple

    Description:
        This test verifies that binding an entire sparse resource works correctly.

        The resource is a buffer of size 1024 bytes, created with sparse binding and
        sparse residency flags and transfer src and dst usages. It is bound to device-
        local memory at offset 0 with size equal to the buffer memory requirements size.
        The expected result is that there is a single memory binding reported, and its
        size is equal to the memory requirements size, with buffer and memory offsets
        both set to 0.

        Requires sparseBinding and sparseResidencyBuffer features to be supported.

    \***********************************************************************************/
    TEST_F( DeviceProfilerMemoryULT, SparseBinding_Simple )
    {
        SkipIfUnsupported( sparseBindingFeature );

        VkBuffer buffer = {};
        VkDeviceMemory deviceMemory = {};
        VkMemoryRequirements memoryRequirements = {};
        ASSERT_EQ( VK_SUCCESS, CreateSparseBufferResource( 1024, &buffer, &deviceMemory, &memoryRequirements ) );

        { // Collect and post-process data
            Prof->FinishFrame();

            std::shared_ptr<DeviceProfilerFrameData> pData = Prof->GetData();
            const DeviceProfilerBufferMemoryData& bufferData = pData->m_Memory.m_Buffers.at( Prof->GetObjectHandle( VkBufferHandle( buffer ) ) );

            ASSERT_EQ( 1, bufferData.GetMemoryBindingCount() );

            const DeviceProfilerBufferMemoryBindingData& bindingData = bufferData.GetMemoryBindings()[0];
            EXPECT_EQ( deviceMemory, bindingData.m_Memory );
            EXPECT_EQ( memoryRequirements.size, bindingData.m_Size );
            EXPECT_EQ( 0, bindingData.m_BufferOffset );
            EXPECT_EQ( 0, bindingData.m_MemoryOffset );
        }

        vkDestroyBuffer( Vk->Device, buffer, nullptr );
        vkFreeMemory( Vk->Device, deviceMemory, nullptr );
    }

    /***********************************************************************************\

    Test:
        SparseBinding_UnbindEntireResource

    Description:
        This test verifies that unbinding an entire sparse resource works correctly.

        First the entire resource is bound at offset 0 to memory at offset 0.
        Then, the entire resource is unbound.
        The expected result is that the number of reported memory bindings is 0.

        Requires sparseBinding and sparseResidencyBuffer features to be supported.

    \***********************************************************************************/
    TEST_F( DeviceProfilerMemoryULT, SparseBinding_UnbindEntireResource )
    {
        SkipIfUnsupported( sparseBindingFeature );

        VkBuffer buffer = {};
        VkDeviceMemory deviceMemory = {};
        VkMemoryRequirements memoryRequirements = {};
        ASSERT_EQ( VK_SUCCESS, CreateSparseBufferResource( 1024, &buffer, &deviceMemory, &memoryRequirements ) );

        { // Unbind the resource
            VkSparseMemoryBind sparseMemoryBind = {};
            sparseMemoryBind.resourceOffset = 0;
            sparseMemoryBind.size = memoryRequirements.size;
            sparseMemoryBind.memory = VK_NULL_HANDLE;
            sparseMemoryBind.memoryOffset = 0;

            ASSERT_EQ( VK_SUCCESS, BindSparseBufferResource( buffer, sparseMemoryBind ) );
        }

        { // Collect and post-process data
            Prof->FinishFrame();

            std::shared_ptr<DeviceProfilerFrameData> pData = Prof->GetData();
            const DeviceProfilerBufferMemoryData& bufferData = pData->m_Memory.m_Buffers.at( Prof->GetObjectHandle( VkBufferHandle( buffer ) ) );

            EXPECT_EQ( 0, bufferData.GetMemoryBindingCount() );
        }

        vkDestroyBuffer( Vk->Device, buffer, nullptr );
        vkFreeMemory( Vk->Device, deviceMemory, nullptr );
    }

    /***********************************************************************************\

    Test:
        SparseBinding_UnbindPartialResource_AtBegin

    Description:
        This test verifies that unbinding a sparse resource at the beginning of an
        existing memory binding works correctly.

        First the entire resource is bound at offset 0 to memory at offset 0.
        Then, first [alignment] bytes of the resource are unbound.
        The expected result is that the number of reported memory bindings remains 1,
        the size of the binding is reduced by [alignment], and the buffer offset and
        memory offset are both increased to [alignment].

        Requires sparseBinding and sparseResidencyBuffer features to be supported.

    \***********************************************************************************/
    TEST_F( DeviceProfilerMemoryULT, SparseBinding_UnbindPartialResource_AtBegin )
    {
        SkipIfUnsupported( sparseBindingFeature );

        VkBuffer buffer = {};
        VkDeviceMemory deviceMemory = {};
        VkMemoryRequirements memoryRequirements = {};
        ASSERT_EQ( VK_SUCCESS, CreateSparseBufferResource( 256 * 1024, &buffer, &deviceMemory, &memoryRequirements ) );

        { // Unbind the resource at the beginning
            VkSparseMemoryBind sparseMemoryBind = {};
            sparseMemoryBind.resourceOffset = 0;
            sparseMemoryBind.size = memoryRequirements.alignment;
            sparseMemoryBind.memory = VK_NULL_HANDLE;
            sparseMemoryBind.memoryOffset = 0;

            ASSERT_EQ( VK_SUCCESS, BindSparseBufferResource( buffer, sparseMemoryBind ) );
        }

        { // Collect and post-process data
            Prof->FinishFrame();

            std::shared_ptr<DeviceProfilerFrameData> pData = Prof->GetData();
            const DeviceProfilerBufferMemoryData& bufferData = pData->m_Memory.m_Buffers.at( Prof->GetObjectHandle( VkBufferHandle( buffer ) ) );

            ASSERT_EQ( 1, bufferData.GetMemoryBindingCount() );

            const DeviceProfilerBufferMemoryBindingData& bindingData = bufferData.GetMemoryBindings()[0];
            EXPECT_EQ( deviceMemory, bindingData.m_Memory );
            EXPECT_EQ( memoryRequirements.size - memoryRequirements.alignment, bindingData.m_Size );
            EXPECT_EQ( memoryRequirements.alignment, bindingData.m_BufferOffset );
            EXPECT_EQ( memoryRequirements.alignment, bindingData.m_MemoryOffset );
        }

        vkDestroyBuffer( Vk->Device, buffer, nullptr );
        vkFreeMemory( Vk->Device, deviceMemory, nullptr );
    }

    /***********************************************************************************\

    Test:
        SparseBinding_UnbindPartialResource_AtEnd

    Description:
        This test verifies that unbinding a sparse resource at the end of an existing
        memory binding works correctly.

        First the entire resource is bound at offset 0 to memory at offset 0.
        Then, last [alignment] bytes of the resource are unbound.
        The expected result is that the number of reported memory bindings remains 1,
        the size of the binding is reduced by [alignment], and the buffer offset and
        memory offset are left unchanged.

        Requires sparseBinding and sparseResidencyBuffer features to be supported.

    \***********************************************************************************/
    TEST_F( DeviceProfilerMemoryULT, SparseBinding_UnbindPartialResource_AtEnd )
    {
        SkipIfUnsupported( sparseBindingFeature );

        VkBuffer buffer = {};
        VkDeviceMemory deviceMemory = {};
        VkMemoryRequirements memoryRequirements = {};
        ASSERT_EQ( VK_SUCCESS, CreateSparseBufferResource( 256 * 1024, &buffer, &deviceMemory, &memoryRequirements ) );

        { // Unbind the resource at the end
            VkSparseMemoryBind sparseMemoryBind = {};
            sparseMemoryBind.resourceOffset = memoryRequirements.size - memoryRequirements.alignment;
            sparseMemoryBind.size = memoryRequirements.alignment;
            sparseMemoryBind.memory = VK_NULL_HANDLE;
            sparseMemoryBind.memoryOffset = 0;

            ASSERT_EQ( VK_SUCCESS, BindSparseBufferResource( buffer, sparseMemoryBind ) );
        }

        { // Collect and post-process data
            Prof->FinishFrame();

            std::shared_ptr<DeviceProfilerFrameData> pData = Prof->GetData();
            const DeviceProfilerBufferMemoryData& bufferData = pData->m_Memory.m_Buffers.at( Prof->GetObjectHandle( VkBufferHandle( buffer ) ) );

            ASSERT_EQ( 1, bufferData.GetMemoryBindingCount() );

            const DeviceProfilerBufferMemoryBindingData& bindingData = bufferData.GetMemoryBindings()[0];
            EXPECT_EQ( deviceMemory, bindingData.m_Memory );
            EXPECT_EQ( memoryRequirements.size - memoryRequirements.alignment, bindingData.m_Size );
            EXPECT_EQ( 0, bindingData.m_BufferOffset );
            EXPECT_EQ( 0, bindingData.m_MemoryOffset );
        }

        vkDestroyBuffer( Vk->Device, buffer, nullptr );
        vkFreeMemory( Vk->Device, deviceMemory, nullptr );
    }

    /***********************************************************************************\

    Test:
        SparseBinding_UnbindPartialResource_InMiddle

    Description:
        This test verifies that unbinding a sparse resource in the middle of an existing
        memory binding works correctly.

        First the entire resource is bound at offset 0 to memory at offset 0.
        Then, middle [alignment] bytes of the resource are unbound.
        The expected result is that the number of reported memory bindings is 2.
        The size of the first binding is equal to [alignment] and memory and buffer
        offsets are left unchanged. The second binding is reduced by [2 x alignment],
        and the buffer offset and memory offset are set to [2 x alignment].

        Requires sparseBinding and sparseResidencyBuffer features to be supported.

    \***********************************************************************************/
    TEST_F( DeviceProfilerMemoryULT, SparseBinding_UnbindPartialResource_InMiddle )
    {
        SkipIfUnsupported( sparseBindingFeature );

        VkBuffer buffer = {};
        VkDeviceMemory deviceMemory = {};
        VkMemoryRequirements memoryRequirements = {};
        ASSERT_EQ( VK_SUCCESS, CreateSparseBufferResource( 256 * 1024, &buffer, &deviceMemory, &memoryRequirements ) );
        ASSERT_GT( memoryRequirements.size, 2 * memoryRequirements.alignment );

        { // Unbind the resource in the middle
            VkSparseMemoryBind sparseMemoryBind = {};
            sparseMemoryBind.resourceOffset = memoryRequirements.alignment;
            sparseMemoryBind.size = memoryRequirements.alignment;
            sparseMemoryBind.memory = VK_NULL_HANDLE;
            sparseMemoryBind.memoryOffset = 0;

            ASSERT_EQ( VK_SUCCESS, BindSparseBufferResource( buffer, sparseMemoryBind ) );
        }

        { // Collect and post-process data
            Prof->FinishFrame();

            std::shared_ptr<DeviceProfilerFrameData> pData = Prof->GetData();
            const DeviceProfilerBufferMemoryData& bufferData = pData->m_Memory.m_Buffers.at( Prof->GetObjectHandle( VkBufferHandle( buffer ) ) );

            ASSERT_EQ( 2, bufferData.GetMemoryBindingCount() );

            const DeviceProfilerBufferMemoryBindingData& bindingData1 = bufferData.GetMemoryBindings()[0];
            EXPECT_EQ( deviceMemory, bindingData1.m_Memory );
            EXPECT_EQ( memoryRequirements.alignment, bindingData1.m_Size );
            EXPECT_EQ( 0, bindingData1.m_BufferOffset );
            EXPECT_EQ( 0, bindingData1.m_MemoryOffset );

            const DeviceProfilerBufferMemoryBindingData& bindingData2 = bufferData.GetMemoryBindings()[1];
            EXPECT_EQ( deviceMemory, bindingData2.m_Memory );
            EXPECT_EQ( memoryRequirements.size - ( 2 * memoryRequirements.alignment ), bindingData2.m_Size );
            EXPECT_EQ( 2 * memoryRequirements.alignment, bindingData2.m_BufferOffset );
            EXPECT_EQ( 2 * memoryRequirements.alignment, bindingData2.m_MemoryOffset );
        }

        vkDestroyBuffer( Vk->Device, buffer, nullptr );
        vkFreeMemory( Vk->Device, deviceMemory, nullptr );
    }

    /***********************************************************************************\

    Test:
        SparseBinding_UnbindPartialResource_Multiple

    Description:
        This test verifies that unbinding a sparse resource in the middle of multiple
        existing memory bindings works correctly.

        First the entire resource is bound at offset 0 to memory at offset 0, with
        a separate binding for each [alignment] bytes, resulting in [count] bindings.
        Then, the last [2 x alignment] bytes of the resource are unbound.
        The expected result is that the number of reported memory bindings decreases by 2,
        and the first [count - 2] bindings remain unchanged.

        Requires sparseBinding and sparseResidencyBuffer features to be supported.

    \***********************************************************************************/
    TEST_F( DeviceProfilerMemoryULT, SparseBinding_UnbindPartialResource_Multiple )
    {
        SkipIfUnsupported( sparseBindingFeature );

        VkBuffer buffer = {};
        VkDeviceMemory deviceMemory = {};
        VkMemoryRequirements memoryRequirements = {};
        ASSERT_EQ( VK_SUCCESS, CreateSparseBufferResource( 256 * 1024, &buffer, &deviceMemory, &memoryRequirements, false ) );
        ASSERT_GT( memoryRequirements.size, 2 * memoryRequirements.alignment );

        const uint32_t count = static_cast<uint32_t>( memoryRequirements.size / memoryRequirements.alignment );

        std::vector<VkSparseMemoryBind> sparseMemoryBinds;
        for( uint32_t i = 0; i < count; ++i )
        {
            VkSparseMemoryBind& sparseMemoryBind = sparseMemoryBinds.emplace_back();
            sparseMemoryBind.resourceOffset = memoryRequirements.alignment * i;
            sparseMemoryBind.size = memoryRequirements.alignment;
            sparseMemoryBind.memory = deviceMemory;
            sparseMemoryBind.memoryOffset = memoryRequirements.alignment * i;
        }

        ASSERT_EQ( VK_SUCCESS, BindSparseBufferResource( buffer, sparseMemoryBinds ) );

        { // Unbind the last 2 [alignment] bytes of the resource
            VkSparseMemoryBind sparseMemoryBind = {};
            sparseMemoryBind.resourceOffset = memoryRequirements.size - ( 2 * memoryRequirements.alignment );
            sparseMemoryBind.size = 2 * memoryRequirements.alignment;
            sparseMemoryBind.memory = VK_NULL_HANDLE;
            sparseMemoryBind.memoryOffset = 0;

            ASSERT_EQ( VK_SUCCESS, BindSparseBufferResource( buffer, sparseMemoryBind ) );
        }

        { // Collect and post-process data
            Prof->FinishFrame();

            std::shared_ptr<DeviceProfilerFrameData> pData = Prof->GetData();
            const DeviceProfilerBufferMemoryData& bufferData = pData->m_Memory.m_Buffers.at( Prof->GetObjectHandle( VkBufferHandle( buffer ) ) );

            ASSERT_EQ( count - 2, bufferData.GetMemoryBindingCount() );

            for( uint32_t i = 0; i < count - 2; ++i )
            {
                const DeviceProfilerBufferMemoryBindingData& bindingData = bufferData.GetMemoryBindings()[i];
                EXPECT_EQ( deviceMemory, bindingData.m_Memory );
                EXPECT_EQ( memoryRequirements.alignment, bindingData.m_Size );
                EXPECT_EQ( memoryRequirements.alignment * i, bindingData.m_BufferOffset );
                EXPECT_EQ( memoryRequirements.alignment * i, bindingData.m_MemoryOffset );
            }
        }

        vkDestroyBuffer( Vk->Device, buffer, nullptr );
        vkFreeMemory( Vk->Device, deviceMemory, nullptr );
    }
}

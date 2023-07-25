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
        VkPhysicalDeviceMemoryProperties MemoryProperties = {};

        inline void SetUp() override
        {
            ProfilerBaseULT::SetUp();
            MemoryProperties = Vk->PhysicalDeviceMemoryProperties;
        }

        inline uint32_t FindMemoryType( VkMemoryPropertyFlags properties ) const
        {
            for( uint32_t i = 0; i < MemoryProperties.memoryTypeCount; ++i )
            {
                if( (MemoryProperties.memoryTypes[ i ].propertyFlags & properties) == properties )
                    return i;
            }
            return -1;
        }

        inline uint32_t GetMemoryTypeHeapIndex( uint32_t memoryTypeIndex ) const
        {
            return MemoryProperties.memoryTypes[ memoryTypeIndex ].heapIndex;
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
            ASSERT_EQ( VK_SUCCESS, DT.AllocateMemory( Vk->Device, &allocateInfo, nullptr, &deviceMemory ) );
        }

        { // Collect and post-process data
            Prof->FinishFrame();

            const auto data = Prof->GetData();
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
            ASSERT_EQ( VK_ERROR_OUT_OF_DEVICE_MEMORY, DT.AllocateMemory( Vk->Device, &allocateInfo, nullptr, &deviceMemory ) );
        }

        { // Collect and post-process data
            Prof->FinishFrame();

            const auto data = Prof->GetData();
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
            ASSERT_EQ( VK_SUCCESS, DT.AllocateMemory( Vk->Device, &allocateInfo, nullptr, &deviceMemory[ 0 ] ) );
            ASSERT_EQ( VK_SUCCESS, DT.AllocateMemory( Vk->Device, &allocateInfo, nullptr, &deviceMemory[ 1 ] ) );
        }

        { // Collect and post-process data
            Prof->FinishFrame();

            const auto data = Prof->GetData();
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
            ASSERT_EQ( VK_SUCCESS, DT.AllocateMemory( Vk->Device, &allocateInfo, nullptr, &deviceMemory[ 0 ] ) );
            ASSERT_EQ( VK_SUCCESS, DT.AllocateMemory( Vk->Device, &allocateInfo, nullptr, &deviceMemory[ 1 ] ) );
            ASSERT_EQ( VK_SUCCESS, DT.AllocateMemory( Vk->Device, &allocateInfo, nullptr, &deviceMemory[ 2 ] ) );
        }

        { // Free memory
            DT.FreeMemory( Vk->Device, deviceMemory[ 1 ], nullptr );
        }

        { // Collect and post-process data
            Prof->FinishFrame();

            const auto data = Prof->GetData();
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
            ASSERT_EQ( VK_SUCCESS, DT.AllocateMemory( Vk->Device, &allocateInfo, nullptr, &deviceMemory[ 0 ] ) );
            Prof->FinishFrame();
            ASSERT_EQ( VK_SUCCESS, DT.AllocateMemory( Vk->Device, &allocateInfo, nullptr, &deviceMemory[ 1 ] ) );
            Prof->FinishFrame();
            ASSERT_EQ( VK_SUCCESS, DT.AllocateMemory( Vk->Device, &allocateInfo, nullptr, &deviceMemory[ 2 ] ) );
            Prof->FinishFrame();
        }

        { // Collect and post-process data
            Prof->FinishFrame();

            const auto data = Prof->GetData();
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
}

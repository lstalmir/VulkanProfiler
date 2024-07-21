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
#include "profiler_memory_manager.h"
#include "profiler_helpers.h"

#include <vulkan/vk_layer.h>

namespace Profiler
{
    class DeviceProfiler;

    class TimestampQueryPoolData
    {
    public:
        TimestampQueryPoolData( DeviceProfiler& profiler, uint32_t commandBufferCount, uint32_t queryCount );
        ~TimestampQueryPoolData();

        TimestampQueryPoolData( const TimestampQueryPoolData& ) = delete;
        TimestampQueryPoolData& operator=( const TimestampQueryPoolData& ) = delete;

        TimestampQueryPoolData( TimestampQueryPoolData&& ) = delete;
        TimestampQueryPoolData& operator=( TimestampQueryPoolData&& ) = delete;

        VkBuffer GetBufferHandle() const
        {
            return m_Buffer;
        }

        uint64_t GetQueryData( uint64_t index ) const
        {
            return reinterpret_cast<const uint64_t*>(m_AllocationInfo.pMappedData)[ index ];
        }

        void* GetCpuAllocation() const
        {
            return m_pCpuAllocation;
        }

        void SetCommandBufferFirstTimestampOffset( uint32_t commandBufferIndex, uint32_t offset )
        {
            m_pCommandBufferOffsets[ commandBufferIndex ] = offset;
        }

        uint32_t GetCommandBufferFirstTimestampOffset( uint32_t commandBufferIndex ) const
        {
            return m_pCommandBufferOffsets[ commandBufferIndex ];
        }

    private:
        DeviceProfiler&                m_Profiler;
        VkBuffer                       m_Buffer;
        VmaAllocation                  m_Allocation;
        VmaAllocationInfo              m_AllocationInfo;
        void*                          m_pCpuAllocation;
        uint32_t*                      m_pCommandBufferOffsets;
    };

    class TimestampQueryPool
    {
    public:
        TimestampQueryPool( DeviceProfiler& profiler, uint32_t queryCount );
        ~TimestampQueryPool();

        TimestampQueryPool( const TimestampQueryPool& ) = delete;

        VkQueryPool GetQueryPoolHandle() const { return m_QueryPool; }

        void ResolveQueryDataGpu( VkCommandBuffer commandBuffer, TimestampQueryPoolData& dst, uint32_t dstOffset, uint32_t queryCount );
        void ResolveQueryDataCpu( TimestampQueryPoolData& dst, uint32_t dstOffset, uint32_t queryCount );

    private:
        DeviceProfiler&                m_Profiler;
        VkQueryPool                    m_QueryPool;
    };
}

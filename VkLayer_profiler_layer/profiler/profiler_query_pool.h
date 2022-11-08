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

    class TimestampQueryPool
    {
    public:
        TimestampQueryPool( DeviceProfiler& profiler, uint32_t queryCount );
        ~TimestampQueryPool();

        TimestampQueryPool( const TimestampQueryPool& ) = delete;

        VkQueryPool GetQueryPoolHandle() const { return m_QueryPool; }
        VkBuffer GetResultsBufferHandle() const { return m_QueryResultsBuffer; }

        void ResolveQueryDataGpu( VkCommandBuffer, uint32_t );
        void ResolveQueryDataCpu( uint32_t );

        PROFILER_FORCE_INLINE uint64_t GetQueryData( uint32_t queryIndex ) const
        {
            return reinterpret_cast<const uint64_t*>( m_QueryResultsBufferAllocation.m_pMappedMemory )[ queryIndex ];
        }

    private:
        DeviceProfiler&                m_Profiler;

        VkQueryPool                    m_QueryPool;

        VkBuffer                       m_QueryResultsBuffer;
        DeviceProfilerMemoryAllocation m_QueryResultsBufferAllocation;
    };
}

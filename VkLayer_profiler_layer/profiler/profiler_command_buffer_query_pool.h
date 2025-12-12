// Copyright (c) 2022-2025 Lukasz Stalmirski
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
#include "profiler_performance_counters.h"

#include <vulkan/vk_layer.h>

#include <vector>

namespace Profiler
{
    class DeviceProfiler;
    class DeviceProfilerQueryDataBufferWriter;
    struct VkDevice_Object;

    /***********************************************************************************\

    Class:
        CommandBufferQueryPool

    Description:
        Wrapper for set of VkQueryPools used by a single command buffer.

    \***********************************************************************************/
    class CommandBufferQueryPool
    {
    public:
        CommandBufferQueryPool( DeviceProfiler&, uint32_t, VkCommandBufferLevel );
        ~CommandBufferQueryPool();

        CommandBufferQueryPool( const CommandBufferQueryPool& ) = delete;
        CommandBufferQueryPool& operator=( const CommandBufferQueryPool& ) = delete;

        CommandBufferQueryPool( CommandBufferQueryPool&& ) = delete;
        CommandBufferQueryPool& operator=( CommandBufferQueryPool&& ) = delete;

        uint32_t GetPerformanceQueryMetricsSetIndex() const;
        uint32_t GetPerformanceQueryStreamMarkerValue() const;
        uint64_t GetTimestampQueryCount() const;
        uint64_t GetRequiredBufferSize() const;

        void PreallocateQueries( VkCommandBuffer commandBuffer );

        void Reset( VkCommandBuffer commandBuffer );

        void BeginPerformanceQuery( VkCommandBuffer commandBuffer );
        void EndPerformanceQuery( VkCommandBuffer commandBuffer );

        void WriteQueryData( DeviceProfilerQueryDataBufferWriter& writer ) const;

        uint64_t WriteTimestamp( VkCommandBuffer commandBuffer, VkPipelineStageFlagBits stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );

    protected:
        DeviceProfiler&                  m_Profiler;
        VkDevice_Object&                 m_Device;

        VkCommandBufferLevel             m_CommandBufferLevel;
        uint32_t                         m_QueueFamilyIndex;

        DeviceProfilerPerformanceCounters* m_pPerformanceCounters;

        std::vector<VkQueryPool>         m_QueryPools;
        uint32_t                         m_QueryPoolSize;
        uint32_t                         m_CurrentQueryPoolIndex;
        uint32_t                         m_CurrentQueryIndex;
        uint64_t                         m_AbsQueryIndex;

        VkQueryPool                      m_PerformanceQueryPool;
        uint32_t                         m_PerformanceQueryMetricsSetIndex;

        uint32_t                         m_PerformanceQueryStreamMarkerValue;

        void AllocateQueryPool( VkCommandBuffer commandBuffer );
        void AllocatePerformanceQueryPool();
    };
}

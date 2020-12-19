// Copyright (c) 2020 Lukasz Stalmirski
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
#include "profiler_data.h"
#include "profiler_counters.h"
#include <vulkan/vk_layer.h>
#include <vector>
#include <unordered_set>
#include <memory>

namespace Profiler
{
    class DeviceProfiler;

    /***********************************************************************************\

    Class:
        TimestampQueryPool

    Description:
        Dynamically-resizable timestamp query pool.

    \***********************************************************************************/
    class TimestampQueryPool
    {
    public:
        TimestampQueryPool( DeviceProfiler& profiler, VkCommandBuffer commandBuffer );
        ~TimestampQueryPool();

        bool IsEmpty() const;

        void Reset();
        void Begin();

        void Allocate();
        bool AllocateIfAlmostFull( float threshold );

        void WriteTimestampQuery( VkPipelineStageFlagBits stage );

        std::vector<uint64_t> GetData();

    private:
        DeviceProfiler& m_Profiler;
        VkCommandBuffer m_CommandBuffer;

        std::vector<VkQueryPool> m_QueryPools;
        uint32_t m_QueryPoolSize;
        uint32_t m_CurrentQueryPoolIndex;
        uint32_t m_CurrentQueryIndex;
    };

    /***********************************************************************************\

    Class:
        ProfilerCommandBuffer

    Description:
        Wrapper for VkCommandBuffer object holding its current state.

    \***********************************************************************************/
    class ProfilerCommandBuffer
    {
    public:
        ProfilerCommandBuffer( DeviceProfiler&, VkCommandPool, VkCommandBuffer, VkCommandBufferLevel );
        ~ProfilerCommandBuffer();

        VkCommandPool GetCommandPool() const;
        VkCommandBuffer GetCommandBuffer() const;

        void Submit();

        void Begin( const VkCommandBufferBeginInfo* );
        void End();

        void Reset( VkCommandBufferResetFlags );

        void PreCommand( std::shared_ptr<Command> );
        void PostCommand( std::shared_ptr<Command> );

        std::shared_ptr<const CommandBufferData> GetData();

    protected:
        DeviceProfiler&       m_Profiler;

        const VkCommandPool   m_CommandPool;
        const VkCommandBuffer m_CommandBuffer;
        const VkCommandBufferLevel m_Level;
        
        std::shared_ptr<CommandBufferData> m_pData;
        bool                  m_Dirty;

        std::unique_ptr<TimestampQueryPool> m_pTimestampQueryPool;

        std::unique_ptr<CommandVisitor> m_pPreCommandQueryVisitor;
        std::unique_ptr<CommandVisitor> m_pPostCommandQueryVisitor;
        std::unique_ptr<CommandVisitor> m_pCommandTreeBuilder;

        std::unordered_set<VkCommandBuffer> m_SecondaryCommandBuffers;

        VkQueryPool     m_PerformanceQueryPoolINTEL;

        std::vector<std::shared_ptr<Command>> m_pCommands;
    };
}

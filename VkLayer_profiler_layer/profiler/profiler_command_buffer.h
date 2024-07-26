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

#pragma once
#include "profiler_command_buffer_query_pool.h"
#include "profiler_data.h"
#include "profiler_counters.h"
#include <vulkan/vk_layer.h>
#include <vector>
#include <unordered_set>

namespace Profiler
{
    class DeviceProfiler;
    class DeviceProfilerCommandPool;
    class DeviceProfilerQueryDataBufferWriter;
    class DeviceProfilerQueryDataBufferReader;

    /***********************************************************************************\

    Class:
        ProfilerCommandBuffer

    Description:
        Wrapper for VkCommandBuffer object holding its current state.

    \***********************************************************************************/
    class ProfilerCommandBuffer
    {
    public:
        ProfilerCommandBuffer( DeviceProfiler&, DeviceProfilerCommandPool&, VkCommandBuffer, VkCommandBufferLevel );
        ~ProfilerCommandBuffer();

        ProfilerCommandBuffer( const ProfilerCommandBuffer& ) = delete;

        DeviceProfilerCommandPool& GetCommandPool() const;
        VkCommandBuffer GetHandle() const;

        void Submit();

        void Begin( const VkCommandBufferBeginInfo* );
        void End();

        void Reset( VkCommandBufferResetFlags );

        void PreBeginRenderPass( const VkRenderPassBeginInfo*, VkSubpassContents );
        void PostBeginRenderPass( const VkRenderPassBeginInfo*, VkSubpassContents );
        void PreEndRenderPass();
        void PostEndRenderPass();

        void PreBeginRendering( const VkRenderingInfo* );
        void PostBeginRendering( const VkRenderingInfo* );
        void PreEndRendering();
        void PostEndRendering();

        void NextSubpass( VkSubpassContents );

        void BindPipeline( const DeviceProfilerPipeline& );
        void BindShaders( uint32_t, const VkShaderStageFlagBits*, const VkShaderEXT* );

        void PreCommand( const DeviceProfilerDrawcall& );
        void PostCommand( const DeviceProfilerDrawcall& );
        void ExecuteCommands( uint32_t, const VkCommandBuffer* );
        void PipelineBarrier(
            uint32_t, const VkMemoryBarrier*,
            uint32_t, const VkBufferMemoryBarrier*,
            uint32_t, const VkImageMemoryBarrier* );
        void PipelineBarrier( const VkDependencyInfo* );

        uint64_t GetRequiredQueryDataBufferSize() const;
        void WriteQueryData( DeviceProfilerQueryDataBufferWriter& ) const;

        const DeviceProfilerCommandBufferData& GetData( DeviceProfilerQueryDataBufferReader& );

    protected:
        DeviceProfiler&                     m_Profiler;
        DeviceProfilerCommandPool&          m_CommandPool;

        const VkCommandBuffer               m_CommandBuffer;
        const VkCommandBufferLevel          m_Level;

        bool                                m_ProfilingEnabled;
        bool                                m_Dirty;

        std::unordered_set<VkCommandBuffer> m_SecondaryCommandBuffers;

        CommandBufferQueryPool*             m_pQueryPool;

        DeviceProfilerDrawcallStats         m_Stats;
        DeviceProfilerCommandBufferData     m_Data;

        DeviceProfilerRenderPass*           m_pCurrentRenderPass;
        DeviceProfilerRenderPassData*       m_pCurrentRenderPassData;
        DeviceProfilerSubpassData*          m_pCurrentSubpassData;
        DeviceProfilerPipelineData*         m_pCurrentPipelineData;
        DeviceProfilerDrawcall*             m_pCurrentDrawcallData;

        uint32_t                            m_CurrentSubpassIndex;

        DeviceProfilerPipeline              m_GraphicsPipeline;
        DeviceProfilerPipeline              m_ComputePipeline;
        DeviceProfilerPipeline              m_RayTracingPipeline;

        void PreBeginRenderPassCommonProlog();
        void PreBeginRenderPassCommonEpilog();

        void EndSubpass();

        void SetupCommandBufferForStatCounting( const DeviceProfilerPipeline&, DeviceProfilerPipelineData** );
        void SetupCommandBufferForSecondaryBuffers();

        DeviceProfilerRenderPassType GetRenderPassTypeFromPipelineType( DeviceProfilerPipelineType ) const;

        void ResolveSubpassPipelineData( const DeviceProfilerQueryDataBufferReader&, DeviceProfilerSubpassData&, size_t );
        void ResolveSubpassSecondaryCommandBufferData( DeviceProfilerQueryDataBufferReader, DeviceProfilerSubpassData&, size_t, size_t, bool&, bool& );
    };
}

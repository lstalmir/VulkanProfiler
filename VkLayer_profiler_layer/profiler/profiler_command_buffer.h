#pragma once
#include "profiler_pipeline.h"
#include <vk_layer.h>
#include <vector>

namespace Profiler
{
    class Profiler;

    /***********************************************************************************\

    Structure:
        ProfilerRenderPass

    Description:
        Contains captured GPU timestamp data for single render pass.

    \***********************************************************************************/
    struct ProfilerRenderPass : ProfilerRangeStatsCollector<VkRenderPass, ProfilerPipeline>
    {
    };

    /***********************************************************************************\

    Structure:
        ProfilerCommandBufferData

    Description:
        Contains captured GPU timestamp data for single command buffer.

    \***********************************************************************************/
    struct ProfilerCommandBufferData : ProfilerRangeStatsCollector<VkCommandBuffer, ProfilerRenderPass>
    {
    };

    /***********************************************************************************\

    Structure:
        ProfilerSubmitData

    Description:
        Contains captured command buffers data for single submit.

    \***********************************************************************************/
    struct ProfilerSubmitData
    {
        std::vector<ProfilerCommandBufferData> m_CommandBuffers;
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
        ProfilerCommandBuffer( Profiler&, VkCommandBuffer );
        ~ProfilerCommandBuffer();

        VkCommandBuffer GetCommandBuffer() const;

        void Submit();

        void Begin( const VkCommandBufferBeginInfo* );
        void End();

        void BeginRenderPass( const VkRenderPassBeginInfo* );
        void EndRenderPass();

        void BindPipeline( ProfilerPipeline );

        void PreDraw();
        void PostDraw();
        void PreDrawIndirect();
        void PostDrawIndirect();
        void PreDispatch();
        void PostDispatch();
        void PreDispatchIndirect();
        void PostDispatchIndirect();
        void PreCopy();
        void PostCopy();
        void PreClear();
        void PostClear( uint32_t );
        void OnPipelineBarrier(
            uint32_t, const VkMemoryBarrier*,
            uint32_t, const VkBufferMemoryBarrier*,
            uint32_t, const VkImageMemoryBarrier* );

        ProfilerCommandBufferData GetData();

    protected:
        Profiler&       m_Profiler;

        VkCommandBuffer m_CommandBuffer;

        ProfilerPipeline m_CurrentPipeline;
        VkRenderPass    m_CurrentRenderPass;

        bool            m_Dirty;
        bool            m_RunningQuery;

        std::vector<VkQueryPool> m_QueryPools;
        uint32_t        m_QueryPoolSize;
        uint32_t        m_CurrentQueryPoolIndex;
        uint32_t        m_CurrentQueryIndex;

        ProfilerCommandBufferData m_Data;

        void Reset();

        void SendTimestampQuery( VkPipelineStageFlagBits );

        void SetupCommandBufferForStatCounting();

    };
}

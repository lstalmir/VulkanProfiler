#pragma once
#include "profiler_pipeline.h"
#include <vk_layer.h>
#include <vector>

namespace Profiler
{
    class Profiler;

    /***********************************************************************************\

    Class:
        ProfilerCommandBufferData

    Description:
        Contains captured GPU timestamp data for single command buffer.

    \***********************************************************************************/
    struct ProfilerCommandBufferData
    {
        VkCommandBuffer m_CommandBuffer;

        uint32_t m_DrawCount;
        uint32_t m_DispatchCount;
        uint32_t m_CopyCount;

        std::vector<std::pair<VkRenderPass, uint32_t>> m_RenderPassPipelineCount;
        std::vector<std::pair<ProfilerPipeline, uint32_t>> m_PipelineDrawCount;

        std::vector<uint64_t> m_CollectedTimestamps;
    };

    /***********************************************************************************\

    Class:
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

        void BeginRenderPass( VkRenderPass );
        void EndRenderPass();

        void BindPipeline( ProfilerPipeline );

        void Draw();
        void Dispatch();
        void Copy();

        ProfilerCommandBufferData GetData();

    protected:
        Profiler&       m_Profiler;

        VkCommandBuffer m_CommandBuffer;

        ProfilerPipeline m_CurrentPipeline;
        VkRenderPass    m_CurrentRenderPass;

        bool            m_Dirty;

        std::vector<VkQueryPool> m_QueryPools;
        uint32_t        m_QueryPoolSize;
        uint32_t        m_CurrentQueryPoolIndex;
        uint32_t        m_CurrentQueryIndex;

        ProfilerCommandBufferData m_Data;

        void Reset();
        void AllocateQueryPool();

        void SendTimestampQuery( VkPipelineStageFlagBits );

    };
}

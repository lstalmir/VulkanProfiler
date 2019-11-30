#pragma once
#include <vk_layer.h>
#include <vector>

namespace Profiler
{
    class Profiler;

    struct ProfilerCommandBufferData
    {
        uint32_t m_DrawCount;
        uint32_t m_DispatchCount;
        uint32_t m_CopyCount;

        std::vector<std::pair<VkRenderPass, uint32_t>> m_RenderPassPipelineCount;
        std::vector<std::pair<VkPipeline, uint32_t>> m_PipelineDrawCount;

        std::vector<uint64_t> m_CollectedTimestamps;
    };

    /***********************************************************************************\

    Function:
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

        void BindPipeline( VkPipeline );

        void Draw();
        void Dispatch();
        void Copy();

        ProfilerCommandBufferData GetData();

    protected:
        Profiler&       m_Profiler;

        VkCommandBuffer m_CommandBuffer;

        VkPipeline      m_CurrentPipeline;
        VkRenderPass    m_CurrentRenderPass;

        bool            m_Dirty;

        std::vector<VkQueryPool> m_QueryPools;
        uint32_t        m_QueryPoolSize;
        uint32_t        m_CurrentQueryPoolIndex;
        uint32_t        m_CurrentQueryIndex;

        ProfilerCommandBufferData m_Data;

        void Reset();

        void SendTimestampQuery( VkPipelineStageFlagBits );

    };
}

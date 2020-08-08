#pragma once
#include "profiler_pipeline.h"
#include <vulkan/vk_layer.h>
#include <vector>

namespace Profiler
{
    class DeviceProfiler;

    /***********************************************************************************\

    Structure:
        ProfilerSubpass

    Description:
        Contains captured GPU timestamp data for render pass subpass.

    \***********************************************************************************/
    struct ProfilerSubpass
    {
        uint32_t                                        m_Index;
        VkSubpassContents                               m_Contents;
        ProfilerRangeStats                              m_Stats;

        std::vector<struct ProfilerPipeline>            m_Pipelines;
        std::vector<struct ProfilerCommandBufferData>   m_SecondaryCommandBuffers;

        inline void Clear()
        {
            m_Stats.Clear();
            m_Pipelines.clear();
            m_SecondaryCommandBuffers.clear();
        }

        template<size_t Stat>
        inline void IncrementStat( uint32_t count = 1 )
        {
            m_Stats.IncrementStat<Stat>( count );

            // IncrementStat shouldn't be called within secondary command buffer subpass
            // (only vkCmdExecuteCommands is allowed according to spec)
            m_Pipelines.back().template IncrementStat<Stat>( count );
        }
    };

    /***********************************************************************************\

    Structure:
        ProfilerRenderPass

    Description:
        Contains captured GPU timestamp data for single render pass.

    \***********************************************************************************/
    struct ProfilerRenderPass : ProfilerRangeStatsCollector<VkRenderPass, ProfilerSubpass>
    {
        uint64_t m_BeginTicks;
        uint64_t m_EndTicks;

        inline void Clear()
        {
            ProfilerRangeStatsCollector::Clear();

            m_BeginTicks = 0;
            m_EndTicks = 0;
        }
    };

    /***********************************************************************************\

    Structure:
        ProfilerCommandBufferData

    Description:
        Contains captured GPU timestamp data for single command buffer.

    \***********************************************************************************/
    struct ProfilerCommandBufferData : ProfilerRangeStatsCollector<VkCommandBuffer, ProfilerRenderPass>
    {
        std::vector<char> m_PerformanceQueryReportINTEL;
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
        ProfilerCommandBuffer( DeviceProfiler&, VkCommandPool, VkCommandBuffer, VkCommandBufferLevel );
        ~ProfilerCommandBuffer();

        VkCommandPool GetCommandPool() const;
        VkCommandBuffer GetCommandBuffer() const;

        void Submit();

        void Begin( const VkCommandBufferBeginInfo* );
        void End();

        void PreBeginRenderPass( const VkRenderPassBeginInfo*, VkSubpassContents );
        void PostBeginRenderPass( const VkRenderPassBeginInfo*, VkSubpassContents );
        void PreEndRenderPass();
        void PostEndRenderPass();

        void EndSubpass();
        void NextSubpass( VkSubpassContents );

        void BindPipeline( VkPipelineBindPoint, ProfilerPipeline );

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
        void ExecuteCommands( uint32_t, const VkCommandBuffer* );
        void OnPipelineBarrier(
            uint32_t, const VkMemoryBarrier*,
            uint32_t, const VkBufferMemoryBarrier*,
            uint32_t, const VkImageMemoryBarrier* );

        ProfilerCommandBufferData GetData();

    protected:
        DeviceProfiler& m_Profiler;

        const VkCommandPool   m_CommandPool;
        const VkCommandBuffer m_CommandBuffer;
        const VkCommandBufferLevel m_Level;

        bool            m_Dirty;

        std::vector<VkQueryPool> m_QueryPools;
        uint32_t        m_QueryPoolSize;
        uint32_t        m_CurrentQueryPoolIndex;
        uint32_t        m_CurrentQueryIndex;

        VkQueryPool     m_PerformanceQueryPoolINTEL;

        ProfilerCommandBufferData m_Data;

        uint32_t        m_CurrentSubpassIndex;

        ProfilerPipeline m_CurrentGraphicsPipeline;
        ProfilerPipeline m_CurrentComputePipeline;

        void Reset();
        void AllocateQueryPool();

        void SendTimestampQuery( VkPipelineStageFlagBits );

        void SetupCommandBufferForStatCounting();
        void SetupCommandBufferForStatCounting( ProfilerPipeline );
        void SetupCommandBufferForSecondaryBuffers();

    };
}

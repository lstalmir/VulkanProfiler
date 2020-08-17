#pragma once
#include "profiler_data.h"
#include "profiler_counters.h"
#include <vulkan/vk_layer.h>
#include <vector>
#include <unordered_set>

namespace Profiler
{
    class DeviceProfiler;

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

        void PreBeginRenderPass( const VkRenderPassBeginInfo*, VkSubpassContents );
        void PostBeginRenderPass( const VkRenderPassBeginInfo*, VkSubpassContents );
        void PreEndRenderPass();
        void PostEndRenderPass();

        void NextSubpass( VkSubpassContents );

        void BindPipeline( const DeviceProfilerPipeline& );

        void PreDraw( const DeviceProfilerDrawcall& );
        void PostDraw();
        void DebugLabel( const char*, const float[ 4 ] );
        void ExecuteCommands( uint32_t, const VkCommandBuffer* );
        void PipelineBarrier(
            uint32_t, const VkMemoryBarrier*,
            uint32_t, const VkBufferMemoryBarrier*,
            uint32_t, const VkImageMemoryBarrier* );

        const DeviceProfilerCommandBufferData& GetData();

    protected:
        DeviceProfiler& m_Profiler;

        const VkCommandPool   m_CommandPool;
        const VkCommandBuffer m_CommandBuffer;
        const VkCommandBufferLevel m_Level;

        bool            m_Dirty;

        std::unordered_set<VkCommandBuffer> m_SecondaryCommandBuffers;

        std::vector<VkQueryPool> m_QueryPools;
        uint32_t        m_QueryPoolSize;
        uint32_t        m_CurrentQueryPoolIndex;
        uint32_t        m_CurrentQueryIndex;

        VkQueryPool     m_PerformanceQueryPoolINTEL;

        DeviceProfilerDrawcallStats m_Stats;
        DeviceProfilerCommandBufferData m_Data;

        DeviceProfilerRenderPass* m_pCurrentRenderPass;
        DeviceProfilerRenderPassData* m_pCurrentRenderPassData;

        uint32_t               m_CurrentSubpassIndex;

        DeviceProfilerPipeline m_GraphicsPipeline;
        DeviceProfilerPipeline m_ComputePipeline;

        uint64_t        m_ProfilerCpuOverheadNs;
        uint64_t        m_ProfilerGetDataCpuOverheadNs;

        void AllocateQueryPool();

        void EndSubpass();

        void IncrementStat( const DeviceProfilerDrawcall& );

        void SendTimestampQuery( VkPipelineStageFlagBits );

        void SetupCommandBufferForStatCounting( const DeviceProfilerPipeline& );
        void SetupCommandBufferForSecondaryBuffers();

        DeviceProfilerPipelineData& GetCurrentPipeline();

    };
}

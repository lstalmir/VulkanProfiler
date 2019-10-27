#pragma once
#include <vk_layer.h>

namespace Profiler
{
    /***********************************************************************************\

    Function:
        ProfilerCommandBuffer

    Description:
        Wrapper for VkCommandBuffer object holding its current state.

    \***********************************************************************************/
    struct ProfilerCommandBuffer
    {
        VkCommandBuffer m_CommandBuffer;

        VkFence         m_ExecutionFence;
        bool            m_IsExecuting;

        VkPipeline      m_CurrentPipeline;
        VkRenderPass    m_CurrentRenderPass;

        VkQueryPool     m_TimestampQueryPool;
        uint32_t        m_TimestampQueryPoolSize;
        uint32_t        m_TimestampQueryCount;

        bool            m_ReallocPool;
        uint32_t        m_TimestampQueryPoolRequiredSize;

        uint64_t*       m_TimestampQueryResults;
        uint32_t        m_TimestampQueryResultsSize;
        uint32_t        m_TimestampQueryResultsCount;
    };
}

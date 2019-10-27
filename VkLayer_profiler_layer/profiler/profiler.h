#pragma once
#include "profiler_allocator.h"
#include "profiler_counters.h"
#include "profiler_frame_stats.h"
#include <unordered_map>
#include <vk_layer.h>
#include <vk_layer_dispatch_table.h>

namespace Profiler
{
    /***********************************************************************************\

    Enum:
        ProfilerMode

    Description:
        Profiling frequency

    \***********************************************************************************/
    enum class ProfilerMode
    {
        ePerDrawcall,
        ePerPipeline,
        ePerRenderPass,
        ePerFrame
    };

    /***********************************************************************************\

    Class:
        Profiler

    Description:

    \***********************************************************************************/
    class Profiler
    {
    public:
        Profiler();

        VkResult Initialize( VkDevice, const VkLayerDispatchTable* );
        void Destroy();

        void PreDraw( VkCommandBuffer );
        void PostDraw( VkCommandBuffer );

        void PreRenderPass( VkCommandBuffer, const VkRenderPassBeginInfo* );
        void PostRenderPass( VkCommandBuffer );

        void PrePresent( VkQueue );
        void PostPresent( VkQueue );

        void OnAllocateMemory( VkDeviceMemory, const VkMemoryAllocateInfo* );
        void OnFreeMemory( VkDeviceMemory );

        FrameStats& GetCurrentFrameStats();
        const FrameStats& GetPreviousFrameStats() const;

    protected:
        VkDevice                m_Device;
        VkLayerDispatchTable    m_Callbacks;

        ProfilerMode            m_Mode;

        ProfilerAllocator       m_TimestampQueryAllocator;
        
        FrameStats*             m_pCurrentFrameStats;
        FrameStats*             m_pPreviousFrameStats;

        uint32_t                m_CurrentFrame;

        VkQueryPool             m_TimestampQueryPool;
        uint32_t                m_TimestampQueryPoolSize;
        uint32_t                m_CurrentTimestampQuery;
        CpuTimestampCounter*    m_pCpuTimestampQueryPool;
        uint32_t                m_CurrentCpuTimestampQuery;

        std::unordered_map<VkDeviceMemory, VkMemoryAllocateInfo> m_Allocations;
        std::atomic_uint64_t    m_AllocatedMemorySize;

        struct TimestampQueryPair
        {
            uint32_t BeginTimestampQueryIndex;
            uint32_t EndTimestampQueryIndex;
        };

        std::unordered_map<VkCommandBuffer, TimestampQueryPair> m_ProfiledCommandBuffers;

        TimestampQueryPair AllocateTimestampQueryPair();
    };
}

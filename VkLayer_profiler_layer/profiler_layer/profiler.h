#pragma once
#include "profiler_counters.h"
#include "profiler_frame_stats.h"
#include <unordered_map>
#include <vk_layer.h>
#include <vk_layer_dispatch_table.h>

namespace Profiler
{
    class ProfilerOverlay;

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

        void PrePresent( VkQueue );
        void PostPresent( VkQueue );

        void OnAllocateMemory( VkDeviceMemory allocatedMemory, const VkMemoryAllocateInfo* pAllocateInfo );
        void OnFreeMemory( VkDeviceMemory allocatedMemory );

        FrameStats& GetCurrentFrameStats();
        const FrameStats& GetPreviousFrameStats() const;

    protected:
        VkDevice                m_Device;
        VkLayerDispatchTable    m_Callbacks;
        
        FrameStats*             m_pCurrentFrameStats;
        FrameStats*             m_pPreviousFrameStats;

        uint32_t                m_CurrentFrame;

        VkQueryPool             m_TimestampQueryPool;
        uint32_t                m_TimestampQueryPoolSize;
        uint32_t                m_CurrentTimestampQuery;
        CpuTimestampCounter*    m_pCpuTimestampQueryPool;
        uint32_t                m_CurrentCpuTimestampQuery;

        std::atomic_uint64_t    m_AllocatedMemorySize;
    };
}

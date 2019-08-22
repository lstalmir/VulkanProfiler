#pragma once
#include "profiler_callbacks.h"
#include "profiler_counters.h"
#include "profiler_frame_stats.h"
#include "profiler_overlay.h"
#include "profiler_layer_objects/VkDevice_object.h"
#include <vulkan/vulkan.h>

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

        VkResult Initialize( VkDevice_Object* pDevice, ProfilerCallbacks callbacks );
        void Destroy();

        void PreDraw( VkCommandBuffer );
        void PostDraw( VkCommandBuffer );

        void PrePresent( VkQueue );
        void PostPresent( VkQueue );

        FrameStats& GetCurrentFrameStats();
        const FrameStats& GetPreviousFrameStats() const;

    protected:
        ProfilerCallbacks       m_Callbacks;
        
        ProfilerOverlay         m_Overlay;

        FrameStats*             m_pCurrentFrameStats;
        FrameStats*             m_pPreviousFrameStats;

        uint32_t                m_CurrentFrame;

        VkDevice_Object         m_Device;

        VkQueryPool             m_TimestampQueryPool;
        uint32_t                m_TimestampQueryPoolSize;
        uint32_t                m_CurrentTimestampQuery;
        CpuTimestampCounter*    m_pCpuTimestampQueryPool;
        uint32_t                m_CurrentCpuTimestampQuery;

    };
}

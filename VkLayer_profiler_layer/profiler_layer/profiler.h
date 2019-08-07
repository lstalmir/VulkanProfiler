#pragma once
#include "counters.h"
#include "frame_stats.h"
#include "profiler_layer_functions/VkDispatch.h"
#include <atomic>
#include <vulkan/vulkan.h>

namespace Profiler
{
    /***********************************************************************************\

    Structure:
        ProfilerCallbacks

    Description:
        VkDevice functions used by the Profiler instances.

    \***********************************************************************************/
    struct ProfilerCallbacks
    {
        PFN_vkCreateQueryPool   pfnCreateQueryPool;
        PFN_vkDestroyQueryPool  pfnDestroyQueryPool;
        PFN_vkCmdWriteTimestamp pfnCmdWriteTimestamp;

        ProfilerCallbacks()
            : pfnCreateQueryPool( nullptr )
            , pfnDestroyQueryPool( nullptr )
            , pfnCmdWriteTimestamp( nullptr )
        {
        }

        ProfilerCallbacks( VkDevice device, PFN_vkGetDeviceProcAddr pfnGetDeviceProcAddr )
            : pfnCreateQueryPool( GETDEVICEPROCADDR( device, vkCreateQueryPool ) )
            , pfnDestroyQueryPool( GETDEVICEPROCADDR( device, vkDestroyQueryPool ) )
            , pfnCmdWriteTimestamp( GETDEVICEPROCADDR( device, vkCmdWriteTimestamp ) )
        {
        }
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

        VkResult Initialize( VkDevice device, ProfilerCallbacks callbacks );
        void Destroy( VkDevice device );

        void PreDraw( VkCommandBuffer );
        void PostDraw( VkCommandBuffer );

        void PrePresent( VkQueue );
        void PostPresent( VkQueue );

        FrameStats& GetCurrentFrameStats();
        FrameStats GetPreviousFrameStats() const;

    protected:
        VkQueryPool             m_TimestampQueryPool;
        uint32_t                m_TimestampQueryPoolSize;
        uint32_t                m_CurrentTimestampQuery;
        CpuTimestampCounter*    m_pCpuTimestampQueryPool;
        uint32_t                m_CurrentCpuTimestampQuery;

        uint32_t                m_CurrentFrame;

        FrameStats              m_CurrentFrameStats;
        FrameStats              m_PreviousFrameStats;

        ProfilerCallbacks       m_Callbacks;
    };
}

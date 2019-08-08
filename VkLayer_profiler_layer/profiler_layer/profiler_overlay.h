#pragma once
#include "profiler_callbacks.h"
#include <vulkan/vulkan.h>

namespace Profiler
{
    class Profiler;

    /***********************************************************************************\

    Class:
        ProfilerOverlay

    Description:

    \***********************************************************************************/
    class ProfilerOverlay
    {
    public:
        ProfilerOverlay();

        VkResult Initialize( VkDevice device, Profiler* pProfiler, ProfilerCallbacks callbacks );
        void Destroy( VkDevice device );

        void DrawFramePerSecStats( VkQueue presentQueue );
        void DrawFrameStats( VkQueue presentQueue );

    protected:
        Profiler*           m_pProfiler;
        ProfilerCallbacks   m_Callbacks;

        VkCommandPool       m_CommandPool;
        VkCommandBuffer     m_CommandBuffer;
    };
}

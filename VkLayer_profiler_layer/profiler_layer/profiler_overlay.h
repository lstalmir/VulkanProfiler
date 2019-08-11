#pragma once
#include "profiler_callbacks.h"
#include "profiler_layer_objects/VkDevice_object.h"
#include "profiler_layer_objects/VkQueue_object.h"
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

        VkResult Initialize( VkDevice_Object* pDevice, Profiler* pProfiler, ProfilerCallbacks callbacks );
        void Destroy( VkDevice device );

        void DrawFramePerSecStats( VkQueue presentQueue );
        void DrawFrameStats( VkQueue presentQueue );

    protected:
        Profiler*           m_pProfiler;
        ProfilerCallbacks   m_Callbacks;

        VkQueue_Object      m_GraphicsQueue;
        VkCommandPool       m_CommandPool;
        VkCommandBuffer     m_CommandBuffer;

        VkRenderPass        m_DrawStatsRenderPass;
        VkShaderModule      m_DrawStatsShaderModule;
        VkPipelineLayout    m_DrawStatsPipelineLayout;
        VkPipeline          m_DrawStatsPipeline;
    };
}

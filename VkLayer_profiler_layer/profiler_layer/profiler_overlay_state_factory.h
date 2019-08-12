#pragma once
#include "profiler_callbacks.h"
#include <vulkan/vulkan.h>

namespace Profiler
{
    class ProfilerOverlayStateFactory
    {
    public:
        ProfilerOverlayStateFactory( VkDevice device, ProfilerCallbacks callbacks );

        VkResult CreateDrawStatsRenderPass( VkRenderPass* pRenderPass );

        VkResult CreateDrawStatsPipelineLayout( VkPipelineLayout* pPipelineLayout );

        VkResult CreateDrawStatsShaderModule( VkShaderModule* pShaderModule );

        VkResult CreateDrawStatsPipeline(
            VkRenderPass renderPass,
            VkPipelineLayout layout,
            VkShaderModule shaderModule,
            VkPipeline* pPipeline );

    private:
        VkDevice m_Device;
        ProfilerCallbacks m_Callbacks;
    };
}

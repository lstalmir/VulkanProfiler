#pragma once
#include "profiler_callbacks.h"
#include "profiler_shaders.h"
#include <vulkan/vulkan.h>

namespace Profiler
{
    class ProfilerOverlayStateFactory
    {
    public:
        ProfilerOverlayStateFactory( VkDevice device, ProfilerCallbacks callbacks );

        VkResult CreateDrawStatsRenderPass( VkRenderPass* pRenderPass );

        VkResult CreateDrawStatsPipelineLayout( VkPipelineLayout* pPipelineLayout );

        VkResult CreateDrawStatsShaderModule( VkShaderModule* pShaderModule, ProfilerShaderType shader );

        VkResult CreateDrawStatsPipeline(
            VkRenderPass renderPass,
            VkPipelineLayout layout,
            const std::unordered_map<VkShaderStageFlagBits, VkShaderModule>& shaderModules,
            VkPipeline* pPipeline );

    private:
        VkDevice m_Device;
        ProfilerCallbacks m_Callbacks;
    };
}

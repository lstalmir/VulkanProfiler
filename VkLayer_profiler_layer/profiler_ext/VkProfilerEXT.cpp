#include "VkProfilerEXT.h"
#include "profiler_layer_functions/VkDevice_functions.h"

using namespace Profiler;

VKAPI_ATTR VkResult VKAPI_CALL vkSetProfilerModeEXT(
    VkDevice device,
    VkProfilerModeEXT mode )
{
    ProfilerMode profilerMode;
    switch( mode )
    {
    case VK_PROFILER_MODE_PER_DRAWCALL_EXT:
        profilerMode = ProfilerMode::ePerDrawcall; break;

    case VK_PROFILER_MODE_PER_PIPELINE_EXT:
        profilerMode = ProfilerMode::ePerPipeline; break;

    case VK_PROFILER_MODE_PER_RENDER_PASS_EXT:
        profilerMode = ProfilerMode::ePerRenderPass; break;

    case VK_PROFILER_MODE_PER_FRAME_EXT:
        profilerMode = ProfilerMode::ePerFrame; break;

    default:
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }

    return VkDevice_Functions::DeviceDispatch.Get( device )
        .Profiler.SetMode( profilerMode );
}

VKAPI_ATTR void VKAPI_CALL vkCmdDrawProfilerOverlayEXT(
    VkCommandBuffer commandBuffer )
{

}

#include "VkProfilerEXT.h"
#include "profiler_layer_functions/VkDevice_functions.h"

using namespace Profiler;

VKAPI_ATTR VkResult VKAPI_CALL vkSetProfilerModeEXT(
    VkDevice device,
    VkProfilerModeEXT mode )
{
    return VkDevice_Functions::DeviceDispatch.Get( device )
        .Profiler.SetMode( mode );
}

VKAPI_ATTR void VKAPI_CALL vkCmdDrawProfilerOverlayEXT(
    VkCommandBuffer commandBuffer )
{

}

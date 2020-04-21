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

VKAPI_ATTR VkResult VKAPI_CALL vkGetProfilerDataEXT(
    VkDevice device,
    VkProfilerDataEXT* pData )
{
    auto& dd = VkDevice_Functions::DeviceDispatch.Get( device );

    // Get latest data from profiler
    ProfilerAggregatedData data = dd.Profiler.GetData();

    // TODO: Translate internal data to VkProfilerDataEXT

    return VK_INCOMPLETE;
}

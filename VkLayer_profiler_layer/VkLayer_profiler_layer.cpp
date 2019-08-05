#include "VkLayer_profiler_layer.h"
#include "profiler_layer/VkInstance_functions.h"

using LayerInstanceFunctions = Profiler::VkInstance_Functions;

////////////////////////////////////////////////////////////////////////////////////////
VK_LAYER_EXPORT PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(
    VkInstance                          instance,
    const char*                         name )
{
    return LayerInstanceFunctions::GetInstanceProcAddr( instance, name );
}

#include "VkLayer_profiler_layer.h"
#include "VkInstance_functions.h"
#include "VkDevice_functions.h"

/***************************************************************************************\

Function:
    vkGetInstanceProcAddr

Description:
    Entrypoint to the VkInstance

\***************************************************************************************/
VK_LAYER_EXPORT PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(
    VkInstance instance,
    const char* name )
{
    return Profiler::VkInstance_Functions::GetInstanceProcAddr( instance, name );
}

/***************************************************************************************\

Function:
    vkGetDeviceProcAddr

Description:
    Entrypoint to the VkDevice

\***************************************************************************************/
VK_LAYER_EXPORT PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(
    VkDevice device,
    const char* name )
{
    return Profiler::VkDevice_Functions::GetDeviceProcAddr( device, name );
}

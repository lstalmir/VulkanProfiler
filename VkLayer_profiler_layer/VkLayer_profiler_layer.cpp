#define VK_NO_PROTOTYPES
#include "profiler_layer_functions/core/VkInstance_functions.h"
#include "profiler_layer_functions/core/VkDevice_functions.h"
#include <vulkan/vk_layer.h>

#undef VK_LAYER_EXPORT
#if defined( _MSC_VER )
#   define VK_LAYER_EXPORT extern "C" __declspec(dllexport)
#elif defined( __GNUC__ ) && __GNUC__ >= 4
#   define VK_LAYER_EXPORT extern "C" __attribute__((visibility("default")))
#elif defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590)
#   define VK_LAYER_EXPORT extern "C" __attribute__((visibility("default")))
#else
#   define VK_LAYER_EXPORT extern "C"
#endif

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

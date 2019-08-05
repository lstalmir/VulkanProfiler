#pragma once
#include <vulkan/vk_layer.h>

#undef EXTERNC
#ifdef __cplusplus
#   define EXTERNC extern "C"
#else
#   define EXTERNC
#endif

#undef VK_LAYER_EXPORT
#if defined( _MSC_VER )
#   define VK_LAYER_EXPORT EXTERNC __declspec(dllexport)
#elif defined( __GNUC__ ) && __GNUC__ >= 4
#   define VK_LAYER_EXPORT EXTERNC __attribute__((visibility("default")))
#elif defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590)
#   define VK_LAYER_EXPORT EXTERNC __attribute__((visibility("default")))
#else
#   define VK_LAYER_EXPORT EXTERNC
#endif

/***************************************************************************************\
 Entrypoints to the layer
\***************************************************************************************/

VK_LAYER_EXPORT PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(
    VkInstance instance,
    const char* name );

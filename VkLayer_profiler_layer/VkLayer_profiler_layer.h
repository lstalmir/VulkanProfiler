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

////////////////////////////////////////////////////////////////////////////////////////
// Initialization and deinitialization of the layer

VK_LAYER_EXPORT VkResult VKAPI_CALL vkCreateInstance(
    const VkInstanceCreateInfo*         pCreateInfo,
    const VkAllocationCallbacks*        pAllocator,
    VkInstance*                         pInstance );

VK_LAYER_EXPORT void VKAPI_CALL vkDestroyInstance(
    VkInstance                          instance,
    const VkAllocationCallbacks*        pAllocator );

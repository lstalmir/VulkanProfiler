#pragma once
#include "Dispatch.h"
#include "VkCommandBuffer_functions.h"
#include "VkQueue_functions.h"
#include "VkDebugMarkerExt_Functions.h"
#include "VkDebugUtilsExt_functions.h"

namespace Profiler
{
    /***********************************************************************************\

    Structure:
        VkDevice_Functions

    Description:
        Set of VkDevice functions which are overloaded in this layer.

    \***********************************************************************************/
    struct VkDevice_Functions
        : VkCommandBuffer_Functions
        , VkQueue_Functions
        , VkDebugMarkerExt_Functions
        , VkDebugUtilsExt_Functions
    {
        // vkGetDeviceProcAddr
        static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetDeviceProcAddr(
            VkDevice device,
            const char* pName );

        // vkDestroyDevice
        static VKAPI_ATTR void VKAPI_CALL DestroyDevice(
            VkDevice device,
            const VkAllocationCallbacks* pAllocator );

        // vkEnumerateDeviceLayerProperties
        static VKAPI_ATTR VkResult VKAPI_CALL EnumerateDeviceLayerProperties(
            VkPhysicalDevice physicalDevice,
            uint32_t* pPropertyCount,
            VkLayerProperties* pLayerProperties );

        // vkEnumerateDeviceExtensionProperties
        static VKAPI_ATTR VkResult VKAPI_CALL EnumerateDeviceExtensionProperties(
            VkPhysicalDevice physicalDevice,
            const char* pLayerName,
            uint32_t* pPropertyCount,
            VkExtensionProperties* pProperties );

        // vkCreateSwapchainKHR
        static VKAPI_ATTR VkResult VKAPI_CALL CreateSwapchainKHR(
            VkDevice device,
            const VkSwapchainCreateInfoKHR* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkSwapchainKHR* pSwapchain );

        // vkDestroySwapchainKHR
        static VKAPI_ATTR void VKAPI_CALL DestroySwapchainKHR(
            VkDevice device,
            VkSwapchainKHR swapchain,
            const VkAllocationCallbacks* pAllocator );

        // vkCreateShaderModule
        static VKAPI_ATTR VkResult VKAPI_CALL CreateShaderModule(
            VkDevice device,
            const VkShaderModuleCreateInfo* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkShaderModule* pShaderModule );

        // vkDestroyShaderModule
        static VKAPI_ATTR void VKAPI_CALL DestroyShaderModule(
            VkDevice device,
            VkShaderModule shaderModule,
            const VkAllocationCallbacks* pAllocator );

        // vkCreateGraphicsPipelines
        static VKAPI_ATTR VkResult VKAPI_CALL CreateGraphicsPipelines(
            VkDevice device,
            VkPipelineCache pipelineCache,
            uint32_t createInfoCount,
            const VkGraphicsPipelineCreateInfo* pCreateInfos,
            const VkAllocationCallbacks* pAllocator,
            VkPipeline* pPipelines );

        // vkCreateComputePipelines
        static VKAPI_ATTR VkResult VKAPI_CALL CreateComputePipelines(
            VkDevice device,
            VkPipelineCache pipelineCache,
            uint32_t createInfoCount,
            const VkComputePipelineCreateInfo* pCreateInfos,
            const VkAllocationCallbacks* pAllocator,
            VkPipeline* pPipelines );

        // vkDestroyPipeline
        static VKAPI_ATTR void VKAPI_CALL DestroyPipeline(
            VkDevice device,
            VkPipeline pipeline,
            const VkAllocationCallbacks* pAllocator );

        // vkCreateRenderPass
        static VKAPI_ATTR VkResult VKAPI_CALL CreateRenderPass(
            VkDevice device,
            const VkRenderPassCreateInfo* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkRenderPass* pRenderPass );

        // vkCreateRenderPass2KHR
        static VKAPI_ATTR VkResult VKAPI_CALL CreateRenderPass2KHR(
            VkDevice device,
            const VkRenderPassCreateInfo2KHR* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkRenderPass* pRenderPass );

        // vkCreateRenderPass2
        static VKAPI_ATTR VkResult VKAPI_CALL CreateRenderPass2(
            VkDevice device,
            const VkRenderPassCreateInfo2* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkRenderPass* pRenderPass );

        // vkDestroyRenderPass
        static VKAPI_ATTR void VKAPI_CALL DestroyRenderPass(
            VkDevice device,
            VkRenderPass renderPass,
            const VkAllocationCallbacks* pAllocator );

        // vkDestroyCommandPool
        static VKAPI_ATTR void VKAPI_CALL DestroyCommandPool(
            VkDevice device,
            VkCommandPool commandPool,
            const VkAllocationCallbacks* pAllocator );

        // vkAllocateCommandBuffers
        static VKAPI_ATTR VkResult VKAPI_CALL AllocateCommandBuffers(
            VkDevice device,
            const VkCommandBufferAllocateInfo* pAllocateInfo,
            VkCommandBuffer* pCommandBuffers );

        // vkFreeCommandBuffers
        static VKAPI_ATTR void VKAPI_CALL FreeCommandBuffers(
            VkDevice device,
            VkCommandPool commandPool,
            uint32_t commandBufferCount,
            const VkCommandBuffer* pCommandBuffers );

        // vkAllocateMemory
        static VKAPI_ATTR VkResult VKAPI_CALL AllocateMemory(
            VkDevice device,
            const VkMemoryAllocateInfo* pAllocateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkDeviceMemory* pMemory );

        // vkFreeMemory
        static VKAPI_ATTR void VKAPI_CALL FreeMemory(
            VkDevice device,
            VkDeviceMemory memory,
            const VkAllocationCallbacks* pAllocator );
    };
}

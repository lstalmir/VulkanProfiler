#include "VkDevice_functions.h"
#include "VkInstance_functions.h"
#include "profiler_layer_objects/VkSwapchainKHR_object.h"
#include "VkLayer_profiler_layer.generated.h"

#include "profiler_ext/VkProfilerEXT.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        GetDeviceProcAddr

    Description:
        Gets pointer to the VkDevice function implementation.

    \***********************************************************************************/
    VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL VkDevice_Functions::GetDeviceProcAddr(
        VkDevice device,
        const char* pName )
    {
        // VkDevice_Functions
        GETPROCADDR( GetDeviceProcAddr );
        GETPROCADDR( DestroyDevice );
        GETPROCADDR( EnumerateDeviceLayerProperties );
        GETPROCADDR( EnumerateDeviceExtensionProperties );
        GETPROCADDR( SetDebugUtilsObjectNameEXT );
        GETPROCADDR( DebugMarkerSetObjectNameEXT );
        GETPROCADDR( CreateSwapchainKHR );
        GETPROCADDR( DestroySwapchainKHR );
        GETPROCADDR( CreateShaderModule );
        GETPROCADDR( DestroyShaderModule );
        GETPROCADDR( CreateGraphicsPipelines );
        GETPROCADDR( AllocateMemory );
        GETPROCADDR( FreeMemory );

        // VkCommandBuffer_Functions
        GETPROCADDR( BeginCommandBuffer );
        GETPROCADDR( EndCommandBuffer );
        GETPROCADDR( CmdBeginRenderPass );
        GETPROCADDR( CmdEndRenderPass );
        GETPROCADDR( CmdBindPipeline );
        GETPROCADDR( CmdPipelineBarrier );
        GETPROCADDR( CmdDraw );
        GETPROCADDR( CmdDrawIndirect );
        GETPROCADDR( CmdDrawIndexed );
        GETPROCADDR( CmdDrawIndexedIndirect );
        GETPROCADDR( CmdDispatch );
        GETPROCADDR( CmdDispatchIndirect );
        GETPROCADDR( CmdCopyBuffer );
        GETPROCADDR( CmdCopyBufferToImage );
        GETPROCADDR( CmdCopyImage );
        GETPROCADDR( CmdCopyImageToBuffer );
        GETPROCADDR( CmdClearAttachments );
        GETPROCADDR( CmdClearColorImage );
        GETPROCADDR( CmdClearDepthStencilImage );

        // VkQueue_Functions
        GETPROCADDR( QueueSubmit );
        GETPROCADDR( QueuePresentKHR );

        // VK_EXT_profiler functions
        GETPROCADDR_EXT( vkSetProfilerModeEXT );
        GETPROCADDR_EXT( vkCmdDrawProfilerOverlayEXT );

        // Get device dispatch table
        return DeviceDispatch.Get( device ).Device.Callbacks.GetDeviceProcAddr( device, pName );
    }

    /***********************************************************************************\

    Function:
        DestroyDevice

    Description:
        Removes dispatch table associated with the VkDevice object.

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkDevice_Functions::DestroyDevice(
        VkDevice device,
        const VkAllocationCallbacks* pAllocator )
    {
        auto& dd = DeviceDispatch.Get( device );
        auto pfnDestroyDevice = dd.Device.Callbacks.DestroyDevice;

        // Cleanup dispatch table and profiler
        VkDevice_Functions_Base::OnDeviceDestroy( device );

        // Destroy the device
        pfnDestroyDevice( device, pAllocator );
    }

    /***********************************************************************************\

    Function:
        EnumerateDeviceLayerProperties

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkDevice_Functions::EnumerateDeviceLayerProperties(
        VkPhysicalDevice physicalDevice,
        uint32_t* pPropertyCount,
        VkLayerProperties* pLayerProperties )
    {
        return VkInstance_Functions::EnumerateInstanceLayerProperties( pPropertyCount, pLayerProperties );
    }

    /***********************************************************************************\

    Function:
        EnumerateDeviceExtensionProperties

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkDevice_Functions::EnumerateDeviceExtensionProperties(
        VkPhysicalDevice physicalDevice,
        const char* pLayerName,
        uint32_t* pPropertyCount,
        VkExtensionProperties* pProperties )
    {
        // Pass through any queries that aren't to us
        if( !pLayerName || strcmp( pLayerName, VK_LAYER_profiler_name ) != 0 )
        {
            if( physicalDevice == VK_NULL_HANDLE )
                return VK_SUCCESS;

            // EnumerateDeviceExtensionProperties is actually VkInstance (VkPhysicalDevice) function.
            // Get dispatch table associated with the VkPhysicalDevice and invoke next layer's
            // vkEnumerateDeviceExtensionProperties implementation.
            auto id = VkInstance_Functions::InstanceDispatch.Get( physicalDevice );

            return id.Instance.Callbacks.EnumerateDeviceExtensionProperties(
                physicalDevice, pLayerName, pPropertyCount, pProperties );
        }

        static VkExtensionProperties layerExtensions[] = {
            { VK_EXT_PROFILER_EXTENSION_NAME, VK_EXT_PROFILER_SPEC_VERSION } };

        if( pProperties )
        {
            const size_t dstBufferSize = *pPropertyCount * sizeof( VkExtensionProperties );

            // Copy device extension properties to output ptr
            memcpy_s( pProperties, dstBufferSize, layerExtensions,
                min( dstBufferSize, sizeof( layerExtensions ) ) );

            if( dstBufferSize < sizeof( layerExtensions ) )
                return VK_INCOMPLETE;
        }

        // SPEC: pPropertyCount MUST be valid uint32_t pointer
        *pPropertyCount = _countof( layerExtensions );

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        SetDebugUtilsObjectNameEXT

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkDevice_Functions::SetDebugUtilsObjectNameEXT(
        VkDevice device,
        const VkDebugUtilsObjectNameInfoEXT* pObjectInfo )
    {
        auto& dd = DeviceDispatch.Get( device );

        // Set the object name
        VkResult result = dd.Device.Callbacks.SetDebugUtilsObjectNameEXT( device, pObjectInfo );

        if( result != VK_SUCCESS )
        {
            // Failed to set object name
            return result;
        }

        // Update profiler
        dd.Profiler.SetDebugObjectName( pObjectInfo->objectHandle, pObjectInfo->pObjectName );

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        DebugMarkerSetObjectNameEXT

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkDevice_Functions::DebugMarkerSetObjectNameEXT(
        VkDevice device,
        const VkDebugMarkerObjectNameInfoEXT* pObjectInfo )
    {
        auto& dd = DeviceDispatch.Get( device );

        // Set the object name
        VkResult result = dd.Device.Callbacks.DebugMarkerSetObjectNameEXT( device, pObjectInfo );

        if( result != VK_SUCCESS )
        {
            // Failed to set object name
            return result;
        }

        // Update profiler
        dd.Profiler.SetDebugObjectName( pObjectInfo->object, pObjectInfo->pObjectName );

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        CreateSwapchainKHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkDevice_Functions::CreateSwapchainKHR(
        VkDevice device,
        const VkSwapchainCreateInfoKHR* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkSwapchainKHR* pSwapchain )
    {
        auto& dd = DeviceDispatch.Get( device );

        VkSwapchainCreateInfoKHR createInfo = *pCreateInfo;

        if( dd.Profiler.m_Config.m_OutputFlags & VK_PROFILER_OUTPUT_FLAG_OVERLAY_BIT_EXT )
        {
            // Make sure we are able to write to presented image
            createInfo.imageUsage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }

        // Create the swapchain
        VkResult result = dd.Device.Callbacks.CreateSwapchainKHR(
            device, &createInfo, pAllocator, pSwapchain );

        if( result != VK_SUCCESS )
        {
            // Swapchain creation failed
            return result;
        }

        VkSwapchainKHR_Object swapchainObject = {};
        swapchainObject.Handle = *pSwapchain;
        swapchainObject.pSurface = &dd.Device.pInstance->Surfaces[ pCreateInfo->surface ];
        
        uint32_t swapchainImageCount = 0;
        dd.Device.Callbacks.GetSwapchainImagesKHR(
            device, *pSwapchain, &swapchainImageCount, nullptr );

        swapchainObject.Images.resize( swapchainImageCount );
        dd.Device.Callbacks.GetSwapchainImagesKHR(
            device, *pSwapchain, &swapchainImageCount, swapchainObject.Images.data() );

        dd.Device.Swapchains.emplace( *pSwapchain, swapchainObject );

        // Register swapchain
        dd.Profiler.CreateSwapchain( pCreateInfo, *pSwapchain );

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        DestroySwapchainKHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkDevice_Functions::DestroySwapchainKHR(
        VkDevice device,
        VkSwapchainKHR swapchain,
        const VkAllocationCallbacks* pAllocator )
    {
        auto& dd = DeviceDispatch.Get( device );

        // Unregister the swapchain from the profiler
        dd.Profiler.DestroySwapchain( swapchain );

        dd.Device.Swapchains.erase( swapchain );

        // Destroy the swapchain
        dd.Device.Callbacks.DestroySwapchainKHR( device, swapchain, pAllocator );
    }

    /***********************************************************************************\

    Function:
        CreateShaderModule

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkDevice_Functions::CreateShaderModule(
        VkDevice device,
        const VkShaderModuleCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkShaderModule* pShaderModule )
    {
        auto& dd = DeviceDispatch.Get( device );

        // Create the shader module
        VkResult result = dd.Device.Callbacks.CreateShaderModule(
            device, pCreateInfo, pAllocator, pShaderModule );

        if( result != VK_SUCCESS )
        {
            // Shader module creation failed
            return result;
        }

        // Register shader module
        dd.Profiler.CreateShaderModule( *pShaderModule, pCreateInfo );

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        DestroyShaderModule

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkDevice_Functions::DestroyShaderModule(
        VkDevice device,
        VkShaderModule shaderModule,
        const VkAllocationCallbacks* pAllocator )
    {
        auto& dd = DeviceDispatch.Get( device );

        // Unregister the shader module from the profiler
        dd.Profiler.DestroyShaderModule( shaderModule );

        // Destroy the shader module
        dd.Device.Callbacks.DestroyShaderModule( device, shaderModule, pAllocator );
    }

    /***********************************************************************************\

    Function:
        CreateGraphicsPipelines

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkDevice_Functions::CreateGraphicsPipelines(
        VkDevice device,
        VkPipelineCache pipelineCache,
        uint32_t createInfoCount,
        const VkGraphicsPipelineCreateInfo* pCreateInfos,
        const VkAllocationCallbacks* pAllocator,
        VkPipeline* pPipelines )
    {
        auto& dd = DeviceDispatch.Get( device );

        // Create the pipelines
        VkResult result = dd.Device.Callbacks.CreateGraphicsPipelines(
            device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines );

        if( result != VK_SUCCESS )
        {
            // Pipeline creation failed
            return result;
        }

        // Register pipelines
        dd.Profiler.CreatePipelines( createInfoCount, pCreateInfos, pPipelines );

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        DestroyPipeline

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkDevice_Functions::DestroyPipeline(
        VkDevice device,
        VkPipeline pipeline,
        const VkAllocationCallbacks* pAllocator )
    {
        auto& dd = DeviceDispatch.Get( device );

        // Unregister the pipeline
        dd.Profiler.DestroyPipeline( pipeline );

        // Destroy the pipeline
        dd.Device.Callbacks.DestroyPipeline( device, pipeline, pAllocator );
    }

    /***********************************************************************************\

    Function:
        FreeCommandBuffers

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkDevice_Functions::FreeCommandBuffers(
        VkDevice device,
        VkCommandPool commandPool,
        uint32_t commandBufferCount,
        const VkCommandBuffer* pCommandBuffers )
    {
        auto& dd = DeviceDispatch.Get( device );

        // Cleanup profiler resources associated with freed command buffers
        dd.Profiler.FreeCommandBuffers( commandBufferCount, pCommandBuffers );

        // Free the command buffers
        dd.Device.Callbacks.FreeCommandBuffers(
            device, commandPool, commandBufferCount, pCommandBuffers );
    }

    /***********************************************************************************\

    Function:
        AllocateMemory

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkDevice_Functions::AllocateMemory(
        VkDevice device,
        const VkMemoryAllocateInfo* pAllocateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDeviceMemory* pMemory )
    {
        auto& dd = DeviceDispatch.Get( device );

        // Allocate the memory
        VkResult result = dd.Device.Callbacks.AllocateMemory(
            device, pAllocateInfo, pAllocator, pMemory );

        if( result != VK_SUCCESS )
        {
            // Allocation failed, do not profile
            return result;
        }

        // Register allocation
        dd.Profiler.OnAllocateMemory( *pMemory, pAllocateInfo );

        return result;
    }

    /***********************************************************************************\

    Function:
        FreeMemory

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkDevice_Functions::FreeMemory(
        VkDevice device,
        VkDeviceMemory memory,
        const VkAllocationCallbacks* pAllocator )
    {
        auto& dd = DeviceDispatch.Get( device );

        // Free the memory
        dd.Device.Callbacks.FreeMemory( device, memory, pAllocator );

        // Unregister allocation
        dd.Profiler.OnFreeMemory( memory );
    }
}

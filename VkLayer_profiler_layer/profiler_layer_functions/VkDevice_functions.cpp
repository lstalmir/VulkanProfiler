#include "VkDevice_functions.h"
#include "VkInstance_functions.h"
#include "VkLayer_profiler_layer.generated.h"

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

        // Get device dispatch table
        return DeviceDispatch.Get( device ).DispatchTable.GetDeviceProcAddr( device, pName );
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
        auto pfnDestroyDevice = dd.DispatchTable.DestroyDevice;

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

            return id.DispatchTable.EnumerateDeviceExtensionProperties(
                physicalDevice, pLayerName, pPropertyCount, pProperties );
        }

        // Don't expose any extensions
        if( pPropertyCount )
        {
            (*pPropertyCount) = 0;
        }

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
        VkResult result = dd.DispatchTable.SetDebugUtilsObjectNameEXT( device, pObjectInfo );

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
        VkResult result = dd.DispatchTable.CreateShaderModule(
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
        dd.DispatchTable.DestroyShaderModule( device, shaderModule, pAllocator );
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
        VkResult result = dd.DispatchTable.CreateGraphicsPipelines(
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
        dd.DispatchTable.DestroyPipeline( device, pipeline, pAllocator );
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
        dd.DispatchTable.FreeCommandBuffers(
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
        VkResult result = dd.DispatchTable.AllocateMemory(
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
        dd.DispatchTable.FreeMemory( device, memory, pAllocator );

        // Unregister allocation
        dd.Profiler.OnFreeMemory( memory );
    }
}

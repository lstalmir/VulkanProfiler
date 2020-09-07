#include "VkDevice_functions.h"
#include "VkInstance_functions.h"
#include "profiler_layer_objects/VkSwapchainKhr_object.h"
#include "VkLayer_profiler_layer.generated.h"
#include "profiler_layer_functions/Helpers.h"

#include "profiler_ext/VkProfilerEXT.h"

namespace
{
    // VkDebugReportObjectTypeEXT to VkObjectType mapping
    static const std::map<VkDebugReportObjectTypeEXT, VkObjectType> VkDebugReportObjectTypeEXT_to_VkObjectType =
    {
        { VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT,                      VK_OBJECT_TYPE_UNKNOWN },
        { VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,                     VK_OBJECT_TYPE_INSTANCE },
        { VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT,              VK_OBJECT_TYPE_PHYSICAL_DEVICE },
        { VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,                       VK_OBJECT_TYPE_DEVICE },
        { VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT,                        VK_OBJECT_TYPE_QUEUE },
        { VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT,                    VK_OBJECT_TYPE_SEMAPHORE },
        { VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,               VK_OBJECT_TYPE_COMMAND_BUFFER },
        { VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT,                        VK_OBJECT_TYPE_FENCE },
        { VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT,                VK_OBJECT_TYPE_DEVICE_MEMORY },
        { VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT,                       VK_OBJECT_TYPE_BUFFER },
        { VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,                        VK_OBJECT_TYPE_IMAGE },
        { VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT,                        VK_OBJECT_TYPE_EVENT },
        { VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT,                   VK_OBJECT_TYPE_QUERY_POOL },
        { VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT,                  VK_OBJECT_TYPE_BUFFER_VIEW },
        { VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT,                   VK_OBJECT_TYPE_IMAGE_VIEW },
        { VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT,                VK_OBJECT_TYPE_SHADER_MODULE },
        { VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT,               VK_OBJECT_TYPE_PIPELINE_CACHE },
        { VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT,              VK_OBJECT_TYPE_PIPELINE_LAYOUT },
        { VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT,                  VK_OBJECT_TYPE_RENDER_PASS },
        { VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT,                     VK_OBJECT_TYPE_PIPELINE },
        { VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT,        VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT },
        { VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT,                      VK_OBJECT_TYPE_SAMPLER },
        { VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT,              VK_OBJECT_TYPE_DESCRIPTOR_POOL },
        { VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT,               VK_OBJECT_TYPE_DESCRIPTOR_SET },
        { VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT,                  VK_OBJECT_TYPE_FRAMEBUFFER },
        { VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT,                 VK_OBJECT_TYPE_COMMAND_POOL },
        { VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT,                  VK_OBJECT_TYPE_SURFACE_KHR },
        { VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT,                VK_OBJECT_TYPE_SWAPCHAIN_KHR },
        { VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT_EXT,    VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT },
        { VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_KHR_EXT,                  VK_OBJECT_TYPE_DISPLAY_KHR },
        { VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_MODE_KHR_EXT,             VK_OBJECT_TYPE_DISPLAY_MODE_KHR },
        { VK_DEBUG_REPORT_OBJECT_TYPE_VALIDATION_CACHE_EXT,             VK_OBJECT_TYPE_VALIDATION_CACHE_EXT },
        { VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION_EXT,     VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION },
        { VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_EXT,   VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE },
        { VK_DEBUG_REPORT_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV_EXT,    VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV }
    };
}

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
        // VkDevice core functions
        GETPROCADDR( GetDeviceProcAddr );
        GETPROCADDR( DestroyDevice );
        GETPROCADDR( CreateShaderModule );
        GETPROCADDR( DestroyShaderModule );
        GETPROCADDR( CreateGraphicsPipelines );
        GETPROCADDR( CreateComputePipelines );
        GETPROCADDR( DestroyPipeline );
        GETPROCADDR( CreateRenderPass );
        GETPROCADDR( CreateRenderPass2 );
        GETPROCADDR( DestroyRenderPass );
        GETPROCADDR( DestroyCommandPool );
        GETPROCADDR( AllocateCommandBuffers );
        GETPROCADDR( FreeCommandBuffers );
        GETPROCADDR( AllocateMemory );
        GETPROCADDR( FreeMemory );

        // VkCommandBuffer core functions
        GETPROCADDR( BeginCommandBuffer );
        GETPROCADDR( EndCommandBuffer );
        GETPROCADDR( ResetCommandBuffer );
        GETPROCADDR( CmdBeginRenderPass );
        GETPROCADDR( CmdEndRenderPass );
        GETPROCADDR( CmdNextSubpass );
        GETPROCADDR( CmdBeginRenderPass2 );
        GETPROCADDR( CmdEndRenderPass2 );
        GETPROCADDR( CmdNextSubpass2 );
        GETPROCADDR( CmdBindPipeline );
        GETPROCADDR( CmdExecuteCommands );
        GETPROCADDR( CmdPipelineBarrier );
        GETPROCADDR( CmdDraw );
        GETPROCADDR( CmdDrawIndirect );
        GETPROCADDR( CmdDrawIndexed );
        GETPROCADDR( CmdDrawIndexedIndirect );
        GETPROCADDR( CmdDrawIndirectCount );
        GETPROCADDR( CmdDrawIndexedIndirectCount );
        GETPROCADDR( CmdDispatch );
        GETPROCADDR( CmdDispatchIndirect );
        GETPROCADDR( CmdCopyBuffer );
        GETPROCADDR( CmdCopyBufferToImage );
        GETPROCADDR( CmdCopyImage );
        GETPROCADDR( CmdCopyImageToBuffer );
        GETPROCADDR( CmdClearAttachments );
        GETPROCADDR( CmdClearColorImage );
        GETPROCADDR( CmdClearDepthStencilImage );
        GETPROCADDR( CmdResolveImage );
        GETPROCADDR( CmdBlitImage );
        GETPROCADDR( CmdFillBuffer );
        GETPROCADDR( CmdUpdateBuffer );

        // VkQueue core functions
        GETPROCADDR( QueueSubmit );

        // VK_KHR_create_renderpass2 functions
        GETPROCADDR( CreateRenderPass2KHR );
        GETPROCADDR( CmdBeginRenderPass2KHR );
        GETPROCADDR( CmdEndRenderPass2KHR );
        GETPROCADDR( CmdNextSubpass2KHR );

        // VK_EXT_debug_marker functions
        GETPROCADDR( DebugMarkerSetObjectNameEXT );
        GETPROCADDR( DebugMarkerSetObjectTagEXT );
        GETPROCADDR( CmdDebugMarkerInsertEXT );
        GETPROCADDR( CmdDebugMarkerBeginEXT );
        GETPROCADDR( CmdDebugMarkerEndEXT );

        // VK_EXT_debug_utils functions
        GETPROCADDR( SetDebugUtilsObjectNameEXT );
        GETPROCADDR( SetDebugUtilsObjectTagEXT );
        GETPROCADDR( CmdInsertDebugUtilsLabelEXT );
        GETPROCADDR( CmdBeginDebugUtilsLabelEXT );
        GETPROCADDR( CmdEndDebugUtilsLabelEXT );

        // VK_AMD_draw_indirect_count functions
        GETPROCADDR( CmdDrawIndirectCountAMD );
        GETPROCADDR( CmdDrawIndexedIndirectCountAMD );

        // VK_KHR_draw_indirect_count functions
        GETPROCADDR( CmdDrawIndirectCountKHR );
        GETPROCADDR( CmdDrawIndexedIndirectCountKHR );

        // VK_KHR_swapchain functions
        GETPROCADDR( QueuePresentKHR );
        GETPROCADDR( CreateSwapchainKHR );
        GETPROCADDR( DestroySwapchainKHR );

        // VK_EXT_profiler functions
        GETPROCADDR_EXT( vkSetProfilerModeEXT );
        GETPROCADDR_EXT( vkSetProfilerSyncModeEXT );
        GETPROCADDR_EXT( vkGetProfilerFrameDataEXT );
        GETPROCADDR_EXT( vkFreeProfilerFrameDataEXT );
        GETPROCADDR_EXT( vkFlushProfilerEXT );

        if( device )
        {
            // Get function address from the next layer
            return DeviceDispatch.Get( device ).Device.Callbacks.GetDeviceProcAddr( device, pName );
        }

        // Function not overloaded
        return nullptr;
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
        VkDevice_Functions_Base::DestroyDeviceBase( device );

        // Destroy the device
        pfnDestroyDevice( device, pAllocator );
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

        if( result == VK_SUCCESS )
        {
            // Register shader module
            dd.Profiler.CreateShaderModule( *pShaderModule, pCreateInfo );
        }

        return result;
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

        if( result == VK_SUCCESS )
        {
            // Register pipelines
            dd.Profiler.CreatePipelines( createInfoCount, pCreateInfos, pPipelines );
        }

        return result;
    }

    /***********************************************************************************\

    Function:
        CreateComputePipelines

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkDevice_Functions::CreateComputePipelines(
        VkDevice device,
        VkPipelineCache pipelineCache,
        uint32_t createInfoCount,
        const VkComputePipelineCreateInfo* pCreateInfos,
        const VkAllocationCallbacks* pAllocator,
        VkPipeline* pPipelines )
    {
        auto& dd = DeviceDispatch.Get( device );

        // Create the pipelines
        VkResult result = dd.Device.Callbacks.CreateComputePipelines(
            device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines );

        if( result == VK_SUCCESS )
        {
            // Register pipelines
            dd.Profiler.CreatePipelines( createInfoCount, pCreateInfos, pPipelines );
        }

        return result;
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
        CreateRenderPass

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkDevice_Functions::CreateRenderPass(
        VkDevice device,
        const VkRenderPassCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkRenderPass* pRenderPass )
    {
        auto& dd = DeviceDispatch.Get( device );

        // Create the render pass
        VkResult result = dd.Device.Callbacks.CreateRenderPass(
            device, pCreateInfo, pAllocator, pRenderPass );

        if( result == VK_SUCCESS )
        {
            // Register new render pass
            dd.Profiler.CreateRenderPass( *pRenderPass, pCreateInfo );
        }

        return result;
    }

    /***********************************************************************************\

    Function:
        CreateRenderPass2

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkDevice_Functions::CreateRenderPass2(
        VkDevice device,
        const VkRenderPassCreateInfo2* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkRenderPass* pRenderPass )
    {
        auto& dd = DeviceDispatch.Get( device );

        // Create the render pass
        VkResult result = dd.Device.Callbacks.CreateRenderPass2(
            device, pCreateInfo, pAllocator, pRenderPass );

        if( result == VK_SUCCESS )
        {
            // Register new render pass
            dd.Profiler.CreateRenderPass( *pRenderPass, pCreateInfo );
        }

        return result;
    }

    /***********************************************************************************\

    Function:
        DestroyRenderPass

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkDevice_Functions::DestroyRenderPass(
        VkDevice device,
        VkRenderPass renderPass,
        const VkAllocationCallbacks* pAllocator )
    {
        auto& dd = DeviceDispatch.Get( device );

        // Unregister the render pass
        dd.Profiler.DestroyRenderPass( renderPass );

        // Destroy the render pass
        dd.Device.Callbacks.DestroyRenderPass( device, renderPass, pAllocator );
    }

    /***********************************************************************************\

    Function:
        DestroyCommandPool

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkDevice_Functions::DestroyCommandPool(
        VkDevice device,
        VkCommandPool commandPool,
        const VkAllocationCallbacks* pAllocator )
    {
        auto& dd = DeviceDispatch.Get( device );

        // Cleanup profiler resources associated with the command pool
        dd.Profiler.FreeCommandBuffers( commandPool );

        // Destroy the command pool
        dd.Device.Callbacks.DestroyCommandPool(
            device, commandPool, pAllocator );
    }

    /***********************************************************************************\

    Function:
        AllocateCommandBuffers

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkDevice_Functions::AllocateCommandBuffers(
        VkDevice device,
        const VkCommandBufferAllocateInfo* pAllocateInfo,
        VkCommandBuffer* pCommandBuffers )
    {
        auto& dd = DeviceDispatch.Get( device );

        // Allocate command buffers
        VkResult result = dd.Device.Callbacks.AllocateCommandBuffers(
            device, pAllocateInfo, pCommandBuffers );

        if( result == VK_SUCCESS )
        {
            // Begin profiling
            dd.Profiler.AllocateCommandBuffers(
                pAllocateInfo->commandPool,
                pAllocateInfo->level,
                pAllocateInfo->commandBufferCount,
                pCommandBuffers );
        }

        return result;
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

        if( result == VK_SUCCESS )
        {
            // Register allocation
            dd.Profiler.AllocateMemory( *pMemory, pAllocateInfo );
        }

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

        // Unregister allocation
        dd.Profiler.FreeMemory( memory );

        // Free the memory
        dd.Device.Callbacks.FreeMemory( device, memory, pAllocator );
    }
}

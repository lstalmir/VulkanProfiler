#include "VkDevice_functions.h"
#include "VkInstance_functions.h"
#include "profiler_layer_objects/VkSwapchainKHR_object.h"
#include "VkLayer_profiler_layer.generated.h"
#include "Helpers.h"

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
        { VK_DEBUG_REPORT_OBJECT_TYPE_OBJECT_TABLE_NVX_EXT,             VK_OBJECT_TYPE_OBJECT_TABLE_NVX },
        { VK_DEBUG_REPORT_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NVX_EXT, VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NVX },
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
        GETPROCADDR_EXT( vkGetProfilerDataEXT );

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

        dd.Device.Debug.ObjectNames.emplace(
            pObjectInfo->objectHandle,
            pObjectInfo->pObjectName );

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

        dd.Device.Debug.ObjectNames.emplace(
            pObjectInfo->object,
            pObjectInfo->pObjectName );

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

        // Create wrapping object
        if( result == VK_SUCCESS )
        {
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
        }

        // Create overlay
        if( result == VK_SUCCESS &&
            dd.Profiler.m_Config.m_OutputFlags & VK_PROFILER_OUTPUT_FLAG_OVERLAY_BIT_EXT )
        {
            // TODO: Multiple swapchain support
            assert( dd.pOverlay == nullptr );

            // Select graphics queue for the overlay draw commands
            VkQueue graphicsQueue = VK_NULL_HANDLE;

            for( auto& it : dd.Device.Queues )
            {
                if( it.second.Flags & VK_QUEUE_GRAPHICS_BIT )
                {
                    graphicsQueue = it.second.Handle;
                    break;
                }
            }

            assert( graphicsQueue != VK_NULL_HANDLE );

            // Create overlay
            result = Create<ProfilerOverlayOutput>(
                dd.pOverlay,
                dd.Device,
                dd.Device.Queues.at( graphicsQueue ),
                dd.Device.Swapchains.at( *pSwapchain ),
                pCreateInfo );

            if( result != VK_SUCCESS )
            {
                // Full initialization failed, don't continue in partially-initialized state
                VkDevice_Functions::DestroySwapchainKHR( device, *pSwapchain, pAllocator );
            }
        }

        return result;
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

        if( dd.pOverlay )
        {
            Destroy<ProfilerOverlayOutput>( dd.pOverlay );
        }

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

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
        GETPROCADDR( CreateSwapchainKHR );
        GETPROCADDR( DestroySwapchainKHR );
        GETPROCADDR( CreateShaderModule );
        GETPROCADDR( DestroyShaderModule );
        GETPROCADDR( CreateGraphicsPipelines );
        GETPROCADDR( CreateComputePipelines );
        GETPROCADDR( DestroyPipeline );
        GETPROCADDR( CreateRenderPass );
        GETPROCADDR( CreateRenderPass2KHR );
        GETPROCADDR( CreateRenderPass2 );
        GETPROCADDR( DestroyRenderPass );
        GETPROCADDR( DestroyCommandPool );
        GETPROCADDR( AllocateCommandBuffers );
        GETPROCADDR( FreeCommandBuffers );
        GETPROCADDR( AllocateMemory );
        GETPROCADDR( FreeMemory );

        // VkCommandBuffer_Functions
        GETPROCADDR( BeginCommandBuffer );
        GETPROCADDR( EndCommandBuffer );
        GETPROCADDR( CmdBeginRenderPass );
        GETPROCADDR( CmdEndRenderPass );
        GETPROCADDR( CmdNextSubpass );
        GETPROCADDR( CmdBeginRenderPass2 );
        GETPROCADDR( CmdEndRenderPass2 );
        GETPROCADDR( CmdNextSubpass2 );
        GETPROCADDR( CmdBeginRenderPass2KHR );
        GETPROCADDR( CmdEndRenderPass2KHR );
        GETPROCADDR( CmdNextSubpass2KHR );
        GETPROCADDR( CmdBindPipeline );
        GETPROCADDR( CmdExecuteCommands );
        GETPROCADDR( CmdPipelineBarrier );
        GETPROCADDR( CmdDraw );
        GETPROCADDR( CmdDrawIndirect );
        GETPROCADDR( CmdDrawIndexed );
        GETPROCADDR( CmdDrawIndexedIndirect );
        GETPROCADDR( CmdDrawIndirectCount );
        GETPROCADDR( CmdDrawIndexedIndirectCount );
        GETPROCADDR( CmdDrawIndirectCountKHR );
        GETPROCADDR( CmdDrawIndexedIndirectCountKHR );
        GETPROCADDR( CmdDrawIndirectCountAMD );
        GETPROCADDR( CmdDrawIndexedIndirectCountAMD );
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

        // VkQueue_Functions
        GETPROCADDR( QueueSubmit );
        GETPROCADDR( QueuePresentKHR );

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

        // VK_EXT_profiler functions
        GETPROCADDR_EXT( vkSetProfilerModeEXT );
        GETPROCADDR_EXT( vkSetProfilerSyncModeEXT );
        GETPROCADDR_EXT( vkGetProfilerFrameDataEXT );
        GETPROCADDR_EXT( vkGetProfilerCommandBufferDataEXT );

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
        const bool queryThisLayerExtensionsOnly =
            (pLayerName) &&
            (strcmp( pLayerName, VK_LAYER_profiler_name ) != 0);

        // EnumerateDeviceExtensionProperties is actually VkInstance (VkPhysicalDevice) function.
        // Get dispatch table associated with the VkPhysicalDevice and invoke next layer's
        // vkEnumerateDeviceExtensionProperties implementation.
        auto id = VkInstance_Functions::InstanceDispatch.Get( physicalDevice );

        VkResult result = VK_SUCCESS;

        // SPEC: pPropertyCount MUST be valid uint32_t pointer
        uint32_t propertyCount = *pPropertyCount;

        if( !pLayerName || !queryThisLayerExtensionsOnly )
        {
            result = id.Instance.Callbacks.EnumerateDeviceExtensionProperties(
                physicalDevice, pLayerName, pPropertyCount, pProperties );
        }
        else
        {
            // pPropertyCount now contains number of pProperties used
            *pPropertyCount = 0;
        }

        static VkExtensionProperties layerExtensions[] = {
            { VK_EXT_PROFILER_EXTENSION_NAME, VK_EXT_PROFILER_SPEC_VERSION },
            { VK_EXT_DEBUG_MARKER_EXTENSION_NAME, VK_EXT_DEBUG_MARKER_SPEC_VERSION } };

        if( !pLayerName || queryThisLayerExtensionsOnly )
        {
            if( pProperties )
            {
                const size_t dstBufferSize =
                    (propertyCount - *pPropertyCount) * sizeof( VkExtensionProperties );

                // Copy device extension properties to output ptr
                std::memcpy( pProperties + (*pPropertyCount), layerExtensions,
                    std::min( dstBufferSize, sizeof( layerExtensions ) ) );

                if( dstBufferSize < sizeof( layerExtensions ) )
                {
                    // Not enough space in the buffer
                    result = VK_INCOMPLETE;
                }
            }

            // Return number of extensions exported by this layer
            *pPropertyCount += std::extent_v<decltype(layerExtensions)>;
        }

        return result;
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

        // TODO: Move to separate layer
        const bool createProfilerOverlay =
            (dd.Profiler.m_Config.m_Flags & VK_PROFILER_CREATE_DISABLE_OVERLAY_BIT_EXT) == 0;

        if( createProfilerOverlay )
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
        // TODO: Move to separate layer
        if( result == VK_SUCCESS &&
            createProfilerOverlay )
        {
            if( dd.pOverlay == nullptr )
            {
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
            else
            {
                // Reinitialize overlay for the new swapchain
                dd.pOverlay->ResetSwapchain(
                    dd.Device.Swapchains.at( *pSwapchain ),
                    pCreateInfo );
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
            // After recreating swapchain using CreateSwapchainKHR parent swapchain of the overlay has changed.
            // The old swapchain is then destroyed and will invalidate the overlay if we don't check which
            // swapchain is actually being destreoyed.
            if( dd.pOverlay->GetSwapchain() == swapchain )
            {
                Destroy<ProfilerOverlayOutput>( dd.pOverlay );
            }
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
        CreateRenderPass2KHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkDevice_Functions::CreateRenderPass2KHR(
        VkDevice device,
        const VkRenderPassCreateInfo2KHR* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkRenderPass* pRenderPass )
    {
        auto& dd = DeviceDispatch.Get( device );

        // Create the render pass
        VkResult result = dd.Device.Callbacks.CreateRenderPass2KHR(
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

// Copyright (c) 2019-2022 Lukasz Stalmirski
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "VkDevice_functions.h"
#include "VkInstance_functions.h"
#include "profiler_layer_objects/VkSwapchainKhr_object.h"
#include "VkLayer_profiler_layer.generated.h"
#include "profiler_layer_functions/Helpers.h"

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
        GETPROCADDR( CreateCommandPool );
        GETPROCADDR( DestroyCommandPool );
        GETPROCADDR( AllocateCommandBuffers );
        GETPROCADDR( FreeCommandBuffers );
        GETPROCADDR( AllocateMemory );
        GETPROCADDR( FreeMemory );
        GETPROCADDR( BindBufferMemory );
        GETPROCADDR( BindImageMemory );
        GETPROCADDR( CreateBuffer );
        GETPROCADDR( DestroyBuffer );
        GETPROCADDR( CreateImage );
        GETPROCADDR( DestroyImage );

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
        GETPROCADDR( CmdBeginRendering );
        GETPROCADDR( CmdEndRendering );
        GETPROCADDR( CmdBindPipeline );
        GETPROCADDR( CmdExecuteCommands );
        GETPROCADDR( CmdPipelineBarrier );
        GETPROCADDR( CmdPipelineBarrier2 );
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
        GETPROCADDR( QueueSubmit2 );

        // VK_KHR_create_renderpass2 functions
        GETPROCADDR( CreateRenderPass2KHR );
        GETPROCADDR( CmdBeginRenderPass2KHR );
        GETPROCADDR( CmdEndRenderPass2KHR );
        GETPROCADDR( CmdNextSubpass2KHR );

        // VK_KHR_dynamic_rendering functions
        GETPROCADDR( CmdBeginRenderingKHR );
        GETPROCADDR( CmdEndRenderingKHR );

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

        // VK_KHR_ray_tracing_pipeline functions
        GETPROCADDR( CreateRayTracingPipelinesKHR );
        GETPROCADDR( CmdTraceRaysKHR );

        // VK_KHR_acceleration_structure functions
        GETPROCADDR( CmdBuildAccelerationStructuresKHR );
        GETPROCADDR( CmdBuildAccelerationStructuresIndirectKHR );
        GETPROCADDR( CmdCopyAccelerationStructureKHR );
        GETPROCADDR( CmdCopyAccelerationStructureToMemoryKHR );
        GETPROCADDR( CmdCopyMemoryToAccelerationStructureKHR );

        // VK_KHR_swapchain functions
        GETPROCADDR( QueuePresentKHR );
        GETPROCADDR( CreateSwapchainKHR );
        GETPROCADDR( DestroySwapchainKHR );

        // VK_KHR_synchronization2 functions
        GETPROCADDR( QueueSubmit2KHR );
        GETPROCADDR( CmdPipelineBarrier2KHR );

        // VK_EXT_profiler functions
        GETPROCADDR_EXT( vkSetProfilerModeEXT );
        GETPROCADDR_EXT( vkSetProfilerSyncModeEXT );
        GETPROCADDR_EXT( vkGetProfilerFrameDataEXT );
        GETPROCADDR_EXT( vkFreeProfilerFrameDataEXT );
        GETPROCADDR_EXT( vkFlushProfilerEXT );

        if( device )
        {
            auto& dd = DeviceDispatch.Get( device );

            // Get function address from the next layer
            return dd.Device.Callbacks.GetDeviceProcAddr( device, pName );
        }

        // Undefined behaviour according to spec - VkDevice handle cannot be null
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
        CreateCommandPool

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkDevice_Functions::CreateCommandPool(
        VkDevice device,
        const VkCommandPoolCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkCommandPool* pCommandPool )
    {
        auto& dd = DeviceDispatch.Get( device );

        // Create the command pool
        VkResult result = dd.Device.Callbacks.CreateCommandPool(
            device, pCreateInfo, pAllocator, pCommandPool );

        if( result == VK_SUCCESS )
        {
            // Create command pool wrapper
            dd.Profiler.CreateCommandPool( *pCommandPool, pCreateInfo );
        }

        return result;
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
        dd.Profiler.DestroyCommandPool( commandPool );

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

    /***********************************************************************************\

    Function:
        BindBufferMemory

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkDevice_Functions::BindBufferMemory(
        VkDevice device,
        VkBuffer buffer,
        VkDeviceMemory deviceMemory,
        VkDeviceSize offset )
    {
        auto& dd = DeviceDispatch.Get( device );

        // Bind the buffer
        VkResult result = dd.Device.Callbacks.BindBufferMemory(
            device, buffer, deviceMemory, offset );

        if( result == VK_SUCCESS )
        {
            // Register memory binding
            dd.Profiler.BindBufferMemory( buffer, deviceMemory, offset );
        }

        return result;
    }

    /***********************************************************************************\

    Function:
        BindImageMemory

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkDevice_Functions::BindImageMemory(
        VkDevice device,
        VkImage image,
        VkDeviceMemory deviceMemory,
        VkDeviceSize offset )
    {
        auto& dd = DeviceDispatch.Get( device );

        // Bind the image
        VkResult result = dd.Device.Callbacks.BindImageMemory(
            device, image, deviceMemory, offset );

        if( result == VK_SUCCESS )
        {
            // Register memory binding
            dd.Profiler.BindImageMemory( image, deviceMemory, offset );
        }

        return result;
    }

    /***********************************************************************************\

    Function:
        CreateBuffer

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkDevice_Functions::CreateBuffer(
        VkDevice device,
        const VkBufferCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkBuffer* pBuffer )
    {
        auto& dd = DeviceDispatch.Get( device );

        VkResult result = dd.Device.Callbacks.CreateBuffer(
            device, pCreateInfo, pAllocator, pBuffer );

        if( result == VK_SUCCESS )
        {
            // Register resource
            dd.Profiler.CreateBuffer( *pBuffer, pCreateInfo );
        }

        return result;
    }

    /***********************************************************************************\

    Function:
        DestroyBuffer

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkDevice_Functions::DestroyBuffer(
        VkDevice device,
        VkBuffer buffer,
        const VkAllocationCallbacks* pAllocator )
    {
        auto& dd = DeviceDispatch.Get( device );

        // Unregister memory binding
        dd.Profiler.DestroyBuffer( buffer );

        // Destroy the buffer
        dd.Device.Callbacks.DestroyBuffer( device, buffer, pAllocator );
    }

    /***********************************************************************************\

    Function:
        CreateImage

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkDevice_Functions::CreateImage(
        VkDevice device,
        const VkImageCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkImage* pImage )
    {
        auto& dd = DeviceDispatch.Get( device );

        VkResult result = dd.Device.Callbacks.CreateImage(
            device, pCreateInfo, pAllocator, pImage );

        if( result == VK_SUCCESS )
        {
            // Register resource
            dd.Profiler.CreateImage( *pImage, pCreateInfo );
        }

        return result;
    }

    /***********************************************************************************\

    Function:
        DestroyImage

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkDevice_Functions::DestroyImage(
        VkDevice device,
        VkImage image,
        const VkAllocationCallbacks* pAllocator )
    {
        auto& dd = DeviceDispatch.Get( device );

        // Unregister memory binding
        dd.Profiler.DestroyImage( image );

        // Destroy the image
        dd.Device.Callbacks.DestroyImage( device, image, pAllocator );
    }
}

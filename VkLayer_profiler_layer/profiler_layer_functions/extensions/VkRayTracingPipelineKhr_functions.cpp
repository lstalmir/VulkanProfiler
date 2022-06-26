// Copyright (c) 2019-2021 Lukasz Stalmirski
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

#include "VkRayTracingPipelineKhr_functions.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        CreateRayTracingPipelinesKHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkRayTracingPipelineKhr_Functions::CreateRayTracingPipelinesKHR(
        VkDevice device,
        VkDeferredOperationKHR deferredOperation,
        VkPipelineCache pipelineCache,
        uint32_t createInfoCount,
        const VkRayTracingPipelineCreateInfoKHR* pCreateInfos,
        const VkAllocationCallbacks* pAllocator,
        VkPipeline* pPipelines )
    {
        auto& dd = DeviceDispatch.Get( device );

        // Create the pipelines
        VkResult result = dd.Device.Callbacks.CreateRayTracingPipelinesKHR(
            device, deferredOperation, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines );

        if( result == VK_SUCCESS )
        {
            // Register pipelines
            dd.Profiler.CreatePipelines( createInfoCount, pCreateInfos, pPipelines );
        }

        return result;
    }

    /***********************************************************************************\

    Function:
        CmdTraceRaysKHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkRayTracingPipelineKhr_Functions::CmdTraceRaysKHR(
        VkCommandBuffer commandBuffer,
        const VkStridedDeviceAddressRegionKHR* pRaygenShaderBindingTable,
        const VkStridedDeviceAddressRegionKHR* pMissShaderBindingTable,
        const VkStridedDeviceAddressRegionKHR* pHitShaderBindingTable,
        const VkStridedDeviceAddressRegionKHR* pCallableShaderBindingTable,
        uint32_t width,
        uint32_t height,
        uint32_t depth )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        // Setup drawcall descriptor
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eTraceRaysKHR;
        drawcall.m_Payload.m_TraceRays.m_Width = width;
        drawcall.m_Payload.m_TraceRays.m_Height = height;
        drawcall.m_Payload.m_TraceRays.m_Depth = depth;

        profiledCommandBuffer.PreCommand( drawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdTraceRaysKHR(
            commandBuffer,
            pRaygenShaderBindingTable,
            pMissShaderBindingTable,
            pHitShaderBindingTable,
            pCallableShaderBindingTable,
            width,
            height,
            depth );

        profiledCommandBuffer.PostCommand( drawcall );
    }

    /***********************************************************************************\

    Function:
        CmdTraceRaysIndirectKHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkRayTracingPipelineKhr_Functions::CmdTraceRaysIndirectKHR(
        VkCommandBuffer commandBuffer,
        const VkStridedDeviceAddressRegionKHR* pRaygenShaderBindingTable,
        const VkStridedDeviceAddressRegionKHR* pMissShaderBindingTable,
        const VkStridedDeviceAddressRegionKHR* pHitShaderBindingTable,
        const VkStridedDeviceAddressRegionKHR* pCallableShaderBindingTable,
        VkDeviceAddress indirectDeviceAddress )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        // Setup drawcall descriptor
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eTraceRaysKHR;
        drawcall.m_Payload.m_TraceRaysIndirect.m_IndirectAddress = indirectDeviceAddress;

        profiledCommandBuffer.PreCommand( drawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdTraceRaysIndirectKHR(
            commandBuffer,
            pRaygenShaderBindingTable,
            pMissShaderBindingTable,
            pHitShaderBindingTable,
            pCallableShaderBindingTable,
            indirectDeviceAddress );

        profiledCommandBuffer.PostCommand( drawcall );
    }

}

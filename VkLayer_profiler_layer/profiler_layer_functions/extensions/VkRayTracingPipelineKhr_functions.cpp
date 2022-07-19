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

        std::vector<VkRayTracingPipelineCreateInfoKHR> createInfos;
        if (dd.Profiler.CapturePipelineExecutableProperties())
        {
            // To capture the properties and internal representations, flags must be passed to the ICD.
            // Create a copy of pCreateInfos list and add these flags.
            createInfos.resize(createInfoCount);
            memcpy(createInfos.data(), pCreateInfos, createInfoCount * sizeof(*pCreateInfos));

            for (auto& createInfo : createInfos)
            {
                createInfo.flags |=
                    VK_PIPELINE_CREATE_CAPTURE_INTERNAL_REPRESENTATIONS_BIT_KHR |
                    VK_PIPELINE_CREATE_CAPTURE_STATISTICS_BIT_KHR;
            }

            pCreateInfos = createInfos.data();
        }

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



    // TODO: Move to VkAccelerationStructureKhr_functions.h
    /***********************************************************************************\

    Function:
        CmdBuildAccelerationStructuresKHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkRayTracingPipelineKhr_Functions::CmdBuildAccelerationStructuresKHR(
        VkCommandBuffer commandBuffer,
        uint32_t infoCount,
        const VkAccelerationStructureBuildGeometryInfoKHR* pInfos,
        const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos)
    {
        auto& dd = DeviceDispatch.Get(commandBuffer);
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer(commandBuffer);

        // Setup drawcall descriptor
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eBuildAccelerationStructuresKHR;
        drawcall.m_Payload.m_BuildAccelerationStructures.m_InfoCount = infoCount;
        drawcall.m_Payload.m_BuildAccelerationStructures.m_pInfos = CopyElements(infoCount, pInfos);

        profiledCommandBuffer.PreCommand(drawcall);

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdBuildAccelerationStructuresKHR(
            commandBuffer,
            infoCount,
            pInfos,
            ppBuildRangeInfos);

        profiledCommandBuffer.PostCommand(drawcall);
    }

    /***********************************************************************************\

    Function:
        CmdBuildAccelerationStructuresIndirectKHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkRayTracingPipelineKhr_Functions::CmdBuildAccelerationStructuresIndirectKHR(
        VkCommandBuffer commandBuffer,
        uint32_t infoCount,
        const VkAccelerationStructureBuildGeometryInfoKHR* pInfos,
        const VkDeviceAddress* pIndirectDeviceAddresses,
        const uint32_t* pIndirectStrides,
        const uint32_t* const* ppMaxPrimitiveCounts)
    {
        auto& dd = DeviceDispatch.Get(commandBuffer);
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer(commandBuffer);

        // Setup drawcall descriptor
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eBuildAccelerationStructuresIndirectKHR;
        drawcall.m_Payload.m_BuildAccelerationStructures.m_InfoCount = infoCount;
        drawcall.m_Payload.m_BuildAccelerationStructures.m_pInfos = CopyElements(infoCount, pInfos);

        profiledCommandBuffer.PreCommand(drawcall);

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdBuildAccelerationStructuresIndirectKHR(
            commandBuffer,
            infoCount,
            pInfos,
            pIndirectDeviceAddresses,
            pIndirectStrides,
            ppMaxPrimitiveCounts);

        profiledCommandBuffer.PostCommand(drawcall);
    }

    /***********************************************************************************\

    Function:
        CmdCopyAccelerationStructureKHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkRayTracingPipelineKhr_Functions::CmdCopyAccelerationStructureKHR(
        VkCommandBuffer commandBuffer,
        const VkCopyAccelerationStructureInfoKHR* pInfo)
    {
        auto& dd = DeviceDispatch.Get(commandBuffer);
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer(commandBuffer);

        // Setup drawcall descriptor
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eCopyAccelerationStructureKHR;
        drawcall.m_Payload.m_CopyAccelerationStructure.m_Src = pInfo->src;
        drawcall.m_Payload.m_CopyAccelerationStructure.m_Dst = pInfo->dst;
        drawcall.m_Payload.m_CopyAccelerationStructure.m_Mode = pInfo->mode;

        profiledCommandBuffer.PreCommand(drawcall);

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdCopyAccelerationStructureKHR(
            commandBuffer,
            pInfo);

        profiledCommandBuffer.PostCommand(drawcall);
    }

    /***********************************************************************************\

    Function:
        CmdCopyAccelerationStructureToMemoryKHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkRayTracingPipelineKhr_Functions::CmdCopyAccelerationStructureToMemoryKHR(
        VkCommandBuffer commandBuffer,
        const VkCopyAccelerationStructureToMemoryInfoKHR* pInfo)
    {
        auto& dd = DeviceDispatch.Get(commandBuffer);
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer(commandBuffer);

        // Setup drawcall descriptor
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eCopyAccelerationStructureToMemoryKHR;
        drawcall.m_Payload.m_CopyAccelerationStructureToMemory.m_Src = pInfo->src;
        drawcall.m_Payload.m_CopyAccelerationStructureToMemory.m_Dst = pInfo->dst;
        drawcall.m_Payload.m_CopyAccelerationStructureToMemory.m_Mode = pInfo->mode;

        profiledCommandBuffer.PreCommand(drawcall);

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdCopyAccelerationStructureToMemoryKHR(
            commandBuffer,
            pInfo);

        profiledCommandBuffer.PostCommand(drawcall);
    }

    /***********************************************************************************\

    Function:
        CmdCopyMemoryToAccelerationStructureKHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkRayTracingPipelineKhr_Functions::CmdCopyMemoryToAccelerationStructureKHR(
        VkCommandBuffer commandBuffer,
        const VkCopyMemoryToAccelerationStructureInfoKHR* pInfo)
    {
        auto& dd = DeviceDispatch.Get(commandBuffer);
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer(commandBuffer);

        // Setup drawcall descriptor
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eCopyMemoryToAccelerationStructureKHR;
        drawcall.m_Payload.m_CopyMemoryToAccelerationStructure.m_Src = pInfo->src;
        drawcall.m_Payload.m_CopyMemoryToAccelerationStructure.m_Dst = pInfo->dst;
        drawcall.m_Payload.m_CopyMemoryToAccelerationStructure.m_Mode = pInfo->mode;

        profiledCommandBuffer.PreCommand(drawcall);

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdCopyMemoryToAccelerationStructureKHR(
            commandBuffer,
            pInfo);

        profiledCommandBuffer.PostCommand(drawcall);
    }
}

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

#include "VkAccelerationStructureKhr_functions.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        CmdBuildAccelerationStructuresKHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkAccelerationStructureKhr_Functions::CmdBuildAccelerationStructuresKHR(
        VkCommandBuffer commandBuffer,
        uint32_t infoCount,
        const VkAccelerationStructureBuildGeometryInfoKHR* pInfos,
        const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        TipGuard tip( dd.Device.TIP, __func__ );

        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        // Setup drawcall descriptor
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eBuildAccelerationStructuresKHR;
        drawcall.m_Payload = DeviceProfilerDrawcallBuildAccelerationStructuresPayload( infoCount, pInfos, ppBuildRangeInfos );

        profiledCommandBuffer.PreCommand( drawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdBuildAccelerationStructuresKHR(
            commandBuffer,
            infoCount,
            pInfos,
            ppBuildRangeInfos );

        profiledCommandBuffer.PostCommand( drawcall );
    }

    /***********************************************************************************\

    Function:
        CmdBuildAccelerationStructuresIndirectKHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkAccelerationStructureKhr_Functions::CmdBuildAccelerationStructuresIndirectKHR(
        VkCommandBuffer commandBuffer,
        uint32_t infoCount,
        const VkAccelerationStructureBuildGeometryInfoKHR* pInfos,
        const VkDeviceAddress* pIndirectDeviceAddresses,
        const uint32_t* pIndirectStrides,
        const uint32_t* const* ppMaxPrimitiveCounts )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        TipGuard tip( dd.Device.TIP, __func__ );

        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        // Setup drawcall descriptor
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eBuildAccelerationStructuresIndirectKHR;
        drawcall.m_Payload = DeviceProfilerDrawcallBuildAccelerationStructuresIndirectPayload( infoCount, pInfos, ppMaxPrimitiveCounts );

        profiledCommandBuffer.PreCommand( drawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdBuildAccelerationStructuresIndirectKHR(
            commandBuffer,
            infoCount,
            pInfos,
            pIndirectDeviceAddresses,
            pIndirectStrides,
            ppMaxPrimitiveCounts );

        profiledCommandBuffer.PostCommand( drawcall );
    }

    /***********************************************************************************\

    Function:
        CmdCopyAccelerationStructureKHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkAccelerationStructureKhr_Functions::CmdCopyAccelerationStructureKHR(
        VkCommandBuffer commandBuffer,
        const VkCopyAccelerationStructureInfoKHR* pInfo )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        TipGuard tip( dd.Device.TIP, __func__ );

        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        // Setup drawcall descriptor
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eCopyAccelerationStructureKHR;
        drawcall.m_Payload.m_CopyAccelerationStructure.m_Src = pInfo->src;
        drawcall.m_Payload.m_CopyAccelerationStructure.m_Dst = pInfo->dst;
        drawcall.m_Payload.m_CopyAccelerationStructure.m_Mode = pInfo->mode;

        profiledCommandBuffer.PreCommand( drawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdCopyAccelerationStructureKHR(
            commandBuffer,
            pInfo );

        profiledCommandBuffer.PostCommand( drawcall );
    }

    /***********************************************************************************\

    Function:
        CmdCopyAccelerationStructureToMemoryKHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkAccelerationStructureKhr_Functions::CmdCopyAccelerationStructureToMemoryKHR(
        VkCommandBuffer commandBuffer,
        const VkCopyAccelerationStructureToMemoryInfoKHR* pInfo )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        TipGuard tip( dd.Device.TIP, __func__ );

        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        // Setup drawcall descriptor
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eCopyAccelerationStructureToMemoryKHR;
        drawcall.m_Payload.m_CopyAccelerationStructureToMemory.m_Src = pInfo->src;
        drawcall.m_Payload.m_CopyAccelerationStructureToMemory.m_Dst = pInfo->dst;
        drawcall.m_Payload.m_CopyAccelerationStructureToMemory.m_Mode = pInfo->mode;

        profiledCommandBuffer.PreCommand( drawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdCopyAccelerationStructureToMemoryKHR(
            commandBuffer,
            pInfo );

        profiledCommandBuffer.PostCommand( drawcall );
    }

    /***********************************************************************************\

    Function:
        CmdCopyMemoryToAccelerationStructureKHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkAccelerationStructureKhr_Functions::CmdCopyMemoryToAccelerationStructureKHR(
        VkCommandBuffer commandBuffer,
        const VkCopyMemoryToAccelerationStructureInfoKHR* pInfo )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        TipGuard tip( dd.Device.TIP, __func__ );

        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        // Setup drawcall descriptor
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eCopyMemoryToAccelerationStructureKHR;
        drawcall.m_Payload.m_CopyMemoryToAccelerationStructure.m_Src = pInfo->src;
        drawcall.m_Payload.m_CopyMemoryToAccelerationStructure.m_Dst = pInfo->dst;
        drawcall.m_Payload.m_CopyMemoryToAccelerationStructure.m_Mode = pInfo->mode;

        profiledCommandBuffer.PreCommand( drawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdCopyMemoryToAccelerationStructureKHR(
            commandBuffer,
            pInfo );

        profiledCommandBuffer.PostCommand( drawcall );
    }
}

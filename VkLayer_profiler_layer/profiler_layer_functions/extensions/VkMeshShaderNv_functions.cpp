// Copyright (c) 2024-2024 Lukasz Stalmirski
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

#include "VkMeshShaderNv_functions.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        CmdDrawMeshTasksNV

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkMeshShaderNv_Functions::CmdDrawMeshTasksNV(
        VkCommandBuffer commandBuffer,
        uint32_t taskCount,
        uint32_t firstTask )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        TipGuard tip( dd.Device.TIP, __func__ );

        auto& cmd = dd.Profiler.GetCommandBuffer( commandBuffer );

        // Setup drawcall descriptor.
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eDrawMeshTasksNV;
        drawcall.m_Extension = DeviceProfilerExtensionType::eNV;
        drawcall.m_Payload.m_DrawMeshTasksNV.m_TaskCount = taskCount;
        drawcall.m_Payload.m_DrawMeshTasksNV.m_FirstTask = firstTask;

        cmd.PreCommand( drawcall );

        // Invoke next layer's implementation.
        dd.Device.Callbacks.CmdDrawMeshTasksNV(
            commandBuffer,
            taskCount,
            firstTask );

        cmd.PostCommand( drawcall );
    }

    /***********************************************************************************\

    Function:
        CmdDrawMeshTasksIndirectNV

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkMeshShaderNv_Functions::CmdDrawMeshTasksIndirectNV(
        VkCommandBuffer commandBuffer,
        VkBuffer buffer,
        VkDeviceSize offset,
        uint32_t drawCount,
        uint32_t stride )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        TipGuard tip( dd.Device.TIP, __func__ );

        auto& cmd = dd.Profiler.GetCommandBuffer( commandBuffer );

        // Setup drawcall descriptor.
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eDrawMeshTasksIndirect;
        drawcall.m_Extension = DeviceProfilerExtensionType::eNV;
        drawcall.m_Payload.m_DrawMeshTasksIndirect.m_Buffer = buffer;
        drawcall.m_Payload.m_DrawMeshTasksIndirect.m_Offset = offset;
        drawcall.m_Payload.m_DrawMeshTasksIndirect.m_DrawCount = drawCount;
        drawcall.m_Payload.m_DrawMeshTasksIndirect.m_Stride = stride;

        cmd.PreCommand( drawcall );

        // Invoke next layer's implementation.
        dd.Device.Callbacks.CmdDrawMeshTasksIndirectNV(
            commandBuffer,
            buffer,
            offset,
            drawCount,
            stride );

        cmd.PostCommand( drawcall );
    }

    /***********************************************************************************\

    Function:
        CmdDrawMeshTasksIndirectCountNV

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkMeshShaderNv_Functions::CmdDrawMeshTasksIndirectCountNV(
        VkCommandBuffer commandBuffer,
        VkBuffer buffer,
        VkDeviceSize offset,
        VkBuffer countBuffer,
        VkDeviceSize countBufferOffset,
        uint32_t maxDrawCount,
        uint32_t stride )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        TipGuard tip( dd.Device.TIP, __func__ );

        auto& cmd = dd.Profiler.GetCommandBuffer( commandBuffer );

        // Setup drawcall descriptor.
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eDrawMeshTasksIndirectCount;
        drawcall.m_Extension = DeviceProfilerExtensionType::eNV;
        drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_Buffer = buffer;
        drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_Offset = offset;
        drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_CountBuffer = countBuffer;
        drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_CountOffset = countBufferOffset;
        drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_MaxDrawCount = maxDrawCount;
        drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_Stride = stride;

        cmd.PreCommand( drawcall );

        // Invoke next layer's implementation.
        dd.Device.Callbacks.CmdDrawMeshTasksIndirectCountNV(
            commandBuffer,
            buffer,
            offset,
            countBuffer,
            countBufferOffset,
            maxDrawCount,
            stride );

        cmd.PostCommand( drawcall );
    }
}

// Copyright (c) 2020 Lukasz Stalmirski
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

#include "VkDrawIndirectCountAmd_functions.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        CmdDrawIndirectCountAMD

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkDrawIndirectCountAmd_Functions::CmdDrawIndirectCountAMD(
        VkCommandBuffer commandBuffer,
        VkBuffer argsBuffer,
        VkDeviceSize argsOffset,
        VkBuffer countBuffer,
        VkDeviceSize countOffset,
        uint32_t maxDrawCount,
        uint32_t stride )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        // Setup drawcall descriptor
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eDrawIndirectCount;
        drawcall.m_Payload.m_DrawIndirectCount.m_Buffer = argsBuffer;
        drawcall.m_Payload.m_DrawIndirectCount.m_Offset = argsOffset;
        drawcall.m_Payload.m_DrawIndirectCount.m_CountBuffer = countBuffer;
        drawcall.m_Payload.m_DrawIndirectCount.m_CountOffset = countOffset;
        drawcall.m_Payload.m_DrawIndirectCount.m_MaxDrawCount = maxDrawCount;
        drawcall.m_Payload.m_DrawIndirectCount.m_Stride = stride;

        profiledCommandBuffer.PreCommand( drawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdDrawIndirectCountAMD(
            commandBuffer, argsBuffer, argsOffset, countBuffer, countOffset, maxDrawCount, stride );

        profiledCommandBuffer.PostCommand( drawcall );
    }

    /***********************************************************************************\

    Function:
        CmdDrawIndexedIndirectCountAMD

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkDrawIndirectCountAmd_Functions::CmdDrawIndexedIndirectCountAMD(
        VkCommandBuffer commandBuffer,
        VkBuffer argsBuffer,
        VkDeviceSize argsOffset,
        VkBuffer countBuffer,
        VkDeviceSize countOffset,
        uint32_t maxDrawCount,
        uint32_t stride )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        // Setup drawcall descriptor
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eDrawIndexedIndirectCount;
        drawcall.m_Payload.m_DrawIndirectCount.m_Buffer = argsBuffer;
        drawcall.m_Payload.m_DrawIndirectCount.m_Offset = argsOffset;
        drawcall.m_Payload.m_DrawIndirectCount.m_CountBuffer = countBuffer;
        drawcall.m_Payload.m_DrawIndirectCount.m_CountOffset = countOffset;
        drawcall.m_Payload.m_DrawIndirectCount.m_MaxDrawCount = maxDrawCount;
        drawcall.m_Payload.m_DrawIndirectCount.m_Stride = stride;

        profiledCommandBuffer.PreCommand( drawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdDrawIndexedIndirectCountAMD(
            commandBuffer, argsBuffer, argsOffset, countBuffer, countOffset, maxDrawCount, stride );

        profiledCommandBuffer.PostCommand( drawcall );
    }
}

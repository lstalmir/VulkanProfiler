// Copyright (c) 2026 Lukasz Stalmirski
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

#include "VkMultiDrawExt_functions.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        CmdDrawMultiEXT

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkMultiDrawExt_Functions::CmdDrawMultiEXT(
        VkCommandBuffer commandBuffer,
        uint32_t drawCount,
        const VkMultiDrawInfoEXT* pVertexInfo,
        uint32_t instanceCount,
        uint32_t firstInstance,
        uint32_t stride )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        TipGuard tip( dd.Device.TIP, __func__ );

        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        // Setup drawcall descriptor
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eDrawMulti;
        drawcall.m_Payload.m_DrawMulti.m_DrawCount = drawCount;
        drawcall.m_Payload.m_DrawMulti.m_InstanceCount = instanceCount;
        drawcall.m_Payload.m_DrawMulti.m_FirstInstance = firstInstance;
        drawcall.m_Payload.m_DrawMulti.m_Stride = stride;
        drawcall.m_Payload.m_DrawMulti.m_pVertexInfo = pVertexInfo;

        profiledCommandBuffer.PreCommand( drawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdDrawMultiEXT(
            commandBuffer, drawCount, pVertexInfo, instanceCount, firstInstance, stride );

        profiledCommandBuffer.PostCommand( drawcall );
    }

    /***********************************************************************************\

    Function:
        CmdDrawMutliIndexedEXT

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkMultiDrawExt_Functions::CmdDrawMultiIndexedEXT(
        VkCommandBuffer commandBuffer,
        uint32_t drawCount,
        const VkMultiDrawIndexedInfoEXT* pIndexInfo,
        uint32_t instanceCount,
        uint32_t firstInstance,
        uint32_t stride,
        const int32_t* pVertexOffset )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        TipGuard tip( dd.Device.TIP, __func__ );

        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        // Setup drawcall descriptor
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eDrawMultiIndexed;
        drawcall.m_Payload.m_DrawMultiIndexed.m_DrawCount = drawCount;
        drawcall.m_Payload.m_DrawMultiIndexed.m_InstanceCount = instanceCount;
        drawcall.m_Payload.m_DrawMultiIndexed.m_FirstInstance = firstInstance;
        drawcall.m_Payload.m_DrawMultiIndexed.m_Stride = stride;
        drawcall.m_Payload.m_DrawMultiIndexed.m_pIndexInfo = pIndexInfo;
        drawcall.m_Payload.m_DrawMultiIndexed.m_pVertexOffset = pVertexOffset;

        profiledCommandBuffer.PreCommand( drawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdDrawMultiIndexedEXT(
            commandBuffer, drawCount, pIndexInfo, instanceCount, firstInstance, stride, pVertexOffset );

        profiledCommandBuffer.PostCommand( drawcall );
    }
}

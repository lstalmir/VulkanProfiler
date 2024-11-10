// Copyright (c) 2023-2023 Lukasz Stalmirski
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

#include "VkDynamicRenderingKhr_functions.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        CmdBeginRenderingKHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkDynamicRenderingKhr_Functions::CmdBeginRenderingKHR(
        VkCommandBuffer commandBuffer,
        const VkRenderingInfoKHR* pRenderingInfo )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        TipGuard tip( dd.Device.TIP, __func__ );

        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        profiledCommandBuffer.PreBeginRendering( pRenderingInfo );

        // Begin the render pass
        dd.Device.Callbacks.CmdBeginRenderingKHR( commandBuffer, pRenderingInfo );

        profiledCommandBuffer.PostBeginRendering( pRenderingInfo );
    }

    /***********************************************************************************\

    Function:
        CmdEndRenderingKHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkDynamicRenderingKhr_Functions::CmdEndRenderingKHR(
        VkCommandBuffer commandBuffer )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        TipGuard tip( dd.Device.TIP, __func__ );

        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        profiledCommandBuffer.PreEndRendering();

        // End the render pass
        dd.Device.Callbacks.CmdEndRenderingKHR( commandBuffer );

        profiledCommandBuffer.PostEndRendering();
    }
}

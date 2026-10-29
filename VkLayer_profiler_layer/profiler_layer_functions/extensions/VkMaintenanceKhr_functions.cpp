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

#include "VkMaintenanceKhr_functions.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        GetDeviceBufferMemoryRequirementsKHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkMaintenanceKhr_Functions::GetDeviceBufferMemoryRequirementsKHR(
        VkDevice device,
        const VkDeviceBufferMemoryRequirementsKHR* pInfo,
        VkMemoryRequirements2KHR* pMemoryRequirements )
    {
        auto& dd = DeviceDispatch.Get( device );
        TipGuard tip( dd.Device.TIP, __func__ );

        // Apply the same adjustments to the buffer create info as in CreateBuffer,
        // so the memory requirements are consistent with the created buffer.
        VkBufferCreateInfo createInfo = *pInfo->pCreateInfo;
        dd.Profiler.SetupBufferCreateInfo( &createInfo );

        VkDeviceBufferMemoryRequirementsKHR info = *pInfo;
        info.pCreateInfo = &createInfo;

        // Get the memory requirements.
        dd.Device.Callbacks.GetDeviceBufferMemoryRequirementsKHR(
            device, &info, pMemoryRequirements );
    }

    /***********************************************************************************\

    Function:
        CmdEndRendering2KHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkMaintenanceKhr_Functions::CmdEndRendering2KHR(
        VkCommandBuffer commandBuffer,
        const VkRenderingEndInfoKHR* pRenderingEndInfo )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        TipGuard tip( dd.Device.TIP, __func__ );

        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        profiledCommandBuffer.PreEndRendering();

        // End the render pass
        dd.Device.Callbacks.CmdEndRendering2KHR( commandBuffer, pRenderingEndInfo );

        profiledCommandBuffer.PostEndRendering();
    }
}

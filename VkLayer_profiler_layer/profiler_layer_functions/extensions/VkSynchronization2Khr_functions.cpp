// Copyright (c) 2023 Lukasz Stalmirski
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

#include "VkSynchronization2Khr_functions.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        QueueSubmit2KHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkSynchronization2Khr_Functions::QueueSubmit2KHR(
        VkQueue queue,
        uint32_t submitCount,
        const VkSubmitInfo2KHR* pSubmits,
        VkFence fence )
    {
        auto& dd = DeviceDispatch.Get( queue );
        TipGuard tip( dd.Device.TIP, __func__ );

        DeviceProfilerSubmitBatch submitBatch;
        dd.Profiler.CreateSubmitBatchInfo( queue, submitCount, pSubmits, &submitBatch );
        dd.Profiler.PreSubmitCommandBuffers( submitBatch );

        // Submit the command buffers
        VkResult result = dd.Device.Callbacks.QueueSubmit2KHR( queue, submitCount, pSubmits, fence );

        dd.Profiler.PostSubmitCommandBuffers( submitBatch );

        // Consume the collected data
        if( dd.Profiler.m_Config.m_FrameDelimiter == VK_PROFILER_FRAME_DELIMITER_SUBMIT_EXT )
        {
            if( dd.pOutput )
            {
                dd.pOutput->Update();
            }
        }

        return result;
    }

    /***********************************************************************************\

    Function:
        CmdPipelineBarrier2KHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkSynchronization2Khr_Functions::CmdPipelineBarrier2KHR(
        VkCommandBuffer commandBuffer,
        const VkDependencyInfoKHR* pDependencyInfo )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        TipGuard tip( dd.Device.TIP, __func__ );

        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        // Record barrier statistics
        profiledCommandBuffer.PipelineBarrier( pDependencyInfo );

        // Insert the barrier
        dd.Device.Callbacks.CmdPipelineBarrier2KHR( commandBuffer, pDependencyInfo );
    }
}

// Copyright (c) 2023-2026 Lukasz Stalmirski
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

        // Synchronize host access to the queue object in case the overlay tries to use it.
        VkQueue_Object_Scope queueScope( dd.Device.Queues.at( queue ) );

        // Prepare the command buffers for profiling.
        // This may insert additional command buffers to each VkSubmitInfo to reset queries and copy data to buffers.
        DeviceProfilerSubmitCommandScratchData<VkSubmitInfo2KHR> scratchData;
        scratchData.m_Queue = queue;
        scratchData.m_SubmitInfos = std::vector( pSubmits, pSubmits + submitCount );
        dd.Profiler.PrepareQueueSubmit( scratchData );

        // Additionally, synchronize all queues if requested by the user.
        std::unique_lock queueLock( dd.Profiler.m_SubmitMutex, std::defer_lock );
        if( dd.Profiler.m_Config.m_SynchronizeQueues )
        {
            queueLock.lock();
        }

        // Submit the command buffers
        VkResult result = dd.Device.Callbacks.QueueSubmit2KHR( queue, static_cast<uint32_t>( scratchData.m_SubmitInfos.size() ), scratchData.m_SubmitInfos.data(), fence );

        // Signal fence to wait for query results.
        if( scratchData.m_Fence.use_count() > 1 )
        {
            dd.Device.Callbacks.QueueSubmit( queue, 0, nullptr, scratchData.m_Fence.get() );
        }

        // Wait for the command buffers to finish executing to ensure the queues are not executing in parallel.
        if( dd.Profiler.m_Config.m_SynchronizeQueues )
        {
            dd.Device.Callbacks.QueueWaitIdle( queue );
            queueLock.unlock();
        }

        // Finalize the submission.
        dd.Profiler.FinishQueueSubmit( scratchData );

        // Consume the collected data
        if( dd.pOutput )
        {
            dd.pOutput->Update();
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

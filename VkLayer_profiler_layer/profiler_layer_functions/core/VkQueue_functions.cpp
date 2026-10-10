// Copyright (c) 2019-2026 Lukasz Stalmirski
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

#include "VkQueue_functions.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        QueueSubmit

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkQueue_Functions::QueueSubmit(
        VkQueue queue,
        uint32_t submitCount,
        const VkSubmitInfo* pSubmits,
        VkFence fence )
    {
        auto& dd = DeviceDispatch.Get( queue );
        TipGuard tip( dd.Device.TIP, __func__ );

        // Synchronize host access to the queue object in case the overlay tries to use it.
        VkQueue_Object_Scope queueScope( dd.Device.Queues.at( queue ) );

        // Prepare the command buffers for profiling.
        // This may insert additional command buffers to each VkSubmitInfo to reset queries and copy data to buffers.
        DeviceProfilerSubmitCommandScratchData scratchData( queue, submitCount, pSubmits );
        dd.Profiler.PrepareQueueSubmit( scratchData );

        // Additionally, synchronize all queues if requested by the user.
        std::unique_lock queueLock( dd.Profiler.m_SubmitMutex, std::defer_lock );
        if( dd.Profiler.m_Config.m_SynchronizeQueues )
        {
            queueLock.lock();
        }

        // Submit the command buffers
        VkResult result = dd.Device.Callbacks.QueueSubmit(
            queue,
            scratchData.GetSubmitInfoCount(),
            scratchData.GetSubmitInfos(),
            fence );

        // Finalize the submission.
        dd.Profiler.FinishQueueSubmit( scratchData, result );

        // Wait for the command buffers to finish executing to ensure the queues are not executing in parallel.
        if( dd.Profiler.m_Config.m_SynchronizeQueues )
        {
            dd.Device.Callbacks.QueueWaitIdle( queue );
            queueLock.unlock();
        }

        // Consume the collected data
        if( dd.pOutput )
        {
            dd.pOutput->Update();
        }

        return result;
    }

    /***********************************************************************************\

    Function:
        QueueSubmit2

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkQueue_Functions::QueueSubmit2(
        VkQueue queue,
        uint32_t submitCount,
        const VkSubmitInfo2* pSubmits,
        VkFence fence )
    {
        auto& dd = DeviceDispatch.Get( queue );
        TipGuard tip( dd.Device.TIP, __func__ );

        // Synchronize host access to the queue object in case the overlay tries to use it.
        VkQueue_Object_Scope queueScope( dd.Device.Queues.at( queue ) );

        // Prepare the command buffers for profiling.
        // This may insert additional command buffers to each VkSubmitInfo to reset queries and copy data to buffers.
        DeviceProfilerSubmitCommandScratchData scratchData( queue, submitCount, pSubmits );
        dd.Profiler.PrepareQueueSubmit( scratchData );

        // Additionally, synchronize all queues if requested by the user.
        std::unique_lock queueLock( dd.Profiler.m_SubmitMutex, std::defer_lock );
        if( dd.Profiler.m_Config.m_SynchronizeQueues )
        {
            queueLock.lock();
        }

        // Submit the command buffers
        VkResult result = dd.Device.Callbacks.QueueSubmit2(
            queue,
            scratchData.GetSubmitInfoCount(),
            scratchData.GetSubmitInfos(),
            fence );

        // Finalize the submission.
        dd.Profiler.FinishQueueSubmit( scratchData, result );

        // Wait for the command buffers to finish executing to ensure the queues are not executing in parallel.
        if( dd.Profiler.m_Config.m_SynchronizeQueues )
        {
            dd.Device.Callbacks.QueueWaitIdle( queue );
            queueLock.unlock();
        }

        // Consume the collected data
        if( dd.pOutput )
        {
            dd.pOutput->Update();
        }

        return result;
    }

    /***********************************************************************************\

    Function:
        QueueBindSparse

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkQueue_Functions::QueueBindSparse(
        VkQueue queue,
        uint32_t bindInfoCount,
        const VkBindSparseInfo* pBindInfo,
        VkFence fence )
    {
        auto& dd = DeviceDispatch.Get( queue );
        TipGuard tip( dd.Device.TIP, __func__ );

        // Synchronize host access to the queue object in case the overlay tries to use it.
        VkQueue_Object_Scope queueScope( dd.Device.Queues.at( queue ) );

        // Bind sparse memory
        VkResult result = dd.Device.Callbacks.QueueBindSparse( queue, bindInfoCount, pBindInfo, fence );

        if( result == VK_SUCCESS )
        {
            dd.Profiler.BindSparseMemory( queue, bindInfoCount, pBindInfo );
        }

        return result;
    }

    /***********************************************************************************\

    Function:
        QueueWaitIdle

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkQueue_Functions::QueueWaitIdle(
        VkQueue queue )
    {
        auto& dd = DeviceDispatch.Get( queue );
        TipGuard tip( dd.Device.TIP, __func__ );

        // Synchronize host access to the queue object in case the overlay tries to use it.
        VkQueue_Object_Scope queueScope( dd.Device.Queues.at( queue ) );

        // Wait for the queue to become idle
        return dd.Device.Callbacks.QueueWaitIdle( queue );
    }
}

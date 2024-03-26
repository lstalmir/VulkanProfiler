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
        dd.Profiler.PreSubmitCommandBuffers( queue );

        // Submit the command buffers
        VkResult result;
        {
            // Make sure the profiler doesn't access the queue until this function exits.
            VkQueue_Object* queueObject = dd.Device.Queues.at(queue);
            std::shared_lock lk(queueObject->Mutex);

            result = dd.Device.Callbacks.QueueSubmit(queue, submitCount, pSubmits, fence);
        }

        dd.Profiler.PostSubmitCommandBuffers( queue, submitCount, pSubmits );
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
        dd.Profiler.PreSubmitCommandBuffers( queue );

        // Submit the command buffers
        VkResult result;
        {
            // Make sure the profiler doesn't access the queue until this function exits.
            VkQueue_Object* queueObject = dd.Device.Queues.at(queue);
            std::shared_lock lk(queueObject->Mutex);

            result = dd.Device.Callbacks.QueueSubmit2(queue, submitCount, pSubmits, fence);
        }

        dd.Profiler.PostSubmitCommandBuffers( queue, submitCount, pSubmits );
        return result;
    }
}

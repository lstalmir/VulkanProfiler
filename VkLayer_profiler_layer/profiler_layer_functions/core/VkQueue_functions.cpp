// Copyright (c) 2019-2025 Lukasz Stalmirski
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

        DeviceProfilerSubmitBatch submitBatch;
        dd.Profiler.CreateSubmitBatchInfo( queue, submitCount, pSubmits, &submitBatch );
        dd.Profiler.PreSubmitCommandBuffers( submitBatch );

        // Submit the command buffers
        VkResult result = dd.Device.Callbacks.QueueSubmit( queue, submitCount, pSubmits, fence );

        dd.Profiler.PostSubmitCommandBuffers( submitBatch );
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

        DeviceProfilerSubmitBatch submitBatch;
        dd.Profiler.CreateSubmitBatchInfo( queue, submitCount, pSubmits, &submitBatch );
        dd.Profiler.PreSubmitCommandBuffers( submitBatch );

        // Submit the command buffers
        VkResult result = dd.Device.Callbacks.QueueSubmit2( queue, submitCount, pSubmits, fence );

        dd.Profiler.PostSubmitCommandBuffers( submitBatch );
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

        VkResult result = dd.Device.Callbacks.QueueBindSparse( queue, bindInfoCount, pBindInfo, fence );

        if( result == VK_SUCCESS )
        {
            for( uint32_t i = 0; i < bindInfoCount; ++i )
            {
                const VkBindSparseInfo& bindInfo = pBindInfo[i];

                // Register sparse buffers memory bindings.
                for( uint32_t j = 0; j < bindInfo.bufferBindCount; ++j )
                {
                    dd.Profiler.BindBufferMemory( &bindInfo.pBufferBinds[j] );
                }
            }
        }

        return result;
    }
}

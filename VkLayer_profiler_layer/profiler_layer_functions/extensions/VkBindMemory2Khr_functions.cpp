// Copyright (c) 2025 Lukasz Stalmirski
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

#include "VkBindMemory2Khr_functions.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        BindBufferMemory2KHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkBindMemory2Khr_Functions::BindBufferMemory2KHR(
            VkDevice device,
            uint32_t bindInfoCount,
            const VkBindBufferMemoryInfoKHR* pBindInfos )
    {
        auto& dd = DeviceDispatch.Get( device );
        TipGuard tip( dd.Device.TIP, __func__ );

        // Bind buffer memory
        VkResult result = dd.Device.Callbacks.BindBufferMemory2KHR(
            device, bindInfoCount, pBindInfos );

        if( result == VK_SUCCESS )
        {
            // Register buffer memory bindings
            for( uint32_t i = 0; i < bindInfoCount; ++i )
            {
                dd.Profiler.BindBufferMemory(
                    pBindInfos[ i ].buffer,
                    pBindInfos[ i ].memory,
                    pBindInfos[ i ].memoryOffset );
            }
        }

        return result;
    }

    /***********************************************************************************\

    Function:
        BindImageMemory2KHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkBindMemory2Khr_Functions::BindImageMemory2KHR(
            VkDevice device,
            uint32_t bindInfoCount,
            const VkBindImageMemoryInfoKHR* pBindInfos )
    {
        auto& dd = DeviceDispatch.Get( device );
        TipGuard tip( dd.Device.TIP, __func__ );

        // Bind image memory
        VkResult result = dd.Device.Callbacks.BindImageMemory2KHR(
            device, bindInfoCount, pBindInfos );

        if( result == VK_SUCCESS )
        {
            // Register image memory bindings
            for( uint32_t i = 0; i < bindInfoCount; ++i )
            {
                dd.Profiler.BindImageMemory(
                    pBindInfos[ i ].image,
                    pBindInfos[ i ].memory,
                    pBindInfos[ i ].memoryOffset );
            }
        }

        return result;
    }
}

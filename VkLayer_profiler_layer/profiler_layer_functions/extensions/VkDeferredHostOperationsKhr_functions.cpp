// Copyright (c) 2019-2023 Lukasz Stalmirski
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

#include "VkDeferredHostOperationsKhr_functions.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        CreateDeferredOperationKHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkDeferredHostOperationsKhr_Functions::CreateDeferredOperationKHR(
        VkDevice device,
        const VkAllocationCallbacks* pAllocator,
        VkDeferredOperationKHR* pDeferredOperation )
    {
        auto& dd = DeviceDispatch.Get( device );

        // Create the deferred operation.
        VkResult result = dd.Device.Callbacks.CreateDeferredOperationKHR( device, pAllocator, pDeferredOperation );

        if( result == VK_SUCCESS )
        {
            // Register the deferred operation.
            dd.Profiler.CreateDeferredOperation( *pDeferredOperation );
        }

        return result;
    }

    /***********************************************************************************\

    Function:
        DestroyDeferredOperationKHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkDeferredHostOperationsKhr_Functions::DestroyDeferredOperationKHR(
        VkDevice device,
        VkDeferredOperationKHR deferredOperation,
        const VkAllocationCallbacks* pAllocator )
    {
        auto& dd = DeviceDispatch.Get( device );

        // Destroy the deferred operation.
        dd.Device.Callbacks.DestroyDeferredOperationKHR( device, deferredOperation, pAllocator );

        // Free the data and unregister the operation.
        dd.Profiler.DestroyDeferredOperation( deferredOperation );
    }

    /***********************************************************************************\

    Function:
        DeferredOperationJoinKHR

    Description:
        Wait for the deferred host operation to complete and execute any actions
        assciated with the operation by the profiler.

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkDeferredHostOperationsKhr_Functions::DeferredOperationJoinKHR(
        VkDevice device,
        VkDeferredOperationKHR deferredOperation )
    {
        auto& dd = DeviceDispatch.Get( device );

        // Wait for the operation to complete.
        VkResult result = dd.Device.Callbacks.DeferredOperationJoinKHR( device, deferredOperation );

        // Invoke the callback associated with the deferred operation when it completes.
        if( result == VK_SUCCESS )
        {
            dd.Profiler.ExecuteDeferredOperationCallback( deferredOperation );
        }

        return result;
    }
}

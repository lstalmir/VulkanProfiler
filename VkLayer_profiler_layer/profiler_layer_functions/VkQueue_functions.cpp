#include "VkQueue_functions.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        QueuePresentKHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkQueue_Functions::QueuePresentKHR(
        VkQueue queue,
        const VkPresentInfoKHR* pPresentInfo )
    {
        auto& dd = DeviceDispatch.Get( queue );

        // Create mutable copy of present info
        VkPresentInfoKHR presentInfo = *pPresentInfo;
        // Get present queue wrapper
        VkQueue_Object& presentQueue = dd.Device.Queues[ queue ];

        dd.Profiler.Present( presentQueue, &presentInfo );

        if( dd.pOverlay )
        {
            // Display overlay
            dd.pOverlay->Present( dd.Profiler.GetData(), presentQueue, &presentInfo );
        }

        // Present the image
        return dd.Device.Callbacks.QueuePresentKHR( queue, &presentInfo );
    }

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

        const VkSubmitInfo* pOriginalSubmits = pSubmits;

        // Submit the command buffers
        VkResult result = dd.Device.Callbacks.QueueSubmit( queue, submitCount, pSubmits, fence );

        // Profile the submit
        dd.Profiler.PostSubmitCommandBuffers( queue, submitCount, pSubmits, fence );

        // Profiler allocated new pSubmits, release it
        if( pOriginalSubmits != pSubmits )
        {
            delete[] pSubmits;
        }

        return result;
    }
}

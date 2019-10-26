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

        dd.Profiler.PrePresent( queue );

        // Present the image
        VkResult result = dd.DispatchTable.QueuePresentKHR( queue, pPresentInfo );

        dd.Profiler.PostPresent( queue );

        return result;
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

        // Increment submit counter
        dd.Profiler.GetCurrentFrameStats().submitCount += submitCount;

        // Submit the command buffers
        VkResult result = dd.DispatchTable.QueueSubmit( queue, submitCount, pSubmits, fence );

        return result;
    }
}

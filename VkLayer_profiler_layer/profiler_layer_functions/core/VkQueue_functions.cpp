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

        const VkSubmitInfo* pOriginalSubmits = pSubmits;

        dd.Profiler.PreSubmitCommandBuffers( queue, submitCount, pSubmits, fence );

        // Submit the command buffers
        VkResult result = dd.Device.Callbacks.QueueSubmit( queue, submitCount, pSubmits, fence );

        dd.Profiler.PostSubmitCommandBuffers( queue, submitCount, pSubmits, fence );

        // Profiler allocated new pSubmits, release it
        if( pOriginalSubmits != pSubmits )
        {
            delete[] pSubmits;
        }

        return result;
    }
}

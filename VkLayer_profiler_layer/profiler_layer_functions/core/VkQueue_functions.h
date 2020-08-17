#pragma once
#include "VkDevice_functions_base.h"

namespace Profiler
{
    /***********************************************************************************\

    Structure:
        VkQueue_Functions

    Description:
        Set of VkQueue functions which are overloaded in this layer.

    \***********************************************************************************/
    struct VkQueue_Functions : VkDevice_Functions_Base
    {
        // vkQueueSubmit
        static VKAPI_ATTR VkResult VKAPI_CALL QueueSubmit(
            VkQueue queue,
            uint32_t submitCount,
            const VkSubmitInfo* pSubmits,
            VkFence fence );
    };
}

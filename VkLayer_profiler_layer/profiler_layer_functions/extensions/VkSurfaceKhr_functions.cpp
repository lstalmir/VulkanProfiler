#include "VkSurfaceKhr_functions.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        DestroySurfaceKHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkSurfaceKhr_Functions::DestroySurfaceKHR(
        VkInstance instance,
        VkSurfaceKHR surface,
        const VkAllocationCallbacks* pAllocator )
    {
        auto& id = InstanceDispatch.Get( instance );

        // Remove surface entry
        id.Instance.Surfaces.erase( surface );

        id.Instance.Callbacks.DestroySurfaceKHR(
            instance, surface, pAllocator );
    }
}

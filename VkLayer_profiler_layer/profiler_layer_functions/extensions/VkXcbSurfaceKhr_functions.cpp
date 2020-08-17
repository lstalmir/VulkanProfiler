#include "VkXcbSurfaceKhr_functions.h"

namespace Profiler
{
    #ifdef VK_USE_PLATFORM_XCB_KHR
    /***********************************************************************************\

    Function:
        CreateXcbSurfaceKHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkXcbSurfaceKhr_Functions::CreateXcbSurfaceKHR(
        VkInstance instance,
        const VkXcbSurfaceCreateInfoKHR* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkSurfaceKHR* pSurface )
    {
        auto& id = InstanceDispatch.Get( instance );

        VkResult result = id.Instance.Callbacks.CreateXcbSurfaceKHR(
            instance, pCreateInfo, pAllocator, pSurface );

        if( result == VK_SUCCESS )
        {
            VkSurfaceKhr_Object surfaceObject = {};
            surfaceObject.Handle = *pSurface;
            surfaceObject.Window = pCreateInfo->window;

            id.Instance.Surfaces.emplace( *pSurface, surfaceObject );
        }

        return result;
    }
    #endif
}

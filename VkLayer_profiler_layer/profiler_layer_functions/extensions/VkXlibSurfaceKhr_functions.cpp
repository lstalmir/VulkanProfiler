#include "VkXlibSurfaceKhr_functions.h"

namespace Profiler
{
    #ifdef VK_USE_PLATFORM_XLIB_KHR
    /***********************************************************************************\

    Function:
        CreateXlibSurfaceKHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkXlibSurfaceKhr_Functions::CreateXlibSurfaceKHR(
        VkInstance instance,
        const VkXlibSurfaceCreateInfoKHR* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkSurfaceKHR* pSurface )
    {
        auto& id = InstanceDispatch.Get( instance );

        VkResult result = id.Instance.Callbacks.CreateXlibSurfaceKHR(
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
    #endif // VK_USE_PLATFORM_XLIB_KHR
}

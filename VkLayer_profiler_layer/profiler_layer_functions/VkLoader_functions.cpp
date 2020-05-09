#include "VkLoader_functions.h"
#include "Dispatch.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        SetInstanceLoaderData

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkLoader_Functions::SetInstanceLoaderData(
        VkInstance instance,
        void* pObject )
    {
        CHECKPROCSIGNATURE( SetInstanceLoaderData );

        // Copy dispatch pointer
        (*(void**)pObject) = (*(void**)instance);

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        SetDeviceLoaderData

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkLoader_Functions::SetDeviceLoaderData(
        VkDevice device,
        void* pObject )
    {
        CHECKPROCSIGNATURE( SetDeviceLoaderData );

        // Copy dispatch pointer
        (*(void**)pObject) = (*(void**)device);

        return VK_SUCCESS;
    }
}

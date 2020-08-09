#include "VkLoader_functions.h"
#include "Dispatch.h"

#ifdef WIN32
#include <Windows.h>
#endif

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

    /***********************************************************************************\

    Function:
        EnumerateInstanceVersion

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkLoader_Functions::EnumerateInstanceVersion(
        uint32_t* pVersion )
    {
        #ifdef WIN32
        // Should be loaded by the application
        HMODULE hLoaderModule = GetModuleHandleA( "vulkan-1.dll" );

        PFN_vkEnumerateInstanceVersion pfnEnumerateInstanceVersion =
            (PFN_vkEnumerateInstanceVersion)GetProcAddress( hLoaderModule, "vkEnumerateInstanceVersion" );

        return pfnEnumerateInstanceVersion( pVersion );
        #else
        // Assume lowest version
        *pVersion = VK_API_VERSION_1_0;

        return VK_SUCCESS;
        #endif
    }
}

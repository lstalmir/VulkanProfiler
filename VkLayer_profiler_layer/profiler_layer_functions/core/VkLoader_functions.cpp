// Copyright (c) 2020 Lukasz Stalmirski
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

#include "VkLoader_functions.h"
#include "profiler_layer_functions/Dispatch.h"

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

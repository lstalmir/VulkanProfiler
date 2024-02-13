// Copyright (c) 2019-2021 Lukasz Stalmirski
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

        // Track host memory operations
        auto pProfilerAllocator = id.Instance.HostMemoryProfiler.CreateAllocator(
            pAllocator,
            __FUNCTION__,
            VK_OBJECT_TYPE_SURFACE_KHR );

        pAllocator = pProfilerAllocator.get();

        VkResult result = id.Instance.Callbacks.CreateXcbSurfaceKHR(
            instance, pCreateInfo, pAllocator, pSurface );

        if( result == VK_SUCCESS )
        {
            id.Instance.HostMemoryProfiler.BindAllocator( *pSurface, pProfilerAllocator );

            VkSurfaceKhr_Object surfaceObject = {};
            surfaceObject.Handle = *pSurface;
            surfaceObject.Window = pCreateInfo->window;

            id.Instance.Surfaces.emplace( *pSurface, surfaceObject );
        }

        return result;
    }
    #endif
}

// Copyright (c) 2019-2025 Lukasz Stalmirski
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

#include "VkWaylandSurfaceKhr_functions.h"

namespace Profiler
{
    #ifdef VK_USE_PLATFORM_WAYLAND_KHR
    /***********************************************************************************\

    Function:
        CreateXcbSurfaceKHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkWaylandSurfaceKhr_Functions::CreateWaylandSurfaceKHR(
        VkInstance instance,
        const VkWaylandSurfaceCreateInfoKHR* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkSurfaceKHR* pSurface )
    {
        auto& id = InstanceDispatch.Get( instance );

        VkResult result = id.Instance.Callbacks.CreateWaylandSurfaceKHR(
            instance, pCreateInfo, pAllocator, pSurface );

        if( result == VK_SUCCESS )
        {
            VkSurfaceKhr_Object surfaceObject = {};
            surfaceObject.Handle = *pSurface;
            surfaceObject.Window = pCreateInfo->surface;

            id.Instance.Surfaces.emplace( *pSurface, surfaceObject );
        }

        return result;
    }
    #endif // VK_USE_PLATFORM_WAYLAND_KHR
}

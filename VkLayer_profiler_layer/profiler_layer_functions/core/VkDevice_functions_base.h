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

#pragma once
#include "profiler/profiler.h"
#include "profiler_overlay/profiler_overlay.h"
#include "profiler_standalone/profiler_standalone_server.h"
#include "profiler_layer_objects/VkDevice_object.h"
#include "profiler_layer_functions/Dispatch.h"
#include <vulkan/vk_layer.h>

namespace Profiler
{
    /***********************************************************************************\

    Structure:
        VkDevice_Functions_Base

    Description:
        Base for all components of VkDevice containing functions, which will be profiled.
        Manages Profiler object for the device.

        Functions OnDeviceCreate and OnDeviceDestroy should be called once for each
        device created.

    \***********************************************************************************/
    struct VkDevice_Functions_Base
    {
        struct Dispatch
        {
            VkDevice_Object Device;
            DeviceProfiler Profiler;

            ProfilerOverlayOutput Overlay;
            NetworkServer Server;
        };

        static DispatchableMap<Dispatch> DeviceDispatch;

        // Invoked on vkCreateDevice
        static VkResult CreateDeviceBase(
            VkPhysicalDevice physicalDevice,
            const VkDeviceCreateInfo* pCreateInfo,
            PFN_vkGetDeviceProcAddr pfnGetDeviceProcAddr,
            PFN_vkSetDeviceLoaderData pfnSetDeviceLoaderData,
            const VkAllocationCallbacks* pAllocator,
            VkDevice device );

        // Invoked on vkDestroyDevice
        static void DestroyDeviceBase( 
            VkDevice device );
    };
}

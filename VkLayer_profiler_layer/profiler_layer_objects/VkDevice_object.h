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
#include "vk_dispatch_tables.h"
#include "VkInstance_object.h"
#include "VkPhysicalDevice_object.h"
#include "VkQueue_object.h"
#include "VkSwapchainKhr_object.h"
#include "VkObject.h"
#include <map>
#include <vector>

#include "../profiler/profiler_counters.h"
#include "../utils/lockable_unordered_map.h"

namespace Profiler
{
    struct VkDevice_debug_Object
    {
        ConcurrentMap<VkObject, std::string> ObjectNames;
    };

    struct VkDevice_Object
    {
        VkDevice Handle;

        VkInstance_Object* pInstance;
        VkPhysicalDevice_Object* pPhysicalDevice;

        // Time in profiler
        TipCounter TIP;

        // Dispatch tables
        VkLayerDeviceDispatchTable Callbacks;
        PFN_vkSetDeviceLoaderData SetDeviceLoaderData;

        VkDevice_debug_Object Debug;

        std::unordered_map<VkQueue, VkQueue_Object> Queues;

        // Enabled extensions
        std::unordered_set<std::string> EnabledExtensions;

        // Swapchains created with this device
        std::unordered_map<VkSwapchainKHR, VkSwapchainKhr_Object> Swapchains;
    };
}

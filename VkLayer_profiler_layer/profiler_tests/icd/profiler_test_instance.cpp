// Copyright (c) 2024 Lukasz Stalmirski
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

#include "profiler_test_instance.h"
#include "profiler_test_physical_device.h"
#include "profiler_test_surface.h"
#include "profiler_test_icd_helpers.h"

namespace Profiler::ICD
{
    Instance::Instance( const VkInstanceCreateInfo& createInfo )
        : m_PhysicalDevice( new VkPhysicalDevice_T( new PhysicalDevice() ) )
    {
    }

    Instance::~Instance()
    {
        delete m_PhysicalDevice;
    }

    void Instance::vkDestroyInstance( const VkAllocationCallbacks* pAllocator )
    {
        delete this;
    }

    VkResult Instance::vkEnumeratePhysicalDevices( uint32_t* pPhysicalDeviceCount, VkPhysicalDevice* pPhysicalDevices )
    {
        if( pPhysicalDevices == nullptr )
        {
            *pPhysicalDeviceCount = 1;
            return VK_SUCCESS;
        }

        if( *pPhysicalDeviceCount < 1 )
        {
            return VK_INCOMPLETE;
        }

        *pPhysicalDevices = m_PhysicalDevice;
        return VK_SUCCESS;
    }

#ifdef VK_KHR_win32_surface
    VkResult Instance::vkCreateWin32SurfaceKHR( const VkWin32SurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface )
    {
        return vk_new_nondispatchable<VkSurfaceKHR_T>( pSurface );
    }
#endif

#ifdef VK_KHR_surface
    void Instance::vkDestroySurfaceKHR( VkSurfaceKHR surface, const VkAllocationCallbacks* pAllocator )
    {
        delete surface;
    }
#endif
}

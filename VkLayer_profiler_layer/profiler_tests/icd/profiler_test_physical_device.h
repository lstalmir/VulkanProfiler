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

#pragma once
#include "profiler_test_icd_base.h"

namespace Profiler::ICD
{
    struct PhysicalDevice : PhysicalDeviceBase
    {
        PhysicalDevice();
        ~PhysicalDevice();

        VkResult vkEnumerateDeviceExtensionProperties( const char* pLayerName, uint32_t* pPropertyCount, VkExtensionProperties* pProperties ) override;

        void vkGetPhysicalDeviceProperties( VkPhysicalDeviceProperties* pProperties ) override;
        void vkGetPhysicalDeviceProperties2( VkPhysicalDeviceProperties2* pProperties ) override;
        void vkGetPhysicalDeviceFeatures( VkPhysicalDeviceFeatures* pFeatures ) override;
        void vkGetPhysicalDeviceFeatures2( VkPhysicalDeviceFeatures2* pFeatures ) override;
        void vkGetPhysicalDeviceMemoryProperties( VkPhysicalDeviceMemoryProperties* pMemoryProperties ) override;
        void vkGetPhysicalDeviceQueueFamilyProperties( uint32_t* pQueueFamilyPropertyCount, VkQueueFamilyProperties* pQueueFamilyProperties ) override;

#ifdef VK_KHR_win32_surface
        VkBool32 vkGetPhysicalDeviceWin32PresentationSupportKHR( uint32_t queueFamilyIndex ) override;
#endif

#ifdef VK_KHR_surface
        VkResult vkGetPhysicalDeviceSurfaceCapabilitiesKHR( VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR* pSurfaceCapabilities ) override;
        VkResult vkGetPhysicalDeviceSurfaceFormatsKHR( VkSurfaceKHR surface, uint32_t* pSurfaceFormatCount, VkSurfaceFormatKHR* pSurfaceFormats ) override;
        VkResult vkGetPhysicalDeviceSurfacePresentModesKHR( VkSurfaceKHR surface, uint32_t* pPresentModeCount, VkPresentModeKHR* pPresentModes ) override;
        VkResult vkGetPhysicalDeviceSurfaceSupportKHR( uint32_t queueFamilyIndex, VkSurfaceKHR surface, VkBool32* pSupported ) override;
#endif

        VkResult vkCreateDevice( const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice ) override;
    };
}

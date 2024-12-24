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

#include "profiler_test_physical_device.h"
#include "profiler_test_device.h"
#include "profiler_test_icd_helpers.h"

namespace Profiler::ICD
{
    PhysicalDevice::PhysicalDevice()
    {
    }

    PhysicalDevice::~PhysicalDevice()
    {
    }

    VkResult PhysicalDevice::vkEnumerateDeviceExtensionProperties( const char* pLayerName, uint32_t* pPropertyCount, VkExtensionProperties* pProperties )
    {
        const VkExtensionProperties properties[] = {
#ifdef VK_KHR_swapchain
            { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_SWAPCHAIN_SPEC_VERSION },
#endif
        };

        const uint32_t propertyCount = std::size( properties );

        if( !pProperties )
        {
            *pPropertyCount = propertyCount;
            return VK_SUCCESS;
        }

        const uint32_t count = std::min( *pPropertyCount, propertyCount );
        for( uint32_t i = 0; i < count; ++i )
        {
            pProperties[ i ] = properties[ i ];
        }

        *pPropertyCount = count;

        if( count < propertyCount )
        {
            return VK_INCOMPLETE;
        }

        return VK_SUCCESS;
    }

    void PhysicalDevice::vkGetPhysicalDeviceProperties( VkPhysicalDeviceProperties* pProperties )
    {
        memset( pProperties, 0, sizeof( VkPhysicalDeviceProperties ) );
        pProperties->apiVersion = VK_API_VERSION_1_3;
        pProperties->driverVersion = 0;
        pProperties->vendorID = 0;
        pProperties->deviceID = 0;
        pProperties->deviceType = VK_PHYSICAL_DEVICE_TYPE_OTHER;

        pProperties->limits.maxImageDimension1D = 4096;
        pProperties->limits.maxImageDimension2D = 4096;
        pProperties->limits.maxImageDimension3D = 1;
        pProperties->limits.maxImageDimensionCube = 6;
        pProperties->limits.maxImageArrayLayers = 4;
        pProperties->limits.maxTexelBufferElements = 65536;
        pProperties->limits.maxUniformBufferRange = 65536;
        pProperties->limits.maxStorageBufferRange = 65536;
        pProperties->limits.maxPushConstantsSize = 256;
        pProperties->limits.maxMemoryAllocationCount = 4096;
        pProperties->limits.maxSamplerAllocationCount = 64;
    }

    void PhysicalDevice::vkGetPhysicalDeviceProperties2( VkPhysicalDeviceProperties2* pProperties )
    {
        vkGetPhysicalDeviceProperties( &pProperties->properties );
    }

    void PhysicalDevice::vkGetPhysicalDeviceMemoryProperties( VkPhysicalDeviceMemoryProperties* pMemoryProperties )
    {
        memset( pMemoryProperties, 0, sizeof( VkPhysicalDeviceMemoryProperties ) );
        pMemoryProperties->memoryTypeCount = 1;
        pMemoryProperties->memoryTypes[ 0 ].propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        pMemoryProperties->memoryTypes[ 0 ].heapIndex = 0;
        pMemoryProperties->memoryHeapCount = 1;
        pMemoryProperties->memoryHeaps[ 0 ].size = 128 * 1024 * 1024;
        pMemoryProperties->memoryHeaps[ 0 ].flags = VK_MEMORY_HEAP_DEVICE_LOCAL_BIT;
    }

    void PhysicalDevice::vkGetPhysicalDeviceQueueFamilyProperties( uint32_t* pQueueFamilyPropertyCount, VkQueueFamilyProperties* pQueueFamilyProperties )
    {
        if( !pQueueFamilyProperties )
        {
            *pQueueFamilyPropertyCount = 1;
            return;
        }

        if( *pQueueFamilyPropertyCount > 0 )
        {
            *pQueueFamilyPropertyCount = 1;
            pQueueFamilyProperties->queueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
            pQueueFamilyProperties->queueCount = 1;
            pQueueFamilyProperties->timestampValidBits = 64;
        }
    }

#ifdef VK_KHR_win32_surface
    VkBool32 PhysicalDevice::vkGetPhysicalDeviceWin32PresentationSupportKHR( uint32_t queueFamilyIndex )
    {
        return VK_TRUE;
    }
#endif

#ifdef VK_KHR_surface
    VkResult PhysicalDevice::vkGetPhysicalDeviceSurfaceCapabilitiesKHR( VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR* pSurfaceCapabilities )
    {
        memset( pSurfaceCapabilities, 0, sizeof( VkSurfaceCapabilitiesKHR ) );
        pSurfaceCapabilities->minImageCount = 1;
        pSurfaceCapabilities->maxImageCount = 1;
        pSurfaceCapabilities->currentExtent.width = 1024;
        pSurfaceCapabilities->currentExtent.height = 768;
        pSurfaceCapabilities->minImageExtent.width = 1;
        pSurfaceCapabilities->minImageExtent.height = 1;
        pSurfaceCapabilities->maxImageExtent.width = 4096;
        pSurfaceCapabilities->maxImageExtent.height = 4096;
        pSurfaceCapabilities->maxImageArrayLayers = 1;
        pSurfaceCapabilities->supportedTransforms = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        pSurfaceCapabilities->currentTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        pSurfaceCapabilities->supportedCompositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        pSurfaceCapabilities->supportedUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        return VK_SUCCESS;
    }

    VkResult PhysicalDevice::vkGetPhysicalDeviceSurfaceFormatsKHR( VkSurfaceKHR surface, uint32_t* pSurfaceFormatCount, VkSurfaceFormatKHR* pSurfaceFormats )
    {
        if( !pSurfaceFormats )
        {
            *pSurfaceFormatCount = 1;
            return VK_SUCCESS;
        }

        if( *pSurfaceFormatCount < 1 )
        {
            return VK_INCOMPLETE;
        }

        pSurfaceFormats[ 0 ].format = VK_FORMAT_B8G8R8A8_UNORM;
        pSurfaceFormats[ 0 ].colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        *pSurfaceFormatCount = 1;
        return VK_SUCCESS;
    }

    VkResult PhysicalDevice::vkGetPhysicalDeviceSurfacePresentModesKHR( VkSurfaceKHR surface, uint32_t* pPresentModeCount, VkPresentModeKHR* pPresentModes )
    {
        if( !pPresentModes )
        {
            *pPresentModeCount = 1;
            return VK_SUCCESS;
        }

        if( *pPresentModeCount < 1 )
        {
            return VK_INCOMPLETE;
        }

        pPresentModes[ 0 ] = VK_PRESENT_MODE_FIFO_KHR;
        *pPresentModeCount = 1;
        return VK_SUCCESS;
    }

    VkResult PhysicalDevice::vkGetPhysicalDeviceSurfaceSupportKHR( uint32_t queueFamilyIndex, VkSurfaceKHR surface, VkBool32* pSupported )
    {
        *pSupported = VK_TRUE;
        return VK_SUCCESS;
    }
#endif

    VkResult PhysicalDevice::vkCreateDevice( const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice )
    {
        return vk_new<Device>( pDevice, *this, *pCreateInfo );
    }
}

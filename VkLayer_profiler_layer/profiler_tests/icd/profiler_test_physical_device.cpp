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
        m_Properties.apiVersion = VK_API_VERSION_1_3;
        m_Properties.driverVersion = 0;
        m_Properties.vendorID = 0;
        m_Properties.deviceID = 0;
        m_Properties.deviceType = VK_PHYSICAL_DEVICE_TYPE_OTHER;

        m_QueueFamilyProperties.queueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
        m_QueueFamilyProperties.queueCount = 1;
        m_QueueFamilyProperties.timestampValidBits = 64;
    }

    PhysicalDevice::~PhysicalDevice()
    {
    }

    void PhysicalDevice::vkGetPhysicalDeviceProperties( VkPhysicalDeviceProperties* pProperties )
    {
        *pProperties = m_Properties;
    }

    void PhysicalDevice::vkGetPhysicalDeviceProperties2( VkPhysicalDeviceProperties2* pProperties )
    {
        vkGetPhysicalDeviceProperties( &pProperties->properties );
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
            pQueueFamilyProperties[ 0 ] = m_QueueFamilyProperties;
        }
    }

    VkResult PhysicalDevice::vkCreateDevice( const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice )
    {
        return vk_new<Device>( pDevice, *this, *pCreateInfo );
    }
}

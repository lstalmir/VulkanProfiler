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

#include "profiler_test_device.h"
#include "profiler_test_physical_device.h"
#include "profiler_test_query_pool.h"
#include "profiler_test_queue.h"
#include "profiler_test_command_buffer.h"
#include "profiler_test_icd_helpers.h"

namespace Profiler::ICD
{
    Device::Device( PhysicalDevice& physicalDevice, const VkDeviceCreateInfo& createInfo )
        : m_PhysicalDevice( physicalDevice )
        , m_Queue( nullptr )
    {
        if( createInfo.queueCreateInfoCount > 0 )
        {
            vk_check( vk_new<Queue>(
                &m_Queue, *this, createInfo.pQueueCreateInfos[ 0 ] ) );
        }
    }

    Device::~Device()
    {
        delete m_Queue;
    }

    void Device::vkDestroyDevice( const VkAllocationCallbacks* pAllocator )
    {
        delete this;
    }

    void Device::vkGetDeviceQueue( uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue* pQueue )
    {
        *pQueue = m_Queue;
    }

    void Device::vkGetDeviceQueue2( const VkDeviceQueueInfo2* pQueueInfo, VkQueue* pQueue )
    {
        *pQueue = m_Queue;
    }

    VkResult Device::vkCreateQueryPool( const VkQueryPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkQueryPool* pQueryPool )
    {
        return vk_new_nondispatchable<VkQueryPool_T>( pQueryPool, *pCreateInfo );
    }

    void Device::vkDestroyQueryPool( VkQueryPool queryPool, const VkAllocationCallbacks* pAllocator )
    {
        delete queryPool;
    }

    VkResult Device::vkAllocateCommandBuffers( const VkCommandBufferAllocateInfo* pAllocateInfo, VkCommandBuffer* pCommandBuffers )
    {
        VkResult result = VK_SUCCESS;

        for( uint32_t i = 0; i < pAllocateInfo->commandBufferCount; ++i )
        {
            result = vk_new<CommandBuffer>( &pCommandBuffers[ i ] );

            if( result != VK_SUCCESS )
            {
                vkFreeCommandBuffers( pAllocateInfo->commandPool, i, pCommandBuffers );
                break;
            }
        }

        return result;
    }

    void Device::vkFreeCommandBuffers( VkCommandPool commandPool, uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers )
    {
        for( uint32_t i = 0; i < commandBufferCount; ++i )
        {
            delete pCommandBuffers[ i ];
        }
    }

#ifdef VK_KHR_swapchain
    VkResult Device::vkAcquireNextImageKHR( VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t* pImageIndex )
    {
        *pImageIndex = 0;
        return VK_SUCCESS;
    }
#endif
}

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

#include "profiler_sync.h"
#include "profiler_helpers.h"
#include <assert.h>

namespace Profiler
{
    /***********************************************************************************\

    Function:
        DeviceProfilerSynchronization

    Description:
        Constructor.

    \***********************************************************************************/
    DeviceProfilerSynchronization::DeviceProfilerSynchronization()
        : m_pDevice( nullptr )
        , m_CommandPools()
        , m_CommandBuffers()
        , m_TimestampQueryPoolSize( 0 )
        , m_TimestampQueryPool( VK_NULL_HANDLE )
        , m_TimestampQueryPoolResetCommandBuffer( VK_NULL_HANDLE )
        , m_TimestampQueryPoolResetSemaphore( VK_NULL_HANDLE )
        , m_TimestampQueryPoolResetQueue( VK_NULL_HANDLE )
        , m_SynchronizationTimestampsSent( false )
    {
    }

    /***********************************************************************************\

    Function:
        Initialize

    Description:
        Setup resources needed for device synchronization.

    \***********************************************************************************/
    VkResult DeviceProfilerSynchronization::Initialize( VkDevice_Object* pDevice )
    {
        assert( !m_pDevice );
        m_pDevice = pDevice;

        m_TimestampQueryPoolSize = static_cast<uint32_t>(m_pDevice->Queues.size());

        // Create a command pool for each queue family
        VkCommandPoolCreateInfo commandPoolCreateInfo = {};
        commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

        for( const auto& queue : m_pDevice->Queues )
        {
            if( !m_CommandPools.count( queue.second.Family ) )
            {
                commandPoolCreateInfo.queueFamilyIndex = queue.second.Family;

                VkCommandPool commandPool = VK_NULL_HANDLE;
                DESTROYANDRETURNONFAIL( m_pDevice->Callbacks.CreateCommandPool(
                    m_pDevice->Handle, &commandPoolCreateInfo, nullptr, &commandPool ) );

                m_CommandPools.insert( std::pair( queue.second.Family, commandPool ) );
            }
        }

        // Create command buffer for each queue
        VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocateInfo.commandBufferCount = 1;

        for( const auto& queue : m_pDevice->Queues )
        {
            // Get command pool created in the previous step
            commandBufferAllocateInfo.commandPool = m_CommandPools.at( queue.second.Family );

            VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
            DESTROYANDRETURNONFAIL( m_pDevice->Callbacks.AllocateCommandBuffers(
                m_pDevice->Handle, &commandBufferAllocateInfo, &commandBuffer ) );

            // Created object must be stored to ensure it is destroyed when next call fails.
            m_CommandBuffers.insert( std::pair( queue.second.Handle, commandBuffer ) );

            DESTROYANDRETURNONFAIL( m_pDevice->SetDeviceLoaderData(
                m_pDevice->Handle, commandBuffer ) );
        }

        // Create query pool for timestamp queries
        VkQueryPoolCreateInfo queryPoolCreateInfo = {};
        queryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        queryPoolCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
        queryPoolCreateInfo.queryCount = m_TimestampQueryPoolSize;
        
        DESTROYANDRETURNONFAIL( m_pDevice->Callbacks.CreateQueryPool(
            m_pDevice->Handle, &queryPoolCreateInfo, nullptr, &m_TimestampQueryPool ) );

        // Select queue used for resetting the query pool
        commandBufferAllocateInfo.commandPool = VK_NULL_HANDLE;

        for( const auto& queue : m_pDevice->Queues )
        {
            if( (queue.second.Flags & VK_QUEUE_GRAPHICS_BIT) ||
                (queue.second.Flags & VK_QUEUE_COMPUTE_BIT) )
            {
                m_TimestampQueryPoolResetQueue = queue.second.Handle;
                // Use command pool created for the queue family
                commandBufferAllocateInfo.commandPool = m_CommandPools.at( queue.second.Family );
                break;
            }
        }

        if( commandBufferAllocateInfo.commandPool == VK_NULL_HANDLE )
        {
            // Application didn't create any graphics/compute queues (?!)
            DESTROYANDRETURNONFAIL( VK_ERROR_INITIALIZATION_FAILED );
        }

        // Create prerecorded command buffer resetting the query pool
        DESTROYANDRETURNONFAIL( m_pDevice->Callbacks.AllocateCommandBuffers(
            m_pDevice->Handle, &commandBufferAllocateInfo, &m_TimestampQueryPoolResetCommandBuffer ) );

        DESTROYANDRETURNONFAIL( RecordTimestmapQueryCommandBuffers() );

        // Create semaphore for synchronization between queues
        VkSemaphoreCreateInfo semaphoreCreateInfo = {};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        
        DESTROYANDRETURNONFAIL( m_pDevice->Callbacks.CreateSemaphore(
            m_pDevice->Handle, &semaphoreCreateInfo, nullptr, &m_TimestampQueryPoolResetSemaphore ) );

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:
        Cleanup internal resources.

    \***********************************************************************************/
    void DeviceProfilerSynchronization::Destroy()
    {
        // Destroy semaphore
        if( m_TimestampQueryPoolResetSemaphore != VK_NULL_HANDLE )
        {
            m_pDevice->Callbacks.DestroySemaphore( m_pDevice->Handle, m_TimestampQueryPoolResetSemaphore, nullptr );
            m_TimestampQueryPoolResetSemaphore = VK_NULL_HANDLE;
        }

        // Command buffer will be destroyed with command pool
        m_TimestampQueryPoolResetCommandBuffer = VK_NULL_HANDLE;
        m_TimestampQueryPoolResetQueue = VK_NULL_HANDLE;

        // Destroy timestamp query pool
        if( m_TimestampQueryPool != VK_NULL_HANDLE )
        {
            m_pDevice->Callbacks.DestroyQueryPool( m_pDevice->Handle, m_TimestampQueryPool, nullptr );
            m_TimestampQueryPool = VK_NULL_HANDLE;
        }

        // Command buffers will be destroyed with command pools
        m_CommandBuffers.clear();

        // Destroy command pools
        for( auto& commandPool : m_CommandPools )
        {
            if( commandPool.second != VK_NULL_HANDLE )
            {
                m_pDevice->Callbacks.DestroyCommandPool( m_pDevice->Handle, commandPool.second, nullptr );
            }
        }
        m_CommandPools.clear();

        m_TimestampQueryPoolSize = 0;
        m_pDevice = nullptr;
    }

    /***********************************************************************************\

    Function:
        WaitForDevice

    Description:
        Synchronize CPU and GPU. Wait until there are no tasks executing on the device.

    \***********************************************************************************/
    void DeviceProfilerSynchronization::WaitForDevice()
    {
        assert( m_pDevice );
        m_pDevice->Callbacks.DeviceWaitIdle( m_pDevice->Handle );
    }

    /***********************************************************************************\

    Function:
        WaitForQueue

    Description:
        Synchronize CPU and GPU. Wait until there are no tasks executing on the queue.

    \***********************************************************************************/
    void DeviceProfilerSynchronization::WaitForQueue( VkQueue queue )
    {
        assert( m_pDevice );
        m_pDevice->Callbacks.QueueWaitIdle( queue );
    }

    /***********************************************************************************\

    Function:
        WaitForFences

    Description:
        Synchronize CPU and GPU. Wait until there are no tasks executing on the queue.

    \***********************************************************************************/
    void DeviceProfilerSynchronization::WaitForFence( VkFence fence, uint64_t timeout )
    {
        assert( m_pDevice );
        m_pDevice->Callbacks.WaitForFences( m_pDevice->Handle, 1, &fence, false, timeout );
    }

    /***********************************************************************************\

    Function:
        SendSynchronizationTimestamps

    Description:
        Submit command buffers with synchronization timestamps to all queues.
        Device must be idle.

    \***********************************************************************************/
    void DeviceProfilerSynchronization::SendSynchronizationTimestamps()
    {
        assert( m_pDevice );

        // Reset query pool
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_TimestampQueryPoolResetCommandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &m_TimestampQueryPoolResetSemaphore;
        
        m_pDevice->Callbacks.QueueSubmit(
            m_TimestampQueryPoolResetQueue, 1, &submitInfo, VK_NULL_HANDLE );

        // Insert timestamps when the pool is ready
        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &m_TimestampQueryPoolResetSemaphore;
        submitInfo.pWaitDstStageMask = &waitStage;
        submitInfo.signalSemaphoreCount = 0;
        submitInfo.pSignalSemaphores = nullptr;

        for( const auto& commandBuffer : m_CommandBuffers )
        {
            submitInfo.pCommandBuffers = &commandBuffer.second;
            m_pDevice->Callbacks.QueueSubmit(
                m_TimestampQueryPoolResetQueue, 1, &submitInfo, VK_NULL_HANDLE );
        }

        m_SynchronizationTimestampsSent = true;
    }

    /***********************************************************************************\

    Function:
        GetSynchronizationTimestamps

    Description:
        Retrieve timestamps submitted in SendSynchronizationTimestamps.
        Device must be idle.

    \***********************************************************************************/
    std::unordered_map<VkQueue, uint64_t> DeviceProfilerSynchronization::GetSynchronizationTimestamps() const
    {
        std::unordered_map<VkQueue, uint64_t> perQueueTimestamps;

        if( m_SynchronizationTimestampsSent )
        {
            // Collect pending queries
            std::vector<uint64_t> timestamps( m_TimestampQueryPoolSize );
            m_pDevice->Callbacks.GetQueryPoolResults(
                m_pDevice->Handle, m_TimestampQueryPool, 0, m_TimestampQueryPoolSize,
                sizeof( uint64_t ) * timestamps.size(), timestamps.data(), sizeof( uint64_t ),
                VK_QUERY_RESULT_64_BIT );

            // Assign queries to the queues
            size_t currentTimestampIndex = 0;

            for( const auto& commandBuffer : m_CommandBuffers )
            {
                perQueueTimestamps.insert( std::pair( commandBuffer.first, timestamps[ currentTimestampIndex ] ) );
                currentTimestampIndex++;
            }
        }
        else
        {
            // Synchronization timestamps not sent, return zeros
            for( const auto& commandBuffer : m_CommandBuffers )
            {
                perQueueTimestamps.insert( std::pair( commandBuffer.first, 0 ) );
            }
        }

        return perQueueTimestamps;
    }

    /***********************************************************************************\

    Function:
        RecordTimestmapQueryCommandBuffers

    Description:

    \***********************************************************************************/
    VkResult DeviceProfilerSynchronization::RecordTimestmapQueryCommandBuffers()
    {
        // Record query reset command buffer
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        
        RETURNONFAIL( m_pDevice->Callbacks.BeginCommandBuffer(
            m_TimestampQueryPoolResetCommandBuffer, &beginInfo ) );

        m_pDevice->Callbacks.CmdResetQueryPool(
            m_TimestampQueryPoolResetCommandBuffer,
            m_TimestampQueryPool,
            0, m_TimestampQueryPoolSize );

        RETURNONFAIL( m_pDevice->Callbacks.EndCommandBuffer(
            m_TimestampQueryPoolResetCommandBuffer ) );

        // Record synchronization command buffers
        uint32_t currentTimestampIndex = 0;

        for( const auto& commandBuffer : m_CommandBuffers )
        {
            RETURNONFAIL( m_pDevice->Callbacks.BeginCommandBuffer(
                commandBuffer.second, &beginInfo ) );

            m_pDevice->Callbacks.CmdWriteTimestamp(
                commandBuffer.second,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                m_TimestampQueryPool,
                currentTimestampIndex );

            RETURNONFAIL( m_pDevice->Callbacks.EndCommandBuffer(
                commandBuffer.second ) );

            currentTimestampIndex++;
        }

        return VK_SUCCESS;
    }
}

#include "profiler_overlay.h"
#include "profiler.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        ProfilerOverlay

    Description:
        Constructor

    \***********************************************************************************/
    ProfilerOverlay::ProfilerOverlay()
    {
    }

    /***********************************************************************************\

    Function:
        Initialize

    Description:
        Initializes profiler overlay resources.

    \***********************************************************************************/
    VkResult ProfilerOverlay::Initialize( VkDevice device, Profiler* pProfiler, ProfilerCallbacks callbacks )
    {
        m_pProfiler = pProfiler;
        m_Callbacks = callbacks;

        // Create the GPU command pool
        VkCommandPoolCreateInfo commandPoolCreateInfo;
        memset( &commandPoolCreateInfo, 0, sizeof( commandPoolCreateInfo ) );

        commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        commandPoolCreateInfo.queueFamilyIndex = 0; // oh fuck

        VkResult result = m_Callbacks.pfnCreateCommandPool(
            device, &commandPoolCreateInfo, nullptr, &m_CommandPool );

        if( result != VK_SUCCESS )
        {
            // Creation of command pool failed

            // Cleanup the overlay
            Destroy( device );

            return result;
        }

        // Allocate command buffer for drawing stats
        VkCommandBufferAllocateInfo commandBufferAllocateInfo;
        memset( &commandBufferAllocateInfo, 0, sizeof( commandBufferAllocateInfo ) );

        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.commandPool = m_CommandPool;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocateInfo.commandBufferCount = 1;

        result = m_Callbacks.pfnAllocateCommandBuffers(
            device, &commandBufferAllocateInfo, &m_CommandBuffer );

        if( result != VK_SUCCESS )
        {
            // Allocation of command buffer failed

            // Cleanup the overlay
            Destroy( device );

            return result;
        }

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:
        Frees resources allocated by the profiler overlay.

    \***********************************************************************************/
    void ProfilerOverlay::Destroy( VkDevice device )
    {
        // Destroy the GPU command buffer
        if( m_CommandBuffer )
        {
            m_Callbacks.pfnFreeCommandBuffers( device, m_CommandPool, 1, &m_CommandBuffer );

            m_CommandBuffer = VK_NULL_HANDLE;
        }

        // Destroy the GPU command pool
        if( m_CommandPool )
        {
            m_Callbacks.pfnDestroyCommandPool( device, m_CommandPool, nullptr );

            m_CommandPool = VK_NULL_HANDLE;
        }
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:
        Appends internal command buffer with frame stats to the queue.

    \***********************************************************************************/
    void ProfilerOverlay::DrawFrameStats( VkQueue presentQueue )
    {
        VkCommandBufferBeginInfo beginInfo;
        memset( &beginInfo, 0, sizeof( beginInfo ) );

        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        VkResult result = m_Callbacks.pfnBeginCommandBuffer( m_CommandBuffer, &beginInfo );

        if( result != VK_SUCCESS )
        {
            // vkBeginCommandBuffer failed
            return;
        }

        // TODO: Drawcalls here

        result = m_Callbacks.pfnEndCommandBuffer( m_CommandBuffer );

        if( result != VK_SUCCESS )
        {
            // vkEndCommandBuffer failed
            return;
        }

        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;

        VkSubmitInfo submitInfo;
        memset( &submitInfo, 0, sizeof( submitInfo ) );

        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_CommandBuffer;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = VK_NULL_HANDLE; // TODO
        submitInfo.pWaitDstStageMask = &waitStage;

        // Uncomment below when pWaitSemaphores is valid handle

       // result = m_Callbacks.pfnQueueSubmit( presentQueue, 1, &submitInfo, VK_NULL_HANDLE );

        if( result != VK_SUCCESS )
        {
            // vkQueueSubmit failed
            return;
        }
    }
}

#include "profiler_overlay.h"
#include "profiler_overlay_state_factory.h"
#include "profiler_helpers.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        ProfilerOverlay

    Description:
        Constructor

    \***********************************************************************************/
    ProfilerOverlay::ProfilerOverlay()
        : m_pProfiler( nullptr )
        , m_Callbacks()
        , m_GraphicsQueue()
        , m_CommandPool( VK_NULL_HANDLE )
        , m_CommandBuffer( VK_NULL_HANDLE )
        , m_DrawStatsRenderPass( VK_NULL_HANDLE )
        , m_DrawStatsShaderModule( VK_NULL_HANDLE )
        , m_DrawStatsPipelineLayout( VK_NULL_HANDLE )
        , m_DrawStatsPipeline( VK_NULL_HANDLE )
    {
        memset( &m_Callbacks, 0, sizeof( m_Callbacks ) );
        memset( &m_GraphicsQueue, 0, sizeof( m_GraphicsQueue ) );
    }

    /***********************************************************************************\

    Function:
        Initialize

    Description:
        Initializes profiler overlay resources.

    \***********************************************************************************/
    VkResult ProfilerOverlay::Initialize( VkDevice_Object* pDevice, Profiler* pProfiler, ProfilerCallbacks callbacks )
    {
        // Helper macro for rolling-back to valid state
#       define DESTROYANDRETURNONFAIL( VKRESULT )   \
            {                                       \
                VkResult result = (VKRESULT);       \
                if( result != VK_SUCCESS )          \
                {                                   \
                    Destroy( pDevice->Device );     \
                    return result;                  \
                }                                   \
            }

        m_pProfiler = pProfiler;
        m_Callbacks = callbacks;

        // Alias for GETDEVICEPROCADDR macro
        PFN_vkGetDeviceProcAddr pfnGetDeviceProcAddr = pDevice->pfnGetDeviceProcAddr;

        // Find the graphics queue
        uint32_t queueFamilyCount = 0;

        m_Callbacks.pfnGetPhysicalDeviceQueueFamilyProperties(
            pDevice->PhysicalDevice, &queueFamilyCount, nullptr );
        
        std::vector<VkQueueFamilyProperties> queueFamilyProperties( queueFamilyCount );

        m_Callbacks.pfnGetPhysicalDeviceQueueFamilyProperties(
            pDevice->PhysicalDevice, &queueFamilyCount, queueFamilyProperties.data() );

        // Select queue with highest priority
        float currentQueuePriority = 0.f;

        for( auto queuePair : pDevice->Queues )
        {
            VkQueue_Object queue = queuePair.second;

            // Check if queue is in graphics family
            VkQueueFamilyProperties familyProperties = queueFamilyProperties[queue.FamilyIndex];

            if( (familyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
                (queue.Priority > currentQueuePriority) )
            {
                m_GraphicsQueue = queue;
                currentQueuePriority = queue.Priority;
            }
        }

        // Create the GPU command pool
        VkStructure<VkCommandPoolCreateInfo> commandPoolCreateInfo;

        commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        commandPoolCreateInfo.queueFamilyIndex = m_GraphicsQueue.FamilyIndex;

        DESTROYANDRETURNONFAIL( m_Callbacks.pfnCreateCommandPool(
            pDevice->Device, &commandPoolCreateInfo, nullptr, &m_CommandPool ) );

        // Allocate command buffer for drawing stats
        VkStructure<VkCommandBufferAllocateInfo> commandBufferAllocateInfo;

        commandBufferAllocateInfo.commandPool = m_CommandPool;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocateInfo.commandBufferCount = 1;

        DESTROYANDRETURNONFAIL( m_Callbacks.pfnAllocateCommandBuffers(
            pDevice->Device, &commandBufferAllocateInfo, &m_CommandBuffer ) );

        // Create temporary pipeline state factory
        ProfilerOverlayStateFactory stateFactory( pDevice->Device, callbacks );

        // Create render pass
        DESTROYANDRETURNONFAIL( stateFactory.CreateDrawStatsRenderPass( &m_DrawStatsRenderPass ) );

        // Create pipeline layout
        DESTROYANDRETURNONFAIL( stateFactory.CreateDrawStatsPipelineLayout( &m_DrawStatsPipelineLayout ) );

        // Create shader module
        DESTROYANDRETURNONFAIL( stateFactory.CreateDrawStatsShaderModule( &m_DrawStatsShaderModule ) );

        // Create pipeline
        DESTROYANDRETURNONFAIL( stateFactory.CreateDrawStatsPipeline(
            m_DrawStatsRenderPass,
            m_DrawStatsPipelineLayout,
            m_DrawStatsShaderModule,
            &m_DrawStatsPipeline ) );

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
        // Destroy pipeline
        if( m_DrawStatsPipeline )
        {
            m_Callbacks.pfnDestroyPipeline( device, m_DrawStatsPipeline, nullptr );

            m_DrawStatsPipeline = VK_NULL_HANDLE;
        }

        // Destroy pipeline layout
        if( m_DrawStatsPipelineLayout )
        {
            m_Callbacks.pfnDestroyPipelineLayout( device, m_DrawStatsPipelineLayout, nullptr );

            m_DrawStatsPipelineLayout = VK_NULL_HANDLE;
        }

        // Destroy render pass
        if( m_DrawStatsRenderPass )
        {
            m_Callbacks.pfnDestroyRenderPass( device, m_DrawStatsRenderPass, nullptr );

            m_DrawStatsRenderPass = VK_NULL_HANDLE;
        }

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

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
    {
        ClearMemory( this );
    }

    /***********************************************************************************\

    Function:
        Initialize

    Description:
        Initializes profiler overlay resources.

    \***********************************************************************************/
    VkResult ProfilerOverlay::Initialize( VkDevice_Object* pDevice, Profiler* pProfiler, ProfilerCallbacks callbacks )
    {
        m_pProfiler = pProfiler;
        m_Callbacks = callbacks;
        m_Device = pDevice->Device;

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

        // Create shader modules
        DESTROYANDRETURNONFAIL( stateFactory.CreateDrawStatsShaderModule( &m_DrawStatsVertexShaderModule,
            ProfilerShaderType::profiler_overlay_draw_stats_vert ) );

        DESTROYANDRETURNONFAIL( stateFactory.CreateDrawStatsShaderModule( &m_DrawStatsPixelShaderModule,
            ProfilerShaderType::profiler_overlay_draw_stats_frag ) );

        const std::unordered_map<VkShaderStageFlagBits, VkShaderModule> shaders =
        {
            { VK_SHADER_STAGE_VERTEX_BIT, m_DrawStatsVertexShaderModule },
            { VK_SHADER_STAGE_FRAGMENT_BIT, m_DrawStatsPixelShaderModule }
        };

        // Create pipeline
        DESTROYANDRETURNONFAIL( stateFactory.CreateDrawStatsPipeline(
            m_DrawStatsRenderPass,
            m_DrawStatsPipelineLayout,
            shaders,
            &m_DrawStatsPipeline ) );

        // Load the font
        DESTROYANDRETURNONFAIL( m_OverlayFont.Initialize( pDevice, this, callbacks ) );

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:
        Frees resources allocated by the profiler overlay.

    \***********************************************************************************/
    void ProfilerOverlay::Destroy()
    {
        // Destroy the font
        m_OverlayFont.Destroy();

        if( m_DrawStatsPipeline )
        {
            // Destroy pipeline
            m_Callbacks.pfnDestroyPipeline( m_Device, m_DrawStatsPipeline, nullptr );
        }

        if( m_DrawStatsVertexShaderModule )
        {
            // Destroy vertex shader
            m_Callbacks.pfnDestroyShaderModule( m_Device, m_DrawStatsVertexShaderModule, nullptr );
        }

        if( m_DrawStatsPixelShaderModule )
        {
            // Destroy pixel shader
            m_Callbacks.pfnDestroyShaderModule( m_Device, m_DrawStatsPixelShaderModule, nullptr );
        }

        if( m_DrawStatsPipelineLayout )
        {
            // Destroy pipeline layout
            m_Callbacks.pfnDestroyPipelineLayout( m_Device, m_DrawStatsPipelineLayout, nullptr );
        }

        if( m_DrawStatsRenderPass )
        {
            // Destroy render pass
            m_Callbacks.pfnDestroyRenderPass( m_Device, m_DrawStatsRenderPass, nullptr );
        }

        if( m_CommandBuffer )
        {
            // Destroy the GPU command buffer
            m_Callbacks.pfnFreeCommandBuffers( m_Device, m_CommandPool, 1, &m_CommandBuffer );
        }

        if( m_CommandPool )
        {
            // Destroy the GPU command pool
            m_Callbacks.pfnDestroyCommandPool( m_Device, m_CommandPool, nullptr );
        }

        ClearMemory( this );
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

    /***********************************************************************************\

    Function:
        GetCommandPool

    Description:

    \***********************************************************************************/
    VkCommandPool ProfilerOverlay::GetCommandPool() const
    {
        return m_CommandPool;
    }
}

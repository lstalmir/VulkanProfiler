#include "profiler_overlay.h"
#include "profiler.h"
#include "helpers.h"

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

        // Create render pass for drawing frame stats
        VkStructure<VkAttachmentReference> renderPassColorAttachmentReference;
        renderPassColorAttachmentReference.attachment = 0;
        renderPassColorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkStructure<VkAttachmentDescription> renderPassAttachmentDescription;
        renderPassAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        renderPassAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        renderPassAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        renderPassAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        renderPassAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        renderPassAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        renderPassAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
        renderPassAttachmentDescription.format = VK_FORMAT_UNDEFINED; // TODO format

        VkStructure<VkSubpassDescription> renderPassSubpassDescription;
        renderPassSubpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        renderPassSubpassDescription.colorAttachmentCount = 1;
        renderPassSubpassDescription.pColorAttachments = &renderPassColorAttachmentReference;

        VkStructure<VkSubpassDependency> subpassDependencies[2];
        subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        subpassDependencies[0].dstSubpass = 0;
        subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
        subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        subpassDependencies[1].srcSubpass = 0;
        subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
        subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

        VkStructure<VkRenderPassCreateInfo> renderPassCreateInfo;
        renderPassCreateInfo.subpassCount = 1;
        renderPassCreateInfo.pSubpasses = &renderPassSubpassDescription;
        renderPassCreateInfo.attachmentCount = 1;
        renderPassCreateInfo.pAttachments = &renderPassAttachmentDescription;
        renderPassCreateInfo.dependencyCount = 2;
        renderPassCreateInfo.pDependencies = subpassDependencies;
        
        DESTROYANDRETURNONFAIL( m_Callbacks.pfnCreateRenderPass(
            pDevice->Device, &renderPassCreateInfo, nullptr, &m_DrawStatsRenderPass ) );

        // Create pipeline layout
        VkStructure<VkPipelineLayoutCreateInfo> pipelineLayoutCreateInfo;

        DESTROYANDRETURNONFAIL( m_Callbacks.pfnCreatePipelineLayout(
            pDevice->Device, &pipelineLayoutCreateInfo, nullptr, &m_DrawStatsPipelineLayout ) );

        // TODO create shaders

        // Create pipeline for the render pass
        VkStructure<VkPipelineShaderStageCreateInfo> shaderStageCreateInfo[2];
        shaderStageCreateInfo[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStageCreateInfo[0].pName = "vsmain";
        shaderStageCreateInfo[0].module = m_DrawStatsShaderModule;
        shaderStageCreateInfo[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStageCreateInfo[1].pName = "csmain";
        shaderStageCreateInfo[1].module = m_DrawStatsShaderModule;

        VkStructure<VkPipelineVertexInputStateCreateInfo> vertexInputStateCreateInfo;
        // TODO

        VkStructure<VkPipelineInputAssemblyStateCreateInfo> inputAssemblyStateCreateInfo;
        inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkStructure<VkPipelineRasterizationStateCreateInfo> rasterizationStateCreateInfo;
        // TODO

        VkStructure<VkPipelineMultisampleStateCreateInfo> multisampleStateCreateInfo;
        // TODO

        VkStructure<VkPipelineDepthStencilStateCreateInfo> depthStencilStateCreateInfo;
        // TODO

        VkStructure<VkPipelineColorBlendStateCreateInfo> colorBlendStateCreateInfo;
        // TODO

        VkDynamicState dynamicStates[2] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR };

        VkStructure<VkPipelineDynamicStateCreateInfo> dynamicStateCreateInfo;
        dynamicStateCreateInfo.dynamicStateCount = 2;
        dynamicStateCreateInfo.pDynamicStates = dynamicStates;

        VkStructure<VkGraphicsPipelineCreateInfo> pipelineCreateInfo;
        pipelineCreateInfo.renderPass = m_DrawStatsRenderPass;
        pipelineCreateInfo.subpass = 0;
        pipelineCreateInfo.layout = m_DrawStatsPipelineLayout;
        pipelineCreateInfo.stageCount = 2;
        pipelineCreateInfo.pStages = shaderStageCreateInfo;
        pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
        pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
        pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
        pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
        pipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
        pipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
        pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;

        DESTROYANDRETURNONFAIL( m_Callbacks.pfnCreateGraphicsPipelines(
            pDevice->Device, nullptr, 1, &pipelineCreateInfo, nullptr, &m_DrawStatsPipeline ) );
        
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

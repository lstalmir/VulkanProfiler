#include "profiler_overlay_state_factory.h"
#include "profiler_helpers.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        ProfilerOverlayStateFactory

    Description:
        Constructor

    \***********************************************************************************/
    ProfilerOverlayStateFactory::ProfilerOverlayStateFactory( VkDevice device, ProfilerCallbacks callbacks )
        : m_Device( device )
        , m_Callbacks( callbacks )
    {
    }

    /***********************************************************************************\

    Function:
        CreateDrawStatsRenderPass

    Description:
        Create render pass for drawing frame stats.

    \***********************************************************************************/
    VkResult ProfilerOverlayStateFactory::CreateDrawStatsRenderPass( VkRenderPass* pRenderPass )
    {
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

        return m_Callbacks.pfnCreateRenderPass(
            m_Device, &renderPassCreateInfo, nullptr, pRenderPass );
    }

    /***********************************************************************************\

    Function:
        CreateDrawStatsPipelineLayout

    Description:
        Create pipeline layout for drawing frame stats.

    \***********************************************************************************/
    VkResult ProfilerOverlayStateFactory::CreateDrawStatsPipelineLayout( VkPipelineLayout* pPipelineLayout )
    {
        VkStructure<VkPipelineLayoutCreateInfo> pipelineLayoutCreateInfo;
        // TODO

        return m_Callbacks.pfnCreatePipelineLayout(
            m_Device, &pipelineLayoutCreateInfo, nullptr, pPipelineLayout );
    }

    /***********************************************************************************\

    Function:
        CreateDrawStatsPipelineLayout

    Description:
        Create pipeline layout for drawing frame stats.

    \***********************************************************************************/
    VkResult ProfilerOverlayStateFactory::CreateDrawStatsShaderModule( VkShaderModule* pShaderModule, ProfilerShaderType shader )
    {
        auto it = ProfilerShadersMap.find( shader );
        if( it == ProfilerShadersMap.end() )
        {
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        auto shaderBytecode = it->second;

        VkStructure<VkShaderModuleCreateInfo> shaderModuleCreateInfo;
        shaderModuleCreateInfo.codeSize = shaderBytecode.first;
        shaderModuleCreateInfo.pCode = shaderBytecode.second;

        return m_Callbacks.pfnCreateShaderModule(
            m_Device, &shaderModuleCreateInfo, nullptr, pShaderModule );
    }

    /***********************************************************************************\

    Function:
        CreateDrawStatsPipeline

    Description:
        Create graphics pipeline for drawing frame stats.

    \***********************************************************************************/
    VkResult ProfilerOverlayStateFactory::CreateDrawStatsPipeline(
        VkRenderPass renderPass,
        VkPipelineLayout layout,
        const std::unordered_map<VkShaderStageFlagBits, VkShaderModule>& shaderModules,
        VkPipeline* pPipeline )
    {
        // Create pipeline for the render pass
        std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos;
        for( auto module : shaderModules )
        {
            VkStructure<VkPipelineShaderStageCreateInfo> shaderStageCreateInfo;
            shaderStageCreateInfo.stage = module.first;
            shaderStageCreateInfo.pName = "main";
            shaderStageCreateInfo.module = module.second;

            shaderStageCreateInfos.push_back( shaderStageCreateInfo );
        }

        VkStructure<VkPipelineVertexInputStateCreateInfo> vertexInputStateCreateInfo;
        vertexInputStateCreateInfo.vertexBindingDescriptionCount = VertexShaderInput::VertexInputBindingCount;
        vertexInputStateCreateInfo.pVertexBindingDescriptions = VertexShaderInput::VertexInputBindings;
        vertexInputStateCreateInfo.vertexAttributeDescriptionCount = VertexShaderInput::VertexInputAttributeCount;
        vertexInputStateCreateInfo.pVertexAttributeDescriptions = VertexShaderInput::VertexInputAttributes;

        VkStructure<VkPipelineInputAssemblyStateCreateInfo> inputAssemblyStateCreateInfo;
        inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkStructure<VkPipelineViewportStateCreateInfo> viewportStateCreateInfo;
        viewportStateCreateInfo.viewportCount = 0; // dynamic
        viewportStateCreateInfo.scissorCount = 0; // dynamic

        VkStructure<VkPipelineRasterizationStateCreateInfo> rasterizationStateCreateInfo;
        rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizationStateCreateInfo.lineWidth = 1.f;

        VkStructure<VkPipelineMultisampleStateCreateInfo> multisampleStateCreateInfo;
        multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkStructure<VkPipelineDepthStencilStateCreateInfo> depthStencilStateCreateInfo;
        depthStencilStateCreateInfo.front.writeMask = 0xFFFFFFFF;

        VkStructure<VkPipelineColorBlendAttachmentState> colorBlendAttachmentState;
        colorBlendAttachmentState.blendEnable = true;
        colorBlendAttachmentState.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | 
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;

        VkStructure<VkPipelineColorBlendStateCreateInfo> colorBlendStateCreateInfo;
        colorBlendStateCreateInfo.attachmentCount = 1;
        colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;
        colorBlendStateCreateInfo.blendConstants[0] = 1.f;
        colorBlendStateCreateInfo.blendConstants[1] = 1.f;
        colorBlendStateCreateInfo.blendConstants[2] = 1.f;
        colorBlendStateCreateInfo.blendConstants[3] = 1.f;

        VkDynamicState dynamicStates[2] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR };

        VkStructure<VkPipelineDynamicStateCreateInfo> dynamicStateCreateInfo;
        dynamicStateCreateInfo.dynamicStateCount = 2;
        dynamicStateCreateInfo.pDynamicStates = dynamicStates;

        VkStructure<VkGraphicsPipelineCreateInfo> pipelineCreateInfo;
        pipelineCreateInfo.renderPass = renderPass;
        pipelineCreateInfo.subpass = 0;
        pipelineCreateInfo.layout = layout;
        pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStageCreateInfos.size());
        pipelineCreateInfo.pStages = shaderStageCreateInfos.data();
        pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
        pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
        pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
        pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
        pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
        pipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
        pipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
        pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;

        return m_Callbacks.pfnCreateGraphicsPipelines(
            m_Device, nullptr, 1, &pipelineCreateInfo, nullptr, pPipeline );
    }
}

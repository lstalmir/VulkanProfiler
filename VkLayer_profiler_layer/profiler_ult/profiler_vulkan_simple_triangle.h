#pragma once
#include "profiler_vulkan_state.h"

#include "shaders/simple_triangle.vert.hlsl.h"
#include "shaders/simple_triangle.frag.hlsl.h"

namespace Profiler
{
    class VulkanSimpleTriangle
    {
    public:
        VkRenderPass        RenderPass;
        VkFramebuffer       Framebuffer;
        VkImage             FramebufferImage;
        VkImageView         FramebufferImageView;
        VkDeviceMemory      FramebufferImageMemory;
        VkPipelineLayout    PipelineLayout;
        VkPipeline          Pipeline;
        VkRect2D            RenderArea;

    public:
        inline VulkanSimpleTriangle( VulkanState* Vk,
            const VkLayerInstanceDispatchTable& IDT,
            const VkLayerDispatchTable& DT )
            : RenderPass( VK_NULL_HANDLE )
            , Framebuffer( VK_NULL_HANDLE )
            , FramebufferImage( VK_NULL_HANDLE )
            , FramebufferImageView( VK_NULL_HANDLE )
            , PipelineLayout( VK_NULL_HANDLE )
            , Pipeline( VK_NULL_HANDLE )
            , RenderArea()
        {
            { // Setup render area
                RenderArea.offset = {};
                RenderArea.extent = { 640, 480 };
            }
            { // Create render pass
                VkAttachmentDescription attachmentDescription = {};
                attachmentDescription.format = VK_FORMAT_R8G8B8A8_UNORM;
                attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;

                VkAttachmentReference attachmentReference = {};
                attachmentReference.attachment = 0;
                attachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                VkSubpassDescription subpassDescription = {};
                subpassDescription.colorAttachmentCount = 1;
                subpassDescription.pColorAttachments = &attachmentReference;
                subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

                VkRenderPassCreateInfo createInfo = {};
                createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
                createInfo.attachmentCount = 1;
                createInfo.pAttachments = &attachmentDescription;
                createInfo.subpassCount = 1;
                createInfo.pSubpasses = &subpassDescription;

                VERIFY_RESULT( Vk, DT.CreateRenderPass( Vk->Device, &createInfo, nullptr, &RenderPass ) );
            }
            { // Create image
                VkImageCreateInfo createInfo = {};
                createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                createInfo.imageType = VK_IMAGE_TYPE_2D;
                createInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
                createInfo.extent.width = RenderArea.extent.width;
                createInfo.extent.height = RenderArea.extent.height;
                createInfo.extent.depth = 1;
                createInfo.arrayLayers = 1;
                createInfo.mipLevels = 1;
                createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
                createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
                createInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

                VERIFY_RESULT( Vk, DT.CreateImage( Vk->Device, &createInfo, nullptr, &FramebufferImage ) );
            }
            { // Allocate image memory
                VkMemoryRequirements memoryRequirements = {};
                DT.GetImageMemoryRequirements( Vk->Device, FramebufferImage, &memoryRequirements );

                // Get memory type index
                uint32_t memoryTypeIndex = -1;
                {
                    VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties = {};
                    IDT.GetPhysicalDeviceMemoryProperties( Vk->PhysicalDevice, &physicalDeviceMemoryProperties );

                    for( uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; ++i )
                    {
                        if( (memoryRequirements.memoryTypeBits & (1U << i)) &&
                            (physicalDeviceMemoryProperties.memoryTypes[ i ].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) )
                        {
                            memoryTypeIndex = i;
                            break;
                        }
                    }
                }

                VkMemoryAllocateInfo allocateInfo = {};
                allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                allocateInfo.allocationSize = memoryRequirements.size;
                allocateInfo.memoryTypeIndex = memoryTypeIndex;

                VERIFY_RESULT( Vk, DT.AllocateMemory( Vk->Device, &allocateInfo, nullptr, &FramebufferImageMemory ) );
            }
            { // Bind image memory
                VERIFY_RESULT( Vk, DT.BindImageMemory( Vk->Device, FramebufferImage, FramebufferImageMemory, 0 ) );
            }
            { // Create image view
                VkImageViewCreateInfo createInfo = {};
                createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                createInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
                createInfo.image = FramebufferImage;
                createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                createInfo.subresourceRange.layerCount = 1;
                createInfo.subresourceRange.levelCount = 1;

                VERIFY_RESULT( Vk, DT.CreateImageView( Vk->Device, &createInfo, nullptr, &FramebufferImageView ) );
            }
            { // Create framebuffer
                VkFramebufferCreateInfo createInfo = {};
                createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                createInfo.renderPass = RenderPass;
                createInfo.attachmentCount = 1;
                createInfo.pAttachments = &FramebufferImageView;
                createInfo.width = RenderArea.extent.width;
                createInfo.height = RenderArea.extent.height;
                createInfo.layers = 1;

                VERIFY_RESULT( Vk, DT.CreateFramebuffer( Vk->Device, &createInfo, nullptr, &Framebuffer ) );
            }
            { // Create pipeline layout
                VkPipelineLayoutCreateInfo createInfo = {};
                createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

                VERIFY_RESULT( Vk, DT.CreatePipelineLayout( Vk->Device, &createInfo, nullptr, &PipelineLayout ) );
            }
            { // Create graphics pipeline
                VkShaderModule vertexShaderModule = VK_NULL_HANDLE;
                VkShaderModule fragmentShaderModule = VK_NULL_HANDLE;

                { // Create vertex shader module
                    VkShaderModuleCreateInfo moduleCreateInfo = {};
                    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
                    moduleCreateInfo.codeSize = sizeof( simple_triangle_vert_hlsl );
                    moduleCreateInfo.pCode = simple_triangle_vert_hlsl;
                    VERIFY_RESULT( Vk, DT.CreateShaderModule( Vk->Device, &moduleCreateInfo, nullptr, &vertexShaderModule ) );
                }
                { // Create fragment shader module
                    VkShaderModuleCreateInfo moduleCreateInfo = {};
                    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
                    moduleCreateInfo.codeSize = sizeof( simple_triangle_frag_hlsl );
                    moduleCreateInfo.pCode = simple_triangle_frag_hlsl;
                    VERIFY_RESULT( Vk, DT.CreateShaderModule( Vk->Device, &moduleCreateInfo, nullptr, &fragmentShaderModule ) );
                }

                VkPipelineShaderStageCreateInfo shaderStageCreateInfos[ 2 ] = {};
                // Vertex shader stage
                shaderStageCreateInfos[ 0 ].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                shaderStageCreateInfos[ 0 ].module = vertexShaderModule;
                shaderStageCreateInfos[ 0 ].stage = VK_SHADER_STAGE_VERTEX_BIT;
                shaderStageCreateInfos[ 0 ].pName = "main";
                // Fragment shader stage
                shaderStageCreateInfos[ 1 ].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                shaderStageCreateInfos[ 1 ].module = fragmentShaderModule;
                shaderStageCreateInfos[ 1 ].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
                shaderStageCreateInfos[ 1 ].pName = "main";

                VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
                vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

                VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
                inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
                inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

                VkViewport viewport = {};
                viewport.width = RenderArea.extent.width;
                viewport.height = RenderArea.extent.height;
                viewport.maxDepth = 1;

                VkPipelineViewportStateCreateInfo viewportCreateInfo = {};
                viewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
                viewportCreateInfo.viewportCount = 1;
                viewportCreateInfo.pViewports = &viewport;
                viewportCreateInfo.scissorCount = 1;
                viewportCreateInfo.pScissors = &RenderArea;

                VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo = {};
                rasterizationCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
                rasterizationCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
                rasterizationCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
                rasterizationCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
                rasterizationCreateInfo.lineWidth = 1;

                VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {};
                depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
                depthStencilCreateInfo.depthTestEnable = false;
                depthStencilCreateInfo.depthWriteEnable = false;
                depthStencilCreateInfo.stencilTestEnable = false;

                VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = {};
                multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
                multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

                VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
                colorBlendAttachment.blendEnable = false;
                colorBlendAttachment.colorWriteMask = 0xF;

                VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo = {};
                colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
                colorBlendCreateInfo.attachmentCount = 1;
                colorBlendCreateInfo.pAttachments = &colorBlendAttachment;

                VkGraphicsPipelineCreateInfo createInfo = {};
                createInfo.layout = PipelineLayout;
                createInfo.renderPass = RenderPass;
                createInfo.subpass = 0;
                createInfo.stageCount = 2;
                createInfo.pStages = shaderStageCreateInfos;
                createInfo.pVertexInputState = &vertexInputCreateInfo;
                createInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
                createInfo.pViewportState = &viewportCreateInfo;
                createInfo.pRasterizationState = &rasterizationCreateInfo;
                createInfo.pMultisampleState = &multisampleCreateInfo;
                createInfo.pDepthStencilState = &depthStencilCreateInfo;
                createInfo.pColorBlendState = &colorBlendCreateInfo;

                VERIFY_RESULT( Vk, DT.CreateGraphicsPipelines( Vk->Device, nullptr, 1, &createInfo, nullptr, &Pipeline ) );

                // Destroy shader modules
                DT.DestroyShaderModule( Vk->Device, vertexShaderModule, nullptr );
                DT.DestroyShaderModule( Vk->Device, fragmentShaderModule, nullptr );
            }
        }
    };
}

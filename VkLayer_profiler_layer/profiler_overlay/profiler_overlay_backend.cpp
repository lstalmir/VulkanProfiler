// Copyright (c) 2025 Lukasz Stalmirski
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

#include "profiler_overlay_backend.h"
#include "profiler_layer_objects/VkDevice_object.h"
#include "profiler_layer_objects/VkQueue_object.h"
#include "profiler_layer_objects/VkSurfaceKhr_object.h"
#include "profiler_layer_objects/VkSwapchainKhr_object.h"
#include "profiler_layer_functions/core/VkDevice_functions_base.h"

#include <imgui.h>
#include <imgui_impl_vulkan.h>

#ifdef VK_USE_PLATFORM_WIN32_KHR
#include "imgui_impl_win32.h"
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
#include "imgui_impl_xcb.h"
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
#include "imgui_impl_xlib.h"
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
#include "imgui_impl_wayland.h"
#endif

#define LoadVulkanFunction( name )                                                           \
    pfn_##name = reinterpret_cast<PFN_##name>( pfn_vkGetDeviceProcAddr( m_Device, #name ) ); \
    if( pfn_##name == nullptr )                                                              \
        return false;

namespace Profiler
{
    /***********************************************************************************\

    Function:
        OverlayVulkanBackend

    Description:
        Constructor.

    \***********************************************************************************/
    OverlayVulkanBackend::OverlayVulkanBackend()
    {
        ResetMembers();
    }

    /***********************************************************************************\

    Function:
        Initialize

    Description:
        Initialize the backend.

    \***********************************************************************************/
    VkResult OverlayVulkanBackend::Initialize( const CreateInfo& createInfo )
    {
        VkResult result = VK_SUCCESS;

        // Set members and load Vulkan functions
        m_Instance = createInfo.Instance;
        m_PhysicalDevice = createInfo.PhysicalDevice;
        m_Device = createInfo.Device;
        m_Queue = createInfo.Queue;
        m_QueueFamilyIndex = createInfo.QueueFamilyIndex;
        pfn_vkGetDeviceProcAddr = createInfo.pfn_vkGetDeviceProcAddr;
        pfn_vkGetInstanceProcAddr = createInfo.pfn_vkGetInstanceProcAddr;

        if( !LoadFunctions() )
        {
            result = VK_ERROR_INITIALIZATION_FAILED;
        }

        // Create descriptor pool
        if( result == VK_SUCCESS )
        {
            VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
            descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

            // ImGui allocates descriptor sets only for textures/fonts for now.
            const uint32_t imguiMaxTextureCount = 16;
            const VkDescriptorPoolSize descriptorPoolSizes[] = {
                { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imguiMaxTextureCount }
            };

            descriptorPoolCreateInfo.maxSets = imguiMaxTextureCount;
            descriptorPoolCreateInfo.poolSizeCount = std::extent_v<decltype( descriptorPoolSizes )>;
            descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes;

            result = pfn_vkCreateDescriptorPool(
                m_Device,
                &descriptorPoolCreateInfo,
                nullptr,
                &m_DescriptorPool );
        }

        // Create command pool
        if( result == VK_SUCCESS )
        {
            VkCommandPoolCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            info.flags |= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            info.queueFamilyIndex = m_QueueFamilyIndex;

            result = pfn_vkCreateCommandPool(
                m_Device,
                &info,
                nullptr,
                &m_CommandPool );
        }

        // Create linear sampler
        if( result == VK_SUCCESS )
        {
            VkSamplerCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            info.magFilter = VK_FILTER_LINEAR;
            info.minFilter = VK_FILTER_LINEAR;
            info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

            result = pfn_vkCreateSampler(
                m_Device,
                &info,
                nullptr,
                &m_LinearSampler );
        }

        // Create memory allocator
        if( result == VK_SUCCESS )
        {
            VmaAllocatorCreateInfo info = {};
            info.physicalDevice = m_PhysicalDevice;
            info.device = m_Device;
            info.instance = m_Instance;
            info.vulkanApiVersion = m_ApiVersion;

            VmaVulkanFunctions functions = {};
            functions.vkGetInstanceProcAddr = pfn_vkGetInstanceProcAddr;
            functions.vkGetDeviceProcAddr = pfn_vkGetDeviceProcAddr;
            info.pVulkanFunctions = &functions;

            result = vmaCreateAllocator( &info, &m_Allocator );
        }

        if( result != VK_SUCCESS )
        {
            Destroy();
        }

        m_Initialized = ( result == VK_SUCCESS );

        return result;
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:
        Destroy the backend.

    \***********************************************************************************/
    void OverlayVulkanBackend::Destroy()
    {
        WaitIdle();

        DestroyImGuiBackend();
        DestroySwapchainResources();
        DestroyResources();

        if( m_DescriptorPool != VK_NULL_HANDLE )
        {
            pfn_vkDestroyDescriptorPool( m_Device, m_DescriptorPool, nullptr );
        }

        if( m_CommandPool != VK_NULL_HANDLE )
        {
            pfn_vkDestroyCommandPool( m_Device, m_CommandPool, nullptr );
        }

        if( m_LinearSampler != VK_NULL_HANDLE )
        {
            pfn_vkDestroySampler( m_Device, m_LinearSampler, nullptr );
        }

        if( m_Allocator != VK_NULL_HANDLE )
        {
            vmaDestroyAllocator( m_Allocator );
        }

        ResetMembers();
    }

    /***********************************************************************************\

    Function:
        IsInitialized

    Description:
        Check whether the backend is initialized.

    \***********************************************************************************/
    bool OverlayVulkanBackend::IsInitialized() const
    {
        return m_Initialized;
    }

    /***********************************************************************************\

    Function:
        ResetSwapchain

    Description:
        Initialize the swapchain-dependent resources.

    \***********************************************************************************/
    VkResult OverlayVulkanBackend::SetSwapchain( VkSwapchainKHR swapchain, const VkSwapchainCreateInfoKHR& createInfo )
    {
        VkResult result = VK_SUCCESS;

        // Get swapchain images
        uint32_t swapchainImageCount = 0;
        pfn_vkGetSwapchainImagesKHR( m_Device, swapchain, &swapchainImageCount, nullptr );

        std::vector<VkImage> images( swapchainImageCount );
        result = pfn_vkGetSwapchainImagesKHR( m_Device, swapchain, &swapchainImageCount, images.data() );
        assert( result == VK_SUCCESS );

        // Recreate render pass if swapchain format has changed
        if( ( result == VK_SUCCESS ) && ( createInfo.imageFormat != m_ImageFormat ) )
        {
            if( m_RenderPass != VK_NULL_HANDLE )
            {
                // Destroy old render pass
                pfn_vkDestroyRenderPass( m_Device, m_RenderPass, nullptr );

                m_RenderPass = VK_NULL_HANDLE;
            }

            VkAttachmentDescription attachment = {};
            attachment.format = createInfo.imageFormat;
            attachment.samples = VK_SAMPLE_COUNT_1_BIT;
            attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            VkAttachmentReference color_attachment = {};
            color_attachment.attachment = 0;
            color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkSubpassDescription subpass = {};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &color_attachment;

            VkSubpassDependency dependency = {};
            dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            dependency.dstSubpass = 0;
            dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.srcAccessMask = 0;
            dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            VkRenderPassCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            info.attachmentCount = 1;
            info.pAttachments = &attachment;
            info.subpassCount = 1;
            info.pSubpasses = &subpass;
            info.dependencyCount = 1;
            info.pDependencies = &dependency;

            result = pfn_vkCreateRenderPass(
                m_Device,
                &info,
                nullptr,
                &m_RenderPass );
        }

        // Recreate image views and framebuffers
        // This is required because swapchain images have changed and current framebuffer is out of date
        if( result == VK_SUCCESS )
        {
            if( !m_Images.empty() )
            {
                // Destroy previous framebuffers
                for( int i = 0; i < m_Images.size(); ++i )
                {
                    pfn_vkDestroyFramebuffer( m_Device, m_Framebuffers[i], nullptr );
                    pfn_vkDestroyImageView( m_Device, m_ImageViews[i], nullptr );
                }

                m_Framebuffers.clear();
                m_ImageViews.clear();
            }

            m_Framebuffers.reserve( swapchainImageCount );
            m_ImageViews.reserve( swapchainImageCount );

            for( uint32_t i = 0; i < swapchainImageCount; i++ )
            {
                VkImageView imageView = VK_NULL_HANDLE;
                VkFramebuffer framebuffer = VK_NULL_HANDLE;

                // Create swapchain image view
                if( result == VK_SUCCESS )
                {
                    VkImageViewCreateInfo info = {};
                    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
                    info.format = createInfo.imageFormat;
                    info.image = images[i];
                    info.components.r = VK_COMPONENT_SWIZZLE_R;
                    info.components.g = VK_COMPONENT_SWIZZLE_G;
                    info.components.b = VK_COMPONENT_SWIZZLE_B;
                    info.components.a = VK_COMPONENT_SWIZZLE_A;

                    VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
                    info.subresourceRange = range;

                    result = pfn_vkCreateImageView(
                        m_Device,
                        &info,
                        nullptr,
                        &imageView );

                    m_ImageViews.push_back( imageView );
                }

                // Create framebuffer
                if( result == VK_SUCCESS )
                {
                    VkFramebufferCreateInfo info = {};
                    info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                    info.renderPass = m_RenderPass;
                    info.attachmentCount = 1;
                    info.pAttachments = &imageView;
                    info.width = createInfo.imageExtent.width;
                    info.height = createInfo.imageExtent.height;
                    info.layers = 1;

                    result = pfn_vkCreateFramebuffer(
                        m_Device,
                        &info,
                        nullptr,
                        &framebuffer );

                    m_Framebuffers.push_back( framebuffer );
                }
            }
        }

        // Allocate additional command buffers, fences and semaphores
        if( ( result == VK_SUCCESS ) && ( swapchainImageCount > m_Images.size() ) )
        {
            VkCommandBufferAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = m_CommandPool;
            allocInfo.commandBufferCount = swapchainImageCount - static_cast<uint32_t>( m_Images.size() );

            std::vector<VkCommandBuffer> commandBuffers( swapchainImageCount );
            result = AllocateCommandBuffers(
                allocInfo,
                commandBuffers.data() );

            if( result == VK_SUCCESS )
            {
                // Append created command buffers to end
                // We need to do this right after allocation to avoid leaks if something fails later
                m_CommandBuffers.insert( m_CommandBuffers.end(), commandBuffers.begin(), commandBuffers.end() );
            }

            m_CommandFences.reserve( swapchainImageCount );
            m_CommandSemaphores.reserve( swapchainImageCount );

            // Create additional per-command-buffer semaphores and fences
            for( size_t i = m_Images.size(); i < swapchainImageCount; ++i )
            {
                VkFence fence;
                VkSemaphore semaphore;

                // Create command buffer fence
                if( result == VK_SUCCESS )
                {
                    VkFenceCreateInfo fenceInfo = {};
                    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
                    fenceInfo.flags |= VK_FENCE_CREATE_SIGNALED_BIT;

                    result = pfn_vkCreateFence(
                        m_Device,
                        &fenceInfo,
                        nullptr,
                        &fence );

                    m_CommandFences.push_back( fence );
                }

                // Create present semaphore
                if( result == VK_SUCCESS )
                {
                    VkSemaphoreCreateInfo semaphoreInfo = {};
                    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

                    result = pfn_vkCreateSemaphore(
                        m_Device,
                        &semaphoreInfo,
                        nullptr,
                        &semaphore );

                    m_CommandSemaphores.push_back( semaphore );
                }
            }
        }

        // Update objects
        m_Swapchain = swapchain;
        m_Surface = createInfo.surface;
        m_RenderArea = createInfo.imageExtent;
        m_ImageFormat = createInfo.imageFormat;
        m_MinImageCount = createInfo.minImageCount;
        m_Images = std::move( images );

        // Force reinitialization of ImGui context at the beginning of the next frame
        m_ImGuiBackendResetBeforeNextFrame = true;

        // Don't leave object in partly-initialized state
        if( result != VK_SUCCESS )
        {
            DestroySwapchainResources();
        }

        return result;
    }

    /***********************************************************************************\

    Function:
        GetSwapchain

    Description:
        Return swapchain handle associated with the backend.

    \***********************************************************************************/
    VkSwapchainKHR OverlayVulkanBackend::GetSwapchain() const
    {
        return m_Swapchain;
    }

    /***********************************************************************************\

    Function:
        SetFramePresentInfo

    Description:
        Prepare VkPresentInfoKHR for the next frame.

    \***********************************************************************************/
    void OverlayVulkanBackend::SetFramePresentInfo( const VkPresentInfoKHR& presentInfo )
    {
        m_PresentInfo = presentInfo;
    }

    /***********************************************************************************\

    Function:
        GetFramePresentInfo

    Description:
        Get overridden VkPresentInfoKHR prepared for the next frame.

    \***********************************************************************************/
    const VkPresentInfoKHR& OverlayVulkanBackend::GetFramePresentInfo() const
    {
        return m_PresentInfo;
    }

    /***********************************************************************************\

    Function:
        InitializeImGuiBackend

    Description:
        Initialize the ImGui backend for Vulkan.

    \***********************************************************************************/
    bool OverlayVulkanBackend::PrepareImGuiBackend()
    {
        if( m_ImGuiBackendResetBeforeNextFrame )
        {
            // Reset ImGui backend due to swapchain recreation.
            DestroyImGuiBackend();

            m_ImGuiBackendResetBeforeNextFrame = false;
            m_ImGuiBackendInitialized = false;
        }

        if( !m_ImGuiBackendInitialized )
        {
            // Load device functions required by the backend.
            if( !ImGui_ImplVulkan_LoadFunctions( FunctionLoader, this ) )
            {
                return false;
            }

            ImGui_ImplVulkan_InitInfo initInfo = {};
            initInfo.Instance = m_Instance;
            initInfo.PhysicalDevice = m_PhysicalDevice;
            initInfo.Device = m_Device;
            initInfo.QueueFamily = m_QueueFamilyIndex;
            initInfo.Queue = m_Queue;
            initInfo.DescriptorPool = m_DescriptorPool;
            initInfo.RenderPass = m_RenderPass;
            initInfo.MinImageCount = m_MinImageCount;
            initInfo.ImageCount = static_cast<uint32_t>( m_Images.size() );
            initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

            // Initialize the backend.
            if( !ImGui_ImplVulkan_Init( &initInfo ) )
            {
                return false;
            }

            m_ImGuiBackendInitialized = true;
        }

        return m_ImGuiBackendInitialized;
    }

    /***********************************************************************************\

    Function:
        InitializeImGuiBackend

    Description:
        Initialize the ImGui backend for Vulkan.

    \***********************************************************************************/
    void OverlayVulkanBackend::DestroyImGuiBackend()
    {
        if( m_ImGuiBackendInitialized )
        {
            ImGui_ImplVulkan_Shutdown();
            m_ImGuiBackendInitialized = false;
        }
    }

    /***********************************************************************************\

    Function:
        WaitIdle

    Description:
        Wait for the GPU to finish rendering.

    \***********************************************************************************/
    void OverlayVulkanBackend::WaitIdle()
    {
        if( m_LastSubmittedFence != VK_NULL_HANDLE )
        {
            pfn_vkWaitForFences( m_Device, 1, &m_LastSubmittedFence, VK_TRUE, UINT64_MAX );

            // No need to wait for this fence again.
            m_LastSubmittedFence = VK_NULL_HANDLE;
        }
    }

    /***********************************************************************************\

    Function:
        GetRenderArea

    Description:
        Get the current render area.

    \***********************************************************************************/
    ImVec2 OverlayVulkanBackend::GetRenderArea() const
    {
        return ImVec2(
            static_cast<float>( m_RenderArea.width ),
            static_cast<float>( m_RenderArea.height ) );
    }

    /***********************************************************************************\

    Function:
        NewFrame

    Description:
        Begin rendering of a new frame.

    \***********************************************************************************/
    bool OverlayVulkanBackend::NewFrame()
    {
        bool backendPrepared = PrepareImGuiBackend();
        if( backendPrepared )
        {
            ImGui_ImplVulkan_NewFrame();
        }

        if( m_ResourcesUploadEvent != VK_NULL_HANDLE )
        {
            DestroyUploadResources();
        }

        return backendPrepared;
    }

    /***********************************************************************************\

    Function:
        RenderDrawData

    Description:
        Render ImGui draw data.

    \***********************************************************************************/
    void OverlayVulkanBackend::RenderDrawData( ImDrawData* pDrawData )
    {
        VkResult result = VK_SUCCESS;

        // Grab command buffer for overlay commands.
        uint32_t imageIndex = 0;
        if( m_PresentInfo.swapchainCount && m_PresentInfo.pImageIndices )
        {
            imageIndex = m_PresentInfo.pImageIndices[0];
        }

        VkFence& fence = m_CommandFences[imageIndex];
        VkSemaphore& semaphore = m_CommandSemaphores[imageIndex];
        VkCommandBuffer& commandBuffer = m_CommandBuffers[imageIndex];
        VkFramebuffer& framebuffer = m_Framebuffers[imageIndex];

        pfn_vkWaitForFences( m_Device, 1, &fence, VK_TRUE, UINT64_MAX );
        pfn_vkResetFences( m_Device, 1, &fence );

        if( result == VK_SUCCESS )
        {
            VkCommandBufferBeginInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            result = pfn_vkBeginCommandBuffer( commandBuffer, &info );
        }

        if( result == VK_SUCCESS )
        {
            // Record upload commands before starting the render pass.
            RecordUploadCommands( commandBuffer );
        }

        if( result == VK_SUCCESS )
        {
            VkRenderPassBeginInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            info.renderPass = m_RenderPass;
            info.framebuffer = framebuffer;
            info.renderArea.extent.width = m_RenderArea.width;
            info.renderArea.extent.height = m_RenderArea.height;

            // Record Imgui Draw Data into the command buffer.
            pfn_vkCmdBeginRenderPass( commandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE );
            ImGui_ImplVulkan_RenderDrawData( pDrawData, commandBuffer );
            pfn_vkCmdEndRenderPass( commandBuffer );

            result = pfn_vkEndCommandBuffer( commandBuffer );
        }

        if( result == VK_SUCCESS )
        {
            // Submit the command buffer to the GPU.
            VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            VkSubmitInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            info.waitSemaphoreCount = m_PresentInfo.waitSemaphoreCount;
            info.pWaitSemaphores = m_PresentInfo.pWaitSemaphores;
            info.pWaitDstStageMask = &waitStage;
            info.commandBufferCount = 1;
            info.pCommandBuffers = &commandBuffer;
            info.signalSemaphoreCount = 1;
            info.pSignalSemaphores = &semaphore;

            result = pfn_vkQueueSubmit( m_Queue, 1, &info, fence );
        }

        if( result == VK_SUCCESS )
        {
            m_LastSubmittedFence = fence;

            // Override wait semaphore.
            m_PresentInfo.waitSemaphoreCount = 1;
            m_PresentInfo.pWaitSemaphores = &semaphore;
        }
    }

    /***********************************************************************************\

    Function:
        CreateImage

    Description:
        Create an image resource.

    \***********************************************************************************/
    void* OverlayVulkanBackend::CreateImage( const ImageCreateInfo& createInfo )
    {
        ImageResource image;
        VkResult result = InitializeImage( image, createInfo );

        if( result == VK_SUCCESS )
        {
            m_ImageResources.push_back( image );
            return image.ImageDescriptorSet;
        }

        return nullptr;
    }

    /***********************************************************************************\

    Function:
        DestroyImage

    Description:
        Destroy an image resource.

    \***********************************************************************************/
    void OverlayVulkanBackend::DestroyImage( void* pImage )
    {
        auto it = std::find_if( m_ImageResources.begin(), m_ImageResources.end(),
            [pImage]( const ImageResource& image ) { return image.ImageDescriptorSet == pImage; } );

        if( it != m_ImageResources.end() )
        {
            DestroyImage( *it );
            m_ImageResources.erase( it );
        }
    }

    /***********************************************************************************\

    Function:
        CreateFontsImage

    Description:
        Create an image resource for fonts.

    \***********************************************************************************/
    void OverlayVulkanBackend::CreateFontsImage()
    {
        ImGui_ImplVulkan_CreateFontsTexture();
    }

    /***********************************************************************************\

    Function:
        DestroyFontsImage

    Description:
        Destroy the image resource for fonts.

    \***********************************************************************************/
    void OverlayVulkanBackend::DestroyFontsImage()
    {
        ImGui_ImplVulkan_DestroyFontsTexture();
    }

    /***********************************************************************************\

    Function:
        LoadFunctions

    Description:
        Load Vulkan functions required by the backend.
        pfn_vkGetDeviceProcAddr must be set by the caller.

    \***********************************************************************************/
    bool OverlayVulkanBackend::LoadFunctions()
    {
        assert( pfn_vkGetDeviceProcAddr != nullptr );
        assert( pfn_vkGetInstanceProcAddr != nullptr );

        LoadVulkanFunction( vkQueueSubmit );
        LoadVulkanFunction( vkCreateRenderPass );
        LoadVulkanFunction( vkDestroyRenderPass );
        LoadVulkanFunction( vkCreateFramebuffer );
        LoadVulkanFunction( vkDestroyFramebuffer );
        LoadVulkanFunction( vkCreateImageView );
        LoadVulkanFunction( vkDestroyImageView );
        LoadVulkanFunction( vkCreateSampler );
        LoadVulkanFunction( vkDestroySampler );
        LoadVulkanFunction( vkCreateFence );
        LoadVulkanFunction( vkDestroyFence );
        LoadVulkanFunction( vkWaitForFences );
        LoadVulkanFunction( vkResetFences );
        LoadVulkanFunction( vkCreateEvent );
        LoadVulkanFunction( vkDestroyEvent );
        LoadVulkanFunction( vkCmdSetEvent );
        LoadVulkanFunction( vkGetEventStatus );
        LoadVulkanFunction( vkCreateSemaphore );
        LoadVulkanFunction( vkDestroySemaphore );
        LoadVulkanFunction( vkCreateDescriptorPool );
        LoadVulkanFunction( vkDestroyDescriptorPool );
        LoadVulkanFunction( vkAllocateDescriptorSets );
        LoadVulkanFunction( vkCreateCommandPool );
        LoadVulkanFunction( vkDestroyCommandPool );
        LoadVulkanFunction( vkAllocateCommandBuffers );
        LoadVulkanFunction( vkFreeCommandBuffers );
        LoadVulkanFunction( vkBeginCommandBuffer );
        LoadVulkanFunction( vkEndCommandBuffer );
        LoadVulkanFunction( vkGetSwapchainImagesKHR );
        LoadVulkanFunction( vkCmdBeginRenderPass );
        LoadVulkanFunction( vkCmdEndRenderPass );
        LoadVulkanFunction( vkCmdPipelineBarrier );
        LoadVulkanFunction( vkCmdCopyBufferToImage );

        return true;
    }

    /***********************************************************************************\

    Function:
        ResetMembers

    Description:
        Set all members to initial values.

    \***********************************************************************************/
    void OverlayVulkanBackend::ResetMembers()
    {
        m_Instance = VK_NULL_HANDLE;
        m_PhysicalDevice = VK_NULL_HANDLE;
        m_Device = VK_NULL_HANDLE;
        m_Queue = VK_NULL_HANDLE;
        m_QueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL;
        m_ApiVersion = VK_API_VERSION_1_0;

        m_Allocator = VK_NULL_HANDLE;
        m_CommandPool = VK_NULL_HANDLE;
        m_DescriptorPool = VK_NULL_HANDLE;

        m_Initialized = false;

        m_ResourcesUploadEvent = VK_NULL_HANDLE;
        m_LinearSampler = VK_NULL_HANDLE;
        m_ImageResources.clear();

        pfn_vkGetDeviceProcAddr = nullptr;
        pfn_vkGetInstanceProcAddr = nullptr;
        pfn_vkQueueSubmit = nullptr;
        pfn_vkCreateRenderPass = nullptr;
        pfn_vkDestroyRenderPass = nullptr;
        pfn_vkCreateFramebuffer = nullptr;
        pfn_vkDestroyFramebuffer = nullptr;
        pfn_vkCreateImageView = nullptr;
        pfn_vkDestroyImageView = nullptr;
        pfn_vkCreateSampler = nullptr;
        pfn_vkDestroySampler = nullptr;
        pfn_vkCreateFence = nullptr;
        pfn_vkDestroyFence = nullptr;
        pfn_vkWaitForFences = nullptr;
        pfn_vkResetFences = nullptr;
        pfn_vkCreateEvent = nullptr;
        pfn_vkDestroyEvent = nullptr;
        pfn_vkCmdSetEvent = nullptr;
        pfn_vkGetEventStatus = nullptr;
        pfn_vkCreateSemaphore = nullptr;
        pfn_vkDestroySemaphore = nullptr;
        pfn_vkCreateDescriptorPool = nullptr;
        pfn_vkDestroyDescriptorPool = nullptr;
        pfn_vkAllocateDescriptorSets = nullptr;
        pfn_vkCreateCommandPool = nullptr;
        pfn_vkDestroyCommandPool = nullptr;
        pfn_vkAllocateCommandBuffers = nullptr;
        pfn_vkFreeCommandBuffers = nullptr;
        pfn_vkBeginCommandBuffer = nullptr;
        pfn_vkEndCommandBuffer = nullptr;
        pfn_vkGetSwapchainImagesKHR = nullptr;
        pfn_vkCmdBeginRenderPass = nullptr;
        pfn_vkCmdEndRenderPass = nullptr;
        pfn_vkCmdPipelineBarrier = nullptr;
        pfn_vkCmdCopyBufferToImage = nullptr;

        ResetSwapchainMembers();
    }

    /***********************************************************************************\

    Function:
        DestroySwapchainResources

    Description:
        Destroy the resources associated with the current swapchain.

    \***********************************************************************************/
    void OverlayVulkanBackend::DestroySwapchainResources()
    {
        if( m_RenderPass != VK_NULL_HANDLE )
        {
            pfn_vkDestroyRenderPass( m_Device, m_RenderPass, nullptr );
        }

        for( VkFramebuffer framebuffer : m_Framebuffers )
        {
            if( framebuffer != VK_NULL_HANDLE )
            {
                pfn_vkDestroyFramebuffer( m_Device, framebuffer, nullptr );
            }
        }

        for( VkImageView imageView : m_ImageViews )
        {
            if( imageView != VK_NULL_HANDLE )
            {
                pfn_vkDestroyImageView( m_Device, imageView, nullptr );
            }
        }

        for( VkFence fence : m_CommandFences )
        {
            if( fence != VK_NULL_HANDLE )
            {
                pfn_vkDestroyFence( m_Device, fence, nullptr );
            }
        }

        for( VkSemaphore semaphore : m_CommandSemaphores )
        {
            if( semaphore != VK_NULL_HANDLE )
            {
                pfn_vkDestroySemaphore( m_Device, semaphore, nullptr );
            }
        }

        if( !m_CommandBuffers.empty() )
        {
            pfn_vkFreeCommandBuffers(
                m_Device,
                m_CommandPool,
                static_cast<uint32_t>( m_CommandBuffers.size() ),
                m_CommandBuffers.data() );
        }

        ResetSwapchainMembers();
    }

    /***********************************************************************************\

    Function:
        ResetSwapchainMembers

    Description:
        Set all members related to the target swapchain to initial values.

    \***********************************************************************************/
    void OverlayVulkanBackend::ResetSwapchainMembers()
    {
        m_ImGuiBackendResetBeforeNextFrame = false;
        m_ImGuiBackendInitialized = false;

        m_Swapchain = VK_NULL_HANDLE;
        m_PresentInfo = {};

        m_RenderPass = VK_NULL_HANDLE;
        m_RenderArea = { 0, 0 };
        m_ImageFormat = VK_FORMAT_UNDEFINED;
        m_MinImageCount = 0;
        m_Images.clear();
        m_ImageViews.clear();
        m_Framebuffers.clear();
        m_CommandBuffers.clear();
        m_CommandFences.clear();
        m_CommandSemaphores.clear();
        m_LastSubmittedFence = VK_NULL_HANDLE;
    }

    /***********************************************************************************\

    Function:
        FunctionLoader

    Description:
        Forwards call to LoadFunction.

    \***********************************************************************************/
    PFN_vkVoidFunction OverlayVulkanBackend::FunctionLoader( const char* pFunctionName, void* pUserData )
    {
        return static_cast<OverlayVulkanBackend*>( pUserData )->LoadFunction( pFunctionName );
    }

    /***********************************************************************************\

    Function:
        LoadFunction

    Description:
        Load Vulkan function for ImGui backend.

    \***********************************************************************************/
    PFN_vkVoidFunction OverlayVulkanBackend::LoadFunction( const char* pFunctionName )
    {
        return pfn_vkGetInstanceProcAddr( m_Instance, pFunctionName );
    }

    /***********************************************************************************\

    Function:
        AllocateCommandBuffers

    Description:
        Allocates command buffers.

    \***********************************************************************************/
    VkResult OverlayVulkanBackend::AllocateCommandBuffers( const VkCommandBufferAllocateInfo& allocateInfo, VkCommandBuffer* pCommandBuffers )
    {
        return pfn_vkAllocateCommandBuffers(
            m_Device,
            &allocateInfo,
            pCommandBuffers );
    }

    /***********************************************************************************\

    Function:
        RecordUploadCommands

    Description:
        Upload resources to the GPU.

    \***********************************************************************************/
    void OverlayVulkanBackend::RecordUploadCommands( VkCommandBuffer commandBuffer )
    {
        if( m_ResourcesUploadEvent == VK_NULL_HANDLE )
        {
            // Record upload commands.
            for( ImageResource& image : m_ImageResources )
            {
                RecordImageUploadCommands( commandBuffer, image );
            }

            // Signal event to notify that all resources have been uploaded.
            VkEventCreateInfo eventCreateInfo = {};
            eventCreateInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;

            pfn_vkCreateEvent(
                m_Device,
                &eventCreateInfo,
                nullptr,
                &m_ResourcesUploadEvent );

            pfn_vkCmdSetEvent(
                commandBuffer,
                m_ResourcesUploadEvent,
                VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT );
        }
    }

    /***********************************************************************************\

    Function:
        DestroyUploadResources

    Description:
        Destroy the temporary resouces used for uploading other resources to GPU.

    \***********************************************************************************/
    void OverlayVulkanBackend::DestroyUploadResources()
    {
        assert( m_ResourcesUploadEvent != VK_NULL_HANDLE );

        VkResult result = pfn_vkGetEventStatus( m_Device, m_ResourcesUploadEvent );
        if( result == VK_SUCCESS )
        {
            pfn_vkDestroyEvent( m_Device, m_ResourcesUploadEvent, nullptr );
            m_ResourcesUploadEvent = VK_NULL_HANDLE;

            for( ImageResource& image : m_ImageResources )
            {
                vmaDestroyBuffer( m_Allocator, image.UploadBuffer, image.UploadBufferAllocation );
                image.UploadBuffer = VK_NULL_HANDLE;
                image.UploadBufferAllocation = VK_NULL_HANDLE;
            }
        }
    }

    /***********************************************************************************\

    Function:
        DestroyResources

    Description:
        Destroy all resources created by this backend.

    \***********************************************************************************/
    void OverlayVulkanBackend::DestroyResources()
    {
        if( m_ResourcesUploadEvent != VK_NULL_HANDLE )
        {
            pfn_vkDestroyEvent( m_Device, m_ResourcesUploadEvent, nullptr );
            m_ResourcesUploadEvent = VK_NULL_HANDLE;
        }

        for( ImageResource& image : m_ImageResources )
        {
            DestroyImage( image );
        }

        m_ImageResources.clear();
    }

    /***********************************************************************************\

    Function:
        InitializeImage

    Description:
        Initialize image resource.

    \***********************************************************************************/
    VkResult OverlayVulkanBackend::InitializeImage( ImageResource& image, const ImageCreateInfo& createInfo )
    {
        VkResult result;
        VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
        VmaAllocationInfo uploadBufferAllocationInfo = {};

        const uint32_t imageDataSize = createInfo.Width * createInfo.Height * 4;

        // Save image size for upload.
        image.ImageExtent.width = createInfo.Width;
        image.ImageExtent.height = createInfo.Height;

        // Create image object.
        {
            VkImageCreateInfo imageCreateInfo = {};
            imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
            imageCreateInfo.format = format;
            imageCreateInfo.extent.width = createInfo.Width;
            imageCreateInfo.extent.height = createInfo.Height;
            imageCreateInfo.extent.depth = 1;
            imageCreateInfo.mipLevels = 1;
            imageCreateInfo.arrayLayers = 1;
            imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            VmaAllocationCreateInfo allocationCreateInfo = {};
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

            result = vmaCreateImage(
                m_Allocator,
                &imageCreateInfo,
                &allocationCreateInfo,
                &image.Image,
                &image.ImageAllocation,
                nullptr );
        }

        // Create image view.
        if( result == VK_SUCCESS )
        {
            VkImageViewCreateInfo imageViewCreateInfo = {};
            imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imageViewCreateInfo.image = image.Image;
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imageViewCreateInfo.format = format;
            imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
            imageViewCreateInfo.subresourceRange.levelCount = 1;
            imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
            imageViewCreateInfo.subresourceRange.layerCount = 1;

            result = pfn_vkCreateImageView(
                m_Device,
                &imageViewCreateInfo,
                nullptr,
                &image.ImageView );
        }

        // Create descriptor set for ImGui binding.
        if( result == VK_SUCCESS )
        {
            image.ImageDescriptorSet = ImGui_ImplVulkan_AddTexture(
                m_LinearSampler,
                image.ImageView,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

            if( !image.ImageDescriptorSet )
            {
                result = VK_ERROR_INITIALIZATION_FAILED;
            }
        }

        // Create buffer for uploading.
        if( result == VK_SUCCESS )
        {
            VkBufferCreateInfo bufferCreateInfo = {};
            bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferCreateInfo.size = imageDataSize;
            bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

            VmaAllocationCreateInfo bufferAllocationCreateInfo = {};
            bufferAllocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
            bufferAllocationCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

            result = vmaCreateBuffer(
                m_Allocator,
                &bufferCreateInfo,
                &bufferAllocationCreateInfo,
                &image.UploadBuffer,
                &image.UploadBufferAllocation,
                &uploadBufferAllocationInfo );
        }

        // Copy texture data to the upload buffer.
        if( result == VK_SUCCESS )
        {
            if( uploadBufferAllocationInfo.pMappedData != nullptr )
            {
                memcpy( uploadBufferAllocationInfo.pMappedData, createInfo.pData, imageDataSize );

                // Flush the buffer to make it visible to the GPU.
                result = vmaFlushAllocation(
                    m_Allocator,
                    image.UploadBufferAllocation,
                    0, imageDataSize );

                image.RequiresUpload = true;
            }
            else
            {
                // Failed to allocate mapped host-visible memory.
                result = VK_ERROR_INITIALIZATION_FAILED;
            }
        }

        // Destroy the image if any of the steps failed.
        if( result != VK_SUCCESS )
        {
            DestroyImage( image );
        }

        return result;
    }

    /***********************************************************************************\

    Function:
        DestroyImage

    Description:
        Destroy image resource.

    \***********************************************************************************/
    void OverlayVulkanBackend::DestroyImage( ImageResource& image )
    {
        if( image.ImageDescriptorSet != VK_NULL_HANDLE )
        {
            ImGui_ImplVulkan_RemoveTexture( image.ImageDescriptorSet );
            image.ImageDescriptorSet = VK_NULL_HANDLE;
        }

        if( image.UploadBuffer != VK_NULL_HANDLE )
        {
            vmaDestroyBuffer( m_Allocator, image.UploadBuffer, image.UploadBufferAllocation );
            image.UploadBuffer = VK_NULL_HANDLE;
            image.UploadBufferAllocation = VK_NULL_HANDLE;
        }

        if( image.ImageView != VK_NULL_HANDLE )
        {
            pfn_vkDestroyImageView( m_Device, image.ImageView, nullptr );
            image.ImageView = VK_NULL_HANDLE;
        }

        if( image.Image != VK_NULL_HANDLE )
        {
            vmaDestroyImage( m_Allocator, image.Image, image.ImageAllocation );
            image.Image = VK_NULL_HANDLE;
            image.ImageAllocation = VK_NULL_HANDLE;
        }
    }

    /***********************************************************************************\

    Function:
        RecordImageUploadCommands

    Description:
        Append image upload commands to the command buffer.

    \***********************************************************************************/
    void OverlayVulkanBackend::RecordImageUploadCommands( VkCommandBuffer commandBuffer, ImageResource& image )
    {
        if( image.RequiresUpload )
        {
            TransitionImageLayout( commandBuffer, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );

            VkBufferImageCopy region = {};
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.layerCount = 1;
            region.imageExtent.width = image.ImageExtent.width;
            region.imageExtent.height = image.ImageExtent.height;
            region.imageExtent.depth = 1;

            pfn_vkCmdCopyBufferToImage(
                commandBuffer,
                image.UploadBuffer,
                image.Image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &region );

            TransitionImageLayout( commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

            image.RequiresUpload = false;
        }
    }

    /***********************************************************************************\

    Function:
        TransitionImageLayout

    Description:
        Transition image to a new layout.

    \***********************************************************************************/
    void OverlayVulkanBackend::TransitionImageLayout( VkCommandBuffer commandBuffer, ImageResource& image, VkImageLayout oldLayout, VkImageLayout newLayout )
    {
        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image.Image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        pfn_vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier );
    }

    /***********************************************************************************\

    Function:
        OverlayVulkanLayerBackend

    Description:
        Constructor.

    \***********************************************************************************/
    OverlayVulkanLayerBackend::OverlayVulkanLayerBackend()
        : m_pDevice( nullptr )
        , m_pGraphicsQueue( nullptr )
        , m_pWindowContext( nullptr )
        , pfn_vkSetDeviceLoaderData( nullptr )
    {
    }

    /***********************************************************************************\

    Function:
        Initialize

    Description:
        Initializes the overlay backend for Vulkan layer environment.

    \***********************************************************************************/
    VkResult OverlayVulkanLayerBackend::Initialize( VkDevice_Object& device )
    {
        // Set members
        m_pDevice = &device;
        m_pGraphicsQueue = nullptr;

        pfn_vkSetDeviceLoaderData = m_pDevice->SetDeviceLoaderData;

        // Find suitable graphics queue.
        for( auto& it : m_pDevice->Queues )
        {
            if( it.second.Flags & VK_QUEUE_GRAPHICS_BIT )
            {
                m_pGraphicsQueue = &it.second;
                break;
            }
        }

        if( !m_pGraphicsQueue )
        {
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        // Initialize Vulkan backend.
        CreateInfo createInfo = {};
        createInfo.Instance = m_pDevice->pInstance->Handle;
        createInfo.PhysicalDevice = m_pDevice->pPhysicalDevice->Handle;
        createInfo.Device = m_pDevice->Handle;
        createInfo.Queue = m_pGraphicsQueue->Handle;
        createInfo.QueueFamilyIndex = m_pGraphicsQueue->Family;
        createInfo.ApiVersion = m_pDevice->pInstance->ApplicationInfo.apiVersion;
        createInfo.pfn_vkGetDeviceProcAddr = m_pDevice->Callbacks.GetDeviceProcAddr;
        createInfo.pfn_vkGetInstanceProcAddr = m_pDevice->pInstance->Callbacks.GetInstanceProcAddr;

        // Use Vulkan 1.0 if no version info was specified by the application.
        if( createInfo.ApiVersion == 0 )
        {
            createInfo.ApiVersion = VK_API_VERSION_1_0;
        }

        // Clamp to the version supported by the physical device.
        createInfo.ApiVersion = std::min(
            createInfo.ApiVersion,
            device.pPhysicalDevice->Properties.apiVersion );

        return OverlayVulkanBackend::Initialize( createInfo );
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:
        Destroys the overlay backend.

    \***********************************************************************************/
    void OverlayVulkanLayerBackend::Destroy()
    {
        OverlayVulkanBackend::Destroy();

        m_pDevice = nullptr;
        m_pGraphicsQueue = nullptr;
        m_pWindowContext = nullptr;
        pfn_vkSetDeviceLoaderData = nullptr;
    }

    /***********************************************************************************\

    Function:
        PrepareImGuiBackend

    Description:
        Initialize the ImGui backend for the current surface.

    \***********************************************************************************/
    bool OverlayVulkanLayerBackend::PrepareImGuiBackend()
    {
        // Create Vulkan backend.
        bool backendPrepared = OverlayVulkanBackend::PrepareImGuiBackend();

        // Create window backend.
        if( !m_pWindowContext )
        {
            try
            {
                const OSWindowHandle windowHandle =
                    m_pDevice->pInstance->Surfaces.at( m_Surface ).Window;

                switch( windowHandle.Type )
                {
#ifdef VK_USE_PLATFORM_WIN32_KHR
                case OSWindowHandleType::eWin32:
                    m_pWindowContext = new ImGui_ImplWin32_Context( windowHandle.Win32Handle );
                    break;
#endif // VK_USE_PLATFORM_WIN32_KHR

#ifdef VK_USE_PLATFORM_XCB_KHR
                case OSWindowHandleType::eXcb:
                    m_pWindowContext = new ImGui_ImplXcb_Context( windowHandle.XcbHandle );
                    break;
#endif // VK_USE_PLATFORM_XCB_KHR

#ifdef VK_USE_PLATFORM_XLIB_KHR
                case OSWindowHandleType::eXlib:
                    m_pWindowContext = new ImGui_ImplXlib_Context( windowHandle.XlibHandle );
                    break;
#endif // VK_USE_PLATFORM_XLIB_KHR

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
                case OSWindowHandleType::eWayland:
                    throw; // TODO: Implement ImGui Wayland context.
#endif                     // VK_USE_PLATFORM_WAYLAND_KHR

                default:
                    throw; // Not supported.
                }
            }
            catch( ... )
            {
                // Failed to create ImGui window context or surface object was not found.
                backendPrepared = false;
            }
        }

        return backendPrepared;
    }

    /***********************************************************************************\

    Function:
        DestroyImGuiBackend

    Description:
        Initialize the ImGui backend for the current surface.

    \***********************************************************************************/
    void OverlayVulkanLayerBackend::DestroyImGuiBackend()
    {
        OverlayVulkanBackend::DestroyImGuiBackend();

        delete m_pWindowContext;
        m_pWindowContext = nullptr;
    }

    /***********************************************************************************\

    Function:
        NewFrame

    Description:
        Begin rendering of a new frame.

    \***********************************************************************************/
    bool OverlayVulkanLayerBackend::NewFrame()
    {
        bool backendPrepared = OverlayVulkanBackend::NewFrame();
        if( backendPrepared )
        {
            m_pWindowContext->NewFrame();
        }

        return backendPrepared;
    }

    /***********************************************************************************\

    Function:
        AddInputCaptureRect

    Description:
        Add a rectangle to the list of input capture rectangles.

    \***********************************************************************************/
    void OverlayVulkanLayerBackend::AddInputCaptureRect( int x, int y, int width, int height )
    {
        m_pWindowContext->AddInputCaptureRect( x, y, width, height );
    }

    /***********************************************************************************\

    Function:
        GetDPIScale

    Description:
        Get the DPI scale of the current surface.

    \***********************************************************************************/
    float OverlayVulkanLayerBackend::GetDPIScale() const
    {
        return m_pWindowContext->GetDPIScale();
    }

    /***********************************************************************************\

    Function:
        LoadFunction

    Description:
        Load Vulkan function for ImGui backend.

    \***********************************************************************************/
    PFN_vkVoidFunction OverlayVulkanLayerBackend::LoadFunction( const char* pFunctionName )
    {
        // If function creates a dispatchable object, it must also set loader data.
        if( !strcmp( pFunctionName, "vkAllocateCommandBuffers" ) )
        {
            return reinterpret_cast<PFN_vkVoidFunction>( vkAllocateCommandBuffers );
        }

        // Nullopt if the function is not known.
        // Nullptr if the function is known but not found/supported.
        std::optional<PFN_vkVoidFunction> pfnKnownFunction;

        // Try to return a known device function first.
        pfnKnownFunction = m_pDevice->Callbacks.Get(
            m_Device,
            pFunctionName,
            VkLayerFunctionNotFoundBehavior::eReturnNullopt );

        if( pfnKnownFunction.has_value() )
        {
            return pfnKnownFunction.value();
        }

        // If the function is not found in the device dispatch table, try to find it in the instance dispatch table.
        pfnKnownFunction = m_pDevice->pInstance->Callbacks.Get(
            m_Instance,
            pFunctionName,
            VkLayerFunctionNotFoundBehavior::eReturnNullopt );

        if( pfnKnownFunction.has_value() )
        {
            return pfnKnownFunction.value();
        }

        // If the function is not known, try to get it from the next layer.
        PFN_vkVoidFunction pfnUnknownFunction = pfn_vkGetDeviceProcAddr(
            m_Device,
            pFunctionName );

        if( pfnUnknownFunction != nullptr )
        {
            return pfnUnknownFunction;
        }

        // Unknown function not found in the device chain, try to get it from the instance chain.
        pfnUnknownFunction = pfn_vkGetInstanceProcAddr(
            m_Instance,
            pFunctionName );

        return pfnUnknownFunction;
    }

    /***********************************************************************************\

    Function:
        vkAllocateCommandBuffers

    Description:
        Allocates command buffers.

    \***********************************************************************************/
    VkResult OverlayVulkanLayerBackend::vkAllocateCommandBuffers( VkDevice device, const VkCommandBufferAllocateInfo* pAllocateInfo, VkCommandBuffer* pCommandBuffers )
    {
        auto& dd = Profiler::VkDevice_Functions_Base::DeviceDispatch.Get( device );

        // Allocate the command buffers.
        VkResult result = dd.Device.Callbacks.AllocateCommandBuffers(
            device, pAllocateInfo, pCommandBuffers );

        // Command buffers are dispatchable handles, update pointers to parent's dispatch table.
        uint32_t initializedCommandBufferCount = 0;
        for( ; ( initializedCommandBufferCount < pAllocateInfo->commandBufferCount ) && ( result == VK_SUCCESS );
             ++initializedCommandBufferCount )
        {
            result = dd.Device.SetDeviceLoaderData(
                device,
                pCommandBuffers[initializedCommandBufferCount] );
        }

        if( result != VK_SUCCESS )
        {
            // Initialization of loader data failed, free all initialized command buffers.
            // Remaining command buffers must not be passed due to missing loader data.
            dd.Device.Callbacks.FreeCommandBuffers(
                device,
                pAllocateInfo->commandPool,
                initializedCommandBufferCount,
                pCommandBuffers );

            // Fill the output array with VK_NULL_HANDLEs.
            memset( pCommandBuffers, 0, sizeof( VkCommandBuffer ) * pAllocateInfo->commandBufferCount );
        }

        return result;
    }

    /***********************************************************************************\

    Function:
        AllocateCommandBuffers

    Description:
        Allocates command buffers.

    \***********************************************************************************/
    VkResult OverlayVulkanLayerBackend::AllocateCommandBuffers( const VkCommandBufferAllocateInfo& allocateInfo, VkCommandBuffer* pCommandBuffers )
    {
        VkResult result = OverlayVulkanBackend::AllocateCommandBuffers(
            allocateInfo,
            pCommandBuffers );

        // Command buffers are dispatchable handles, update pointers to parent's dispatch table.
        uint32_t initializedCommandBufferCount = 0;
        for( ; ( initializedCommandBufferCount < allocateInfo.commandBufferCount ) && ( result == VK_SUCCESS );
             ++initializedCommandBufferCount )
        {
            result = pfn_vkSetDeviceLoaderData(
                m_Device,
                pCommandBuffers[initializedCommandBufferCount] );
        }

        if( result != VK_SUCCESS )
        {
            // Initialization of loader data failed, free all initialized command buffers.
            // Remaining command buffers must not be passed due to missing loader data.
            pfn_vkFreeCommandBuffers(
                m_Device,
                allocateInfo.commandPool,
                initializedCommandBufferCount,
                pCommandBuffers );

            // Fill the output array with VK_NULL_HANDLEs.
            memset( pCommandBuffers, 0, sizeof( VkCommandBuffer ) * allocateInfo.commandBufferCount );
        }

        return result;
    }
}

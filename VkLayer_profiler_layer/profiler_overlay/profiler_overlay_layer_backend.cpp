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

#include "profiler_overlay_layer_backend.h"

#include "profiler_layer_objects/VkDevice_object.h"
#include "profiler_layer_objects/VkQueue_object.h"
#include "profiler_layer_objects/VkSurfaceKhr_object.h"
#include "profiler_layer_objects/VkSwapchainKhr_object.h"
#include "profiler_layer_functions/core/VkDevice_functions_base.h"

#include <imgui.h>
#include <imgui_impl_vulkan.h>

#ifdef VK_USE_PLATFORM_WIN32_KHR
#include "profiler_overlay_layer_backend_win32.h"
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
#include "profiler_overlay_layer_backend_xcb.h"
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
#include "profiler_overlay_layer_backend_xlib.h"
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
#include "profiler_overlay_layer_backend_wayland.h"
#endif

namespace Profiler
{
    /***********************************************************************************\

    Function:
        OverlayLayerBackend

    Description:
        Constructor.

    \***********************************************************************************/
    OverlayLayerBackend::OverlayLayerBackend()
    {
        ResetMembers();
    }

    /***********************************************************************************\

    Function:
        Initialize

    Description:
        Initialize the backend.

    \***********************************************************************************/
    VkResult OverlayLayerBackend::Initialize( VkDevice_Object& device )
    {
        VkResult result = VK_SUCCESS;

        // Set members
        m_pDevice = &device;
        m_pGraphicsQueue = nullptr;

        // Find suitable graphics queue
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

            result = m_pDevice->Callbacks.CreateDescriptorPool(
                m_pDevice->Handle,
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
            info.queueFamilyIndex = m_pGraphicsQueue->Family;

            result = m_pDevice->Callbacks.CreateCommandPool(
                m_pDevice->Handle,
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

            result = m_pDevice->Callbacks.CreateSampler(
                m_pDevice->Handle,
                &info,
                nullptr,
                &m_LinearSampler );
        }

        // Create memory allocator
        if( result == VK_SUCCESS )
        {
            result = m_MemoryManager.Initialize( m_pDevice );
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
    void OverlayLayerBackend::Destroy()
    {
        WaitIdle();

        DestroyImGuiBackend();
        DestroySwapchainResources();
        DestroyResources();

        if( m_DescriptorPool != VK_NULL_HANDLE )
        {
            m_pDevice->Callbacks.DestroyDescriptorPool(
                m_pDevice->Handle,
                m_DescriptorPool,
                nullptr );
        }

        if( m_CommandPool != VK_NULL_HANDLE )
        {
            m_pDevice->Callbacks.DestroyCommandPool(
                m_pDevice->Handle,
                m_CommandPool,
                nullptr );
        }

        if( m_LinearSampler != VK_NULL_HANDLE )
        {
            m_pDevice->Callbacks.DestroySampler(
                m_pDevice->Handle,
                m_LinearSampler,
                nullptr );
        }

        m_MemoryManager.Destroy();

        ResetMembers();
    }

    /***********************************************************************************\

    Function:
        IsInitialized

    Description:
        Check whether the backend is initialized.

    \***********************************************************************************/
    bool OverlayLayerBackend::IsInitialized() const
    {
        return m_Initialized;
    }

    /***********************************************************************************\

    Function:
        ResetSwapchain

    Description:
        Initialize the swapchain-dependent resources.

    \***********************************************************************************/
    VkResult OverlayLayerBackend::SetSwapchain( VkSwapchainKHR swapchain, const VkSwapchainCreateInfoKHR& createInfo )
    {
        VkResult result = VK_SUCCESS;

        // Get swapchain images
        uint32_t swapchainImageCount = 0;
        m_pDevice->Callbacks.GetSwapchainImagesKHR(
            m_pDevice->Handle, swapchain, &swapchainImageCount, nullptr );

        std::vector<VkImage> images( swapchainImageCount );
        result = m_pDevice->Callbacks.GetSwapchainImagesKHR(
            m_pDevice->Handle, swapchain, &swapchainImageCount, images.data() );

        assert( result == VK_SUCCESS );

        // Recreate render pass if swapchain format has changed
        if( ( result == VK_SUCCESS ) && ( createInfo.imageFormat != m_ImageFormat ) )
        {
            if( m_RenderPass != VK_NULL_HANDLE )
            {
                // Destroy old render pass
                m_pDevice->Callbacks.DestroyRenderPass( m_pDevice->Handle, m_RenderPass, nullptr );
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

            result = m_pDevice->Callbacks.CreateRenderPass(
                m_pDevice->Handle,
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
                    m_pDevice->Callbacks.DestroyFramebuffer( m_pDevice->Handle, m_Framebuffers[i], nullptr );
                    m_pDevice->Callbacks.DestroyImageView( m_pDevice->Handle, m_ImageViews[i], nullptr );
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

                    result = m_pDevice->Callbacks.CreateImageView(
                        m_pDevice->Handle,
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

                    result = m_pDevice->Callbacks.CreateFramebuffer(
                        m_pDevice->Handle,
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
                m_pDevice->Handle,
                &allocInfo,
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

                    result = m_pDevice->Callbacks.CreateFence(
                        m_pDevice->Handle,
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

                    result = m_pDevice->Callbacks.CreateSemaphore(
                        m_pDevice->Handle,
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
        m_ResetBackendsBeforeNextFrame = true;

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
    VkSwapchainKHR OverlayLayerBackend::GetSwapchain() const
    {
        return m_Swapchain;
    }

    /***********************************************************************************\

    Function:
        SetFramePresentInfo

    Description:
        Prepare VkPresentInfoKHR for the next frame.

    \***********************************************************************************/
    void OverlayLayerBackend::SetFramePresentInfo( const VkPresentInfoKHR& presentInfo )
    {
        m_PresentInfo = presentInfo;
    }

    /***********************************************************************************\

    Function:
        GetFramePresentInfo

    Description:
        Get overridden VkPresentInfoKHR prepared for the next frame.

    \***********************************************************************************/
    const VkPresentInfoKHR& OverlayLayerBackend::GetFramePresentInfo() const
    {
        return m_PresentInfo;
    }

    /***********************************************************************************\

    Function:
        InitializeImGuiBackend

    Description:
        Initialize the ImGui backend for Vulkan.

    \***********************************************************************************/
    bool OverlayLayerBackend::PrepareImGuiBackend()
    {
        if( m_ResetBackendsBeforeNextFrame )
        {
            // Reset ImGui backend due to swapchain recreation.
            DestroyImGuiBackend();
            m_ResetBackendsBeforeNextFrame = false;
        }

        if( !m_VulkanBackendInitialized )
        {
            // Load device functions required by the backend.
            if( !ImGui_ImplVulkan_LoadFunctions( FunctionLoader, this ) )
            {
                return false;
            }

            ImGui_ImplVulkan_InitInfo initInfo = {};
            initInfo.Instance = m_pDevice->pInstance->Handle;
            initInfo.PhysicalDevice = m_pDevice->pPhysicalDevice->Handle;
            initInfo.Device = m_pDevice->Handle;
            initInfo.QueueFamily = m_pGraphicsQueue->Family;
            initInfo.Queue = m_pGraphicsQueue->Handle;
            initInfo.DescriptorPool = m_DescriptorPool;
            initInfo.RenderPass = m_RenderPass;
            initInfo.MinImageCount = m_MinImageCount;
            initInfo.ImageCount = static_cast<uint32_t>( m_Images.size() );
            initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

            // Initialize the Vulkan backend.
            if( !ImGui_ImplVulkan_Init( &initInfo ) )
            {
                return false;
            }

            m_VulkanBackendInitialized = true;
        }

        if( !m_pPlatformBackend )
        {
            // Initialize the platform backend.
            try
            {
                const OSWindowHandle windowHandle =
                    m_pDevice->pInstance->Surfaces.at( m_Surface ).Window;

                switch( windowHandle.Type )
                {
#ifdef VK_USE_PLATFORM_WIN32_KHR
                case OSWindowHandleType::eWin32:
                    m_pPlatformBackend = new OverlayLayerWin32PlatformBackend( windowHandle.Win32Handle );
                    break;
#endif // VK_USE_PLATFORM_WIN32_KHR

#ifdef VK_USE_PLATFORM_XCB_KHR
                case OSWindowHandleType::eXcb:
                    m_pPlatformBackend = new OverlayLayerXcbPlatformBackend( windowHandle.XcbHandle );
                    break;
#endif // VK_USE_PLATFORM_XCB_KHR

#ifdef VK_USE_PLATFORM_XLIB_KHR
                case OSWindowHandleType::eXlib:
                    m_pPlatformBackend = new OverlayLayerXlibPlatformBackend( windowHandle.XlibHandle );
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
                return false;
            }
        }

        return true;
    }

    /***********************************************************************************\

    Function:
        InitializeImGuiBackend

    Description:
        Initialize the ImGui backend for Vulkan.

    \***********************************************************************************/
    void OverlayLayerBackend::DestroyImGuiBackend()
    {
        if( m_VulkanBackendInitialized )
        {
            ImGui_ImplVulkan_Shutdown();
            m_VulkanBackendInitialized = false;
        }

        if( m_pPlatformBackend )
        {
            delete m_pPlatformBackend;
            m_pPlatformBackend = nullptr;
        }
    }

    /***********************************************************************************\

    Function:
        WaitIdle

    Description:
        Wait for the GPU to finish rendering.

    \***********************************************************************************/
    void OverlayLayerBackend::WaitIdle()
    {
        if( m_LastSubmittedFence != VK_NULL_HANDLE )
        {
            m_pDevice->Callbacks.WaitForFences(
                m_pDevice->Handle, 1, &m_LastSubmittedFence, VK_TRUE, UINT64_MAX );

            // No need to wait for this fence again.
            m_LastSubmittedFence = VK_NULL_HANDLE;
        }
    }

    /***********************************************************************************\

    Function:
        GetDPIScale

    Description:
        Get the DPI scale of the current surface.

    \***********************************************************************************/
    float OverlayLayerBackend::GetDPIScale() const
    {
        return m_pPlatformBackend->GetDPIScale();
    }

    /***********************************************************************************\

    Function:
        GetRenderArea

    Description:
        Get the current render area.

    \***********************************************************************************/
    ImVec2 OverlayLayerBackend::GetRenderArea() const
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
    bool OverlayLayerBackend::NewFrame()
    {
        bool backendPrepared = PrepareImGuiBackend();
        if( backendPrepared )
        {
            ImGui_ImplVulkan_NewFrame();
            m_pPlatformBackend->NewFrame();
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
    void OverlayLayerBackend::RenderDrawData( ImDrawData* pDrawData )
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

        m_pDevice->Callbacks.WaitForFences( m_pDevice->Handle, 1, &fence, VK_TRUE, UINT64_MAX );
        m_pDevice->Callbacks.ResetFences( m_pDevice->Handle, 1, &fence );

        if( result == VK_SUCCESS )
        {
            VkCommandBufferBeginInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            result = m_pDevice->Callbacks.BeginCommandBuffer( commandBuffer, &info );
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
            m_pDevice->Callbacks.CmdBeginRenderPass( commandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE );
            ImGui_ImplVulkan_RenderDrawData( pDrawData, commandBuffer );
            m_pDevice->Callbacks.CmdEndRenderPass( commandBuffer );

            result = m_pDevice->Callbacks.EndCommandBuffer( commandBuffer );
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

            // Host access to the queue must be synchronized.
            // A lock is required because Present may be executed on a different queue (e.g., not supporting graphics operations).
            VkQueue_Object_InternalScope queueScope( *m_pGraphicsQueue );

            result = m_pDevice->Callbacks.QueueSubmit( m_pGraphicsQueue->Handle, 1, &info, fence );
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
    void* OverlayLayerBackend::CreateImage( int width, int height, const void* pData )
    {
        ImageResource image;
        VkResult result = InitializeImage( image, width, height, pData );

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
    void OverlayLayerBackend::DestroyImage( void* pImage )
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
    void OverlayLayerBackend::CreateFontsImage()
    {
        ImGui_ImplVulkan_CreateFontsTexture();
    }

    /***********************************************************************************\

    Function:
        DestroyFontsImage

    Description:
        Destroy the image resource for fonts.

    \***********************************************************************************/
    void OverlayLayerBackend::DestroyFontsImage()
    {
        ImGui_ImplVulkan_DestroyFontsTexture();
    }

    /***********************************************************************************\

    Function:
        ResetMembers

    Description:
        Set all members to initial values.

    \***********************************************************************************/
    void OverlayLayerBackend::ResetMembers()
    {
        m_pDevice = nullptr;
        m_pGraphicsQueue = nullptr;

        m_CommandPool = VK_NULL_HANDLE;
        m_DescriptorPool = VK_NULL_HANDLE;

        m_Initialized = false;

        m_ResourcesUploadEvent = VK_NULL_HANDLE;
        m_LinearSampler = VK_NULL_HANDLE;
        m_ImageResources.clear();

        ResetSwapchainMembers();
    }

    /***********************************************************************************\

    Function:
        DestroySwapchainResources

    Description:
        Destroy the resources associated with the current swapchain.

    \***********************************************************************************/
    void OverlayLayerBackend::DestroySwapchainResources()
    {
        if( m_RenderPass != VK_NULL_HANDLE )
        {
            m_pDevice->Callbacks.DestroyRenderPass( m_pDevice->Handle, m_RenderPass, nullptr );
        }

        for( VkFramebuffer framebuffer : m_Framebuffers )
        {
            if( framebuffer != VK_NULL_HANDLE )
            {
                m_pDevice->Callbacks.DestroyFramebuffer( m_pDevice->Handle, framebuffer, nullptr );
            }
        }

        for( VkImageView imageView : m_ImageViews )
        {
            if( imageView != VK_NULL_HANDLE )
            {
                m_pDevice->Callbacks.DestroyImageView( m_pDevice->Handle, imageView, nullptr );
            }
        }

        for( VkFence fence : m_CommandFences )
        {
            if( fence != VK_NULL_HANDLE )
            {
                m_pDevice->Callbacks.DestroyFence( m_pDevice->Handle, fence, nullptr );
            }
        }

        for( VkSemaphore semaphore : m_CommandSemaphores )
        {
            if( semaphore != VK_NULL_HANDLE )
            {
                m_pDevice->Callbacks.DestroySemaphore( m_pDevice->Handle, semaphore, nullptr );
            }
        }

        if( !m_CommandBuffers.empty() )
        {
            m_pDevice->Callbacks.FreeCommandBuffers(
                m_pDevice->Handle,
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
    void OverlayLayerBackend::ResetSwapchainMembers()
    {
        m_ResetBackendsBeforeNextFrame = false;
        m_VulkanBackendInitialized = false;

        m_pPlatformBackend = nullptr;

        m_Surface = VK_NULL_HANDLE;
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
        RecordUploadCommands

    Description:
        Upload resources to the GPU.

    \***********************************************************************************/
    void OverlayLayerBackend::RecordUploadCommands( VkCommandBuffer commandBuffer )
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

            VkResult result = m_pDevice->Callbacks.CreateEvent(
                m_pDevice->Handle,
                &eventCreateInfo,
                nullptr,
                &m_ResourcesUploadEvent );

            if( result == VK_SUCCESS )
            {
                m_pDevice->Callbacks.CmdSetEvent(
                    commandBuffer,
                    m_ResourcesUploadEvent,
                    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT );
            }
        }
    }

    /***********************************************************************************\

    Function:
        DestroyUploadResources

    Description:
        Destroy the temporary resouces used for uploading other resources to GPU.

    \***********************************************************************************/
    void OverlayLayerBackend::DestroyUploadResources()
    {
        assert( m_ResourcesUploadEvent != VK_NULL_HANDLE );

        VkResult result = m_pDevice->Callbacks.GetEventStatus( m_pDevice->Handle, m_ResourcesUploadEvent );
        if( result == VK_SUCCESS )
        {
            m_pDevice->Callbacks.DestroyEvent( m_pDevice->Handle, m_ResourcesUploadEvent, nullptr );
            m_ResourcesUploadEvent = VK_NULL_HANDLE;

            for( ImageResource& image : m_ImageResources )
            {
                m_MemoryManager.FreeBuffer( image.UploadBuffer, image.UploadBufferAllocation );
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
    void OverlayLayerBackend::DestroyResources()
    {
        if( m_ResourcesUploadEvent != VK_NULL_HANDLE )
        {
            m_pDevice->Callbacks.DestroyEvent( m_pDevice->Handle, m_ResourcesUploadEvent, nullptr );
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
    VkResult OverlayLayerBackend::InitializeImage( ImageResource& image, int width, int height, const void* pData )
    {
        VkResult result;
        VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
        VmaAllocationInfo uploadBufferAllocationInfo = {};

        const int imageDataSize = width * height * 4;

        // Save image size for upload.
        image.ImageExtent.width = static_cast<uint32_t>( width );
        image.ImageExtent.height = static_cast<uint32_t>( height );

        // Create image object.
        {
            VkImageCreateInfo imageCreateInfo = {};
            imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
            imageCreateInfo.format = format;
            imageCreateInfo.extent.width = image.ImageExtent.width;
            imageCreateInfo.extent.height = image.ImageExtent.height;
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

            result = m_MemoryManager.AllocateImage(
                imageCreateInfo,
                allocationCreateInfo,
                &image.Image,
                &image.ImageAllocation );
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

            result = m_pDevice->Callbacks.CreateImageView(
                m_pDevice->Handle,
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

            result = m_MemoryManager.AllocateBuffer(
                bufferCreateInfo,
                bufferAllocationCreateInfo,
                &image.UploadBuffer,
                &image.UploadBufferAllocation,
                &uploadBufferAllocationInfo );
        }

        // Copy texture data to the upload buffer.
        if( result == VK_SUCCESS )
        {
            if( uploadBufferAllocationInfo.pMappedData != nullptr )
            {
                memcpy( uploadBufferAllocationInfo.pMappedData, pData, imageDataSize );

                // Flush the buffer to make it visible to the GPU.
                result = m_MemoryManager.Flush( image.UploadBufferAllocation, 0, imageDataSize );

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
    void OverlayLayerBackend::DestroyImage( ImageResource& image )
    {
        if( image.ImageDescriptorSet != VK_NULL_HANDLE )
        {
            ImGui_ImplVulkan_RemoveTexture( image.ImageDescriptorSet );
            image.ImageDescriptorSet = VK_NULL_HANDLE;
        }

        if( image.UploadBuffer != VK_NULL_HANDLE )
        {
            m_MemoryManager.FreeBuffer( image.UploadBuffer, image.UploadBufferAllocation );
            image.UploadBuffer = VK_NULL_HANDLE;
            image.UploadBufferAllocation = VK_NULL_HANDLE;
        }

        if( image.ImageView != VK_NULL_HANDLE )
        {
            m_pDevice->Callbacks.DestroyImageView( m_pDevice->Handle, image.ImageView, nullptr );
            image.ImageView = VK_NULL_HANDLE;
        }

        if( image.Image != VK_NULL_HANDLE )
        {
            m_MemoryManager.FreeImage( image.Image, image.ImageAllocation );
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
    void OverlayLayerBackend::RecordImageUploadCommands( VkCommandBuffer commandBuffer, ImageResource& image )
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

            m_pDevice->Callbacks.CmdCopyBufferToImage(
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
    void OverlayLayerBackend::TransitionImageLayout( VkCommandBuffer commandBuffer, ImageResource& image, VkImageLayout oldLayout, VkImageLayout newLayout )
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

        m_pDevice->Callbacks.CmdPipelineBarrier(
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
        LoadFunction

    Description:
        Load Vulkan function for ImGui backend.

    \***********************************************************************************/
    PFN_vkVoidFunction OverlayLayerBackend::FunctionLoader( const char* pFunctionName, void* pUserData )
    {
        OverlayLayerBackend* pBackend = static_cast<OverlayLayerBackend*>( pUserData );

        // If function creates a dispatchable object, it must also set loader data.
        if( !strcmp( pFunctionName, "vkAllocateCommandBuffers" ) )
        {
            return reinterpret_cast<PFN_vkVoidFunction>( AllocateCommandBuffers );
        }

        // Nullopt if the function is not known.
        // Nullptr if the function is known but not found/supported.
        std::optional<PFN_vkVoidFunction> pfnKnownFunction;

        // Try to return a known device function first.
        pfnKnownFunction = pBackend->m_pDevice->Callbacks.Get(
            pBackend->m_pDevice->Handle,
            pFunctionName,
            VkLayerFunctionNotFoundBehavior::eReturnNullopt );

        if( pfnKnownFunction.has_value() )
        {
            return pfnKnownFunction.value();
        }

        // If the function is not found in the device dispatch table, try to find it in the instance dispatch table.
        pfnKnownFunction = pBackend->m_pDevice->pInstance->Callbacks.Get(
            pBackend->m_pDevice->pInstance->Handle,
            pFunctionName,
            VkLayerFunctionNotFoundBehavior::eReturnNullopt );

        if( pfnKnownFunction.has_value() )
        {
            return pfnKnownFunction.value();
        }

        // If the function is not known, try to get it from the next layer.
        PFN_vkVoidFunction pfnUnknownFunction = pBackend->m_pDevice->Callbacks.GetDeviceProcAddr(
            pBackend->m_pDevice->Handle,
            pFunctionName );

        if( pfnUnknownFunction != nullptr )
        {
            return pfnUnknownFunction;
        }

        // Unknown function not found in the device chain, try to get it from the instance chain.
        pfnUnknownFunction = pBackend->m_pDevice->pInstance->Callbacks.GetInstanceProcAddr(
            pBackend->m_pDevice->pInstance->Handle,
            pFunctionName );

        return pfnUnknownFunction;
    }

    /***********************************************************************************\

    Function:
        vkAllocateCommandBuffers

    Description:
        Allocates command buffers.

    \***********************************************************************************/
    VkResult OverlayLayerBackend::AllocateCommandBuffers( VkDevice device, const VkCommandBufferAllocateInfo* pAllocateInfo, VkCommandBuffer* pCommandBuffers )
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
}

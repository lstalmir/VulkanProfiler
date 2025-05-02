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

        // Create command pool for each queue family
        if( result == VK_SUCCESS )
        {
            VkCommandPoolCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            info.flags |= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

            for( auto& [_, queue] : m_pDevice->Queues )
            {
                auto& commandPool = m_CommandPools[queue.Family];
                if( !commandPool.Handle )
                {
                    result = m_pDevice->Callbacks.CreateCommandPool(
                        m_pDevice->Handle,
                        &info,
                        nullptr,
                        &commandPool.Handle );
                }

                if( result != VK_SUCCESS )
                {
                    break; 
                }
            }
        }

        // Create render semaphore
        if( result == VK_SUCCESS )
        {
            VkSemaphoreCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            result = m_pDevice->Callbacks.CreateSemaphore(
                m_pDevice->Handle,
                &info,
                nullptr,
                &m_RenderSemaphore );
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

        for( auto& [_, commandPool] : m_CommandPools )
        {
            for( VkFence fence : commandPool.CommandFences )
            {
                if( fence != VK_NULL_HANDLE )
                {
                    m_pDevice->Callbacks.DestroyFence(
                        m_pDevice->Handle,
                        fence,
                        nullptr );
                }
            }

            if( commandPool.Handle != VK_NULL_HANDLE )
            {
                m_pDevice->Callbacks.DestroyCommandPool(
                    m_pDevice->Handle,
                    commandPool.Handle,
                    nullptr );
            }
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
            attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkAttachmentReference color_attachment = {};
            color_attachment.attachment = 0;
            color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkSubpassDescription subpass = {};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &color_attachment;

            VkRenderPassCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            info.attachmentCount = 1;
            info.pAttachments = &attachment;
            info.subpassCount = 1;
            info.pSubpasses = &subpass;

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
                    m_pDevice->Callbacks.DestroyImageView( m_pDevice->Handle, m_ImageViews[i], nullptr );
                }

                m_ImageViews.clear();
            }

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
            }
        }

        // Allocate intermediate image resources
        if( ( result == VK_SUCCESS ) &&
            ( m_RenderArea.width != createInfo.imageExtent.width ||
                m_RenderArea.height != createInfo.imageExtent.height ) )
        {
            DestroyGuiImageResources();

            VkImageCreateInfo imageCreateInfo = {};
            imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
            imageCreateInfo.format = createInfo.imageFormat;
            imageCreateInfo.extent.width = createInfo.imageExtent.width;
            imageCreateInfo.extent.height = createInfo.imageExtent.height;
            imageCreateInfo.extent.depth = 1;
            imageCreateInfo.mipLevels = 1;
            imageCreateInfo.arrayLayers = 1;
            imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

            VmaAllocationCreateInfo allocationCreateInfo = {};
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

            result = m_MemoryManager.AllocateImage(
                imageCreateInfo,
                allocationCreateInfo,
                &m_GuiImage,
                &m_GuiImageAllocation );

            if( result == VK_SUCCESS )
            {
                VkImageViewCreateInfo imageViewCreateInfo = {};
                imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                imageViewCreateInfo.format = createInfo.imageFormat;
                imageViewCreateInfo.image = m_GuiImage;
                imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
                imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
                imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
                imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;

                VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
                imageViewCreateInfo.subresourceRange = range;

                result = m_pDevice->Callbacks.CreateImageView(
                    m_pDevice->Handle,
                    &imageViewCreateInfo,
                    nullptr,
                    &m_GuiImageView );
            }

            if( result == VK_SUCCESS )
            {
                VkFramebufferCreateInfo framebufferCreateInfo = {};
                framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebufferCreateInfo.renderPass = m_RenderPass;
                framebufferCreateInfo.attachmentCount = 1;
                framebufferCreateInfo.pAttachments = &m_GuiImageView;
                framebufferCreateInfo.width = createInfo.imageExtent.width;
                framebufferCreateInfo.height = createInfo.imageExtent.height;
                framebufferCreateInfo.layers = 1;

                result = m_pDevice->Callbacks.CreateFramebuffer(
                    m_pDevice->Handle,
                    &framebufferCreateInfo,
                    nullptr,
                    &m_GuiFramebuffer );
            }
        }

        // Update objects
        m_Swapchain = swapchain;
        m_Surface = createInfo.surface;
        m_RenderArea = createInfo.imageExtent;
        m_ImageFormat = createInfo.imageFormat;
        m_MinImageCount = createInfo.minImageCount;
        m_Images = std::move( images );

        m_GuiImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        m_GuiImageQueueFamilyIndex = m_pGraphicsQueue->Family;

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
    void OverlayLayerBackend::SetFramePresentInfo( VkQueue_Object& queue, const VkPresentInfoKHR& presentInfo )
    {
        m_PresentInfo = presentInfo;
        m_pPresentQueue = &queue;
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

        // Select queue for rendering.
        VkQueue_Object& graphicsQueue =
            ( ( m_pPresentQueue != nullptr ) && ( m_pPresentQueue->Flags & VK_QUEUE_GRAPHICS_BIT ) )
                ? *m_pPresentQueue
                : *m_pGraphicsQueue;

        // Record copy commands to the same command buffer if the present happens on the same queue,
        // or the present queue doesn't support compute operations.
        const bool submitCopyCommandsOnGraphicsQueue =
            ( m_pPresentQueue == nullptr ) ||
            ( m_pPresentQueue->Handle == graphicsQueue.Handle ) ||
            ( m_pPresentQueue->Flags & VK_QUEUE_COMPUTE_BIT ) == 0;

        // Grab command buffer for overlay commands.
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        VkFence fence = VK_NULL_HANDLE;
        result = AcquireCommandBuffer( graphicsQueue, &commandBuffer, &fence );

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
            RecordRenderCommands( commandBuffer, pDrawData );

            if( submitCopyCommandsOnGraphicsQueue )
            {
                RecordCopyCommands( commandBuffer );
            }

            result = m_pDevice->Callbacks.EndCommandBuffer( commandBuffer );
        }

        if( result == VK_SUCCESS )
        {
            std::vector<VkPipelineStageFlags> waitStages;

            // Submit upload and render commands to the graphics queue.
            VkSubmitInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            info.commandBufferCount = 1;
            info.pCommandBuffers = &commandBuffer;
            info.signalSemaphoreCount = 1;
            info.pSignalSemaphores = &m_RenderSemaphore;

            if( submitCopyCommandsOnGraphicsQueue )
            {
                waitStages.resize( m_PresentInfo.waitSemaphoreCount, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );

                info.waitSemaphoreCount = m_PresentInfo.waitSemaphoreCount;
                info.pWaitSemaphores = m_PresentInfo.pWaitSemaphores;
                info.pWaitDstStageMask = waitStages.data();
            }

            result = m_pDevice->Callbacks.QueueSubmit( graphicsQueue.Handle, 1, &info, fence );
        }

        if( result == VK_SUCCESS )
        {
            // Override wait semaphore.
            m_PresentInfo.waitSemaphoreCount = 1;
            m_PresentInfo.pWaitSemaphores = &m_RenderSemaphore;
        }

        if( result == VK_SUCCESS )
        {
            if( !submitCopyCommandsOnGraphicsQueue )
            {
                // Record copy commands to the additional command buffer and execute it before the present.
                result = AcquireCommandBuffer( *m_pPresentQueue, &commandBuffer, &fence );

                if( result == VK_SUCCESS )
                {
                    VkCommandBufferBeginInfo info = {};
                    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                    info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

                    result = m_pDevice->Callbacks.BeginCommandBuffer( commandBuffer, &info );
                }

                if( result == VK_SUCCESS )
                {
                    RecordCopyCommands( commandBuffer );
                    result = m_pDevice->Callbacks.EndCommandBuffer( commandBuffer );
                }

                if( result == VK_SUCCESS )
                {
                    // Synchronize with the graphics queue.
                    std::vector<VkSemaphore> waitSemaphores( m_PresentInfo.waitSemaphoreCount + 1 );
                    std::copy( m_PresentInfo.pWaitSemaphores,
                        m_PresentInfo.pWaitSemaphores + m_PresentInfo.waitSemaphoreCount,
                        waitSemaphores.begin() );
                    waitSemaphores[m_PresentInfo.waitSemaphoreCount] = m_RenderSemaphore;

                    std::vector<VkPipelineStageFlags> waitStages(
                        m_PresentInfo.waitSemaphoreCount + 1, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );

                    // Submit copy commands to the present queue.
                    VkSubmitInfo info = {};
                    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                    info.commandBufferCount = 1;
                    info.pCommandBuffers = &commandBuffer;
                    info.waitSemaphoreCount = static_cast<uint32_t>( waitSemaphores.size() );
                    info.pWaitSemaphores = waitSemaphores.data();
                    info.pWaitDstStageMask = waitStages.data();

                    result = m_pDevice->Callbacks.QueueSubmit( m_pPresentQueue->Handle, 1, &info, fence );
                }

                if( result == VK_SUCCESS )
                {
                    // Implicit synchronization with the QueueSubmit above.
                    m_PresentInfo.waitSemaphoreCount = 0;
                    m_PresentInfo.pWaitSemaphores = nullptr;
                }
            }
        }

        if( result == VK_SUCCESS )
        {
            m_LastSubmittedFence = fence;
        }
    }

    /***********************************************************************************\

    Function:
        RecordRenderCommands

    Description:
        Record draw commands into the command buffer.

    \***********************************************************************************/
    void OverlayLayerBackend::RecordRenderCommands( VkCommandBuffer commandBuffer, ImDrawData* pDrawData )
    {
        {
            // Transfer the image to the graphics queue and set color attachment layout for rendering.
            VkImageMemoryBarrier imageMemoryBarrier = {};
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.srcAccessMask = 0;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            imageMemoryBarrier.oldLayout = m_GuiImageLayout;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            imageMemoryBarrier.srcQueueFamilyIndex = m_GuiImageQueueFamilyIndex;
            imageMemoryBarrier.dstQueueFamilyIndex = m_pGraphicsQueue->Family;
            imageMemoryBarrier.image = m_GuiImage;
            imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
            imageMemoryBarrier.subresourceRange.levelCount = 1;
            imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
            imageMemoryBarrier.subresourceRange.layerCount = 1;

            m_pDevice->Callbacks.CmdPipelineBarrier(
                commandBuffer,
                VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_DEPENDENCY_BY_REGION_BIT,
                0, nullptr, 0, nullptr, 1, &imageMemoryBarrier );
        }
        {
            // Record Imgui Draw Data into the command buffer.
            VkRenderPassBeginInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            info.renderPass = m_RenderPass;
            info.framebuffer = m_GuiFramebuffer;
            info.renderArea.extent.width = m_RenderArea.width;
            info.renderArea.extent.height = m_RenderArea.height;

            m_pDevice->Callbacks.CmdBeginRenderPass( commandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE );
            ImGui_ImplVulkan_RenderDrawData( pDrawData, commandBuffer );
            m_pDevice->Callbacks.CmdEndRenderPass( commandBuffer );
        }
        {
            // Transfer the rendered image to the present queue.
            VkImageMemoryBarrier imageMemoryBarrier = {};
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            imageMemoryBarrier.srcQueueFamilyIndex = m_pGraphicsQueue->Family;
            imageMemoryBarrier.dstQueueFamilyIndex = m_pPresentQueue->Family;
            imageMemoryBarrier.image = m_GuiImage;
            imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
            imageMemoryBarrier.subresourceRange.levelCount = 1;
            imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
            imageMemoryBarrier.subresourceRange.layerCount = 1;

            m_pDevice->Callbacks.CmdPipelineBarrier(
                commandBuffer,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_DEPENDENCY_BY_REGION_BIT,
                0, nullptr, 0, nullptr, 1, &imageMemoryBarrier );

            m_GuiImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            m_GuiImageQueueFamilyIndex = m_pPresentQueue->Family;
        }
    }

    /***********************************************************************************\

    Function:
        RecordCopyCommands

    Description:
        Record copy commands into the command buffer.

    \***********************************************************************************/
    void OverlayLayerBackend::RecordCopyCommands( VkCommandBuffer commandBuffer )
    {
        if( !m_PresentInfo.swapchainCount || !m_PresentInfo.pImageIndices )
        {
            return;
        }

        VkImage swapchainImage = m_Images[m_PresentInfo.pImageIndices[0]];

        {
            // Transfer the presented image to the correct layout.
            VkImageMemoryBarrier imageMemoryBarrier = {};
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.srcAccessMask = 0;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.image = swapchainImage;
            imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
            imageMemoryBarrier.subresourceRange.levelCount = 1;
            imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
            imageMemoryBarrier.subresourceRange.layerCount = 1;

            m_pDevice->Callbacks.CmdPipelineBarrier(
                commandBuffer,
                VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_DEPENDENCY_BY_REGION_BIT,
                0, nullptr, 0, nullptr, 1, &imageMemoryBarrier );
        }
        {
            // Copy the rendered overlay to the presented image.
            VkImageCopy imageCopy = {};
            imageCopy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageCopy.srcSubresource.layerCount = 1;
            imageCopy.srcSubresource.mipLevel = 0;
            imageCopy.srcSubresource.baseArrayLayer = 0;
            imageCopy.srcOffset = { 0, 0, 0 };
            imageCopy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageCopy.dstSubresource.layerCount = 1;
            imageCopy.dstSubresource.mipLevel = 0;
            imageCopy.dstSubresource.baseArrayLayer = 0;
            imageCopy.dstOffset = { 0, 0, 0 };
            imageCopy.extent.width = m_RenderArea.width;
            imageCopy.extent.height = m_RenderArea.height;
            imageCopy.extent.depth = 1;

            m_pDevice->Callbacks.CmdCopyImage(
                commandBuffer,
                m_GuiImage,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                swapchainImage,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &imageCopy );
        }
        {
            // Transfer the presented image to the correct layout.
            VkImageMemoryBarrier imageMemoryBarrier = {};
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.image = swapchainImage;
            imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
            imageMemoryBarrier.subresourceRange.levelCount = 1;
            imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
            imageMemoryBarrier.subresourceRange.layerCount = 1;

            m_pDevice->Callbacks.CmdPipelineBarrier(
                commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_DEPENDENCY_BY_REGION_BIT,
                0, nullptr, 0, nullptr, 1, &imageMemoryBarrier );
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

        m_CommandPools.clear();
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

        for( VkImageView imageView : m_ImageViews )
        {
            if( imageView != VK_NULL_HANDLE )
            {
                m_pDevice->Callbacks.DestroyImageView( m_pDevice->Handle, imageView, nullptr );
            }
        }

        if( m_RenderSemaphore != VK_NULL_HANDLE )
        {
            m_pDevice->Callbacks.DestroySemaphore( m_pDevice->Handle, m_RenderSemaphore, nullptr );
        }

        DestroyGuiImageResources();
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
        m_pPresentQueue = nullptr;

        m_RenderPass = VK_NULL_HANDLE;
        m_RenderArea = { 0, 0 };
        m_ImageFormat = VK_FORMAT_UNDEFINED;
        m_MinImageCount = 0;
        m_Images.clear();
        m_ImageViews.clear();
        m_RenderSemaphore = VK_NULL_HANDLE;
        m_LastSubmittedFence = VK_NULL_HANDLE;

        ResetGuiImageMembers();
    }

    /***********************************************************************************\

    Function:
        DestroyGuiImageResources

    Description:
        Destroy the resources associated with the intermediate gui image.

    \***********************************************************************************/
    void OverlayLayerBackend::DestroyGuiImageResources()
    {
        if( m_GuiImage != VK_NULL_HANDLE )
        {
            m_MemoryManager.FreeImage( m_GuiImage, m_GuiImageAllocation );
        }

        if( m_GuiImageView != VK_NULL_HANDLE )
        {
            m_pDevice->Callbacks.DestroyImageView( m_pDevice->Handle, m_GuiImageView, nullptr );
        }

        if( m_GuiFramebuffer != VK_NULL_HANDLE )
        {
            m_pDevice->Callbacks.DestroyFramebuffer( m_pDevice->Handle, m_GuiFramebuffer, nullptr );
        }

        ResetGuiImageMembers();
    }

    /***********************************************************************************\

    Function:
        ResetGuiImageMembers

    Description:
        Set all members related to the intermediate gui image to initial values.

    \***********************************************************************************/
    void OverlayLayerBackend::ResetGuiImageMembers()
    {
        m_GuiImage = VK_NULL_HANDLE;
        m_GuiImageView = VK_NULL_HANDLE;
        m_GuiImageAllocation = VK_NULL_HANDLE;
        m_GuiFramebuffer = VK_NULL_HANDLE;
        m_GuiImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        m_GuiImageQueueFamilyIndex = 0;
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
        AcquireCommandBuffer

    Description:
        Get the command buffer for rendering on the selected command queue.

    \***********************************************************************************/
    VkResult OverlayLayerBackend::AcquireCommandBuffer( const VkQueue_Object& queue, VkCommandBuffer* pCommandBuffer, VkFence* pFence )
    {
        const uint32_t maxCommandBufferCount = ( m_Images.size() * 2 ) + 1;

        // Get the command pool for the selected queue.
        CommandPool& commandPool = m_CommandPools.at( queue.Family );

        // Check if the next command buffer has already finished rendering.
        if( !commandPool.CommandBuffers.empty() )
        {
            VkCommandBuffer commandBuffer = commandPool.CommandBuffers[commandPool.NextCommandBufferIndex];
            VkFence fence = commandPool.CommandFences[commandPool.NextCommandBufferIndex];

            uint64_t timeout = 0;
            if( commandPool.CommandBuffers.size() >= maxCommandBufferCount )
            {
                timeout = UINT64_MAX;
            }

            VkResult result = m_pDevice->Callbacks.WaitForFences( m_pDevice->Handle, 1, &fence, VK_TRUE, timeout );
            if( result == VK_SUCCESS )
            {
                // Reset the fence and command buffer.
                m_pDevice->Callbacks.ResetFences( m_pDevice->Handle, 1, &fence );
                m_pDevice->Callbacks.ResetCommandBuffer( commandBuffer, 0 );

                // Update the command buffer index.
                commandPool.NextCommandBufferIndex = ( commandPool.NextCommandBufferIndex + 1 ) % commandPool.CommandBuffers.size();

                *pCommandBuffer = commandBuffer;
                *pFence = fence;
                return VK_SUCCESS;
            }

            if( commandPool.CommandBuffers.size() >= maxCommandBufferCount )
            {
                // Don't allocate the new command buffers if limit has been reached and the wait failed.
                return result;
            }
        }

        // Allocate a new command buffer.
        VkCommandBufferAllocateInfo allocateInfo = {};
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.commandPool = commandPool.Handle;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = 1;

        VkResult result = AllocateCommandBuffers( m_pDevice->Handle, &allocateInfo, pCommandBuffer );
        if( result == VK_SUCCESS )
        {
            // Create a new fence for the command buffer.
            VkFenceCreateInfo fenceInfo = {};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            result = m_pDevice->Callbacks.CreateFence( m_pDevice->Handle, &fenceInfo, nullptr, pFence );
            if( result == VK_SUCCESS )
            {
                // Insert the command buffer and the fence at the current index.
                commandPool.CommandBuffers.insert(
                    commandPool.CommandBuffers.begin() + commandPool.NextCommandBufferIndex, *pCommandBuffer );
                commandPool.CommandFences.insert(
                    commandPool.CommandFences.begin() + commandPool.NextCommandBufferIndex, *pFence );

                commandPool.NextCommandBufferIndex = ( commandPool.NextCommandBufferIndex + 1 ) % commandPool.CommandBuffers.size();
            }
            else
            {
                // Failed to create a fence, free the command buffer.
                m_pDevice->Callbacks.FreeCommandBuffers( m_pDevice->Handle, commandPool.Handle, 1, pCommandBuffer );
            }
        }

        return result;
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

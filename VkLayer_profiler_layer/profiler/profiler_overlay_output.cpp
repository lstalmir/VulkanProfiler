#include "profiler_overlay_output.h"
#include "profiler.h"
#include "profiler_command_buffer.h"
#include "imgui_impl_vulkan_layer.h"
#include "imgui/examples/imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

namespace Profiler
{
    ProfilerOverlayOutput::ProfilerOverlayOutput( Profiler& profiler )
        : m_Profiler( profiler )
        , m_pContext( nullptr )
    {
    }

    VkResult ProfilerOverlayOutput::Initialize(
        const VkSwapchainCreateInfoKHR* pCreateInfo,
        VkSwapchainKHR swapchain )
    {
        ImGui_ImplVulkan_InitInfo imGuiInitInfo;
        ClearMemory( &imGuiInitInfo );

        // Find suitable graphics queue
        for( const VkQueue_Object& queue : m_Profiler.m_pDevice->Queues )
        {
            if( queue.Flags & VK_QUEUE_GRAPHICS_BIT )
            {
                imGuiInitInfo.Queue = queue.Handle;
                imGuiInitInfo.QueueFamily = queue.Family;

                m_GraphicsQueue = queue.Handle;
                break;
            }
        }

        // Get swapchain images
        uint32_t swapchainImageCount = 0;
        m_Profiler.m_pDevice->Callbacks.GetSwapchainImagesKHR(
            m_Profiler.m_pDevice->Handle,
            swapchain,
            &swapchainImageCount,
            nullptr );

        std::vector<VkImage> swapchainImages( swapchainImageCount );
        m_Profiler.m_pDevice->Callbacks.GetSwapchainImagesKHR(
            m_Profiler.m_pDevice->Handle,
            swapchain,
            &swapchainImageCount,
            swapchainImages.data() );

        // Create internal descriptor pool
        VkDescriptorPoolCreateInfo descriptorPoolCreateInfo;
        ClearStructure( &descriptorPoolCreateInfo, VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO );

        // TODO: Is this necessary?
        const VkDescriptorPoolSize descriptorPoolSizes[] = {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } };

        descriptorPoolCreateInfo.maxSets = 1000;
        descriptorPoolCreateInfo.poolSizeCount = std::extent_v<decltype(descriptorPoolSizes)>;
        descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes;

        m_Profiler.m_pDevice->Callbacks.CreateDescriptorPool(
            m_Profiler.m_pDevice->Handle,
            &descriptorPoolCreateInfo,
            nullptr,
            &m_DescriptorPool );

        // Create the Render Pass
        {
            VkAttachmentDescription attachment = {};
            attachment.format = pCreateInfo->imageFormat;
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

            VkResult result = m_Profiler.m_pDevice->Callbacks.CreateRenderPass(
                m_Profiler.m_pDevice->Handle, &info, nullptr, &m_RenderPass );

            if( result != VK_SUCCESS )
            {
                Destroy();
                return result;
            }

            m_RenderArea = pCreateInfo->imageExtent;
        }

        // Create The Image Views and Framebuffers
        {
            VkImageViewCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            info.format = pCreateInfo->imageFormat;
            info.components.r = VK_COMPONENT_SWIZZLE_R;
            info.components.g = VK_COMPONENT_SWIZZLE_G;
            info.components.b = VK_COMPONENT_SWIZZLE_B;
            info.components.a = VK_COMPONENT_SWIZZLE_A;

            VkImageSubresourceRange image_range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
            info.subresourceRange = image_range;

            for( uint32_t i = 0; i < swapchainImageCount; i++ )
            {
                VkImageView imageView = nullptr;
                VkFramebuffer framebuffer = nullptr;

                info.image = swapchainImages[ i ];

                VkResult result = m_Profiler.m_pDevice->Callbacks.CreateImageView(
                    m_Profiler.m_pDevice->Handle, &info, nullptr, &imageView );

                if( result != VK_SUCCESS )
                {
                    Destroy();
                    return result;
                }

                m_ImageViews.push_back( imageView );

                VkFramebufferCreateInfo info = {};
                info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                info.renderPass = m_RenderPass;
                info.attachmentCount = 1;
                info.pAttachments = &imageView;
                info.width = pCreateInfo->imageExtent.width;
                info.height = pCreateInfo->imageExtent.height;
                info.layers = 1;

                result = m_Profiler.m_pDevice->Callbacks.CreateFramebuffer(
                    m_Profiler.m_pDevice->Handle, &info, nullptr, &framebuffer );

                if( result != VK_SUCCESS )
                {
                    Destroy();
                    return result;
                }

                m_Framebuffers.push_back( framebuffer );
            }
        }

        // Create command buffers
        {
            VkCommandPoolCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            info.flags |= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            info.queueFamilyIndex = imGuiInitInfo.QueueFamily;

            VkResult result = m_Profiler.m_pDevice->Callbacks.CreateCommandPool(
                m_Profiler.m_pDevice->Handle, &info, nullptr, &m_CommandPool );

            if( result != VK_SUCCESS )
            {
                Destroy();
                return result;
            }

            VkCommandBufferAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = m_CommandPool;
            allocInfo.commandBufferCount = swapchainImageCount;

            std::vector<VkCommandBuffer> commandBuffers( swapchainImageCount );
            
            result = m_Profiler.m_pDevice->Callbacks.AllocateCommandBuffers(
                m_Profiler.m_pDevice->Handle, &allocInfo, commandBuffers.data() );

            if( result != VK_SUCCESS )
            {
                Destroy();
                return result;
            }

            m_CommandBuffers = commandBuffers;

            for( int i = 0; i < swapchainImageCount; ++i )
            {
                VkFenceCreateInfo fenceInfo = {};
                fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
                fenceInfo.flags |= VK_FENCE_CREATE_SIGNALED_BIT;

                VkFence fence;

                result = m_Profiler.m_pDevice->Callbacks.CreateFence(
                    m_Profiler.m_pDevice->Handle, &fenceInfo, nullptr, &fence );

                if( result != VK_SUCCESS )
                {
                    Destroy();
                    return result;
                }

                m_CommandFences.push_back( fence );

                VkSemaphoreCreateInfo semaphoreInfo = {};
                semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
                
                VkSemaphore semaphore;

                result = m_Profiler.m_pDevice->Callbacks.CreateSemaphore(
                    m_Profiler.m_pDevice->Handle, &semaphoreInfo, nullptr, &semaphore );

                if( result != VK_SUCCESS )
                {
                    Destroy();
                    return result;
                }

                m_CommandSemaphores.push_back( semaphore );
            }

            m_CommandBufferIndex = UINT32_MAX;
            m_AcquiredImageIndex = UINT32_MAX;
        }

        // Init ImGui
        IMGUI_CHECKVERSION();

        m_pContext = ImGui::CreateContext();
        ImGui::SetCurrentContext( m_pContext );

        ImGui::StyleColorsDark();

        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2( 1920, 1080 );
        io.DeltaTime = 1.0f / 60.0f;

        io.ConfigFlags = ImGuiConfigFlags_None;

        // Build atlas
        unsigned char* tex_pixels = NULL;
        int tex_w, tex_h;
        io.Fonts->GetTexDataAsRGBA32( &tex_pixels, &tex_w, &tex_h );

        // Init window
        void* windowHandle = m_Profiler.m_pDevice->pInstance->Surfaces.at( pCreateInfo->surface ).WindowHandle;

        ImGui_ImplWin32_Init( windowHandle );
        OriginalWindowProc = (WNDPROC)GetWindowLongPtr( (HWND)windowHandle, GWLP_WNDPROC );

        SetWindowLongPtr( (HWND)windowHandle, GWLP_WNDPROC, (LONG_PTR)ProfilerOverlayOutput::WindowProc );

        imGuiInitInfo.Instance = m_Profiler.m_pDevice->pInstance->Handle;
        imGuiInitInfo.PhysicalDevice = m_Profiler.m_pDevice->PhysicalDevice;
        imGuiInitInfo.Device = m_Profiler.m_pDevice->Handle;

        imGuiInitInfo.pInstanceDispatchTable = &m_Profiler.m_pDevice->pInstance->Callbacks;
        imGuiInitInfo.pDispatchTable = &m_Profiler.m_pDevice->Callbacks;

        imGuiInitInfo.Allocator = nullptr;
        imGuiInitInfo.PipelineCache = nullptr;
        imGuiInitInfo.CheckVkResultFn = nullptr;

        imGuiInitInfo.MinImageCount = pCreateInfo->minImageCount;
        imGuiInitInfo.ImageCount = swapchainImageCount;
        imGuiInitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

        imGuiInitInfo.DescriptorPool = m_DescriptorPool;

        if( !ImGui_ImplVulkan_Init( &imGuiInitInfo, m_RenderPass ) )
        {
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        // Initialize fonts
        m_Profiler.m_pDevice->Callbacks.ResetFences(
            m_Profiler.m_pDevice->Handle, 1, &m_CommandFences[ 0 ] );

        {
            VkCommandBufferBeginInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            m_Profiler.m_pDevice->Callbacks.BeginCommandBuffer(
                m_CommandBuffers[ 0 ], &info );
        }

        ImGui_ImplVulkan_CreateFontsTexture( m_CommandBuffers[ 0 ] );

        {
            VkSubmitInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            info.commandBufferCount = 1;
            info.pCommandBuffers = &m_CommandBuffers[ 0 ];

            m_Profiler.m_pDevice->Callbacks.EndCommandBuffer( m_CommandBuffers[ 0 ] );
            m_Profiler.m_pDevice->Callbacks.QueueSubmit( m_GraphicsQueue, 1, &info, m_CommandFences[ 0 ] );
        }

        ImGui::SetCurrentContext( m_pContext );

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        return VK_SUCCESS;
    }

    void ProfilerOverlayOutput::Destroy()
    {
        m_Profiler.m_pDevice->Callbacks.DeviceWaitIdle(
            m_Profiler.m_pDevice->Handle );

        if( m_pContext )
        {
            ImGui_ImplVulkan_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext( m_pContext );

            m_pContext = nullptr;
        }

        if( m_DescriptorPool )
        {
            m_Profiler.m_pDevice->Callbacks.DestroyDescriptorPool(
                m_Profiler.m_pDevice->Handle, m_DescriptorPool, nullptr );

            m_DescriptorPool = nullptr;
        }

        if( m_RenderPass )
        {
            m_Profiler.m_pDevice->Callbacks.DestroyRenderPass(
                m_Profiler.m_pDevice->Handle, m_RenderPass, nullptr );

            m_RenderPass = nullptr;
        }

        for( auto& framebuffer : m_Framebuffers )
        {
            m_Profiler.m_pDevice->Callbacks.DestroyFramebuffer(
                m_Profiler.m_pDevice->Handle, framebuffer, nullptr );

            framebuffer = nullptr;
        }

        m_Framebuffers.clear();

        for( auto& imageView : m_ImageViews )
        {
            m_Profiler.m_pDevice->Callbacks.DestroyImageView(
                m_Profiler.m_pDevice->Handle, imageView, nullptr );

            imageView = nullptr;
        }

        m_ImageViews.clear();

        if( m_CommandPool )
        {
            m_Profiler.m_pDevice->Callbacks.FreeCommandBuffers(
                m_Profiler.m_pDevice->Handle,
                m_CommandPool,
                m_CommandBuffers.size(),
                m_CommandBuffers.data() );

            m_CommandBuffers.clear();

            m_Profiler.m_pDevice->Callbacks.DestroyCommandPool(
                m_Profiler.m_pDevice->Handle,
                m_CommandPool,
                nullptr );

            for( auto& fence : m_CommandFences )
            {
                m_Profiler.m_pDevice->Callbacks.DestroyFence(
                    m_Profiler.m_pDevice->Handle, fence, nullptr );

                fence = nullptr;
            }

            m_CommandFences.clear();

            for( auto& semaphore : m_CommandSemaphores )
            {
                m_Profiler.m_pDevice->Callbacks.DestroySemaphore(
                    m_Profiler.m_pDevice->Handle, semaphore, nullptr );

                semaphore = nullptr;
            }

            m_CommandSemaphores.clear();
        }
    }

    void ProfilerOverlayOutput::AcquireNextImage( uint32_t acquiredImageIndex )
    {
        m_CommandBufferIndex = (m_CommandBufferIndex + 1) % m_CommandBuffers.size();
        m_AcquiredImageIndex = acquiredImageIndex;
    }

    void ProfilerOverlayOutput::Present( VkPresentInfoKHR* pPresentInfo )
    {
        ImGui::Render();

        VkFence fence = m_CommandFences[ m_CommandBufferIndex ];
        VkSemaphore semaphore = m_CommandSemaphores[ m_CommandBufferIndex ];
        VkCommandBuffer commandBuffer = m_CommandBuffers[ m_CommandBufferIndex ];
        VkFramebuffer framebuffer = m_Framebuffers[ m_AcquiredImageIndex ];

        m_Profiler.m_pDevice->Callbacks.WaitForFences(
            m_Profiler.m_pDevice->Handle, 1, &fence, VK_TRUE, UINT64_MAX );

        m_Profiler.m_pDevice->Callbacks.ResetFences(
            m_Profiler.m_pDevice->Handle, 1, &fence );

        {
            VkCommandBufferBeginInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            m_Profiler.m_pDevice->Callbacks.BeginCommandBuffer( commandBuffer, &info );
        }
        {
            VkRenderPassBeginInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            info.renderPass = m_RenderPass;
            info.framebuffer = framebuffer;
            info.renderArea.extent.width = m_RenderArea.width;
            info.renderArea.extent.height = m_RenderArea.height;
            m_Profiler.m_pDevice->Callbacks.CmdBeginRenderPass( commandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE );
        }

        // Record Imgui Draw Data and draw funcs into command buffer
        ImGui_ImplVulkan_RenderDrawData( ImGui::GetDrawData(), commandBuffer );

        // Submit command buffer
        m_Profiler.m_pDevice->Callbacks.CmdEndRenderPass( commandBuffer );

        {
            VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            VkSubmitInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            info.waitSemaphoreCount = pPresentInfo->waitSemaphoreCount;
            info.pWaitSemaphores = pPresentInfo->pWaitSemaphores;
            info.pWaitDstStageMask = &wait_stage;
            info.commandBufferCount = 1;
            info.pCommandBuffers = &commandBuffer;
            info.signalSemaphoreCount = 1;
            info.pSignalSemaphores = &semaphore;

            m_Profiler.m_pDevice->Callbacks.EndCommandBuffer( commandBuffer );
            m_Profiler.m_pDevice->Callbacks.QueueSubmit( m_GraphicsQueue, 1, &info, fence );
        }

        // Override wait semaphore
        pPresentInfo->waitSemaphoreCount = 1;
        pPresentInfo->pWaitSemaphores = &semaphore;

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    void ProfilerOverlayOutput::BeginWindow()
    {
        ImGui::Begin( "VkProfiler" );
    }

    void ProfilerOverlayOutput::WriteSubmit( const ProfilerSubmitData& submit )
    {
        for( const auto& cmdBuffer : submit.m_CommandBuffers )
        {
            if( ImGui::TreeNode( m_Profiler.m_Debug.GetDebugObjectName( (uint64_t)cmdBuffer.m_CommandBuffer ).c_str() ) )
            {
                for( const auto& renderPass : cmdBuffer.m_RenderPassPipelineCount )
                {
                    if( ImGui::TreeNode( m_Profiler.m_Debug.GetDebugObjectName( (uint64_t)renderPass.first ).c_str() ) )
                    {
                        ImGui::TreePop();
                    }
                }
                ImGui::TreePop();
            }
        }
    }

    void ProfilerOverlayOutput::EndWindow()
    {
        ImGui::End();
    }

    void ProfilerOverlayOutput::Flush()
    {
    }

    LRESULT CALLBACK ProfilerOverlayOutput::WindowProc( HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam )
    {
        // Update overlay
        ImGui_ImplWin32_WndProcHandler( hWnd, Msg, wParam, lParam );
        return CallWindowProc( OriginalWindowProc, hWnd, Msg, wParam, lParam );
    }
}

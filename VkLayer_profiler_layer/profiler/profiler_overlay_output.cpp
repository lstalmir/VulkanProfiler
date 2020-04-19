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
        for( const auto& queue : m_Profiler.m_pDevice->Queues )
        {
            if( queue.second.Flags & VK_QUEUE_GRAPHICS_BIT )
            {
                m_pGraphicsQueue = &queue.second;

                imGuiInitInfo.Queue = m_pGraphicsQueue->Handle;
                imGuiInitInfo.QueueFamily = m_pGraphicsQueue->Family;
                break;
            }
        }

        m_pSwapchain = &m_Profiler.m_pDevice->Swapchains[ swapchain ];

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

            VkSubpassDependency dependeny = {};
            dependeny.srcSubpass = VK_SUBPASS_EXTERNAL;
            dependeny.dstSubpass = 0;
            dependeny.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependeny.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependeny.srcAccessMask = 0;
            dependeny.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            VkRenderPassCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            info.attachmentCount = 1;
            info.pAttachments = &attachment;
            info.subpassCount = 1;
            info.pSubpasses = &subpass;
            info.dependencyCount = 1;
            info.pDependencies = &dependeny;

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
            
            // TODO: WTF?
            result = m_Profiler.m_pDevice->Callbacks.AllocateCommandBuffers(
                m_Profiler.m_pDevice->Handle, &allocInfo, commandBuffers.data() );

            if( result != VK_SUCCESS )
            {
                Destroy();
                return result;
            }

            m_CommandBuffers = commandBuffers;

            for( auto cmdBuffer : m_CommandBuffers )
            {
                VkDebugMarkerObjectNameInfoEXT info = {};
                info.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
                info.objectType = VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT;
                info.pObjectName = "ProfilerOverlayCommandBuffer";
                info.object = (uint64_t)cmdBuffer;

                m_Profiler.m_pDevice->Callbacks.DebugMarkerSetObjectNameEXT(
                    m_Profiler.m_pDevice->Handle, &info );
            }

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
            m_Profiler.m_pDevice->Callbacks.QueueSubmit( m_pGraphicsQueue->Handle, 1, &info, m_CommandFences[ 0 ] );
        }

        return VK_SUCCESS;
    }

    void ProfilerOverlayOutput::Destroy()
    {
        m_Profiler.m_pDevice->Callbacks.DeviceWaitIdle(
            m_Profiler.m_pDevice->Handle );

        if( OriginalWindowProc )
        {
            // Restore original window proc
            SetWindowLongPtr( (HWND)m_pSwapchain->pSurface->WindowHandle,
                GWLP_WNDPROC, (LONG_PTR)OriginalWindowProc );

            OriginalWindowProc = nullptr;
        }

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

    void ProfilerOverlayOutput::Present( const VkQueue_Object& queue, VkPresentInfoKHR* pPresentInfo )
    {
        if( ImGui::GetDrawData() )
        {
            // Grab command buffer for overlay commands
            const uint32_t imageIndex = pPresentInfo->pImageIndices[ 0 ];

            // Per-
            VkFence& fence = m_CommandFences[ imageIndex ];
            VkSemaphore& semaphore = m_CommandSemaphores[ imageIndex ];
            VkCommandBuffer& commandBuffer = m_CommandBuffers[ imageIndex ];
            VkFramebuffer& framebuffer = m_Framebuffers[ imageIndex ];

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
                m_Profiler.m_pDevice->Callbacks.QueueSubmit( m_pGraphicsQueue->Handle, 1, &info, fence );
            }

            // Override wait semaphore
            pPresentInfo->waitSemaphoreCount = 1;
            pPresentInfo->pWaitSemaphores = &semaphore;
        }
    }

    void ProfilerOverlayOutput::Update( const ProfilerAggregatedData& data )
    {
        ImGui::SetCurrentContext( m_pContext );

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin( "VkProfiler" );

        // GPU properties
        ImGui::Text( "Device: %s", m_Profiler.m_DeviceProperties.deviceName );

        TextAlignRight( "Vulkan %u.%u",
            VK_VERSION_MAJOR( m_Profiler.m_pDevice->pInstance->ApplicationInfo.apiVersion ),
            VK_VERSION_MINOR( m_Profiler.m_pDevice->pInstance->ApplicationInfo.apiVersion ) );

        ImGui::BeginTabBar( "" );

        if( ImGui::BeginTabItem( "Performance" ) )
        {
            UpdatePerformanceTab( data );
            ImGui::EndTabItem();
        }
        if( ImGui::BeginTabItem( "Memory" ) )
        {
            UpdateMemoryTab( data );
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();

        ImGui::TextColored( { 1, 1, 0, 1 }, "%s", Summary.Message.c_str() );

        ImGui::End();
        ImGui::Render();
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

    void ProfilerOverlayOutput::UpdatePerformanceTab( const ProfilerAggregatedData& data )
    {
        // Histogram
        {
            std::vector<float> contributions;

            if( data.m_Stats.m_TotalTicks != 0 )
                for( const auto& submit : data.m_Submits )
                {
                    for( const auto& cmdBuffer : submit.m_CommandBuffers )
                    {
                        for( const auto& renderPass : cmdBuffer.m_Subregions )
                        {
                            contributions.push_back( renderPass.m_Stats.m_TotalTicks );
                        }
                    }
                }

            ImGui::PushItemWidth( -1 );
            ImGui::PlotHistogram(
                "",
                contributions.data(),
                contributions.size(),
                0, "GPU Cycles", FLT_MAX, FLT_MAX, { 0, 80 } );
        }

        ImGui::Separator();

        // Frame browser
        if( ImGui::TreeNode( "Frame browser" ) )
        {
            uint32_t index = 0;

            for( const auto& submit : data.m_Submits )
            {
                if( ImGui::TreeNode( "VkSubmitInfo" ) )
                {
                    for( const auto& cmdBuffer : submit.m_CommandBuffers )
                    {
                        if( ImGui::TreeNode( "",
                            m_Profiler.m_Debug.GetDebugObjectName( (uint64_t)cmdBuffer.m_Handle ).c_str() ) )
                        {
                            TextAlignRight( "%f ms",
                                (cmdBuffer.m_Stats.m_TotalTicks / m_Profiler.m_TimestampPeriod) / 1000000.f );

                            for( const auto& renderPass : cmdBuffer.m_Subregions )
                            {
                                if( renderPass.m_Handle == VK_NULL_HANDLE )
                                {
                                    // TODO: "API Calls"
                                    continue;
                                }

                                if( ImGui::TreeNode( m_Profiler.m_Debug.GetDebugObjectName( (uint64_t)renderPass.m_Handle ).c_str() ) )
                                {
                                    TextAlignRight( "%f ms",
                                        (renderPass.m_Stats.m_TotalTicks / m_Profiler.m_TimestampPeriod) / 1000000.f );

                                    for( const auto& pipeline : renderPass.m_Subregions )
                                    {
                                        if( pipeline.m_Handle == VK_NULL_HANDLE )
                                        {
                                            // TODO: "API Calls"
                                            continue;
                                        }

                                        if( ImGui::TreeNode( m_Profiler.m_Debug.GetDebugObjectName( (uint64_t)pipeline.m_Handle ).c_str() ) )
                                        {
                                            TextAlignRight( "%f ms",
                                                (pipeline.m_Stats.m_TotalTicks / m_Profiler.m_TimestampPeriod) / 1000000.f );

                                            ImGui::TreePop();
                                        }
                                        else TextAlignRight( "%f ms",
                                            (pipeline.m_Stats.m_TotalTicks / m_Profiler.m_TimestampPeriod) / 1000000.f );
                                    }

                                    ImGui::TreePop();
                                }
                                else TextAlignRight( "%f ms",
                                    (renderPass.m_Stats.m_TotalTicks / m_Profiler.m_TimestampPeriod) / 1000000.f );

                            }
                            ImGui::TreePop();
                        }
                        else TextAlignRight( "%f ms",
                            (cmdBuffer.m_Stats.m_TotalTicks / m_Profiler.m_TimestampPeriod ) / 1000000.f );
                    }
                    ImGui::TreePop();
                }

                index++;
            }
            ImGui::TreePop();
        }
    }

    void ProfilerOverlayOutput::UpdateMemoryTab( const ProfilerAggregatedData& data )
    {
        const VkPhysicalDeviceMemoryProperties* pMemoryProperties =
            &m_Profiler.m_MemoryProperties2.memoryProperties;

        const VkPhysicalDeviceMemoryBudgetPropertiesEXT* pMemoryBudgetProperties =
            (const VkPhysicalDeviceMemoryBudgetPropertiesEXT*)m_Profiler.m_MemoryProperties2.pNext;

        while( pMemoryBudgetProperties &&
            pMemoryBudgetProperties->sType != VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT )
        {
            pMemoryBudgetProperties =
                (const VkPhysicalDeviceMemoryBudgetPropertiesEXT*)pMemoryBudgetProperties->pNext;
        }

        ImGui::TextUnformatted( "Memory heap usage" );

        float totalMemoryUsage = 0.f;
        if( pMemoryBudgetProperties )
        {
            for( int i = 0; i < pMemoryProperties->memoryHeapCount; ++i )
            {
                float usage = 0.f;
                char usageStr[ 64 ] = {};

                if( pMemoryBudgetProperties->heapBudget[ i ] != 0 )
                {
                    usage = (float)pMemoryBudgetProperties->heapUsage[ i ] /
                        pMemoryBudgetProperties->heapBudget[ i ];

                    sprintf_s( usageStr, "%.2f/%.2f MB (%.1f%%)",
                        pMemoryBudgetProperties->heapUsage[ i ] / 1048576.f,
                        pMemoryBudgetProperties->heapBudget[ i ] / 1048576.f,
                        usage * 100.f );
                }

                ImGui::ProgressBar( usage, { -1, 0 }, usageStr );
            }
        }
        else
        {
            for( int i = 0; i < pMemoryProperties->memoryHeapCount; ++i )
            {
                float usage = 0.f;
                char usageStr[ 64 ] = {};

                if( pMemoryProperties->memoryHeaps[ i ].size != 0 )
                {
                    uint64_t allocatedSize = 0;

                    if( pMemoryProperties->memoryHeaps[ i ].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT )
                    {
                        allocatedSize = data.m_Memory.m_DeviceLocalAllocationSize;
                    }

                    usage = (float)allocatedSize / pMemoryProperties->memoryHeaps[ i ].size;

                    sprintf_s( usageStr, "%.2f/%.2f MB (%.1f%%)",
                        allocatedSize / 1048576.f,
                        pMemoryProperties->memoryHeaps[ i ].size / 1048576.f,
                        usage * 100.f );
                }

                ImGui::ProgressBar( usage, { -1, 0 }, usageStr );
            }
        }

        ImGui::TextUnformatted( "Memory allocations" );


    }

    void ProfilerOverlayOutput::TextAlignRight( const char* fmt, ... )
    {
        va_list args;
        va_start( args, fmt );

        char text[ 128 ];
        vsprintf_s( text, fmt, args );

        va_end( args );

        uint32_t textSize = ImGui::CalcTextSize( text ).x;

        ImGui::SameLine( ImGui::GetWindowContentRegionMax().x - textSize );
        ImGui::TextUnformatted( text );
    }
}

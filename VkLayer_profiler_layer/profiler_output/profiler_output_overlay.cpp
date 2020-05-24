#include "profiler_output_overlay.h"
#include "imgui_impl_vulkan_layer.h"
#include "imgui/examples/imgui_impl_win32.h"
#include <string>
#include <sstream>

#include "imgui_widgets/imgui_histogram_ex.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

namespace Profiler
{
    // Define static members
    std::mutex ProfilerOverlayOutput::s_ImGuiMutex;
    LockableUnorderedMap<void*, WNDPROC> ProfilerOverlayOutput::s_pfnWindowProc;

    /***********************************************************************************\

    Function:
        ProfilerOverlayOutput

    Description:
        Constructor.

    \***********************************************************************************/
    ProfilerOverlayOutput::ProfilerOverlayOutput(
        VkDevice_Object& device,
        VkQueue_Object& graphicsQueue,
        VkSwapchainKHR_Object& swapchain,
        const VkSwapchainCreateInfoKHR* pCreateInfo )
        : m_Device( device )
        , m_GraphicsQueue( graphicsQueue )
        , m_Swapchain( swapchain )
        , m_pWindowHandle( nullptr )
        , m_pImGuiContext( nullptr )
        , m_pImGuiVulkanContext( nullptr )
        , m_DescriptorPool( nullptr )
        , m_RenderPass( nullptr )
        , m_RenderArea( {} )
        , m_Images()
        , m_ImageViews()
        , m_Framebuffers()
        , m_CommandPool( nullptr )
        , m_CommandBuffers()
        , m_CommandFences()
        , m_CommandSemaphores()
        , m_TimestampPeriod( device.Properties.limits.timestampPeriod / 1000000.f )
        , m_FrameBrowserSortMode( FrameBrowserSortMode::eSubmissionOrder )
        , m_Pause( false )
    {
        // Get swapchain images
        uint32_t swapchainImageCount = 0;
        m_Device.Callbacks.GetSwapchainImagesKHR(
            m_Device.Handle,
            m_Swapchain.Handle,
            &swapchainImageCount,
            nullptr );

        m_Images.resize( swapchainImageCount );
        m_Device.Callbacks.GetSwapchainImagesKHR(
            m_Device.Handle,
            m_Swapchain.Handle,
            &swapchainImageCount,
            m_Images.data() );

        // Create internal descriptor pool
        VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
        descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;

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

        m_Device.Callbacks.CreateDescriptorPool(
            m_Device.Handle,
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

            VkResult result = m_Device.Callbacks.CreateRenderPass(
                m_Device.Handle, &info, nullptr, &m_RenderPass );

            if( result != VK_SUCCESS )
            {
                // Class is marked final, destructor must not be virtual
                ProfilerOverlayOutput::~ProfilerOverlayOutput();
                throw result;
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

                info.image = m_Images[ i ];

                VkResult result = m_Device.Callbacks.CreateImageView(
                    m_Device.Handle, &info, nullptr, &imageView );

                if( result != VK_SUCCESS )
                {
                    // Class is marked final, destructor must not be virtual
                    ProfilerOverlayOutput::~ProfilerOverlayOutput();
                    throw result;
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

                result = m_Device.Callbacks.CreateFramebuffer(
                    m_Device.Handle, &info, nullptr, &framebuffer );

                if( result != VK_SUCCESS )
                {
                    // Class is marked final, destructor must not be virtual
                    ProfilerOverlayOutput::~ProfilerOverlayOutput();
                    throw result;
                }

                m_Framebuffers.push_back( framebuffer );
            }
        }

        // Create command buffers
        {
            VkCommandPoolCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            info.flags |= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            info.queueFamilyIndex = m_GraphicsQueue.Family;

            VkResult result = m_Device.Callbacks.CreateCommandPool(
                m_Device.Handle, &info, nullptr, &m_CommandPool );

            if( result != VK_SUCCESS )
            {
                // Class is marked final, destructor must not be virtual
                ProfilerOverlayOutput::~ProfilerOverlayOutput();
                throw result;
            }

            VkCommandBufferAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = m_CommandPool;
            allocInfo.commandBufferCount = swapchainImageCount;

            std::vector<VkCommandBuffer> commandBuffers( swapchainImageCount );
            
            result = m_Device.Callbacks.AllocateCommandBuffers(
                m_Device.Handle, &allocInfo, commandBuffers.data() );

            if( result != VK_SUCCESS )
            {
                // Class is marked final, destructor must not be virtual
                ProfilerOverlayOutput::~ProfilerOverlayOutput();
                throw result;
            }

            m_CommandBuffers = commandBuffers;

            for( auto cmdBuffer : m_CommandBuffers )
            {
                // Command buffers are dispatchable handles, update pointers to parent's dispatch table
                m_Device.SetDeviceLoaderData( m_Device.Handle, cmdBuffer );
                
                VkDebugMarkerObjectNameInfoEXT info = {};
                info.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
                info.objectType = VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT;
                info.pObjectName = "ProfilerOverlayCommandBuffer";
                info.object = (uint64_t)cmdBuffer;

                m_Device.Callbacks.DebugMarkerSetObjectNameEXT( m_Device.Handle, &info );
            }

            for( int i = 0; i < swapchainImageCount; ++i )
            {
                VkFenceCreateInfo fenceInfo = {};
                fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
                fenceInfo.flags |= VK_FENCE_CREATE_SIGNALED_BIT;

                VkFence fence;

                result = m_Device.Callbacks.CreateFence(
                    m_Device.Handle, &fenceInfo, nullptr, &fence );

                if( result != VK_SUCCESS )
                {
                    // Class is marked final, destructor must not be virtual
                    ProfilerOverlayOutput::~ProfilerOverlayOutput();
                    throw result;
                }

                m_CommandFences.push_back( fence );

                VkSemaphoreCreateInfo semaphoreInfo = {};
                semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
                
                VkSemaphore semaphore;

                result = m_Device.Callbacks.CreateSemaphore(
                    m_Device.Handle, &semaphoreInfo, nullptr, &semaphore );

                if( result != VK_SUCCESS )
                {
                    // Class is marked final, destructor must not be virtual
                    ProfilerOverlayOutput::~ProfilerOverlayOutput();
                    throw result;
                }

                m_CommandSemaphores.push_back( semaphore );
            }
        }

        // Init ImGui
        std::scoped_lock lk( s_ImGuiMutex );
        IMGUI_CHECKVERSION();

        m_pImGuiContext = ImGui::CreateContext();

        ImGui::SetCurrentContext( m_pImGuiContext );
        ImGui::StyleColorsDark();

        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = { (float)m_RenderArea.width, (float)m_RenderArea.height };
        io.DeltaTime = 1.0f / 60.0f;

        io.ConfigFlags = ImGuiConfigFlags_None;

        // Build atlas
        unsigned char* tex_pixels = NULL;
        int tex_w, tex_h;
        io.Fonts->GetTexDataAsRGBA32( &tex_pixels, &tex_w, &tex_h );

        // Init window
        void* windowHandle = m_Device.pInstance->Surfaces.at( pCreateInfo->surface ).WindowHandle;

        ImGui_ImplWin32_Init( windowHandle );

        // Override window procedure
        {
            WNDPROC wndProc = (WNDPROC)GetWindowLongPtr( (HWND)windowHandle, GWLP_WNDPROC );
            s_pfnWindowProc.interlocked_emplace( windowHandle, wndProc );

            SetWindowLongPtr( (HWND)windowHandle,
                GWLP_WNDPROC, (LONG_PTR)ProfilerOverlayOutput::WindowProc );
        }

        ImGui_ImplVulkan_InitInfo imGuiInitInfo;
        std::memset( &imGuiInitInfo, 0, sizeof( imGuiInitInfo ) );

        imGuiInitInfo.Queue = m_GraphicsQueue.Handle;
        imGuiInitInfo.QueueFamily = m_GraphicsQueue.Family;

        imGuiInitInfo.Instance = m_Device.pInstance->Handle;
        imGuiInitInfo.PhysicalDevice = m_Device.PhysicalDevice;
        imGuiInitInfo.Device = m_Device.Handle;

        imGuiInitInfo.pInstanceDispatchTable = &m_Device.pInstance->Callbacks;
        imGuiInitInfo.pDispatchTable = &m_Device.Callbacks;

        imGuiInitInfo.Allocator = nullptr;
        imGuiInitInfo.PipelineCache = nullptr;
        imGuiInitInfo.CheckVkResultFn = nullptr;

        imGuiInitInfo.MinImageCount = pCreateInfo->minImageCount;
        imGuiInitInfo.ImageCount = swapchainImageCount;
        imGuiInitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

        imGuiInitInfo.DescriptorPool = m_DescriptorPool;

        m_pImGuiVulkanContext = new (std::nothrow) ImGui_ImplVulkan_Context();

        if( !m_pImGuiVulkanContext )
        {
            // Class is marked final, destructor must not be virtual
            ProfilerOverlayOutput::~ProfilerOverlayOutput();
            throw VK_ERROR_OUT_OF_HOST_MEMORY;
        }

        if( !m_pImGuiVulkanContext->Init( &imGuiInitInfo, m_RenderPass ) )
        {
            // Class is marked final, destructor must not be virtual
            ProfilerOverlayOutput::~ProfilerOverlayOutput();
            throw VK_ERROR_INITIALIZATION_FAILED;
        }

        // Initialize fonts
        m_Device.Callbacks.ResetFences(
            m_Device.Handle, 1, &m_CommandFences[ 0 ] );

        {
            VkCommandBufferBeginInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            m_Device.Callbacks.BeginCommandBuffer(
                m_CommandBuffers[ 0 ], &info );
        }

        m_pImGuiVulkanContext->CreateFontsTexture( m_CommandBuffers[ 0 ] );

        {
            VkSubmitInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            info.commandBufferCount = 1;
            info.pCommandBuffers = &m_CommandBuffers[ 0 ];

            m_Device.Callbacks.EndCommandBuffer( m_CommandBuffers[ 0 ] );
            m_Device.Callbacks.QueueSubmit( m_GraphicsQueue.Handle, 1, &info, m_CommandFences[ 0 ] );
        }
    }

    /***********************************************************************************\

    Function:
        ~ProfilerOverlayOutput

    Description:
        Destructor.

    \***********************************************************************************/
    ProfilerOverlayOutput::~ProfilerOverlayOutput()
    {
        m_Device.Callbacks.DeviceWaitIdle( m_Device.Handle );

        if( m_pWindowHandle )
        {
            std::scoped_lock lk( s_pfnWindowProc );

            // Restore original window proc
            SetWindowLongPtr( (HWND)m_pWindowHandle,
                GWLP_WNDPROC, (LONG_PTR)s_pfnWindowProc.at( m_pWindowHandle ) );

            s_pfnWindowProc.erase( m_pWindowHandle );

            m_pWindowHandle = nullptr;
        }

        if( m_pImGuiVulkanContext )
        {
            m_pImGuiVulkanContext->Shutdown();
            delete m_pImGuiVulkanContext;

            m_pImGuiVulkanContext = nullptr;
        }

        if( m_pImGuiContext )
        {
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext( m_pImGuiContext );

            m_pImGuiContext = nullptr;
        }

        if( m_DescriptorPool )
        {
            m_Device.Callbacks.DestroyDescriptorPool(
                m_Device.Handle, m_DescriptorPool, nullptr );

            m_DescriptorPool = nullptr;
        }

        if( m_RenderPass )
        {
            m_Device.Callbacks.DestroyRenderPass(
                m_Device.Handle, m_RenderPass, nullptr );

            m_RenderPass = nullptr;
        }

        for( auto& framebuffer : m_Framebuffers )
        {
            m_Device.Callbacks.DestroyFramebuffer(
                m_Device.Handle, framebuffer, nullptr );

            framebuffer = nullptr;
        }

        m_Framebuffers.clear();

        for( auto& imageView : m_ImageViews )
        {
            m_Device.Callbacks.DestroyImageView(
                m_Device.Handle, imageView, nullptr );

            imageView = nullptr;
        }

        m_ImageViews.clear();

        if( m_CommandPool )
        {
            m_Device.Callbacks.FreeCommandBuffers(
                m_Device.Handle,
                m_CommandPool,
                m_CommandBuffers.size(),
                m_CommandBuffers.data() );

            m_CommandBuffers.clear();

            m_Device.Callbacks.DestroyCommandPool(
                m_Device.Handle,
                m_CommandPool,
                nullptr );

            for( auto& fence : m_CommandFences )
            {
                m_Device.Callbacks.DestroyFence(
                    m_Device.Handle, fence, nullptr );

                fence = nullptr;
            }

            m_CommandFences.clear();

            for( auto& semaphore : m_CommandSemaphores )
            {
                m_Device.Callbacks.DestroySemaphore(
                    m_Device.Handle, semaphore, nullptr );

                semaphore = nullptr;
            }

            m_CommandSemaphores.clear();
        }
    }

    /***********************************************************************************\

    Function:
        Present

    Description:
        Draw profiler overlay before presenting the image to screen.

    \***********************************************************************************/
    void ProfilerOverlayOutput::Present(
        const ProfilerAggregatedData& data,
        const VkQueue_Object& queue,
        VkPresentInfoKHR* pPresentInfo )
    {
        // Record interface draw commands
        Update( data );

        if( ImGui::GetDrawData() )
        {
            // Grab command buffer for overlay commands
            const uint32_t imageIndex = pPresentInfo->pImageIndices[ 0 ];

            // Per-
            VkFence& fence = m_CommandFences[ imageIndex ];
            VkSemaphore& semaphore = m_CommandSemaphores[ imageIndex ];
            VkCommandBuffer& commandBuffer = m_CommandBuffers[ imageIndex ];
            VkFramebuffer& framebuffer = m_Framebuffers[ imageIndex ];

            m_Device.Callbacks.WaitForFences(
                m_Device.Handle, 1, &fence, VK_TRUE, UINT64_MAX );

            m_Device.Callbacks.ResetFences(
                m_Device.Handle, 1, &fence );

            {
                VkCommandBufferBeginInfo info = {};
                info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
                m_Device.Callbacks.BeginCommandBuffer( commandBuffer, &info );
            }
            {
                VkRenderPassBeginInfo info = {};
                info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                info.renderPass = m_RenderPass;
                info.framebuffer = framebuffer;
                info.renderArea.extent.width = m_RenderArea.width;
                info.renderArea.extent.height = m_RenderArea.height;
                m_Device.Callbacks.CmdBeginRenderPass( commandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE );
            }

            // Record Imgui Draw Data and draw funcs into command buffer
            m_pImGuiVulkanContext->RenderDrawData( ImGui::GetDrawData(), commandBuffer );

            // Submit command buffer
            m_Device.Callbacks.CmdEndRenderPass( commandBuffer );

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

                m_Device.Callbacks.EndCommandBuffer( commandBuffer );
                m_Device.Callbacks.QueueSubmit( m_GraphicsQueue.Handle, 1, &info, fence );
            }

            // Override wait semaphore
            pPresentInfo->waitSemaphoreCount = 1;
            pPresentInfo->pWaitSemaphores = &semaphore;
        }
    }

    /***********************************************************************************\

    Function:
        Update

    Description:
        Update overlay.

    \***********************************************************************************/
    void ProfilerOverlayOutput::Update( const ProfilerAggregatedData& data )
    {
        std::scoped_lock lk( s_ImGuiMutex );
        ImGui::SetCurrentContext( m_pImGuiContext );

        m_pImGuiVulkanContext->NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin( "VkProfiler" );

        // GPU properties
        ImGui::Text( "Device: %s", m_Device.Properties.deviceName );

        TextAlignRight( "Vulkan %u.%u",
            VK_VERSION_MAJOR( m_Device.pInstance->ApplicationInfo.apiVersion ),
            VK_VERSION_MINOR( m_Device.pInstance->ApplicationInfo.apiVersion ) );

        // Keep results
        ImGui::Checkbox( "Pause", &m_Pause );

        if( !m_Pause )
        {
            // Update data
            m_Data = data;
        }

        ImGui::BeginTabBar( "" );

        if( ImGui::BeginTabItem( "Performance" ) )
        {
            UpdatePerformanceTab();
            ImGui::EndTabItem();
        }
        if( ImGui::BeginTabItem( "Memory" ) )
        {
            UpdateMemoryTab();
            ImGui::EndTabItem();
        }
        if( ImGui::BeginTabItem( "Statistics" ) )
        {
            UpdateStatisticsTab();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();

        ImGui::End();
        ImGui::Render();
    }

    /***********************************************************************************\

    Function:
        WindowProc

    Description:
        Overrides standard window procedure on Windows. Invokes ImGui handler to intercept
        incoming user input, then calls original window procedure.

    \***********************************************************************************/
    LRESULT CALLBACK ProfilerOverlayOutput::WindowProc( HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam )
    {
        // Update overlay
        const LRESULT result = ImGui_ImplWin32_WndProcHandler( hWnd, Msg, wParam, lParam );
        
        // Don't pass handled mouse messages to application
        if( Msg >= WM_MOUSEFIRST &&
            Msg <= WM_MOUSELAST &&
            ImGui::GetIO().WantCaptureMouse )
        {
            return result;
        }

        // Call original window proc
        return CallWindowProc( s_pfnWindowProc.interlocked_at( hWnd ), hWnd, Msg, wParam, lParam );
    }

    /***********************************************************************************\

    Function:
        UpdatePerformanceTab

    Description:
        Updates "Performance" tab.

    \***********************************************************************************/
    void ProfilerOverlayOutput::UpdatePerformanceTab()
    {
        // Header
        {
            const float gpuTime = m_Data.m_Stats.m_TotalTicks * m_TimestampPeriod;
            const float cpuTime = m_Data.m_CPU.m_TimeNs / 1000000.f;

            ImGui::Text( "GPU Time: %.2f ms", gpuTime );
            ImGui::Text( "CPU Time: %.2f ms", cpuTime );
            TextAlignRight( "%.0f fps", 1000.f / cpuTime );
        }

        // Histogram
        {
            std::vector<float> contributions;

            if( m_Data.m_Stats.m_TotalTicks > 0 )
            {
                // Enumerate submits in frame
                for( const auto& submit : m_Data.m_Submits )
                {
                    // Enumerate command buffers in submit
                    for( const auto& cmdBuffer : submit.m_CommandBuffers )
                    {
                        // Enumerate render passes in command buffer
                        for( const auto& renderPass : cmdBuffer.m_Subregions )
                        {
                            // Insert render pass cycle count to histogram
                            contributions.push_back( renderPass.m_Stats.m_TotalTicks );
                        }
                    }
                }
            }

            ImGui::PushItemWidth( -1 );
            ImGuiX::PlotHistogramEx(
                "",
                contributions.data(), // Scale x with y
                contributions.data(),
                contributions.size(),
                0, "GPU Cycles (Render passes)", 0, FLT_MAX, { 0, 80 } );
        }

        // Top pipelines
        if( ImGui::CollapsingHeader( "Top pipelines" ) )
        {
            uint32_t i = 0;

            for( const auto& pipeline : m_Data.m_TopPipelines )
            {
                if( pipeline.m_Handle != VK_NULL_HANDLE )
                {
                    ImGui::Text( "%2u. %s", i + 1,
                        GetDebugObjectName( VK_OBJECT_TYPE_UNKNOWN, (uint64_t)pipeline.m_Handle ).c_str() );

                    TextAlignRight( "%.2f ms", pipeline.m_Stats.m_TotalTicks * m_TimestampPeriod );

                    // Print up to 10 top pipelines
                    if( (++i) == 10 ) break;
                }
            }
        }

        // Vendor-specific
        if( m_Device.VendorID == VkDevice_Vendor_ID::eINTEL &&
            ImGui::CollapsingHeader( "INTEL Performance counters" ) )
        {
            if( !m_Data.m_VendorMetrics.empty() )
            {
                for( const auto& [label, value] : m_Data.m_VendorMetrics )
                {
                    ImGui::Text( "%s", label.c_str() );
                    TextAlignRight( "%.2f", value );
                }
            }
            else
            {
                ImGui::Text( "No metrics available" );
            }
        }

        // Frame browser
        if( ImGui::CollapsingHeader( "Frame browser" ) )
        {
            // Select sort mode
            {
                static const char* sortOptions[] = {
                    "Submission order",
                    "Duration descending",
                    "Duration ascending" };

                const char* selectedOption = sortOptions[ (size_t)m_FrameBrowserSortMode ];

                ImGui::Text( "Sort" );
                ImGui::SameLine();

                if( ImGui::BeginCombo( "FrameBrowserSortMode", selectedOption ) )
                {
                    for( size_t i = 0; i < std::extent_v<decltype(sortOptions)>; ++i )
                    {
                        bool isSelected = (selectedOption == sortOptions[ i ]);

                        if( ImGui::Selectable( sortOptions[ i ], isSelected ) )
                        {
                            // Selection changed
                            selectedOption = sortOptions[ i ];
                            isSelected = true;

                            m_FrameBrowserSortMode = FrameBrowserSortMode( i );
                        }

                        if( isSelected )
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }

                    ImGui::EndCombo();
                }
            }

            // Mark hotspots with color
            const uint64_t frameTicks = m_Data.m_Stats.m_TotalTicks;

            uint64_t index = 0;

            // Enumerate submits in frame
            for( const auto& submit : m_Data.m_Submits )
            {
                char indexStr[ 17 ] = {};
                _ui64toa_s( index, indexStr, 17, 16 );

                if( ImGui::TreeNode( indexStr, "Submit #%u", index ) )
                {
                    // Sort frame browser data
                    std::list<const ProfilerCommandBufferData*> pCommandBuffers =
                        SortFrameBrowserData( submit.m_CommandBuffers );

                    // Enumerate command buffers in submit
                    uint64_t commandBufferIndex = 0;

                    for( const auto* pCommandBuffer : pCommandBuffers )
                    {
                        PrintCommandBuffer( *pCommandBuffer, index | (commandBufferIndex << 12), frameTicks );
                        commandBufferIndex++;
                    }

                    // Finish submit subtree
                    ImGui::TreePop();
                }

                index++;
            }
        }
    }

    /***********************************************************************************\

    Function:
        UpdateMemoryTab

    Description:
        Updates "Memory" tab.

    \***********************************************************************************/
    void ProfilerOverlayOutput::UpdateMemoryTab()
    {
        const VkPhysicalDeviceMemoryProperties& memoryProperties =
            m_Device.MemoryProperties;

        const VkPhysicalDeviceMemoryBudgetPropertiesEXT* pMemoryBudgetProperties =
            //(const VkPhysicalDeviceMemoryBudgetPropertiesEXT*)m_Profiler.m_MemoryProperties2.pNext;
            nullptr; // TODO: Get MemoryProperties2 structure

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
            for( int i = 0; i < memoryProperties.memoryHeapCount; ++i )
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
            for( int i = 0; i < memoryProperties.memoryHeapCount; ++i )
            {
                float usage = 0.f;
                char usageStr[ 64 ] = {};

                if( memoryProperties.memoryHeaps[ i ].size != 0 )
                {
                    uint64_t allocatedSize = 0;

                    if( memoryProperties.memoryHeaps[ i ].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT )
                    {
                        allocatedSize = m_Data.m_Memory.m_DeviceLocalAllocationSize;
                    }

                    usage = (float)allocatedSize / memoryProperties.memoryHeaps[ i ].size;

                    sprintf_s( usageStr, "%.2f/%.2f MB (%.1f%%)",
                        allocatedSize / 1048576.f,
                        memoryProperties.memoryHeaps[ i ].size / 1048576.f,
                        usage * 100.f );
                }

                ImGui::ProgressBar( usage, { -1, 0 }, usageStr );
            }
        }

        ImGui::TextUnformatted( "Memory allocations" );


    }

    /***********************************************************************************\

    Function:
        UpdateStatisticsTab

    Description:
        Updates "Statistics" tab.

    \***********************************************************************************/
    void ProfilerOverlayOutput::UpdateStatisticsTab()
    {
        // Draw count statistics
        {
            ImGui::Text( "Draw calls:                       %u", m_Data.m_Stats.m_TotalDrawCount );
            ImGui::Text( "Draw calls (indirect):            %u", m_Data.m_Stats.m_TotalDrawIndirectCount );
            ImGui::Text( "Dispatch calls:                   %u", m_Data.m_Stats.m_TotalDispatchCount );
            ImGui::Text( "Dispatch calls (indirect):        %u", m_Data.m_Stats.m_TotalDispatchIndirectCount );
            ImGui::Text( "Pipeline barriers:                %u", m_Data.m_Stats.m_TotalBarrierCount );
            ImGui::Text( "Pipeline barriers (implicit):     %u", m_Data.m_Stats.m_TotalImplicitBarrierCount );
            ImGui::Text( "Clear calls:                      %u", m_Data.m_Stats.m_TotalClearCount );
            ImGui::Text( "Clear calls (implicit):           %u", m_Data.m_Stats.m_TotalClearImplicitCount );
            ImGui::Text( "Resolve calls:                    %u", m_Data.m_Stats.m_TotalResolveCount );
            ImGui::Text( "Resolve calls (implicit):         %u", m_Data.m_Stats.m_TotalResolveImplicitCount );
            ImGui::Separator();
            ImGui::Text( "Total calls:                      %u", m_Data.m_Stats.m_TotalDrawcallCount );
        }
    }

    /***********************************************************************************\

    Function:
        PrintCommandBuffer

    Description:
        Writes command buffer data to the overlay.

    \***********************************************************************************/
    void ProfilerOverlayOutput::PrintCommandBuffer( const ProfilerCommandBufferData& cmdBuffer, uint64_t index, uint64_t frameTicks )
    {
        // Mark hotspots with color
        DrawSignificanceRect( (float)cmdBuffer.m_Stats.m_TotalTicks / frameTicks );

        char indexStr[ 17 ] = {};
        _ui64toa_s( index, indexStr, 17, 16 );

        if( ImGui::TreeNode( indexStr,
            GetDebugObjectName( VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)cmdBuffer.m_Handle ).c_str() ) )
        {
            // Command buffer opened
            TextAlignRight( "%.2f ms", cmdBuffer.m_Stats.m_TotalTicks * m_TimestampPeriod );

            // Sort frame browser data
            std::list<const ProfilerRenderPass*> pRenderPasses =
                SortFrameBrowserData( cmdBuffer.m_Subregions );

            // Enumerate render passes in command buffer
            uint64_t renderPassIndex = 0;

            for( const ProfilerRenderPass* pRenderPass : pRenderPasses )
            {
                PrintRenderPass( *pRenderPass, index | (renderPassIndex << 24), frameTicks );
                renderPassIndex++;
            }

            ImGui::TreePop();
        }
        else
        {
            // Command buffer collapsed
            TextAlignRight( "%.2f ms", cmdBuffer.m_Stats.m_TotalTicks * m_TimestampPeriod );
        }
    }

    /***********************************************************************************\

    Function:
        PrintRenderPass

    Description:
        Writes render pass data to the overlay.

    \***********************************************************************************/
    void ProfilerOverlayOutput::PrintRenderPass( const ProfilerRenderPass& renderPass, uint64_t index, uint64_t frameTicks )
    {
        // Mark hotspots with color
        DrawSignificanceRect( (float)renderPass.m_Stats.m_TotalTicks / frameTicks );

        char indexStr[ 17 ] = {};
        // Render pass ID
        _ui64toa_s( index, indexStr, 17, 16 );

        // At least one subpass must be present
        assert( !renderPass.m_Subregions.empty() );

        const bool inRenderPassSubtree =
            (renderPass.m_Handle != VK_NULL_HANDLE) &&
            (ImGui::TreeNode( indexStr,
                GetDebugObjectName( VK_OBJECT_TYPE_RENDER_PASS, (uint64_t)renderPass.m_Handle ).c_str() ));

        if( inRenderPassSubtree )
        {
            // Render pass subtree opened
            TextAlignRight( "%.2f ms", renderPass.m_Stats.m_TotalTicks * m_TimestampPeriod );

            if( renderPass.m_Handle != VK_NULL_HANDLE )
            {
                DrawSignificanceRect( (float)renderPass.m_BeginTicks / frameTicks );

                ImGui::TextUnformatted( "vkCmdBeginRenderPass" );
                TextAlignRight( "%.2f ms", renderPass.m_BeginTicks * m_TimestampPeriod );
            }
        }

        if( inRenderPassSubtree ||
            (renderPass.m_Handle == VK_NULL_HANDLE) )
        {
            // If renderpass handle is invalid, one subpass must be present, up to one pipeline
            // must be present and it must be invalid too
            assert( (renderPass.m_Handle != VK_NULL_HANDLE) || (
                (renderPass.m_Subregions.size() == 1) && (
                    ((renderPass.m_Subregions[ 0 ].m_Subregions.size() == 1) &&
                        (renderPass.m_Subregions[ 0 ].m_Subregions[ 0 ].m_Handle == VK_NULL_HANDLE)) ||
                    (renderPass.m_Subregions[ 0 ].m_Subregions.empty())) ) );

            if( renderPass.m_Subregions.size() > 1 )
            {
                // Sort frame browser data
                std::list<const ProfilerSubpass*> pSubpasses =
                    SortFrameBrowserData( renderPass.m_Subregions );

                // Enumerate subpasses (who uses subpasses anyway)
                uint64_t subpassIndex = 0;

                for( const ProfilerSubpass* pSubpass : pSubpasses )
                {
                    const uint64_t i = index | (subpassIndex << 36);
                    // Subpass ID
                    _ui64toa_s( i, indexStr, 17, 16 );

                    if( ImGui::TreeNode( indexStr, "Subpass #%llu", subpassIndex ) )
                    {
                        // Subpass subtree opened
                        TextAlignRight( "%.2f ms", pSubpass->m_Stats.m_TotalTicks * m_TimestampPeriod );

                        // Sort frame browser data
                        std::list<const ProfilerPipeline*> pPipelines =
                            SortFrameBrowserData( pSubpass->m_Subregions );

                        // Enumerate pipelines in subpass
                        uint64_t pipelineIndex = 0;

                        for( const ProfilerPipeline* pPipeline : pPipelines )
                        {
                            PrintPipeline( *pPipeline, i | (pipelineIndex << 48), frameTicks );
                            pipelineIndex++;
                        }

                        // Finish subpass tree
                        ImGui::TreePop();
                    }
                    else
                    {
                        // Subpass collapsed
                        TextAlignRight( "%.2f ms", pSubpass->m_Stats.m_TotalTicks * m_TimestampPeriod );
                    }

                    subpassIndex++;
                }
            }
            else
            {
                const ProfilerSubpass& subpass = renderPass.m_Subregions.front();

                // Sort frame browser data
                std::list<const ProfilerPipeline*> pPipelines =
                    SortFrameBrowserData( subpass.m_Subregions );

                // Enumerate pipelines in render pass
                uint64_t pipelineIndex = 0;

                for( const ProfilerPipeline* pPipeline : pPipelines )
                {
                    PrintPipeline( *pPipeline, index | (pipelineIndex << 48), frameTicks );
                    pipelineIndex++;
                }
            }
        }

        if( inRenderPassSubtree )
        {
            DrawSignificanceRect( (float)renderPass.m_EndTicks / frameTicks );

            // Finish render pass subtree
            ImGui::TextUnformatted( "vkCmdEndRenderPass" );
            TextAlignRight( "%.2f ms", renderPass.m_EndTicks * m_TimestampPeriod );

            ImGui::TreePop();
        }

        if( !inRenderPassSubtree &&
            (renderPass.m_Handle != VK_NULL_HANDLE) )
        {
            // Render pass collapsed
            TextAlignRight( "%.2f ms", renderPass.m_Stats.m_TotalTicks * m_TimestampPeriod );
        }
    }

    /***********************************************************************************\

    Function:
        PrintPipeline

    Description:
        Writes pipeline data to the overlay.

    \***********************************************************************************/
    void ProfilerOverlayOutput::PrintPipeline( const ProfilerPipeline& pipeline, uint64_t index, uint64_t frameTicks )
    {
        // Mark hotspots with color
        DrawSignificanceRect( (float)pipeline.m_Stats.m_TotalTicks / frameTicks );

        char indexStr[ 17 ] = {};
        _ui64toa_s( index, indexStr, 17, 16 );

        const bool inPipelineSubtree =
            (pipeline.m_Handle != VK_NULL_HANDLE) &&
            (ImGui::TreeNode( indexStr,
                GetDebugObjectName( VK_OBJECT_TYPE_PIPELINE, (uint64_t)pipeline.m_Handle ).c_str() ));

        if( inPipelineSubtree )
        {
            // Pipeline subtree opened
            TextAlignRight( "%.2f ms", pipeline.m_Stats.m_TotalTicks * m_TimestampPeriod );
        }

        if( inPipelineSubtree ||
            (pipeline.m_Handle == VK_NULL_HANDLE) )
        {
            // Sort frame browser data
            std::list<const ProfilerDrawcall*> pDrawcalls =
                SortFrameBrowserData( pipeline.m_Subregions );

            // Enumerate drawcalls in pipeline
            for( const ProfilerDrawcall* pDrawcall : pDrawcalls )
            {
                const char* pDrawcallCmd = "";
                switch( pDrawcall->m_Type )
                {
                case ProfilerDrawcallType::eDraw:
                    pDrawcallCmd = "vkCmdDraw"; break;
                case ProfilerDrawcallType::eDispatch:
                    pDrawcallCmd = "vkCmdDispatch"; break;
                case ProfilerDrawcallType::eCopy:
                    pDrawcallCmd = "vkCmdCopy"; break;
                case ProfilerDrawcallType::eClear:
                    pDrawcallCmd = "vkCmdClear"; break;
                }

                // Mark hotspots with color
                DrawSignificanceRect( (float)pDrawcall->m_Ticks / frameTicks );

                ImGui::Text( "%s", pDrawcallCmd );
                TextAlignRight( "%.2f ms", pDrawcall->m_Ticks * m_TimestampPeriod );
            }
        }

        if( inPipelineSubtree )
        {
            // Finish pipeline subtree
            ImGui::TreePop();
        }

        if( !inPipelineSubtree &&
            (pipeline.m_Handle != VK_NULL_HANDLE) )
        {
            // Pipeline collapsed
            TextAlignRight( "%.2f ms", pipeline.m_Stats.m_TotalTicks * m_TimestampPeriod );
        }
    }

    /***********************************************************************************\

    Function:
        TextAlignRight

    Description:
        Displays text in the same line, aligned to right.

    \***********************************************************************************/
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

    /***********************************************************************************\

    Function:
        DrawSignificanceRect

    Description:

    \***********************************************************************************/
    void ProfilerOverlayOutput::DrawSignificanceRect( float significance )
    {
        ImVec2 cursorPosition = ImGui::GetCursorScreenPos();
        ImVec2 cmdBufferNameSize;

        cursorPosition.x = ImGui::GetWindowPos().x;

        cmdBufferNameSize.x = cursorPosition.x + ImGui::GetWindowSize().x;
        cmdBufferNameSize.y = cursorPosition.y + ImGui::GetTextLineHeight();

        ImU32 color = ImGui::GetColorU32( { 1, 0, 0, significance } );

        ImDrawList* pDrawList = ImGui::GetWindowDrawList();
        pDrawList->AddRectFilled( cursorPosition, cmdBufferNameSize, color );
    }

    /***********************************************************************************\

    Function:
        GetDebugObjectName

    Description:
        Returns string representation of vulkan handle.

    \***********************************************************************************/
    std::string ProfilerOverlayOutput::GetDebugObjectName( VkObjectType type, uint64_t handle ) const
    {
        std::stringstream sstr;

        // Object type to string
        switch( type )
        {
        case VK_OBJECT_TYPE_COMMAND_BUFFER: sstr << "VkCommandBuffer "; break;
        case VK_OBJECT_TYPE_RENDER_PASS: sstr << "VkRenderPass "; break;
        case VK_OBJECT_TYPE_PIPELINE: sstr << "VkPipeline "; break;
        }

        auto it = m_Device.Debug.ObjectNames.find( handle );
        if( it != m_Device.Debug.ObjectNames.end() )
        {
            sstr << it->second;
        }
        else
        {
            char unnamedObjectName[ 20 ] = "0x";
            _ui64toa_s( handle, unnamedObjectName + 2, 18, 16 );

            sstr << unnamedObjectName;
        }

        return sstr.str();
    }
}

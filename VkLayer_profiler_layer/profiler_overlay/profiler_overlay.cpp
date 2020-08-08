#include "profiler_overlay.h"
#include "imgui_impl_vulkan_layer.h"
#include <string>
#include <sstream>
#include <stack>

#include "imgui_widgets/imgui_histogram_ex.h"
#include "imgui_widgets/imgui_table_ex.h"

#ifdef VK_USE_PLATFORM_WIN32_KHR
#include "imgui_impl_win32.h"
#endif

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
#include <wayland-client.h>
#endif

#ifdef VK_USE_PLATFORM_XCB_KHR
#include <xcb/xcb.h>
#endif

#ifdef VK_USE_PLATFORM_XLIB_KHR
#include "imgui_impl_xlib.h"
#endif

#ifdef VK_USE_PLATFORM_XLIB_XRANDR_EXT
#include <X11/extensions/Xrandr.h>
#endif

namespace Profiler
{
    // Define static members
    std::mutex ProfilerOverlayOutput::s_ImGuiMutex;

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
        , m_pSwapchain( nullptr )
        , m_Window()
        , m_pImGuiContext( nullptr )
        , m_pImGuiVulkanContext( nullptr )
        , m_pImGuiWindowContext( nullptr )
        , m_DescriptorPool( nullptr )
        , m_RenderPass( nullptr )
        , m_RenderArea( {} )
        , m_ImageFormat( VK_FORMAT_UNDEFINED )
        , m_Images()
        , m_ImageViews()
        , m_Framebuffers()
        , m_CommandPool( nullptr )
        , m_CommandBuffers()
        , m_CommandFences()
        , m_CommandSemaphores()
        , m_VendorMetricProperties()
        , m_TimestampPeriod( device.Properties.limits.timestampPeriod / 1000000.f )
        , m_FrameBrowserSortMode( FrameBrowserSortMode::eSubmissionOrder )
        , m_HistogramGroupMode( HistogramGroupMode::eRenderPass )
        , m_Pause( false )
    {
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

        // Create command pool
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
        }

        // Create swapchain-dependent resources
        ResetSwapchain( swapchain, pCreateInfo );

        // Init ImGui
        std::scoped_lock lk( s_ImGuiMutex );
        IMGUI_CHECKVERSION();

        m_pImGuiContext = ImGui::CreateContext();

        ImGui::SetCurrentContext( m_pImGuiContext );
        ImGui::StyleColorsDark();

        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = { (float)m_RenderArea.width, (float)m_RenderArea.height };
        io.DeltaTime = 1.0f / 60.0f;
        io.IniFilename = "VK_LAYER_profiler_imgui.ini";
        io.ConfigFlags = ImGuiConfigFlags_None;

        // Build atlas
        unsigned char* tex_pixels = NULL;
        int tex_w, tex_h;
        io.Fonts->GetTexDataAsRGBA32( &tex_pixels, &tex_w, &tex_h );

        // Init window
        InitializeImGuiWindowHooks( pCreateInfo );

        // Init vulkan
        InitializeImGuiVulkanContext( pCreateInfo );

        // Get vendor metric properties
        {
            uint32_t vendorMetricCount = 0;
            vkEnumerateProfilerPerformanceCounterPropertiesEXT( device.Handle, &vendorMetricCount, nullptr );

            m_VendorMetricProperties.resize( vendorMetricCount );
            vkEnumerateProfilerPerformanceCounterPropertiesEXT( device.Handle, &vendorMetricCount, m_VendorMetricProperties.data() );
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

        m_Window = OSWindowHandle();

        if( m_pImGuiVulkanContext )
        {
            m_pImGuiVulkanContext->Shutdown();
            delete m_pImGuiVulkanContext;

            m_pImGuiVulkanContext = nullptr;
        }

        if( m_pImGuiWindowContext )
        {
            delete m_pImGuiWindowContext;
            m_pImGuiWindowContext = nullptr;
        }

        if( m_pImGuiContext )
        {
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
        GetSwapchain

    Description:
        Return swapchain the overlay is associated with.

    \***********************************************************************************/
    VkSwapchainKHR ProfilerOverlayOutput::GetSwapchain() const
    {
        return m_pSwapchain->Handle;
    }

    /***********************************************************************************\

    Function:
        ResetSwapchain

    Description:
        Move overlay to the new swapchain.

    \***********************************************************************************/
    void ProfilerOverlayOutput::ResetSwapchain(
        VkSwapchainKHR_Object& swapchain,
        const VkSwapchainCreateInfoKHR* pCreateInfo )
    {
        assert( m_pSwapchain == nullptr ||
            pCreateInfo->oldSwapchain == m_pSwapchain->Handle ||
            pCreateInfo->oldSwapchain == VK_NULL_HANDLE );

        // Get swapchain images
        uint32_t swapchainImageCount = 0;
        m_Device.Callbacks.GetSwapchainImagesKHR(
            m_Device.Handle,
            swapchain.Handle,
            &swapchainImageCount,
            nullptr );

        std::vector<VkImage> images( swapchainImageCount );
        m_Device.Callbacks.GetSwapchainImagesKHR(
            m_Device.Handle,
            swapchain.Handle,
            &swapchainImageCount,
            images.data() );

        // Recreate render pass
        if( pCreateInfo->imageFormat != m_ImageFormat )
        {
            if( m_RenderPass != VK_NULL_HANDLE )
            {
                m_Device.Callbacks.DestroyRenderPass( m_Device.Handle, m_RenderPass, nullptr );
                m_RenderPass = VK_NULL_HANDLE;
            }

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

            m_ImageFormat = pCreateInfo->imageFormat;
        }

        // Recreate image views and framebuffers 
        {
            if( !m_Images.empty() )
            {
                assert( m_ImageViews.size() == m_Images.size() );
                assert( m_Framebuffers.size() == m_Images.size() );

                // Destroy previous framebuffers
                for( int i = 0; i < m_Images.size(); ++i )
                {
                    m_Device.Callbacks.DestroyFramebuffer( m_Device.Handle, m_Framebuffers[ i ], nullptr );
                    m_Device.Callbacks.DestroyImageView( m_Device.Handle, m_ImageViews[ i ], nullptr );
                }

                m_Framebuffers.clear();
                m_ImageViews.clear();
            }

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

                info.image = images[ i ];

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

            m_RenderArea = pCreateInfo->imageExtent;
        }

        // Allocate additional command buffers
        if( swapchainImageCount > m_Images.size() )
        {
            VkCommandBufferAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = m_CommandPool;
            allocInfo.commandBufferCount = swapchainImageCount - m_Images.size();

            std::vector<VkCommandBuffer> commandBuffers( swapchainImageCount );

            VkResult result = m_Device.Callbacks.AllocateCommandBuffers(
                m_Device.Handle, &allocInfo, commandBuffers.data() );

            if( result != VK_SUCCESS )
            {
                // Class is marked final, destructor must not be virtual
                ProfilerOverlayOutput::~ProfilerOverlayOutput();
                throw result;
            }

            // Append created command buffers to end
            m_CommandBuffers.insert( m_CommandBuffers.end(), commandBuffers.begin(), commandBuffers.end() );

            for( auto cmdBuffer : commandBuffers )
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

            for( int i = m_Images.size(); i < swapchainImageCount; ++i )
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

        m_pSwapchain = &swapchain;
        m_Images = images;

        // Reinitialize ImGui
        if( m_pImGuiContext )
        {
            // Init window
            InitializeImGuiWindowHooks( pCreateInfo );

            // Init vulkan
            InitializeImGuiVulkanContext( pCreateInfo );
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

            m_Device.Callbacks.WaitForFences( m_Device.Handle, 1, &fence, VK_TRUE, UINT64_MAX );
            m_Device.Callbacks.ResetFences( m_Device.Handle, 1, &fence );

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

        m_pImGuiWindowContext->NewFrame();

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
        if( ImGui::BeginTabItem( "Self" ) )
        {
            UpdateSelfTab();
            ImGui::EndTabItem();
        }
        if( ImGui::BeginTabItem( "Settings" ) )
        {
            UpdateSettingsTab();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();

        ImGui::End();
        ImGui::Render();
    }

    /***********************************************************************************\

    Function:
        InitializeImGuiWindowHooks

    Description:

    \***********************************************************************************/
    void ProfilerOverlayOutput::InitializeImGuiWindowHooks( const VkSwapchainCreateInfoKHR* pCreateInfo )
    {
        OSWindowHandle window = m_Device.pInstance->Surfaces.at( pCreateInfo->surface ).Window;

        #ifdef VK_USE_PLATFORM_WIN32_KHR
        if( window.Type == OSWindowHandleType::eWin32 )
        {
            if( m_pImGuiWindowContext )
            {
                delete m_pImGuiWindowContext;
            }

            m_pImGuiWindowContext = new ImGui_ImplWin32_Context( m_pImGuiContext, window.Win32Handle );
        }
        #endif // VK_USE_PLATFORM_WIN32_KHR

        #ifdef VK_USE_PLATFORM_XCB_KHR
        if( window.Type == OSWindowHandleType::eXCB )
        {
            m_Window = window;
        }
        #endif // VK_USE_PLATFORM_XCB_KHR

        #ifdef VK_USE_PLATFORM_XLIB_KHR
        if( window.Type == OSWindowHandleType::eX11 )
        {
            if( m_pImGuiWindowContext )
            {
                delete m_pImGuiWindowContext;
            }

            m_pImGuiWindowContext = new ImGui_ImplXlib_Context( window.X11Handle );
        }
        #endif // VK_USE_PLATFORM_XLIB_KHR

        m_Window = window;
    }

    /***********************************************************************************\

    Function:
        InitializeImGuiVulkanContext

    Description:

    \***********************************************************************************/
    void ProfilerOverlayOutput::InitializeImGuiVulkanContext( const VkSwapchainCreateInfoKHR* pCreateInfo )
    {
        if( m_pImGuiVulkanContext )
        {
            // Reuse already allocated context memory
            m_pImGuiVulkanContext->Shutdown();
        }
        else
        {
            m_pImGuiVulkanContext = new (std::nothrow) ImGui_ImplVulkan_Context();

            if( !m_pImGuiVulkanContext )
            {
                // Class is marked final, destructor must not be virtual
                ProfilerOverlayOutput::~ProfilerOverlayOutput();
                throw VK_ERROR_OUT_OF_HOST_MEMORY;
            }
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
        imGuiInitInfo.ImageCount = m_Images.size();
        imGuiInitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

        imGuiInitInfo.DescriptorPool = m_DescriptorPool;

        if( !m_pImGuiVulkanContext->Init( &imGuiInitInfo, m_RenderPass ) )
        {
            // Class is marked final, destructor must not be virtual
            ProfilerOverlayOutput::~ProfilerOverlayOutput();
            throw VK_ERROR_INITIALIZATION_FAILED;
        }

        // Initialize fonts
        m_Device.Callbacks.ResetFences( m_Device.Handle, 1, &m_CommandFences[ 0 ] );

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

            static const char* groupOptions[] = {
                "Render passes",
                "Pipelines",
                "Drawcalls" };

            const char* selectedOption = groupOptions[ (size_t)m_HistogramGroupMode ];

            // Select group mode
            {
                if( ImGui::BeginCombo( "Histogram groups", selectedOption, ImGuiComboFlags_NoPreview ) )
                {
                    for( size_t i = 0; i < std::extent_v<decltype(groupOptions)>; ++i )
                    {
                        bool isSelected = (selectedOption == groupOptions[ i ]);

                        if( ImGui::Selectable( groupOptions[ i ], isSelected ) )
                        {
                            // Selection changed
                            selectedOption = groupOptions[ i ];
                            isSelected = true;

                            m_HistogramGroupMode = HistogramGroupMode( i );
                        }

                        if( isSelected )
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }

                    ImGui::EndCombo();
                }
            }

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
                            if( m_HistogramGroupMode > HistogramGroupMode::eRenderPass )
                            {
                                // Enumerate subpasses in render pass
                                for( const auto& subpass : renderPass.m_Subregions )
                                {
                                    if( subpass.m_Contents == VK_SUBPASS_CONTENTS_INLINE )
                                    {
                                        // Enumerate pipelines in subpass
                                        for( const auto& pipeline : subpass.m_Pipelines )
                                        {
                                            if( m_HistogramGroupMode > HistogramGroupMode::ePipeline )
                                            {
                                                // Enumerate drawcalls in pipeline
                                                for( const auto& drawcall : pipeline.m_Subregions )
                                                {
                                                    // Insert drawcall cycle count to histogram
                                                    contributions.push_back( drawcall.m_Ticks );
                                                }
                                            }
                                            else
                                            {
                                                // Insert pipeline cycle count to histogram
                                                contributions.push_back( pipeline.m_Stats.m_TotalTicks );
                                            }
                                        }
                                    }
                                    else if( subpass.m_Contents == VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS )
                                    {
                                        // TODO
                                    }
                                }
                            }
                            else
                            {
                                // Insert render pass cycle count to histogram
                                contributions.push_back( renderPass.m_Stats.m_TotalTicks );
                            }
                        }
                    }
                }
            }

            char pHistogramDescription[ 32 ];
            sprintf( pHistogramDescription, "GPU Cycles (%s)", selectedOption );

            ImGui::PushItemWidth( -1 );
            ImGuiX::PlotHistogramEx(
                "",
                contributions.data(), // Scale x with y
                contributions.data(),
                contributions.size(),
                0, pHistogramDescription, 0, FLT_MAX, { 0, 100 } );
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

                    TextAlignRight( "(%.1f %%) %.2f ms",
                        pipeline.m_Stats.m_TotalTicks * 100.f / m_Data.m_Stats.m_TotalTicks, 
                        pipeline.m_Stats.m_TotalTicks * m_TimestampPeriod );

                    // Print up to 10 top pipelines
                    if( (++i) == 10 ) break;
                }
            }
        }

        // Vendor-specific
        if( !m_Data.m_VendorMetrics.empty() &&
            ImGui::CollapsingHeader( "Performance counters" ) )
        {
            assert( m_Data.m_VendorMetrics.size() == m_VendorMetricProperties.size() );

            ImGui::BeginTable( "Performance counters table",
                /* columns_count */ 3,
                ImGuiTableFlags_Resizable |
                ImGuiTableFlags_NoClipX |
                ImGuiTableFlags_Borders );

            // Headers
            ImGui::TableSetupColumn( "Metric", ImGuiTableColumnFlags_WidthAlwaysAutoResize );
            ImGui::TableSetupColumn( "Frame", ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableSetupColumn( "", ImGuiTableColumnFlags_WidthAlwaysAutoResize );
            ImGui::TableAutoHeaders();

            for( uint32_t i = 0; i < m_Data.m_VendorMetrics.size(); ++i )
            {
                const VkProfilerPerformanceCounterResultEXT& metric = m_Data.m_VendorMetrics[ i ];
                const VkProfilerPerformanceCounterPropertiesEXT& metricProperties = m_VendorMetricProperties[ i ];

                ImGui::TableNextCell();
                {
                    ImGui::Text( "%s", metricProperties.shortName );

                    if( ImGui::IsItemHovered() &&
                        metricProperties.description[ 0 ] )
                    {
                        ImGui::BeginTooltip();
                        ImGui::PushTextWrapPos( 350.f );
                        ImGui::TextUnformatted( metricProperties.description );
                        ImGui::PopTextWrapPos();
                        ImGui::EndTooltip();
                    }
                }

                ImGui::TableNextCell();
                {
                    const float columnWidth = ImGuiX::TableGetColumnWidth();
                    switch( metricProperties.storage )
                    {
                    case VK_PERFORMANCE_COUNTER_STORAGE_FLOAT32_KHR:
                        TextAlignRight( columnWidth, "%.2f", metric.float32 );
                        break;

                    case VK_PERFORMANCE_COUNTER_STORAGE_UINT32_KHR:
                        TextAlignRight( columnWidth, "%u", metric.uint32 );
                        break;

                    case VK_PERFORMANCE_COUNTER_STORAGE_UINT64_KHR:
                        TextAlignRight( columnWidth, "%llu", metric.uint64 );
                        break;
                    }
                }

                ImGui::TableNextCell();
                {
                    const char* pUnitString = "???";

                    assert( metricProperties.unit < 11 );
                    static const char* const ppUnitString[ 11 ] =
                    {
                        "" /* VK_PERFORMANCE_COUNTER_UNIT_GENERIC_KHR */,
                        "%" /* VK_PERFORMANCE_COUNTER_UNIT_PERCENTAGE_KHR */,
                        "ns" /* VK_PERFORMANCE_COUNTER_UNIT_NANOSECONDS_KHR */,
                        "B" /* VK_PERFORMANCE_COUNTER_UNIT_BYTES_KHR */,
                        "B/s" /* VK_PERFORMANCE_COUNTER_UNIT_BYTES_PER_SECOND_KHR */,
                        "K" /* VK_PERFORMANCE_COUNTER_UNIT_KELVIN_KHR */,
                        "W" /* VK_PERFORMANCE_COUNTER_UNIT_WATTS_KHR */,
                        "V" /* VK_PERFORMANCE_COUNTER_UNIT_VOLTS_KHR */,
                        "A" /* VK_PERFORMANCE_COUNTER_UNIT_AMPS_KHR */,
                        "Hz" /* VK_PERFORMANCE_COUNTER_UNIT_HERTZ_KHR */,
                        "clk" /* VK_PERFORMANCE_COUNTER_UNIT_CYCLES_KHR */
                    };

                    if( metricProperties.unit < 11 )
                    {
                        pUnitString = ppUnitString[ metricProperties.unit ];
                    }

                    ImGui::TextUnformatted( pUnitString );
                }
            }

            ImGui::EndTable();
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
                u64tohex( indexStr, index );

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

                    sprintf( usageStr, "%.2f/%.2f MB (%.1f%%)",
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

                    sprintf( usageStr, "%.2f/%.2f MB (%.1f%%)",
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
            ImGui::Text( "Copy calls:                       %u", m_Data.m_Stats.m_TotalCopyCount );
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
        UpdateSelfTab

    Description:
        Updates "Self" tab.

    \***********************************************************************************/
    void ProfilerOverlayOutput::UpdateSelfTab()
    {
        // Print self test values
        {
            ImGui::Text( "VkCommandBuffer lookup time:      %.2f ms", m_Data.m_Self.m_CommandBufferLookupTimeNs / 1000000.f );
            ImGui::Text( "VkPipeline lookup time:           %.2f ms", m_Data.m_Self.m_PipelineLookupTimeNs / 1000000.f );
        }
    }

    /***********************************************************************************\

    Function:
        UpdateSettingsTab

    Description:
        Updates "Settings" tab.

    \***********************************************************************************/
    void ProfilerOverlayOutput::UpdateSettingsTab()
    {
        // Select synchronization mode
        {
            static const char* groupOptions[] = {
                "Present",
                "Submit" };

            // TMP
            static int selectedOption = 0;
            int previousSelectedOption = selectedOption;

            ImGui::Combo( "Sync mode", &selectedOption, groupOptions, 2 );

            if( selectedOption != previousSelectedOption )
            {
                vkSetProfilerSyncModeEXT( m_Device.Handle, (VkProfilerSyncModeEXT)selectedOption );
            }

            //if( ImGui::BeginCombo( "Sync mode", selectedOption ) )
            //{
            //    for( size_t i = 0; i < std::extent_v<decltype(groupOptions)>; ++i )
            //    {
            //        bool isSelected = (selectedOption == groupOptions[ i ]);
            //
            //        if( ImGui::Selectable( groupOptions[ i ], isSelected ) )
            //        {
            //            // Selection changed
            //            selectedOption = groupOptions[ i ];
            //            isSelected = true;
            //
            //            vkSetProfilerSyncModeEXT( m_Device.Handle, (VkProfilerSyncModeEXT)i );
            //        }
            //
            //        if( isSelected )
            //        {
            //            ImGui::SetItemDefaultFocus();
            //        }
            //    }
            //
            //    ImGui::EndCombo();
            //}
        }

        // Draw profiler settings
        {

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

        const std::string cmdBufferName =
            GetDebugObjectName( VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)cmdBuffer.m_Handle );

        #if 0
        // Disable empty command buffer expanding
        if( cmdBuffer.m_Subregions.empty() )
        {
            assert( cmdBuffer.m_Stats.m_TotalTicks == 0 );

            ImGui::TextUnformatted( cmdBufferName.c_str() );
            return;
        }
        #endif

        char indexStr[ 17 ] = {};
        u64tohex( indexStr, index );

        if( ImGui::TreeNode( indexStr, cmdBufferName.c_str() ) )
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
        u64tohex( indexStr, index );

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
            // Sort frame browser data
            std::list<const ProfilerSubpass*> pSubpasses =
                SortFrameBrowserData( renderPass.m_Subregions );

            // Enumerate subpasses
            uint64_t subpassIndex = 0;

            for( const ProfilerSubpass* pSubpass : pSubpasses )
            {
                PrintSubpass( *pSubpass, index | (subpassIndex << 36), frameTicks );
                subpassIndex++;
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
        PrintSubpass

    Description:
        Writes subpass data to the overlay.

    \***********************************************************************************/
    void ProfilerOverlayOutput::PrintSubpass( const ProfilerSubpass& subpass, uint64_t index, uint64_t frameTicks )
    {
        // Subpass ID
        char indexStr[ 17 ] = {};
        u64tohex( indexStr, index );

        const bool inSubpassSubtree =
            (subpass.m_Index != -1) &&
            (ImGui::TreeNode( indexStr, "Subpass #%u", subpass.m_Index ));

        if( inSubpassSubtree )
        {
            // Subpass subtree opened
            TextAlignRight( "%.2f ms", subpass.m_Stats.m_TotalTicks * m_TimestampPeriod );
        }

        if( inSubpassSubtree ||
            (subpass.m_Index == -1) )
        {
            if( subpass.m_Contents == VK_SUBPASS_CONTENTS_INLINE )
            {
                // Sort frame browser data
                std::list<const ProfilerPipeline*> pPipelines =
                    SortFrameBrowserData( subpass.m_Pipelines );

                // Enumerate pipelines in subpass
                uint64_t pipelineIndex = 0;

                for( const ProfilerPipeline* pPipeline : pPipelines )
                {
                    PrintPipeline( *pPipeline, index | (pipelineIndex << 48), frameTicks );
                    pipelineIndex++;
                }
            }

            else if( subpass.m_Contents == VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS )
            {
                // Sort command buffers
                std::list<const ProfilerCommandBufferData*> pCommandBuffers =
                    SortFrameBrowserData( subpass.m_SecondaryCommandBuffers );

                // Enumerate command buffers in subpass
                uint64_t commandBufferIndex = 0;

                for( const ProfilerCommandBufferData* pCommandBuffer : pCommandBuffers )
                {
                    PrintCommandBuffer( *pCommandBuffer, index | (commandBufferIndex << 58), frameTicks );
                    commandBufferIndex++;
                }
            }
        }

        if( inSubpassSubtree )
        {
            // Finish subpass tree
            ImGui::TreePop();
        }

        if( !inSubpassSubtree &&
            (subpass.m_Index != -1) )
        {
            // Subpass collapsed
            TextAlignRight( "%.2f ms", subpass.m_Stats.m_TotalTicks * m_TimestampPeriod );
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
        u64tohex( indexStr, index );

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
        Displays text in the next line, aligned to right.

    \***********************************************************************************/
    void ProfilerOverlayOutput::TextAlignRight( float contentAreaWidth, const char* fmt, ... )
    {
        va_list args;
        va_start( args, fmt );

        char text[ 128 ];
        vsprintf( text, fmt, args );

        va_end( args );

        uint32_t textSize = ImGui::CalcTextSize( text ).x;

        ImGui::SameLine( contentAreaWidth - textSize );
        ImGui::TextUnformatted( text );
    }


    /***********************************************************************************\

    Function:
        TextAlignRightSameLine

    Description:
        Displays text in the same line, aligned to right.

    \***********************************************************************************/
    void ProfilerOverlayOutput::TextAlignRight( const char* fmt, ... )
    {
        va_list args;
        va_start( args, fmt );

        char text[ 128 ];
        vsprintf( text, fmt, args );

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
            u64tohex( unnamedObjectName + 2, handle );

            sstr << unnamedObjectName;
        }

        return sstr.str();
    }
}

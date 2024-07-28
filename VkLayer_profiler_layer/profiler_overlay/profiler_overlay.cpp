// Copyright (c) 2019-2024 Lukasz Stalmirski
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

#include "profiler_overlay.h"
#include "profiler_overlay_shader_view.h"
#include "profiler_trace/profiler_trace.h"
#include "profiler_helpers/profiler_data_helpers.h"

#include "imgui_impl_vulkan_layer.h"
#include <string>
#include <sstream>
#include <stack>
#include <fstream>
#include <regex>

#include <fmt/format.h>

#include "utils/lockable_unordered_map.h"

#include "imgui_widgets/imgui_breakdown_ex.h"
#include "imgui_widgets/imgui_histogram_ex.h"
#include "imgui_widgets/imgui_table_ex.h"
#include "imgui_widgets/imgui_ex.h"

// Languages
#include "lang/en_us.h"
#include "lang/pl_pl.h"

#if 1
using Lang = Profiler::DeviceProfilerOverlayLanguage_Base;
#else
using Lang = Profiler::DeviceProfilerOverlayLanguage_PL;
#endif

#ifdef VK_USE_PLATFORM_WIN32_KHR
#include "imgui_impl_win32.h"
#endif

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
#include <wayland-client.h>
#endif

#ifdef VK_USE_PLATFORM_XCB_KHR
#include "imgui_impl_xcb.h"
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
    std::mutex s_ImGuiMutex;

    struct ProfilerOverlayOutput::PerformanceGraphColumn : ImGuiX::HistogramColumnData
    {
        HistogramGroupMode groupMode;
        FrameBrowserTreeNodeIndex nodeIndex;
    };

    /***********************************************************************************\

    Function:
        ProfilerOverlayOutput

    Description:
        Constructor.

    \***********************************************************************************/
    ProfilerOverlayOutput::ProfilerOverlayOutput()
        : m_pDevice( nullptr )
        , m_pGraphicsQueue( nullptr )
        , m_pSwapchain( nullptr )
        , m_Window()
        , m_pImGuiContext( nullptr )
        , m_pImGuiVulkanContext( nullptr )
        , m_pImGuiWindowContext( nullptr )
        , m_DescriptorPool( VK_NULL_HANDLE )
        , m_RenderPass( VK_NULL_HANDLE )
        , m_RenderArea( {} )
        , m_ImageFormat( VK_FORMAT_UNDEFINED )
        , m_Images()
        , m_ImageViews()
        , m_Framebuffers()
        , m_CommandPool( VK_NULL_HANDLE )
        , m_CommandBuffers()
        , m_CommandFences()
        , m_CommandSemaphores()
        , m_Title( Lang::WindowName )
        , m_ActiveMetricsSetIndex( UINT32_MAX )
        , m_VendorMetricsSets()
        , m_VendorMetricFilter()
        , m_TimestampPeriod( 0 )
        , m_TimestampDisplayUnit( 1.0f )
        , m_pTimestampDisplayUnitStr( Lang::Milliseconds )
        , m_FrameBrowserSortMode( FrameBrowserSortMode::eSubmissionOrder )
        , m_HistogramGroupMode( HistogramGroupMode::eRenderPass )
        , m_Pause( false )
        , m_ShowDebugLabels( true )
        , m_ShowShaderCapabilities( true )
        , m_ShowEmptyStatistics( false )
        , m_TimeUnit( TimeUnit::eMilliseconds )
        , m_SamplingMode( VK_PROFILER_MODE_PER_DRAWCALL_EXT )
        , m_SyncMode( VK_PROFILER_SYNC_MODE_PRESENT_EXT )
        , m_SelectedFrameBrowserNodeIndex( { 0xFFFF } )
        , m_ScrollToSelectedFrameBrowserNode( false )
        , m_SelectionUpdateTimestamp( std::chrono::high_resolution_clock::duration::zero() )
        , m_SerializationFinishTimestamp( std::chrono::high_resolution_clock::duration::zero() )
        , m_InspectorPipeline()
        , m_InspectorShaderView( m_Fonts )
        , m_InspectorTabs( 0 )
        , m_InspectorTabIndex( 0 )
        , m_PerformanceQueryCommandBufferFilter( VK_NULL_HANDLE )
        , m_PerformanceQueryCommandBufferFilterName( "Frame" )
        , m_SerializationSucceeded( false )
        , m_SerializationWindowVisible( false )
        , m_SerializationMessage()
        , m_SerializationOutputWindowSize( { 0, 0 } )
        , m_SerializationOutputWindowDuration( std::chrono::seconds( 4 ) )
        , m_SerializationOutputWindowFadeOutDuration( std::chrono::seconds( 1 ) )
        , m_RenderPassColumnColor( 0 )
        , m_GraphicsPipelineColumnColor( 0 )
        , m_ComputePipelineColumnColor( 0 )
        , m_RayTracingPipelineColumnColor( 0 )
        , m_InternalPipelineColumnColor( 0 )
        , m_pStringSerializer( nullptr )
        , m_MainDockSpaceId( 0 )
        , m_PerformanceTabDockSpaceId( 0 )
        , m_PerformanceWindowState{ m_Settings.AddBool( "PerformanceWindowOpen", true ), true}
        , m_TopPipelinesWindowState{ m_Settings.AddBool( "TopPipelinesWindowOpen", true ), true}
        , m_PerformanceCountersWindowState{ m_Settings.AddBool( "PerformanceCountersWindowOpen", true ), true}
        , m_MemoryWindowState{ m_Settings.AddBool( "MemoryWindowOpen", true ), true}
        , m_InspectorWindowState{ m_Settings.AddBool( "InspectorWindowOpen", true ), true}
        , m_StatisticsWindowState{ m_Settings.AddBool( "StatisticsWindowOpen", true ), true}
        , m_SettingsWindowState{ m_Settings.AddBool( "SettingsWindowOpen", true ), true}
    {
    }

    /***********************************************************************************\

    Function:
        Initialize

    Description:
        Initializes profiler overlay.

    \***********************************************************************************/
    VkResult ProfilerOverlayOutput::Initialize(
        VkDevice_Object& device,
        VkQueue_Object& graphicsQueue,
        VkSwapchainKhr_Object& swapchain,
        const VkSwapchainCreateInfoKHR* pCreateInfo )
    {
        VkResult result = VK_SUCCESS;

        // Setup objects
        m_pDevice = &device;
        m_pGraphicsQueue = &graphicsQueue;
        m_pSwapchain = &swapchain;

        // Set main window title
        m_Title = fmt::format( "{0} - {1}###VkProfiler",
            Lang::WindowName,
            m_pDevice->pPhysicalDevice->Properties.deviceName );

        // Create descriptor pool
        if( result == VK_SUCCESS )
        {
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

        // Get timestamp query period
        if( result == VK_SUCCESS )
        {
            m_TimestampPeriod = Nanoseconds( m_pDevice->pPhysicalDevice->Properties.limits.timestampPeriod );
        }

        // Create swapchain-dependent resources
        if( result == VK_SUCCESS )
        {
            result = ResetSwapchain( swapchain, pCreateInfo );
        }

        // Init ImGui
        if( result == VK_SUCCESS )
        {
            std::scoped_lock lk( s_ImGuiMutex );
            IMGUI_CHECKVERSION();

            m_pImGuiContext = ImGui::CreateContext();

            ImGui::SetCurrentContext( m_pImGuiContext );

            // Register settings handler to the new context
            m_Settings.InitializeHandlers();

            ImGuiIO& io = ImGui::GetIO();
            io.DisplaySize = { (float)m_RenderArea.width, (float)m_RenderArea.height };
            io.DeltaTime = 1.0f / 60.0f;
            io.IniFilename = "VK_LAYER_profiler_imgui.ini";
            io.ConfigFlags = ImGuiConfigFlags_DockingEnable;

            m_Settings.Validate( io.IniFilename );

            InitializeImGuiDefaultFont();
            InitializeImGuiStyle();
        }

        // Init window
        if( result == VK_SUCCESS )
        {
            result = InitializeImGuiWindowHooks( pCreateInfo );
        }

        // Init vulkan
        if( result == VK_SUCCESS )
        {
            result = InitializeImGuiVulkanContext( pCreateInfo );
        }

        // Get vendor metrics sets
        if( result == VK_SUCCESS )
        {
            uint32_t vendorMetricsSetCount = 0;
            vkEnumerateProfilerPerformanceMetricsSetsEXT( device.Handle, &vendorMetricsSetCount, nullptr );

            std::vector<VkProfilerPerformanceMetricsSetPropertiesEXT> metricsSets( vendorMetricsSetCount );
            vkEnumerateProfilerPerformanceMetricsSetsEXT( device.Handle, &vendorMetricsSetCount, metricsSets.data() );

            m_VendorMetricsSets.reserve( vendorMetricsSetCount );
            m_VendorMetricsSetVisibility.reserve( vendorMetricsSetCount );

            for( uint32_t i = 0; i < vendorMetricsSetCount; ++i )
            {
                VendorMetricsSet& metricsSet = m_VendorMetricsSets.emplace_back();
                memcpy( &metricsSet.m_Properties, &metricsSets[i], sizeof( metricsSet.m_Properties ) );

                // Get metrics belonging to this set.
                metricsSet.m_Metrics.resize( metricsSet.m_Properties.metricsCount );

                uint32_t metricsCount = metricsSet.m_Properties.metricsCount;
                vkEnumerateProfilerPerformanceCounterPropertiesEXT( device.Handle, i,
                    &metricsCount,
                    metricsSet.m_Metrics.data() );

                m_VendorMetricsSetVisibility.push_back( true );
            }

            vkGetProfilerActivePerformanceMetricsSetIndexEXT( device.Handle, &m_ActiveMetricsSetIndex );
        }

        // Initialize the disassembler in the shader view
        if( result == VK_SUCCESS )
        {
            m_InspectorShaderView.InitializeStyles();
            m_InspectorShaderView.SetTargetDevice( m_pDevice );
            m_InspectorShaderView.SetShaderSavedCallback( std::bind(
                &ProfilerOverlayOutput::ShaderRepresentationSaved,
                this,
                std::placeholders::_1,
                std::placeholders::_2 ) );
        }

        // Initialize serializer
        if( result == VK_SUCCESS )
        {
            result = (m_pStringSerializer = new (std::nothrow) DeviceProfilerStringSerializer( device ))
                         ? VK_SUCCESS
                         : VK_ERROR_OUT_OF_HOST_MEMORY;
        }

        // Initialize settings
        if( result == VK_SUCCESS )
        {
            vkGetProfilerModeEXT( m_pDevice->Handle, &m_SamplingMode );
            vkGetProfilerSyncModeEXT( m_pDevice->Handle, &m_SyncMode );
        }

        // Don't leave object in partly-initialized state if something went wrong
        if( result != VK_SUCCESS )
        {
            Destroy();
        }

        return result;
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:
        Destructor.

    \***********************************************************************************/
    void ProfilerOverlayOutput::Destroy()
    {
        if( m_pDevice )
        {
            m_pDevice->Callbacks.DeviceWaitIdle( m_pDevice->Handle );
        }

        if( m_pStringSerializer )
        {
            delete m_pStringSerializer;
            m_pStringSerializer = nullptr;
        }

        if( m_pImGuiVulkanContext )
        {
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
            std::scoped_lock imGuiLock( s_ImGuiMutex );
            ImGui::DestroyContext( m_pImGuiContext );
            m_pImGuiContext = nullptr;
        }

        if( m_DescriptorPool )
        {
            m_pDevice->Callbacks.DestroyDescriptorPool( m_pDevice->Handle, m_DescriptorPool, nullptr );
            m_DescriptorPool = VK_NULL_HANDLE;
        }

        if( m_RenderPass )
        {
            m_pDevice->Callbacks.DestroyRenderPass( m_pDevice->Handle, m_RenderPass, nullptr );
            m_RenderPass = VK_NULL_HANDLE;
        }

        if( m_CommandPool )
        {
            m_pDevice->Callbacks.DestroyCommandPool( m_pDevice->Handle, m_CommandPool, nullptr );
            m_CommandPool = VK_NULL_HANDLE;
        }

        m_CommandBuffers.clear();

        for( auto& framebuffer : m_Framebuffers )
        {
            m_pDevice->Callbacks.DestroyFramebuffer( m_pDevice->Handle, framebuffer, nullptr );
        }

        m_Framebuffers.clear();

        for( auto& imageView : m_ImageViews )
        {
            m_pDevice->Callbacks.DestroyImageView( m_pDevice->Handle, imageView, nullptr );
        }

        m_ImageViews.clear();

        m_Images.clear();

        for( auto& fence : m_CommandFences )
        {
            m_pDevice->Callbacks.DestroyFence( m_pDevice->Handle, fence, nullptr );
        }

        m_CommandFences.clear();

        for( auto& semaphore : m_CommandSemaphores )
        {
            m_pDevice->Callbacks.DestroySemaphore( m_pDevice->Handle, semaphore, nullptr );
        }

        m_CommandSemaphores.clear();

        m_ImageFormat = VK_FORMAT_UNDEFINED;

        m_Window = OSWindowHandle();
        m_pDevice = nullptr;
        m_pSwapchain = nullptr;
    }

    /***********************************************************************************\

    Function:
        IsAvailable

    Description:
        Check if profiler overlay is ready for presenting.

    \***********************************************************************************/
    bool ProfilerOverlayOutput::IsAvailable() const
    {
#ifndef _DEBUG
        // There are many other objects that could be checked here, but we're keeping
        // object quite consistent in case of any errors during initialization, so
        // checking just one should be sufficient.
        return (m_pSwapchain);
#else
        // Check object state to confirm the note above
        return (m_pSwapchain)
            && (m_pDevice)
            && (m_pGraphicsQueue)
            && (m_pImGuiContext)
            && (m_pImGuiVulkanContext)
            && (m_pImGuiWindowContext)
            && (m_RenderPass)
            && (!m_CommandBuffers.empty());
#endif
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
    VkResult ProfilerOverlayOutput::ResetSwapchain(
        VkSwapchainKhr_Object& swapchain,
        const VkSwapchainCreateInfoKHR* pCreateInfo )
    {
        assert( m_pSwapchain == nullptr ||
                pCreateInfo->oldSwapchain == m_pSwapchain->Handle ||
                pCreateInfo->oldSwapchain == VK_NULL_HANDLE );

        VkResult result = VK_SUCCESS;

        // Get swapchain images
        uint32_t swapchainImageCount = 0;
        m_pDevice->Callbacks.GetSwapchainImagesKHR(
            m_pDevice->Handle,
            swapchain.Handle,
            &swapchainImageCount,
            nullptr );

        std::vector<VkImage> images( swapchainImageCount );
        result = m_pDevice->Callbacks.GetSwapchainImagesKHR(
            m_pDevice->Handle,
            swapchain.Handle,
            &swapchainImageCount,
            images.data() );

        assert( result == VK_SUCCESS );

        // Recreate render pass if swapchain format has changed
        if( (result == VK_SUCCESS) && (pCreateInfo->imageFormat != m_ImageFormat) )
        {
            if( m_RenderPass != VK_NULL_HANDLE )
            {
                // Destroy old render pass
                m_pDevice->Callbacks.DestroyRenderPass( m_pDevice->Handle, m_RenderPass, nullptr );
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

            m_ImageFormat = pCreateInfo->imageFormat;
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
                    m_pDevice->Callbacks.DestroyFramebuffer( m_pDevice->Handle, m_Framebuffers[ i ], nullptr );
                    m_pDevice->Callbacks.DestroyImageView( m_pDevice->Handle, m_ImageViews[ i ], nullptr );
                }

                m_Framebuffers.clear();
                m_ImageViews.clear();
            }

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
                    info.format = pCreateInfo->imageFormat;
                    info.image = images[ i ];
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
                    info.width = pCreateInfo->imageExtent.width;
                    info.height = pCreateInfo->imageExtent.height;
                    info.layers = 1;

                    result = m_pDevice->Callbacks.CreateFramebuffer(
                        m_pDevice->Handle,
                        &info,
                        nullptr,
                        &framebuffer );

                    m_Framebuffers.push_back( framebuffer );
                }
            }

            m_RenderArea = pCreateInfo->imageExtent;
        }

        // Allocate additional command buffers, fences and semaphores
        if( (result == VK_SUCCESS) && (swapchainImageCount > m_Images.size()) )
        {
            VkCommandBufferAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = m_CommandPool;
            allocInfo.commandBufferCount = swapchainImageCount - static_cast<uint32_t>( m_Images.size() );

            std::vector<VkCommandBuffer> commandBuffers( swapchainImageCount );

            result = m_pDevice->Callbacks.AllocateCommandBuffers(
                m_pDevice->Handle,
                &allocInfo,
                commandBuffers.data() );

            if( result == VK_SUCCESS )
            {
                // Append created command buffers to end
                // We need to do this right after allocation to avoid leaks if something fails later
                m_CommandBuffers.insert( m_CommandBuffers.end(), commandBuffers.begin(), commandBuffers.end() );
            }

            for( auto cmdBuffer : commandBuffers )
            {
                if( result == VK_SUCCESS )
                {
                    // Command buffers are dispatchable handles, update pointers to parent's dispatch table
                    result = m_pDevice->SetDeviceLoaderData( m_pDevice->Handle, cmdBuffer );
                }
            }

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
        if( result == VK_SUCCESS )
        {
            m_pSwapchain = &swapchain;
            m_Images = images;
        }

        // Reinitialize ImGui
        if( (m_pImGuiContext) )
        {
            std::scoped_lock lk( s_ImGuiMutex );
            ImGui::SetCurrentContext( m_pImGuiContext );

            if( result == VK_SUCCESS )
            {
                // Reinit window
                result = InitializeImGuiWindowHooks( pCreateInfo );
            }

            if( result == VK_SUCCESS )
            {
                // Init vulkan
                result = InitializeImGuiVulkanContext( pCreateInfo );
            }
        }

        // Don't leave object in partly-initialized state
        if( result != VK_SUCCESS )
        {
            Destroy();
        }

        return result;
    }

    /***********************************************************************************\

    Function:
        Present

    Description:
        Draw profiler overlay before presenting the image to screen.

    \***********************************************************************************/
    void ProfilerOverlayOutput::Present(
        const DeviceProfilerFrameData& data,
        const VkQueue_Object& queue,
        VkPresentInfoKHR* pPresentInfo )
    {
        std::scoped_lock lk( s_ImGuiMutex );
        ImGui::SetCurrentContext( m_pImGuiContext );

        // Record interface draw commands
        Update( data );

        ImDrawData* pDrawData = ImGui::GetDrawData();
        if( pDrawData )
        {
            // Grab command buffer for overlay commands
            const uint32_t imageIndex = pPresentInfo->pImageIndices[ 0 ];

            // Per-
            VkFence& fence = m_CommandFences[ imageIndex ];
            VkSemaphore& semaphore = m_CommandSemaphores[ imageIndex ];
            VkCommandBuffer& commandBuffer = m_CommandBuffers[ imageIndex ];
            VkFramebuffer& framebuffer = m_Framebuffers[ imageIndex ];

            m_pDevice->Callbacks.WaitForFences( m_pDevice->Handle, 1, &fence, VK_TRUE, UINT64_MAX );
            m_pDevice->Callbacks.ResetFences( m_pDevice->Handle, 1, &fence );

            {
                VkCommandBufferBeginInfo info = {};
                info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
                m_pDevice->Callbacks.BeginCommandBuffer( commandBuffer, &info );
            }
            {
                VkRenderPassBeginInfo info = {};
                info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                info.renderPass = m_RenderPass;
                info.framebuffer = framebuffer;
                info.renderArea.extent.width = m_RenderArea.width;
                info.renderArea.extent.height = m_RenderArea.height;
                m_pDevice->Callbacks.CmdBeginRenderPass( commandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE );
            }

            // Record Imgui Draw Data and draw funcs into command buffer
            m_pImGuiVulkanContext->RenderDrawData( pDrawData, commandBuffer );

            // Submit command buffer
            m_pDevice->Callbacks.CmdEndRenderPass( commandBuffer );

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

                m_pDevice->Callbacks.EndCommandBuffer( commandBuffer );
                m_pDevice->Callbacks.QueueSubmit( m_pGraphicsQueue->Handle, 1, &info, fence );
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
    void ProfilerOverlayOutput::Update( const DeviceProfilerFrameData& data )
    {
        m_pImGuiVulkanContext->NewFrame();

        m_pImGuiWindowContext->NewFrame();

        ImGui::NewFrame();
        ImGui::PushFont( m_Fonts.GetDefaultFont() );

        ImGui::Begin( m_Title.c_str(), nullptr, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_MenuBar );

        // Update input clipping rect
        ImVec2 pos = ImGui::GetWindowPos();
        ImVec2 size = ImGui::GetWindowSize();
        m_pImGuiWindowContext->AddInputCaptureRect( pos.x, pos.y, size.x, size.y );

        if( ImGui::BeginMenuBar() )
        {
            if( ImGui::BeginMenu( Lang::FileMenu ) )
            {
                if( ImGui::MenuItem( Lang::Save ) )
                {
                    SaveTrace();
                }
                ImGui::EndMenu();
            }

            if( ImGui::BeginMenu( Lang::WindowMenu ) )
            {
                ImGui::MenuItem( Lang::PerformanceMenuItem, nullptr, m_PerformanceWindowState.pOpen );
                ImGui::MenuItem( Lang::TopPipelinesMenuItem, nullptr, m_TopPipelinesWindowState.pOpen );
                ImGui::MenuItem( Lang::PerformanceCountersMenuItem, nullptr, m_PerformanceCountersWindowState.pOpen );
                ImGui::MenuItem( Lang::MemoryMenuItem, nullptr, m_MemoryWindowState.pOpen );
                ImGui::MenuItem( Lang::InspectorMenuItem, nullptr, m_InspectorWindowState.pOpen );
                ImGui::MenuItem( Lang::StatisticsMenuItem, nullptr, m_StatisticsWindowState.pOpen );
                ImGui::MenuItem( Lang::SettingsMenuItem, nullptr, m_SettingsWindowState.pOpen );
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        // Save results to file
        if( ImGui::Button( Lang::Save ) )
        {
            SaveTrace();
        }

        // Keep results
        ImGui::SameLine();
        ImGui::Checkbox( Lang::Pause, &m_Pause );

        ImGuiX::TextAlignRight( "Vulkan %u.%u",
            VK_API_VERSION_MAJOR( m_pDevice->pInstance->ApplicationInfo.apiVersion ),
            VK_API_VERSION_MINOR( m_pDevice->pInstance->ApplicationInfo.apiVersion ) );

        if( !m_Pause )
        {
            // Update data
            m_Data = data;
        }

        // Add padding
        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + 5 );

        m_MainDockSpaceId = ImGui::GetID( "##m_MainDockSpaceId" );
        m_PerformanceTabDockSpaceId = ImGui::GetID( "##m_PerformanceTabDockSpaceId" );

        ImU32 defaultWindowBg = ImGui::GetColorU32( ImGuiCol_WindowBg );
        ImU32 defaultTitleBg = ImGui::GetColorU32( ImGuiCol_TitleBg );
        ImU32 defaultTitleBgActive = ImGui::GetColorU32( ImGuiCol_TitleBgActive );
        int numPushedColors = 0;
        bool isOpen = false;
        bool isExpanded = false;

        auto BeginDockingWindow = [&]( const char* pTitle, int dockSpaceId, WindowState& state )
            {
                isExpanded = false;
                if( isOpen = (!state.pOpen || *state.pOpen) )
                {
                    if( !state.Docked )
                    {
                        ImGui::PushStyleColor( ImGuiCol_WindowBg, defaultWindowBg );
                        ImGui::PushStyleColor( ImGuiCol_TitleBg, defaultTitleBg );
                        ImGui::PushStyleColor( ImGuiCol_TitleBgActive, defaultTitleBgActive );
                        numPushedColors = 3;
                    }

                    if( state.Focus )
                    {
                        ImGui::SetNextWindowFocus();
                    }

                    ImGui::SetNextWindowDockID( dockSpaceId, ImGuiCond_FirstUseEver );

                    isExpanded = ImGui::Begin( pTitle, state.pOpen );

                    ImGuiID dockSpaceId = ImGuiX::GetWindowDockSpaceID();
                    state.Docked = ImGui::IsWindowDocked() &&
                        (dockSpaceId == m_MainDockSpaceId ||
                            dockSpaceId == m_PerformanceTabDockSpaceId);
                    
                    if( !state.Docked )
                    {
                        // Add input clipping rect for this window
                        ImVec2 pos = ImGui::GetWindowPos();
                        ImVec2 size = ImGui::GetWindowSize();
                        m_pImGuiWindowContext->AddInputCaptureRect( pos.x, pos.y, size.x, size.y );
                    }

                    state.Focus = false;
                }
                return isExpanded;
            };

        auto EndDockingWindow = [&]()
            {
                if( isOpen )
                {
                    ImGui::End();
                    ImGui::PopStyleColor( numPushedColors );
                    numPushedColors = 0;
                }
            };

        ImU32 transparentColor = ImGui::GetColorU32( { 0, 0, 0, 0 } );
        ImGui::PushStyleColor( ImGuiCol_WindowBg, transparentColor );
        ImGui::PushStyleColor( ImGuiCol_TitleBg, transparentColor );
        ImGui::PushStyleColor( ImGuiCol_TitleBgActive, transparentColor );

        ImGui::DockSpace( m_MainDockSpaceId );

        if( BeginDockingWindow( Lang::Performance, m_MainDockSpaceId, m_PerformanceWindowState ) )
        {
            UpdatePerformanceTab();
        }
        else
        {
            ImGui::DockSpace( m_PerformanceTabDockSpaceId, {}, ImGuiDockNodeFlags_KeepAliveOnly );
        }
        EndDockingWindow();

        // Top pipelines
        if( BeginDockingWindow( Lang::TopPipelines, m_PerformanceTabDockSpaceId, m_TopPipelinesWindowState ) )
        {
            UpdateTopPipelinesTab();
        }
        EndDockingWindow();

        if( BeginDockingWindow( Lang::PerformanceCounters, m_PerformanceTabDockSpaceId, m_PerformanceCountersWindowState ) )
        {
            UpdatePerformanceCountersTab();
        }
        EndDockingWindow();

        if( BeginDockingWindow( Lang::Memory, m_MainDockSpaceId, m_MemoryWindowState ) )
        {
            UpdateMemoryTab();
        }
        EndDockingWindow();

        if( BeginDockingWindow( Lang::Inspector, m_MainDockSpaceId, m_InspectorWindowState ) )
        {
            UpdateInspectorTab();
        }
        EndDockingWindow();

        if( BeginDockingWindow( Lang::Statistics, m_MainDockSpaceId, m_StatisticsWindowState ) )
        {
            UpdateStatisticsTab();
        }
        EndDockingWindow();

        if( BeginDockingWindow( Lang::Settings, m_MainDockSpaceId, m_SettingsWindowState ) )
        {
            UpdateSettingsTab();
        }
        EndDockingWindow();

        ImGui::PopStyleColor( 3 );
        ImGui::End();

        // Draw other windows
        DrawTraceSerializationOutputWindow();

        // Set initial tab
        if( ImGui::GetFrameCount() == 1 )
        {
            ImGui::SetWindowFocus( Lang::Performance );
        }

        // Draw foreground overlay
        ImDrawList* pForegroundDrawList = ImGui::GetForegroundDrawList();
        if( pForegroundDrawList != nullptr )
        {
            // Draw cursor pointer in case the application doesn't render one.
            // It is also needed when the app uses raw input because relative movements may be
            // translated differently by the application and by the layer.
            pForegroundDrawList->AddCircleFilled( ImGui::GetIO().MousePos, 2.f, 0xffffffff, 4 );
        }

        ImGui::PopFont();
        ImGui::Render();
    }

    /***********************************************************************************\

    Function:
        InitializeImGuiWindowHooks

    Description:

    \***********************************************************************************/
    VkResult ProfilerOverlayOutput::InitializeImGuiWindowHooks( const VkSwapchainCreateInfoKHR* pCreateInfo )
    {
        VkResult result = VK_SUCCESS;

        // Get window handle from the swapchain surface
        OSWindowHandle window = m_pDevice->pInstance->Surfaces.at( pCreateInfo->surface ).Window;

        if( m_Window == window )
        {
            // No need to update window hooks
            return result;
        }

        // Free current window
        delete m_pImGuiWindowContext;

        try
        {
#ifdef VK_USE_PLATFORM_WIN32_KHR
            if( window.Type == OSWindowHandleType::eWin32 )
            {
                m_pImGuiWindowContext = new ImGui_ImplWin32_Context( window.Win32Handle );
            }
#endif // VK_USE_PLATFORM_WIN32_KHR

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
            if( window.Type == OSWindowHandleType::eWayland )
            {
                m_pImGuiWindowContext = new ImGui_ImplWayland_Context( window.WaylandHandle );
            }
#endif // VK_USE_PLATFORM_WAYLAND_KHR

#ifdef VK_USE_PLATFORM_XCB_KHR
            if( window.Type == OSWindowHandleType::eXcb )
            {
                m_pImGuiWindowContext = new ImGui_ImplXcb_Context( window.XcbHandle );
            }
#endif // VK_USE_PLATFORM_XCB_KHR

#ifdef VK_USE_PLATFORM_XLIB_KHR
            if( window.Type == OSWindowHandleType::eXlib )
            {
                m_pImGuiWindowContext = new ImGui_ImplXlib_Context( window.XlibHandle );
            }
#endif // VK_USE_PLATFORM_XLIB_KHR
        }
        catch( ... )
        {
            // Catch exceptions thrown by OS-specific ImGui window constructors
            result = VK_ERROR_INITIALIZATION_FAILED;
        }

        // Set DPI scaling.
        if( result == VK_SUCCESS )
        {
            ImGuiIO& io = ImGui::GetIO();
            io.FontGlobalScale = m_pImGuiWindowContext->GetDPIScale();
            assert(io.FontGlobalScale > 0.0f);
        }

        // Deinitialize context if something failed
        if( result != VK_SUCCESS )
        {
            delete m_pImGuiWindowContext;
            m_pImGuiWindowContext = nullptr;
        }

        // Update objects
        m_Window = window;

        return result;
    }

    /***********************************************************************************\

    Function:
        InitializeImGuiDefaultFont

    Description:

    \***********************************************************************************/
    void ProfilerOverlayOutput::InitializeImGuiDefaultFont()
    {
        m_Fonts.Initialize();
    }

    /***********************************************************************************\

    Function:
        InitializeImGuiColors

    Description:

    \***********************************************************************************/
    void ProfilerOverlayOutput::InitializeImGuiStyle()
    {
        ImGui::StyleColorsDark();

        ImGuiStyle& style = ImGui::GetStyle();
        // Round window corners
        style.WindowRounding = 7.f;

        // Performance graph colors
        m_RenderPassColumnColor = ImGui::GetColorU32( { 0.9f, 0.7f, 0.0f, 1.0f } ); // #e6b200
        m_GraphicsPipelineColumnColor = ImGui::GetColorU32( { 0.9f, 0.7f, 0.0f, 1.0f } ); // #e6b200
        m_ComputePipelineColumnColor = ImGui::GetColorU32( { 0.9f, 0.55f, 0.0f, 1.0f } ); // #ffba42
        m_RayTracingPipelineColumnColor = ImGui::GetColorU32( { 0.2f, 0.73f, 0.92f, 1.0f } ); // #34baeb
        m_InternalPipelineColumnColor = ImGui::GetColorU32( { 0.5f, 0.22f, 0.9f, 1.0f } ); // #9e30ff
    }

    /***********************************************************************************\

    Function:
        InitializeImGuiVulkanContext

    Description:

    \***********************************************************************************/
    VkResult ProfilerOverlayOutput::InitializeImGuiVulkanContext( const VkSwapchainCreateInfoKHR* pCreateInfo )
    {
        VkResult result = VK_SUCCESS;

        // Free current context
        delete m_pImGuiVulkanContext;

        try
        {
            ImGui_ImplVulkan_InitInfo imGuiInitInfo;
            std::memset( &imGuiInitInfo, 0, sizeof( imGuiInitInfo ) );

            imGuiInitInfo.Queue = m_pGraphicsQueue->Handle;
            imGuiInitInfo.QueueFamily = m_pGraphicsQueue->Family;

            imGuiInitInfo.Instance = m_pDevice->pInstance->Handle;
            imGuiInitInfo.PhysicalDevice = m_pDevice->pPhysicalDevice->Handle;
            imGuiInitInfo.Device = m_pDevice->Handle;

            imGuiInitInfo.pInstanceDispatchTable = &m_pDevice->pInstance->Callbacks;
            imGuiInitInfo.pDispatchTable = &m_pDevice->Callbacks;

            imGuiInitInfo.Allocator = nullptr;
            imGuiInitInfo.PipelineCache = VK_NULL_HANDLE;
            imGuiInitInfo.CheckVkResultFn = nullptr;

            imGuiInitInfo.MinImageCount = pCreateInfo->minImageCount;
            imGuiInitInfo.ImageCount = static_cast<uint32_t>( m_Images.size() );
            imGuiInitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

            imGuiInitInfo.DescriptorPool = m_DescriptorPool;

            m_pImGuiVulkanContext = new ImGui_ImplVulkan_Context( &imGuiInitInfo, m_RenderPass );
        }
        catch( ... )
        {
            // Catch all exceptions thrown by the context constructor and return VkResult
            result = VK_ERROR_INITIALIZATION_FAILED;
        }

        // Initialize fonts
        if( result == VK_SUCCESS )
        {
            result = m_pDevice->Callbacks.ResetFences( m_pDevice->Handle, 1, &m_CommandFences[ 0 ] );
        }

        if( result == VK_SUCCESS )
        {
            VkCommandBufferBeginInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            result = m_pDevice->Callbacks.BeginCommandBuffer( m_CommandBuffers[ 0 ], &info );
        }

        if( result == VK_SUCCESS )
        {
            m_pImGuiVulkanContext->CreateFontsTexture( m_CommandBuffers[ 0 ] );
        }

        if( result == VK_SUCCESS )
        {
            result = m_pDevice->Callbacks.EndCommandBuffer( m_CommandBuffers[ 0 ] );
        }

        // Submit initialization work
        if( result == VK_SUCCESS )
        {
            VkSubmitInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            info.commandBufferCount = 1;
            info.pCommandBuffers = &m_CommandBuffers[ 0 ];

            result = m_pDevice->Callbacks.QueueSubmit( m_pGraphicsQueue->Handle, 1, &info, m_CommandFences[ 0 ] );
        }

        // Deinitialize context if something failed
        if( result != VK_SUCCESS )
        {
            delete m_pImGuiVulkanContext;
            m_pImGuiVulkanContext = nullptr;
        }

        return result;
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
            const Milliseconds gpuTimeMs = m_Data.m_Ticks * m_TimestampPeriod;
            const Milliseconds cpuTimeMs = m_Data.m_CPU.m_EndTimestamp - m_Data.m_CPU.m_BeginTimestamp;

            ImGui::Text( "%s: %.2f ms", Lang::GPUTime, gpuTimeMs.count() );
            ImGui::Text( "%s: %.2f ms", Lang::CPUTime, cpuTimeMs.count() );
            ImGuiX::TextAlignRight( "%.1f %s", m_Data.m_CPU.m_FramesPerSec, Lang::FPS );
        }

        // Histogram
        {
            static const char* groupOptions[] = {
                Lang::RenderPasses,
                Lang::Pipelines,
                Lang::Drawcalls };

            const char* selectedOption = groupOptions[ (size_t)m_HistogramGroupMode ];

            // Select group mode
            {
                if( ImGui::BeginCombo( Lang::HistogramGroups, selectedOption, ImGuiComboFlags_NoPreview ) )
                {
                    for( size_t i = 0; i < std::extent_v<decltype(groupOptions)>; ++i )
                    {
                        if( ImGuiX::TSelectable( groupOptions[ i ], selectedOption, groupOptions[ i ] ) )
                        {
                            // Selection changed
                            m_HistogramGroupMode = HistogramGroupMode( i );
                        }
                    }

                    ImGui::EndCombo();
                }
            }

            // Enumerate columns for selected group mode
            std::vector<PerformanceGraphColumn> columns;
            GetPerformanceGraphColumns( columns );

            char pHistogramDescription[ 32 ];
            snprintf( pHistogramDescription, sizeof( pHistogramDescription ),
                "%s (%s)",
                Lang::GPUCycles,
                selectedOption );

            ImGui::PushItemWidth( -1 );
            ImGuiX::PlotHistogramEx(
                "",
                columns.data(),
                static_cast<int>( columns.size() ),
                0,
                sizeof( columns.front() ),
                pHistogramDescription, 0, FLT_MAX, { 0, 100 },
                std::bind( &ProfilerOverlayOutput::DrawPerformanceGraphLabel, this, std::placeholders::_1 ),
                std::bind( &ProfilerOverlayOutput::SelectPerformanceGraphColumn, this, std::placeholders::_1 ) );
        }

        ImGui::DockSpace( m_PerformanceTabDockSpaceId );

        // Force frame browser open
        if( m_ScrollToSelectedFrameBrowserNode )
        {
            ImGui::SetNextItemOpen( true );
        }

        // Frame browser
        ImGui::SetNextWindowDockID( m_PerformanceTabDockSpaceId );
        if( ImGui::Begin( Lang::FrameBrowser ) )
        {
            // Select sort mode
            {
                static const char* sortOptions[] = {
                    Lang::SubmissionOrder,
                    Lang::DurationDescending,
                    Lang::DurationAscending };

                const char* selectedOption = sortOptions[ (size_t)m_FrameBrowserSortMode ];

                ImGui::Text( Lang::Sort );
                ImGui::SameLine();

                if( ImGui::BeginCombo( "##FrameBrowserSortMode", selectedOption ) )
                {
                    for( size_t i = 0; i < std::extent_v<decltype(sortOptions)>; ++i )
                    {
                        if( ImGuiX::TSelectable( sortOptions[ i ], selectedOption, sortOptions[ i ] ) )
                        {
                            // Selection changed
                            m_FrameBrowserSortMode = FrameBrowserSortMode( i );
                        }
                    }

                    ImGui::EndCombo();
                }
            }

            FrameBrowserTreeNodeIndex index;
            index.emplace_back( 0 );

            // Enumerate submits in frame
            for( const auto& submitBatch : m_Data.m_Submits )
            {
                const std::string queueName = m_pStringSerializer->GetName( submitBatch.m_Handle );

                if( ScrollToSelectedFrameBrowserNode( index ) )
                {
                    ImGui::SetNextItemOpen( true );
                }

                const char* indexStr = GetFrameBrowserNodeIndexStr( index );
                if( ImGui::TreeNode( indexStr, "vkQueueSubmit(%s, %u)",
                        queueName.c_str(),
                        static_cast<uint32_t>(submitBatch.m_Submits.size()) ) )
                {
                    index.emplace_back( 0 );

                    for( const auto& submit : submitBatch.m_Submits )
                    {
                        if( ScrollToSelectedFrameBrowserNode( index ) )
                        {
                            ImGui::SetNextItemOpen( true );
                        }

                        const char* indexStr = GetFrameBrowserNodeIndexStr( index );
                        const bool inSubmitSubtree =
                            (submitBatch.m_Submits.size() > 1) &&
                            (ImGui::TreeNode( indexStr, "VkSubmitInfo #%u", index.back() ));

                        if( (inSubmitSubtree) || (submitBatch.m_Submits.size() == 1) )
                        {
                            index.emplace_back( 0 );

                            // Sort frame browser data
                            std::list<const DeviceProfilerCommandBufferData*> pCommandBuffers =
                                SortFrameBrowserData( submit.m_CommandBuffers );

                            // Enumerate command buffers in submit
                            for( const auto* pCommandBuffer : pCommandBuffers )
                            {
                                PrintCommandBuffer( *pCommandBuffer, index );
                                index.back()++;
                            }

                            index.pop_back();
                        }

                        if( inSubmitSubtree )
                        {
                            // Finish submit subtree
                            ImGui::TreePop();
                        }

                        index.back()++;
                    }

                    // Finish submit batch subtree
                    ImGui::TreePop();

                    // Invalidate submit index
                    index.pop_back();
                }

                index.back()++;
            }
        }

        ImGui::End();

        m_ScrollToSelectedFrameBrowserNode = false;
    }

    /***********************************************************************************\

    Function:
        UpdateTopPipelinesTab

    Description:
        Updates "Top pipelines" tab.

    \***********************************************************************************/
    void ProfilerOverlayOutput::UpdateTopPipelinesTab()
    {
        uint32_t i = 0;

        for( const auto& pipeline : m_Data.m_TopPipelines )
        {
            if( pipeline.m_Handle != VK_NULL_HANDLE )
            {
                const uint64_t pipelineTicks = GetDuration( pipeline );

                ImGui::Text( "%2u. %s", i + 1, m_pStringSerializer->GetName( pipeline ).c_str() );
                ImGuiX::TextAlignRight( "(%.1f %%) %.2f ms",
                    pipelineTicks * 100.f / m_Data.m_Ticks,
                    pipelineTicks * m_TimestampPeriod.count() );

                // Print up to 10 top pipelines
                if( (++i) == 10 ) break;
            }
        }
    }

    /***********************************************************************************\

    Function:
        UpdatePerformanceCountersTab

    Description:
        Updates "Performance Counters" tab.

    \***********************************************************************************/
    void ProfilerOverlayOutput::UpdatePerformanceCountersTab()
    {
        // Vendor-specific
        if( !m_Data.m_VendorMetrics.empty() )
        {
            std::unordered_set<VkCommandBuffer> uniqueCommandBuffers;

            // Data source
            const std::vector<VkProfilerPerformanceCounterResultEXT>* pVendorMetrics = &m_Data.m_VendorMetrics;

            bool performanceQueryResultsFiltered = false;

            // Find the first command buffer that matches the filter.
            // TODO: Aggregation.
            for( const auto& submitBatch : m_Data.m_Submits )
            {
                for( const auto& submit : submitBatch.m_Submits )
                {
                    for( const auto& commandBuffer : submit.m_CommandBuffers )
                    {
                        if( (performanceQueryResultsFiltered == false) &&
                            (commandBuffer.m_Handle != VK_NULL_HANDLE) &&
                            (commandBuffer.m_Handle == m_PerformanceQueryCommandBufferFilter) )
                        {
                            // Use the data from this command buffer.
                            pVendorMetrics = &commandBuffer.m_PerformanceQueryResults;
                            performanceQueryResultsFiltered = true;
                        }

                        uniqueCommandBuffers.insert( commandBuffer.m_Handle );
                    }
                }
            }

            // Show a combo box that allows the user to select the filter the profiled range.
            ImGui::TextUnformatted( Lang::PerformanceCountersRange );
            ImGui::SameLine( 100.f );
            if( ImGui::BeginCombo( "##PerformanceQueryFilter", m_PerformanceQueryCommandBufferFilterName.c_str() ) )
            {
                if( ImGuiX::TSelectable( "Frame", m_PerformanceQueryCommandBufferFilter, VkCommandBuffer() ) )
                {
                    // Selection changed.
                    m_PerformanceQueryCommandBufferFilterName = "Frame";
                }

                // Enumerate command buffers.
                for( VkCommandBuffer commandBuffer : uniqueCommandBuffers )
                {
                    std::string commandBufferName = m_pStringSerializer->GetName( commandBuffer );

                    if( ImGuiX::TSelectable( commandBufferName.c_str(), m_PerformanceQueryCommandBufferFilter, commandBuffer ) )
                    {
                        // Selection changed.
                        m_PerformanceQueryCommandBufferFilterName = std::move( commandBufferName );
                    }
                }

                ImGui::EndCombo();
            }

            // Show a combo box that allows the user to change the active metrics set.
            ImGui::TextUnformatted( Lang::PerformanceCountersSet );
            ImGui::SameLine( 100.f );
            if( ImGui::BeginCombo( "##PerformanceQueryMetricsSet", m_VendorMetricsSets[ m_ActiveMetricsSetIndex ].m_Properties.name ) )
            {
                // Enumerate metrics sets.
                for( uint32_t metricsSetIndex = 0; metricsSetIndex < m_VendorMetricsSets.size(); ++metricsSetIndex )
                {
                    if( m_VendorMetricsSetVisibility[ metricsSetIndex ] )
                    {
                        const auto& metricsSet = m_VendorMetricsSets[ metricsSetIndex ];

                        if( ImGuiX::Selectable( metricsSet.m_Properties.name, (m_ActiveMetricsSetIndex == metricsSetIndex) ) )
                        {
                            // Notify the profiler.
                            if( vkSetProfilerPerformanceMetricsSetEXT( m_pDevice->Handle, metricsSetIndex ) == VK_SUCCESS )
                            {
                                // Refresh the performance metric properties.
                                m_ActiveMetricsSetIndex = metricsSetIndex;
                            }
                        }
                    }
                }

                ImGui::EndCombo();
            }

            // Show a search box for filtering metrics sets to find specific metrics.
            ImGui::TextUnformatted( Lang::PerformanceCountersFilter );
            ImGui::SameLine( 100.f );
            if( ImGui::InputText( "##PerformanceQueryMetricsFilter", m_VendorMetricFilter, std::extent_v<decltype(m_VendorMetricFilter)> ) )
            {
                try
                {
                    // Text changed, construct a regex from the string and find the matching metrics sets.
                    std::regex regexFilter( m_VendorMetricFilter );

                    // Enumerate only sets that match the query.
                    for( uint32_t metricsSetIndex = 0; metricsSetIndex < m_VendorMetricsSets.size(); ++metricsSetIndex )
                    {
                        const auto& metricsSet = m_VendorMetricsSets[ metricsSetIndex ];

                        // Match by metrics set name.
                        if( std::regex_search( metricsSet.m_Properties.name, regexFilter ) )
                        {
                            m_VendorMetricsSetVisibility[ metricsSetIndex ] = true;
                            continue;
                        }

                        m_VendorMetricsSetVisibility[ metricsSetIndex ] = false;

                        // Match by metric name.
                        for( const auto& metric : metricsSet.m_Metrics )
                        {
                            if( std::regex_search( metric.shortName, regexFilter ) )
                            {
                                m_VendorMetricsSetVisibility[ metricsSetIndex ] = true;
                                break;
                            }
                        }
                    }
                }
                catch( ... )
                {
                    // Regex compilation failed, don't change the visibility of the sets.
                }
            }

            if( pVendorMetrics->empty() )
            {
                // Vendor metrics not available.
                ImGui::TextUnformatted( Lang::PerformanceCountersNotAvailableForCommandBuffer );
            }

            const auto& activeMetricsSet = m_VendorMetricsSets[ m_ActiveMetricsSetIndex ];
            if( pVendorMetrics->size() == activeMetricsSet.m_Metrics.size() )
            {
                const auto& vendorMetrics = *pVendorMetrics;

                ImGui::BeginTable( "Performance counters table",
                    /* columns_count */ 3,
                    ImGuiTableFlags_NoClip |
                    (ImGuiTableFlags_Borders & ~ImGuiTableFlags_BordersInnerV) );

                // Headers
                ImGui::TableSetupColumn( Lang::Metric, ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize );
                ImGui::TableSetupColumn( Lang::Frame, ImGuiTableColumnFlags_WidthStretch );
                ImGui::TableSetupColumn( "", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize );
                ImGui::TableHeadersRow();

                for( uint32_t i = 0; i < vendorMetrics.size(); ++i )
                {
                    const VkProfilerPerformanceCounterResultEXT& metric = vendorMetrics[ i ];
                    const VkProfilerPerformanceCounterPropertiesEXT& metricProperties = activeMetricsSet.m_Metrics[ i ];

                    ImGui::TableNextColumn();
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

                    ImGui::TableNextColumn();
                    {
                        const float columnWidth = ImGuiX::TableGetColumnWidth();
                        switch( metricProperties.storage )
                        {
                        case VK_PERFORMANCE_COUNTER_STORAGE_FLOAT32_KHR:
                            ImGuiX::TextAlignRight( columnWidth, "%.2f", metric.float32 );
                            break;

                        case VK_PERFORMANCE_COUNTER_STORAGE_UINT32_KHR:
                            ImGuiX::TextAlignRight( columnWidth, "%u", metric.uint32 );
                            break;

                        case VK_PERFORMANCE_COUNTER_STORAGE_UINT64_KHR:
                            ImGuiX::TextAlignRight( columnWidth, "%llu", metric.uint64 );
                            break;
                        }
                    }

                    ImGui::TableNextColumn();
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
        }
        else
        {
            ImGui::TextUnformatted( Lang::PerformanceCountesrNotAvailable );
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
            m_pDevice->pPhysicalDevice->MemoryProperties;

        if( ImGui::CollapsingHeader( Lang::MemoryHeapUsage ) )
        {
            for( uint32_t i = 0; i < memoryProperties.memoryHeapCount; ++i )
            {
                ImGui::Text( "%s %u", Lang::MemoryHeap, i );

                ImGuiX::TextAlignRight( "%u %s", m_Data.m_Memory.m_Heaps[ i ].m_AllocationCount, Lang::Allocations );

                float usage = 0.f;
                char usageStr[ 64 ] = {};

                if( memoryProperties.memoryHeaps[ i ].size != 0 )
                {
                    usage = (float)m_Data.m_Memory.m_Heaps[ i ].m_AllocationSize / memoryProperties.memoryHeaps[ i ].size;

                    snprintf( usageStr, sizeof( usageStr ),
                        "%.2f/%.2f MB (%.1f%%)",
                        m_Data.m_Memory.m_Heaps[ i ].m_AllocationSize / 1048576.f,
                        memoryProperties.memoryHeaps[ i ].size / 1048576.f,
                        usage * 100.f );
                }

                ImGui::ProgressBar( usage, { -1, 0 }, usageStr );

                if( ImGui::IsItemHovered() && (memoryProperties.memoryHeaps[ i ].flags != 0) )
                {
                    ImGui::BeginTooltip();

                    if( memoryProperties.memoryHeaps[ i ].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT )
                    {
                        ImGui::TextUnformatted( "VK_MEMORY_HEAP_DEVICE_LOCAL_BIT" );
                    }

                    if( memoryProperties.memoryHeaps[ i ].flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT )
                    {
                        ImGui::TextUnformatted( "VK_MEMORY_HEAP_MULTI_INSTANCE_BIT" );
                    }

                    ImGui::EndTooltip();
                }

                std::vector<float> memoryTypeUsages( memoryProperties.memoryTypeCount );
                std::vector<std::string> memoryTypeDescriptors( memoryProperties.memoryTypeCount );

                for( uint32_t typeIndex = 0; typeIndex < memoryProperties.memoryTypeCount; ++typeIndex )
                {
                    if( memoryProperties.memoryTypes[ typeIndex ].heapIndex == i )
                    {
                        memoryTypeUsages[ typeIndex ] = static_cast<float>( m_Data.m_Memory.m_Types[ typeIndex ].m_AllocationSize );

                        // Prepare descriptor for memory type
                        std::stringstream sstr;

                        sstr << Lang::MemoryTypeIndex << " " << typeIndex << "\n"
                             << m_Data.m_Memory.m_Types[ typeIndex ].m_AllocationCount << " " << Lang::Allocations << "\n";

                        if( memoryProperties.memoryTypes[ typeIndex ].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT )
                        {
                            sstr << "VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT\n";
                        }

                        if( memoryProperties.memoryTypes[ typeIndex ].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD )
                        {
                            sstr << "VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD\n";
                        }

                        if( memoryProperties.memoryTypes[ typeIndex ].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD )
                        {
                            sstr << "VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD\n";
                        }

                        if( memoryProperties.memoryTypes[ typeIndex ].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT )
                        {
                            sstr << "VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT\n";
                        }

                        if( memoryProperties.memoryTypes[ typeIndex ].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT )
                        {
                            sstr << "VK_MEMORY_PROPERTY_HOST_COHERENT_BIT\n";
                        }

                        if( memoryProperties.memoryTypes[ typeIndex ].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT )
                        {
                            sstr << "VK_MEMORY_PROPERTY_HOST_CACHED_BIT\n";
                        }

                        if( memoryProperties.memoryTypes[ typeIndex ].propertyFlags & VK_MEMORY_PROPERTY_PROTECTED_BIT )
                        {
                            sstr << "VK_MEMORY_PROPERTY_PROTECTED_BIT\n";
                        }

                        if( memoryProperties.memoryTypes[ typeIndex ].propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT )
                        {
                            sstr << "VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT\n";
                        }

                        memoryTypeDescriptors[ typeIndex ] = sstr.str();
                    }
                }

                // Get descriptor pointers
                std::vector<const char*> memoryTypeDescriptorPointers( memoryProperties.memoryTypeCount );

                for( uint32_t typeIndex = 0; typeIndex < memoryProperties.memoryTypeCount; ++typeIndex )
                {
                    memoryTypeDescriptorPointers[ typeIndex ] = memoryTypeDescriptors[ typeIndex ].c_str();
                }

                ImGuiX::PlotBreakdownEx(
                    "HEAP_BREAKDOWN",
                    memoryTypeUsages.data(),
                    memoryProperties.memoryTypeCount, 0,
                    memoryTypeDescriptorPointers.data() );
            }
        }
    }

    /***********************************************************************************\

    Function:
        UpdateInspectorTab

    Description:
        Updates "Inspector" tab.

    \***********************************************************************************/
    void ProfilerOverlayOutput::UpdateInspectorTab()
    {
        // Early out if no valid pipeline is selected.
        if( !m_InspectorPipeline.m_Handle && !m_InspectorPipeline.m_UsesShaderObjects )
        {
            ImGui::TextUnformatted( "No pipeline selected for inspection." );
            return;
        }

        // Enumerate inspector tabs.
        ImGui::PushItemWidth( -1 );

        if( ImGui::BeginCombo( "##InspectorTabs", m_InspectorTabs[m_InspectorTabIndex].Name.c_str() ) )
        {
            const size_t tabCount = m_InspectorTabs.size();
            for( size_t i = 0; i < tabCount; ++i )
            {
                if( ImGuiX::TSelectable( m_InspectorTabs[ i ].Name.c_str(), m_InspectorTabIndex, i ) )
                {
                    // Change tab.
                    SetInspectorTabIndex( i );
                }
            }

            ImGui::EndCombo();
        }

        // Render the inspector tab.
        const InspectorTab& tab = m_InspectorTabs[m_InspectorTabIndex];
        if( tab.Draw )
        {
            tab.Draw();
        }
    }

    /***********************************************************************************\

    Function:
        Inspect

    Description:
        Sets the inspected pipeline and switches the view to the "Inspector" tab.

    \***********************************************************************************/
    void ProfilerOverlayOutput::Inspect( const DeviceProfilerPipeline& pipeline )
    {
        m_InspectorPipeline = pipeline;

        // Resolve inspected pipeline shader stage names.
        m_InspectorTabs.clear();
        m_InspectorTabs.push_back( { Lang::PipelineState,
            nullptr,
            std::bind( &ProfilerOverlayOutput::DrawInspectorPipelineState, this ) } );

        const size_t shaderCount = m_InspectorPipeline.m_ShaderTuple.m_Shaders.size();
        const ProfilerShader* pShaders = m_InspectorPipeline.m_ShaderTuple.m_Shaders.data();
        for( size_t shaderIndex = 0; shaderIndex < shaderCount; ++shaderIndex )
        {
            m_InspectorTabs.push_back( { m_pStringSerializer->GetShaderName( pShaders[shaderIndex] ),
                std::bind( &ProfilerOverlayOutput::SelectInspectorShaderStage, this, shaderIndex ),
                std::bind( &ProfilerOverlayOutput::DrawInspectorShaderStage, this ) } );
        }

        SetInspectorTabIndex( 0 );

        // Switch to the inspector tab.
        *m_InspectorWindowState.pOpen = true;
        m_InspectorWindowState.Focus = true;
    }

    /***********************************************************************************\

    Function:
        SelectInspectorShaderStage

    Description:
        Sets the inspected shader stage and updates the view.

    \***********************************************************************************/
    void ProfilerOverlayOutput::SelectInspectorShaderStage( size_t shaderIndex )
    {
        const ProfilerShader& shader = m_InspectorPipeline.m_ShaderTuple.m_Shaders[ shaderIndex ];

        m_InspectorShaderView.Clear();

        // Shader module may not be available if the VkShaderEXT has been created directly from a binary.
        if( shader.m_pShaderModule )
        {
            const auto& bytecode = shader.m_pShaderModule->m_Bytecode;
            m_InspectorShaderView.AddBytecode( bytecode.data(), bytecode.size() );
        }

        // Enumerate shader internal representations associated with the selected stage.
        for( const ProfilerShaderExecutable& executable : m_InspectorPipeline.m_ShaderTuple.m_ShaderExecutables )
        {
            if( executable.GetStages() & shader.m_Stage )
            {
                m_InspectorShaderView.AddShaderExecutable( executable );
            }
        }
    }

    /***********************************************************************************\

    Function:
        DrawInspectorShaderStage

    Description:
        Draws the inspected shader stage.

    \***********************************************************************************/
    void ProfilerOverlayOutput::DrawInspectorShaderStage()
    {
        m_InspectorShaderView.Draw();
    }

    /***********************************************************************************\

    Function:
        DrawInspectorPipelineState

    Description:
        Draws the inspected pipeline state.

    \***********************************************************************************/
    void ProfilerOverlayOutput::DrawInspectorPipelineState()
    {
        if( m_InspectorPipeline.m_pCreateInfo == nullptr )
        {
            ImGui::TextUnformatted( Lang::PipelineStateNotAvailable );
            return;
        }

        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + 5 );

        switch( m_InspectorPipeline.m_Type )
        {
        case DeviceProfilerPipelineType::eGraphics:
            DrawInspectorGraphicsPipelineState();
            break;
        }
    }

    /***********************************************************************************\

    Function:
        DrawInspectorGraphicsPipelineState

    Description:
        Draws the inspected graphics pipeline state.

    \***********************************************************************************/
    static bool IsPipelineStateDynamic( const VkPipelineDynamicStateCreateInfo* pDynamicStateInfo, VkDynamicState dynamicState )
    {
        if( pDynamicStateInfo != nullptr )
        {
            for( uint32_t i = 0; i < pDynamicStateInfo->dynamicStateCount; ++i )
            {
                if( pDynamicStateInfo->pDynamicStates[i] == dynamicState )
                {
                    return true;
                }
            }
        }
        return false;
    }

    template<typename T>
    static void DrawPipelineStateValue( const char* pName, const char* pFormat, T&& value, const VkPipelineDynamicStateCreateInfo* pDynamicStateInfo = nullptr, VkDynamicState dynamicState = {} )
    {
        ImGui::TableNextRow();

        if( ImGui::TableNextColumn() )
        {
            ImGui::TextUnformatted( pName );
        }

        if( ImGui::TableNextColumn() )
        {
            if( IsPipelineStateDynamic( pDynamicStateInfo, dynamicState ) )
            {
                ImGui::PushStyleColor( ImGuiCol_Text, IM_COL32( 128, 128, 128, 255 ) );
                ImGui::TextUnformatted( "Dynamic" );
                ImGui::PopStyleColor();

                if( ImGui::IsItemHovered() )
                {
                    ImGui::SetTooltip( "This state is set dynamically." );
                }
            }
        }

        if( ImGui::TableNextColumn() )
        {
            ImGui::Text( pFormat, value );
        }
    }

    void ProfilerOverlayOutput::DrawInspectorGraphicsPipelineState()
    {
        assert( m_InspectorPipeline.m_Type == DeviceProfilerPipelineType::eGraphics );
        assert( m_InspectorPipeline.m_pCreateInfo != nullptr );
        const VkGraphicsPipelineCreateInfo& gci = m_InspectorPipeline.m_pCreateInfo->m_GraphicsPipelineCreateInfo;

        const ImGuiTableFlags tableFlags =
            //ImGuiTableFlags_BordersInnerH |
            ImGuiTableFlags_PadOuterX |
            ImGuiTableFlags_SizingStretchSame;

        const float contentPaddingTop = 2.0f;
        const float contentPaddingLeft = 5.0f;
        const float contentPaddingRight = 10.0f;
        const float contentPaddingBottom = 10.0f;

        const float dynamicColumnWidth = ImGui::CalcTextSize( "Dynamic" ).x + 5;

        auto SetupDefaultPipelineStateColumns = [&]() {
            ImGui::TableSetupColumn( "Name", 0, 1.5f );
            ImGui::TableSetupColumn( "Dynamic", ImGuiTableColumnFlags_WidthFixed, dynamicColumnWidth );
        };

        ImGui::PushStyleColor( ImGuiCol_Header, IM_COL32( 40, 40, 43, 128 ) );

        // VkPipelineVertexInputStateCreateInfo
        ImGui::BeginDisabled( gci.pVertexInputState == nullptr );
        if( ImGui::CollapsingHeader( Lang::PipelineStateVertexInput ) )
        {
            const VkPipelineVertexInputStateCreateInfo& state = *gci.pVertexInputState;

            ImGuiX::BeginPadding( contentPaddingTop, contentPaddingRight, contentPaddingLeft );
            if( ImGui::BeginTable( "##VertexInputState", 6, tableFlags ) )
            {
                ImGui::TableSetupColumn( "Location" );
                ImGui::TableSetupColumn( "Binding" );
                ImGui::TableSetupColumn( "Format", 0, 3.0f );
                ImGui::TableSetupColumn( "Offset" );
                ImGui::TableSetupColumn( "Stride" );
                ImGui::TableSetupColumn( "Input rate", 0, 1.5f );
                ImGuiX::TableHeadersRow( m_Fonts.GetBoldFont() );

                for( uint32_t i = 0; i < state.vertexAttributeDescriptionCount; ++i )
                {
                    const VkVertexInputAttributeDescription* pAttribute = &state.pVertexAttributeDescriptions[ i ];
                    const VkVertexInputBindingDescription* pBinding = nullptr;

                    // Find the binding description of the current attribute.
                    for( uint32_t j = 0; j < state.vertexBindingDescriptionCount; ++j )
                    {
                        if( state.pVertexBindingDescriptions[ j ].binding == pAttribute->binding )
                        {
                            pBinding = &state.pVertexBindingDescriptions[ j ];
                            break;
                        }
                    }

                    ImGui::TableNextRow();
                    ImGuiX::TableTextColumn( "%u", pAttribute->location );
                    ImGuiX::TableTextColumn( "%u", pAttribute->binding );
                    ImGuiX::TableTextColumn( "%s", m_pStringSerializer->GetFormatName( pAttribute->format ).c_str() );
                    ImGuiX::TableTextColumn( "%u", pAttribute->offset );

                    if( pBinding != nullptr )
                    {
                        ImGuiX::TableTextColumn( "%u", pBinding->stride );
                        ImGuiX::TableTextColumn( "%s", m_pStringSerializer->GetVertexInputRateName( pBinding->inputRate ).c_str() );
                    }
                }

                ImGui::EndTable();
            }

            if( state.vertexAttributeDescriptionCount == 0 )
            {
                ImGuiX::BeginPadding( 0, 0, contentPaddingLeft + 4 );
                ImGui::TextUnformatted( "No vertex data on input." );
            }

            ImGuiX::EndPadding( contentPaddingBottom );
        }
        ImGui::EndDisabled();

        // VkPipelineInputAssemblyStateCreateInfo
        ImGui::BeginDisabled( gci.pInputAssemblyState == nullptr );
        if( ImGui::CollapsingHeader( Lang::PipelineStateInputAssembly ) )
        {
            ImGuiX::BeginPadding( contentPaddingTop, contentPaddingRight, contentPaddingLeft );
            if( ImGui::BeginTable( "##InputAssemblyState", 3, tableFlags ) )
            {
                SetupDefaultPipelineStateColumns();

                const VkPipelineInputAssemblyStateCreateInfo& state = *gci.pInputAssemblyState;
                DrawPipelineStateValue( "Topology", "%s", m_pStringSerializer->GetPrimitiveTopologyName( state.topology ).c_str(), gci.pDynamicState, VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY_EXT );
                DrawPipelineStateValue( "Primitive restart", "%s", m_pStringSerializer->GetBool( state.primitiveRestartEnable ).c_str(), gci.pDynamicState, VK_DYNAMIC_STATE_PRIMITIVE_RESTART_ENABLE_EXT );
                ImGui::EndTable();
            }
            ImGuiX::EndPadding( contentPaddingBottom );
        }
        ImGui::EndDisabled();

        // VkPipelineTessellationStateCreateInfo
        ImGui::BeginDisabled( gci.pTessellationState == nullptr );
        if( ImGui::CollapsingHeader( Lang::PipelineStateTessellation ) )
        {
            ImGuiX::BeginPadding( 5, 10, 10 );

            if( ImGui::BeginTable( "##TessellationState", 3, tableFlags ) )
            {
                SetupDefaultPipelineStateColumns();

                const VkPipelineTessellationStateCreateInfo& state = *gci.pTessellationState;
                DrawPipelineStateValue( "Patch control points", "%u", state.patchControlPoints, gci.pDynamicState, VK_DYNAMIC_STATE_PATCH_CONTROL_POINTS_EXT );
                ImGui::EndTable();
            }
            ImGuiX::EndPadding( contentPaddingBottom );
        }
        ImGui::EndDisabled();

        // VkPipelineViewportStateCreateInfo
        ImGui::BeginDisabled( gci.pViewportState == nullptr );
        if( ImGui::CollapsingHeader( Lang::PipelineStateViewport ) )
        {
            const float firstColumnWidth = ImGui::CalcTextSize( "00 (Dynamic)" ).x + 5;

            ImGuiX::BeginPadding( contentPaddingTop, contentPaddingRight, contentPaddingLeft );
            if( ImGui::BeginTable( "##Viewports", 7, tableFlags ) )
            {
                ImGui::TableSetupColumn( "Viewport", ImGuiTableColumnFlags_WidthFixed, firstColumnWidth );
                ImGui::TableSetupColumn( "X" );
                ImGui::TableSetupColumn( "Y" );
                ImGui::TableSetupColumn( "Width" );
                ImGui::TableSetupColumn( "Height" );
                ImGui::TableSetupColumn( "Min Z" );
                ImGui::TableSetupColumn( "Max Z" );
                ImGuiX::TableHeadersRow( m_Fonts.GetBoldFont() );

                const char* format = "%u";
                if( IsPipelineStateDynamic( gci.pDynamicState, VK_DYNAMIC_STATE_VIEWPORT ) )
                {
                    format = "%u (Dynamic)";
                }

                const VkPipelineViewportStateCreateInfo& state = *gci.pViewportState;
                for( uint32_t i = 0; i < state.viewportCount; ++i )
                {
                    ImGui::TableNextRow();
                    ImGuiX::TableTextColumn( format, i );

                    const VkViewport* pViewport = (state.pViewports ? &state.pViewports[i] : nullptr);
                    if( pViewport )
                    {
                        ImGuiX::TableTextColumn( "%.2f", pViewport->x );
                        ImGuiX::TableTextColumn( "%.2f", pViewport->y );
                        ImGuiX::TableTextColumn( "%.2f", pViewport->width );
                        ImGuiX::TableTextColumn( "%.2f", pViewport->height );
                        ImGuiX::TableTextColumn( "%.2f", pViewport->minDepth );
                        ImGuiX::TableTextColumn( "%.2f", pViewport->maxDepth );
                    }
                }

                ImGui::EndTable();
            }
            ImGuiX::EndPadding( contentPaddingBottom );

            ImGuiX::BeginPadding( contentPaddingTop, contentPaddingRight, contentPaddingLeft );
            if( ImGui::BeginTable( "##Scissors", 7, tableFlags ) )
            {
                ImGui::TableSetupColumn( "Scissor", ImGuiTableColumnFlags_WidthFixed, firstColumnWidth );
                ImGui::TableSetupColumn( "X" );
                ImGui::TableSetupColumn( "Y" );
                ImGui::TableSetupColumn( "Width" );
                ImGui::TableSetupColumn( "Height" );
                ImGuiX::TableHeadersRow( m_Fonts.GetBoldFont() );

                const char* format = "%u";
                if( IsPipelineStateDynamic( gci.pDynamicState, VK_DYNAMIC_STATE_SCISSOR ) )
                {
                    format = "%u (Dynamic)";
                }

                const VkPipelineViewportStateCreateInfo& state = *gci.pViewportState;
                for( uint32_t i = 0; i < state.scissorCount; ++i )
                {
                    ImGui::TableNextRow();
                    ImGuiX::TableTextColumn( format, i );

                    const VkRect2D* pScissor = (state.pScissors ? &state.pScissors[i] : nullptr);
                    if( pScissor )
                    {
                        ImGuiX::TableTextColumn( "%u", pScissor->offset.x );
                        ImGuiX::TableTextColumn( "%u", pScissor->offset.y );
                        ImGuiX::TableTextColumn( "%u", pScissor->extent.width );
                        ImGuiX::TableTextColumn( "%u", pScissor->extent.height );
                    }
                }

                ImGui::EndTable();
            }
            ImGuiX::EndPadding( contentPaddingBottom );
        }
        ImGui::EndDisabled();

        // VkPipelineRasterizationStateCreateInfo
        ImGui::BeginDisabled( gci.pRasterizationState == nullptr );
        if( ImGui::CollapsingHeader( Lang::PipelineStateRasterization ) )
        {
            ImGuiX::BeginPadding( contentPaddingTop, contentPaddingRight, contentPaddingLeft );
            if( ImGui::BeginTable( "##RasterizationState", 3, tableFlags ) )
            {
                SetupDefaultPipelineStateColumns();

                const VkPipelineRasterizationStateCreateInfo& state = *gci.pRasterizationState;
                DrawPipelineStateValue( "Depth clamp enable", "%s", m_pStringSerializer->GetBool( state.depthClampEnable ).c_str(), gci.pDynamicState, VK_DYNAMIC_STATE_DEPTH_CLAMP_ENABLE_EXT );
                DrawPipelineStateValue( "Rasterizer discard enable", "%s", m_pStringSerializer->GetBool( state.rasterizerDiscardEnable ).c_str(), gci.pDynamicState, VK_DYNAMIC_STATE_RASTERIZER_DISCARD_ENABLE_EXT );
                DrawPipelineStateValue( "Polygon mode", "%s", m_pStringSerializer->GetPolygonModeName( state.polygonMode ).c_str(), gci.pDynamicState, VK_DYNAMIC_STATE_POLYGON_MODE_EXT );
                DrawPipelineStateValue( "Cull mode", "%s", m_pStringSerializer->GetCullModeName( state.cullMode ).c_str(), gci.pDynamicState, VK_DYNAMIC_STATE_CULL_MODE_EXT );
                DrawPipelineStateValue( "Front face", "%s", m_pStringSerializer->GetFrontFaceName( state.frontFace ).c_str(), gci.pDynamicState, VK_DYNAMIC_STATE_FRONT_FACE_EXT );
                DrawPipelineStateValue( "Depth bias enable", "%s", m_pStringSerializer->GetBool( state.depthBiasEnable ).c_str(), gci.pDynamicState, VK_DYNAMIC_STATE_DEPTH_BIAS_ENABLE_EXT );
                DrawPipelineStateValue( "Depth bias constant factor", "%f", state.depthBiasConstantFactor, gci.pDynamicState, VK_DYNAMIC_STATE_DEPTH_BIAS );
                DrawPipelineStateValue( "Depth bias clamp", "%f", state.depthBiasClamp, gci.pDynamicState, VK_DYNAMIC_STATE_DEPTH_BIAS );
                DrawPipelineStateValue( "Depth bias slope factor", "%f", state.depthBiasSlopeFactor, gci.pDynamicState, VK_DYNAMIC_STATE_DEPTH_BIAS );
                DrawPipelineStateValue( "Line width", "%f", state.lineWidth, gci.pDynamicState, VK_DYNAMIC_STATE_LINE_WIDTH );
                ImGui::EndTable();
            }
            ImGuiX::EndPadding( contentPaddingBottom );
        }
        ImGui::EndDisabled();

        // VkPipelineMultisampleStateCreateInfo
        ImGui::BeginDisabled( gci.pMultisampleState == nullptr );
        if( ImGui::CollapsingHeader( Lang::PipelineStateMultisampling ) )
        {
            ImGuiX::BeginPadding( contentPaddingTop, contentPaddingRight, contentPaddingLeft );
            if( ImGui::BeginTable( "##MultisampleState", 3, tableFlags ) )
            {
                SetupDefaultPipelineStateColumns();

                const VkPipelineMultisampleStateCreateInfo& state = *gci.pMultisampleState;
                DrawPipelineStateValue( "Rasterization samples", "%u", state.rasterizationSamples, gci.pDynamicState, VK_DYNAMIC_STATE_RASTERIZATION_SAMPLES_EXT );
                DrawPipelineStateValue( "Sample shading enable", "%s", m_pStringSerializer->GetBool( state.sampleShadingEnable ).c_str() );
                DrawPipelineStateValue( "Min sample shading", "%u", state.minSampleShading );
                DrawPipelineStateValue( "Sample mask", "0x%08X", state.pSampleMask ? *state.pSampleMask : 0xFFFFFFFF, gci.pDynamicState, VK_DYNAMIC_STATE_SAMPLE_MASK_EXT );
                DrawPipelineStateValue( "Alpha to coverage enable", "%s", m_pStringSerializer->GetBool( state.alphaToCoverageEnable ).c_str(), gci.pDynamicState, VK_DYNAMIC_STATE_ALPHA_TO_COVERAGE_ENABLE_EXT );
                DrawPipelineStateValue( "Alpha to one enable", "%s", m_pStringSerializer->GetBool( state.alphaToOneEnable ).c_str(), gci.pDynamicState, VK_DYNAMIC_STATE_ALPHA_TO_ONE_ENABLE_EXT );
                ImGui::EndTable();
            }
            ImGuiX::EndPadding( contentPaddingBottom );
        }
        ImGui::EndDisabled();

        // VkPipelineDepthStencilStateCreateInfo
        ImGui::BeginDisabled( gci.pDepthStencilState == nullptr );
        if( ImGui::CollapsingHeader( Lang::PipelineStateDepthStencil ) )
        {
            ImGuiX::BeginPadding( contentPaddingTop, contentPaddingRight, contentPaddingLeft );
            if( ImGui::BeginTable( "##DepthStencilState", 3, tableFlags ) )
            {
                SetupDefaultPipelineStateColumns();

                const VkPipelineDepthStencilStateCreateInfo& state = *gci.pDepthStencilState;
                DrawPipelineStateValue( "Depth test enable", "%s", m_pStringSerializer->GetBool( state.depthTestEnable ).c_str(), gci.pDynamicState, VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE_EXT );
                DrawPipelineStateValue( "Depth write enable", "%s", m_pStringSerializer->GetBool( state.depthWriteEnable ).c_str(), gci.pDynamicState, VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE_EXT );
                DrawPipelineStateValue( "Depth compare op", "%s", m_pStringSerializer->GetCompareOpName( state.depthCompareOp ).c_str(), gci.pDynamicState, VK_DYNAMIC_STATE_DEPTH_COMPARE_OP_EXT );
                DrawPipelineStateValue( "Depth bounds test enable", "%s", m_pStringSerializer->GetBool( state.depthBoundsTestEnable ).c_str(), gci.pDynamicState, VK_DYNAMIC_STATE_DEPTH_BOUNDS_TEST_ENABLE_EXT );
                DrawPipelineStateValue( "Min depth bounds", "%f", state.minDepthBounds, gci.pDynamicState, VK_DYNAMIC_STATE_DEPTH_BOUNDS );
                DrawPipelineStateValue( "Max depth bounds", "%f", state.maxDepthBounds, gci.pDynamicState, VK_DYNAMIC_STATE_DEPTH_BOUNDS );
                DrawPipelineStateValue( "Stencil test enable", "%s", m_pStringSerializer->GetBool( state.stencilTestEnable ).c_str(), gci.pDynamicState, VK_DYNAMIC_STATE_STENCIL_TEST_ENABLE_EXT );

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if( ImGui::TreeNodeEx( "Front face stencil op", ImGuiTreeNodeFlags_SpanAllColumns ) )
                {
                    DrawPipelineStateValue( "Fail op", "%u", state.front.failOp );
                    DrawPipelineStateValue( "Pass op", "%u", state.front.passOp );
                    DrawPipelineStateValue( "Depth fail op", "%u", state.front.depthFailOp );
                    DrawPipelineStateValue( "Compare op", "%s", m_pStringSerializer->GetCompareOpName( state.front.compareOp ).c_str() );
                    DrawPipelineStateValue( "Compare mask", "0x%02X", state.front.compareMask, gci.pDynamicState, VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK );
                    DrawPipelineStateValue( "Write mask", "0x%02X", state.front.writeMask, gci.pDynamicState, VK_DYNAMIC_STATE_STENCIL_WRITE_MASK );
                    DrawPipelineStateValue( "Reference", "0x%02X", state.front.reference, gci.pDynamicState, VK_DYNAMIC_STATE_STENCIL_REFERENCE );
                    ImGui::TreePop();
                }

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if( ImGui::TreeNodeEx( "Back face stencil op", ImGuiTreeNodeFlags_SpanAllColumns ) )
                {
                    DrawPipelineStateValue( "Fail op", "%u", state.back.failOp );
                    DrawPipelineStateValue( "Pass op", "%u", state.back.passOp );
                    DrawPipelineStateValue( "Depth fail op", "%u", state.back.depthFailOp );
                    DrawPipelineStateValue( "Compare op", "%s", m_pStringSerializer->GetCompareOpName( state.back.compareOp ).c_str() );
                    DrawPipelineStateValue( "Compare mask", "0x%02X", state.back.compareMask, gci.pDynamicState, VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK );
                    DrawPipelineStateValue( "Write mask", "0x%02X", state.back.writeMask, gci.pDynamicState, VK_DYNAMIC_STATE_STENCIL_WRITE_MASK );
                    DrawPipelineStateValue( "Reference", "0x%02X", state.back.reference, gci.pDynamicState, VK_DYNAMIC_STATE_STENCIL_REFERENCE );
                    ImGui::TreePop();
                }

                ImGui::EndTable();
            }
            ImGuiX::EndPadding( contentPaddingBottom );
        }
        ImGui::EndDisabled();

        // VkPipelineColorBlendStateCreateInfo
        ImGui::BeginDisabled( gci.pColorBlendState == nullptr );
        if( ImGui::CollapsingHeader( Lang::PipelineStateColorBlend ) )
        {
            const VkPipelineColorBlendStateCreateInfo& state = *gci.pColorBlendState;

            ImGuiX::BeginPadding( contentPaddingTop, contentPaddingRight, contentPaddingLeft );
            if( ImGui::BeginTable( "##ColorBlendState", 3, tableFlags ) )
            {
                SetupDefaultPipelineStateColumns();
                DrawPipelineStateValue( "Logic op enable", "%s", m_pStringSerializer->GetBool( state.logicOpEnable ).c_str(), gci.pDynamicState, VK_DYNAMIC_STATE_LOGIC_OP_ENABLE_EXT );
                DrawPipelineStateValue( "Logic op", "%s", m_pStringSerializer->GetLogicOpName( state.logicOp ).c_str(), gci.pDynamicState, VK_DYNAMIC_STATE_LOGIC_OP_EXT );
                DrawPipelineStateValue( "Blend constants", "%s", m_pStringSerializer->GetVec4( state.blendConstants ).c_str(), gci.pDynamicState, VK_DYNAMIC_STATE_BLEND_CONSTANTS );
                ImGui::EndTable();
            }
            ImGuiX::EndPadding( contentPaddingBottom );

            ImGuiX::BeginPadding( contentPaddingTop, contentPaddingRight, contentPaddingLeft );
            if( ImGui::BeginTable( "##ColorBlendAttachments", 9, tableFlags ) )
            {
                const float indexColumnWidth = ImGui::CalcTextSize( "000" ).x + 5;
                const float maskColumnWidth = ImGui::CalcTextSize( "RGBA" ).x + 5;

                ImGui::TableSetupColumn( "#", ImGuiTableColumnFlags_WidthFixed, indexColumnWidth );
                ImGui::TableSetupColumn( "Enable" );
                ImGui::TableSetupColumn( "Src color" );
                ImGui::TableSetupColumn( "Dst color" );
                ImGui::TableSetupColumn( "Color op" );
                ImGui::TableSetupColumn( "Src alpha" );
                ImGui::TableSetupColumn( "Dst alpha" );
                ImGui::TableSetupColumn( "Alpha op" );
                ImGui::TableSetupColumn( "Mask", ImGuiTableColumnFlags_WidthFixed, maskColumnWidth );
                ImGuiX::TableHeadersRow( m_Fonts.GetBoldFont() );

                for( uint32_t i = 0; i < state.attachmentCount; ++i )
                {
                    const VkPipelineColorBlendAttachmentState& attachment = state.pAttachments[ i ];
                    ImGui::TableNextRow();
                    ImGuiX::TableTextColumn( "%u", i );
                    ImGuiX::TableTextColumn( "%s", m_pStringSerializer->GetBool( attachment.blendEnable ).c_str() );
                    ImGuiX::TableTextColumn( "%s", m_pStringSerializer->GetBlendFactorName( attachment.srcColorBlendFactor ).c_str() );
                    ImGuiX::TableTextColumn( "%s", m_pStringSerializer->GetBlendFactorName( attachment.dstColorBlendFactor ).c_str() );
                    ImGuiX::TableTextColumn( "%s", m_pStringSerializer->GetBlendOpName( attachment.colorBlendOp ).c_str() );
                    ImGuiX::TableTextColumn( "%s", m_pStringSerializer->GetBlendFactorName( attachment.dstAlphaBlendFactor ).c_str() );
                    ImGuiX::TableTextColumn( "%s", m_pStringSerializer->GetBlendFactorName( attachment.dstAlphaBlendFactor ).c_str() );
                    ImGuiX::TableTextColumn( "%s", m_pStringSerializer->GetBlendOpName( attachment.alphaBlendOp ).c_str() );
                    ImGuiX::TableTextColumn( "%s", m_pStringSerializer->GetColorComponentFlagNames( attachment.colorWriteMask ).c_str() );
                }

                ImGui::EndTable();
            }

            if( state.attachmentCount == 0 )
            {
                ImGuiX::BeginPadding( 0, 0, contentPaddingLeft + 4 );
                ImGui::TextUnformatted( "No color attachments on output." );
            }

            ImGuiX::EndPadding( contentPaddingBottom );
        }
        ImGui::EndDisabled();

        ImGui::PopStyleColor();
    }

    /***********************************************************************************\

    Function:
        SetInspectorTabIndex

    Description:
        Switches the inspector to another tab.

    \***********************************************************************************/
    void ProfilerOverlayOutput::SetInspectorTabIndex( size_t index )
    {
        const InspectorTab& tab = m_InspectorTabs[index];
        if( tab.Select )
        {
            // Call tab-specific setup callback.
            tab.Select();
        }

        m_InspectorTabIndex = index;
    }

    /***********************************************************************************\

    Function:
        ShaderRepresentationSaved

    Description:
        Called when a shader is saved.

    \***********************************************************************************/
    void ProfilerOverlayOutput::ShaderRepresentationSaved( bool succeeded, const std::string& message )
    {
        m_SerializationSucceeded = succeeded;
        m_SerializationMessage = message;

        // Display message box
        m_SerializationFinishTimestamp = std::chrono::high_resolution_clock::now();
        m_SerializationOutputWindowSize = { 0, 0 };
        m_SerializationWindowVisible = false;
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
            auto PrintStatsDuration = [&]( const DeviceProfilerDrawcallStats::Stats& stats, uint64_t ticks )
                {
                    if( stats.m_TicksSum > 0 )
                    {
                        ImGuiX::TextAlignRight(
                            ImGuiX::TableGetColumnWidth(),
                            "%.2f %s",
                            m_TimestampDisplayUnit * ticks * m_TimestampPeriod.count(),
                            m_pTimestampDisplayUnitStr );
                    }
                    else
                    {
                        ImGuiX::TextAlignRight(
                            ImGuiX::TableGetColumnWidth(),
                            "-" );
                    }
                };

            auto PrintStats = [&]( const char* pName, const DeviceProfilerDrawcallStats::Stats& stats )
                {
                    if( stats.m_Count == 0 && !m_ShowEmptyStatistics )
                    {
                        return;
                    }

                    ImGui::TableNextRow();

                    // Stat name
                    if( ImGui::TableNextColumn() )
                    {
                        ImGui::TextUnformatted( pName );
                    }

                    // Count
                    if( ImGui::TableNextColumn() )
                    {
                        ImGuiX::TextAlignRight(
                            ImGuiX::TableGetColumnWidth(),
                            "%u",
                            stats.m_Count );
                    }

                    // Total duration
                    if( ImGui::TableNextColumn() )
                    {
                        PrintStatsDuration( stats, stats.m_TicksSum );
                    }

                    // Min duration
                    if( ImGui::TableNextColumn() )
                    {
                        PrintStatsDuration( stats, stats.m_TicksMin );
                    }

                    // Max duration
                    if( ImGui::TableNextColumn() )
                    {
                        PrintStatsDuration( stats, stats.m_TicksMax );
                    }

                    // Average duration
                    if( ImGui::TableNextColumn() )
                    {
                        PrintStatsDuration( stats, stats.GetTicksAvg() );
                    }
                };

            ImGui::BeginTable( "##StatisticsTable", 6,
                ImGuiTableFlags_BordersInnerH |
                ImGuiTableFlags_PadOuterX |
                ImGuiTableFlags_Hideable |
                ImGuiTableFlags_ContextMenuInBody |
                ImGuiTableFlags_NoClip |
                ImGuiTableFlags_SizingStretchProp );

            ImGui::TableSetupColumn( Lang::StatName, ImGuiTableColumnFlags_NoHide, 3.0f );
            ImGui::TableSetupColumn( Lang::StatCount, 0, 1.0f );
            ImGui::TableSetupColumn( Lang::StatTotal, 0, 1.0f );
            ImGui::TableSetupColumn( Lang::StatMin, 0, 1.0f );
            ImGui::TableSetupColumn( Lang::StatMax, 0, 1.0f );
            ImGui::TableSetupColumn( Lang::StatAvg, 0, 1.0f );
            ImGui::TableNextRow();

            ImGui::PushFont( m_Fonts.GetBoldFont() );
            ImGui::TableNextColumn();
            ImGui::TextUnformatted( Lang::StatName );
            ImGui::TableNextColumn();
            ImGuiX::TextAlignRight( ImGuiX::TableGetColumnWidth(), Lang::StatCount );
            ImGui::TableNextColumn();
            ImGuiX::TextAlignRight( ImGuiX::TableGetColumnWidth(), Lang::StatTotal );
            ImGui::TableNextColumn();
            ImGuiX::TextAlignRight( ImGuiX::TableGetColumnWidth(), Lang::StatMin );
            ImGui::TableNextColumn();
            ImGuiX::TextAlignRight( ImGuiX::TableGetColumnWidth(), Lang::StatMax );
            ImGui::TableNextColumn();
            ImGuiX::TextAlignRight( ImGuiX::TableGetColumnWidth(), Lang::StatAvg );
            ImGui::PopFont();

            PrintStats( Lang::DrawCalls, m_Data.m_Stats.m_DrawStats );
            PrintStats( Lang::DrawCallsIndirect, m_Data.m_Stats.m_DrawIndirectStats );
            PrintStats( Lang::DrawMeshTasksCalls, m_Data.m_Stats.m_DrawMeshTasksStats );
            PrintStats( Lang::DrawMeshTasksIndirectCalls, m_Data.m_Stats.m_DrawMeshTasksIndirectStats );
            PrintStats( Lang::DispatchCalls, m_Data.m_Stats.m_DispatchStats );
            PrintStats( Lang::DispatchCallsIndirect, m_Data.m_Stats.m_DispatchIndirectStats );
            PrintStats( Lang::TraceRaysCalls, m_Data.m_Stats.m_TraceRaysStats );
            PrintStats( Lang::TraceRaysIndirectCalls, m_Data.m_Stats.m_TraceRaysIndirectStats );
            PrintStats( Lang::CopyBufferCalls, m_Data.m_Stats.m_CopyBufferStats );
            PrintStats( Lang::CopyBufferToImageCalls, m_Data.m_Stats.m_CopyBufferToImageStats );
            PrintStats( Lang::CopyImageCalls, m_Data.m_Stats.m_CopyImageStats );
            PrintStats( Lang::CopyImageToBufferCalls, m_Data.m_Stats.m_CopyImageToBufferStats );
            PrintStats( Lang::PipelineBarriers, m_Data.m_Stats.m_PipelineBarrierStats );
            PrintStats( Lang::ColorClearCalls, m_Data.m_Stats.m_ClearColorStats );
            PrintStats( Lang::DepthStencilClearCalls, m_Data.m_Stats.m_ClearDepthStencilStats );
            PrintStats( Lang::ResolveCalls, m_Data.m_Stats.m_ResolveStats );
            PrintStats( Lang::BlitCalls, m_Data.m_Stats.m_BlitImageStats );
            PrintStats( Lang::FillBufferCalls, m_Data.m_Stats.m_FillBufferStats );
            PrintStats( Lang::UpdateBufferCalls, m_Data.m_Stats.m_UpdateBufferStats );

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            if( m_ShowEmptyStatistics )
            {
                if( ImGui::TextLink( Lang::HideEmptyStatistics ) )
                {
                    m_ShowEmptyStatistics = false;
                }
            }
            else
            {
                if( ImGui::TextLink( Lang::ShowEmptyStatistics ) )
                {
                    m_ShowEmptyStatistics = true;
                }
            }

            ImGui::EndTable();
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
        // Set interface scaling.
        float interfaceScale = ImGui::GetIO().FontGlobalScale;
        if( ImGui::InputFloat( Lang::InterfaceScale, &interfaceScale ) )
        {
            ImGui::GetIO().FontGlobalScale = std::clamp( interfaceScale, 0.25f, 4.0f );
        }

        // Select sampling mode (constant in runtime for now)
        ImGui::BeginDisabled();
        {
            static const char* samplingGroupOptions[] = {
                "Drawcall",
                "Pipeline",
                "Render pass",
                "Command buffer"
            };

            int samplingModeSelectedOption = static_cast<int>(m_SamplingMode);
            if( ImGui::Combo( Lang::SamplingMode, &samplingModeSelectedOption, samplingGroupOptions, 4 ) )
            {
                assert( false );
            }
        }
        ImGui::EndDisabled();

        // Select synchronization mode
        {
            static const char* syncGroupOptions[] = {
                Lang::Present,
                Lang::Submit };

            int syncModeSelectedOption = static_cast<int>(m_SyncMode);
            if( ImGui::Combo( Lang::SyncMode, &syncModeSelectedOption, syncGroupOptions, 2 ) )
            {
                VkProfilerSyncModeEXT syncMode = static_cast<VkProfilerSyncModeEXT>(syncModeSelectedOption);
                VkResult result = vkSetProfilerSyncModeEXT( m_pDevice->Handle, syncMode );
                if( result == VK_SUCCESS )
                {
                    m_SyncMode = syncMode;
                }
            }
        }

        // Select time display unit.
        {
            static const char* timeUnitGroupOptions[] = {
                Lang::Milliseconds,
                Lang::Microseconds,
                Lang::Nanoseconds };

            int timeUnitSelectedOption = static_cast<int>(m_TimeUnit);
            if( ImGui::Combo( Lang::TimeUnit, &timeUnitSelectedOption, timeUnitGroupOptions, 3 ) )
            {
                static float timeUnitFactors[] = {
                    1.0f,
                    1'000.0f,
                    1'000'000.0f
                };

                m_TimeUnit = static_cast<TimeUnit>(timeUnitSelectedOption);
                m_TimestampDisplayUnit = timeUnitFactors[ timeUnitSelectedOption ];
                m_pTimestampDisplayUnitStr = timeUnitGroupOptions[ timeUnitSelectedOption ];
            }
        }

        // Display debug labels in frame browser.
        ImGui::Checkbox( Lang::ShowDebugLabels, &m_ShowDebugLabels );

        // Display shader capability badges in frame browser.
        ImGui::Checkbox( Lang::ShowShaderCapabilities, &m_ShowShaderCapabilities );
    }

    /***********************************************************************************\

    Function:
        GetPerformanceGraphColumns

    Description:
        Enumerate performance graph columns.

    \***********************************************************************************/
    void ProfilerOverlayOutput::GetPerformanceGraphColumns( std::vector<PerformanceGraphColumn>& columns ) const
    {
        FrameBrowserTreeNodeIndex index;
        index.emplace_back( 0 );

        // Enumerate submits batches in frame
        for( const auto& submitBatch : m_Data.m_Submits )
        {
            index.emplace_back( 0 );

            // Enumerate submits in submit batch
            for( const auto& submit : submitBatch.m_Submits )
            {
                index.emplace_back( 0 );

                // Enumerate command buffers in submit
                for( const auto& commandBuffer : submit.m_CommandBuffers )
                {
                    GetPerformanceGraphColumns( commandBuffer, index, columns );
                    index.back()++;
                }

                index.pop_back();
                index.back()++;
            }

            index.pop_back();
            index.back()++;
        }

        index.pop_back();
        assert( index.empty() );
    }

    /***********************************************************************************\

    Function:
        GetPerformanceGraphColumns

    Description:
        Enumerate performance graph columns.

    \***********************************************************************************/
    void ProfilerOverlayOutput::GetPerformanceGraphColumns(
        const DeviceProfilerCommandBufferData& data,
        FrameBrowserTreeNodeIndex& index,
        std::vector<PerformanceGraphColumn>& columns ) const
    {
        index.emplace_back( 0 );

        // Enumerate render passes in command buffer
        for( const auto& renderPass : data.m_RenderPasses )
        {
            GetPerformanceGraphColumns( renderPass, index, columns );
            index.back()++;
        }

        index.pop_back();
    }

    /***********************************************************************************\

    Function:
        GetPerformanceGraphColumns

    Description:
        Enumerate performance graph columns.

    \***********************************************************************************/
    void ProfilerOverlayOutput::GetPerformanceGraphColumns(
        const DeviceProfilerRenderPassData& data,
        FrameBrowserTreeNodeIndex& index,
        std::vector<PerformanceGraphColumn>& columns ) const
    {
        if( (m_HistogramGroupMode <= HistogramGroupMode::eRenderPass) &&
            (data.m_Handle != VK_NULL_HANDLE || data.m_Dynamic) )
        {
            const float cycleCount = static_cast<float>(GetDuration( data ));

            PerformanceGraphColumn column = {};
            column.x = cycleCount;
            column.y = cycleCount;
            column.color = m_RenderPassColumnColor;
            column.userData = &data;
            column.groupMode = HistogramGroupMode::eRenderPass;
            column.nodeIndex = index;

            // Insert render pass cycle count to histogram
            columns.push_back( column );
        }
        else
        {
            index.emplace_back( 0 );
            if( data.HasBeginCommand() )
                index.back()++;

            // Enumerate subpasses in render pass
            for( const auto& subpass : data.m_Subpasses )
            {
                index.emplace_back( 0 );

                // Treat data as pipelines if subpass contents are inline-only.
                if( subpass.m_Contents == VK_SUBPASS_CONTENTS_INLINE )
                {
                    for( const auto& data : subpass.m_Data )
                    {
                        GetPerformanceGraphColumns( std::get<DeviceProfilerPipelineData>( data ), index, columns );
                        index.back()++;
                    }
                }

                // Treat data as secondary command buffers if subpass contents are secondary command buffers only.
                else if( subpass.m_Contents == VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS )
                {
                    for( const auto& data : subpass.m_Data )
                    {
                        GetPerformanceGraphColumns( std::get<DeviceProfilerCommandBufferData>( data ), index, columns );
                        index.back()++;
                    }
                }

                // With VK_EXT_nested_command_buffer, it is possible to insert both command buffers and inline commands in the same subpass.
                else if( subpass.m_Contents == VK_SUBPASS_CONTENTS_INLINE_AND_SECONDARY_COMMAND_BUFFERS_EXT )
                {
                    for( const auto& data : subpass.m_Data )
                    {
                        switch( data.GetType() )
                        {
                        case DeviceProfilerSubpassDataType::ePipeline:
                            GetPerformanceGraphColumns( std::get<DeviceProfilerPipelineData>( data ), index, columns );
                            break;

                        case DeviceProfilerSubpassDataType::eCommandBuffer:
                            GetPerformanceGraphColumns( std::get<DeviceProfilerCommandBufferData>( data ), index, columns );
                            break;
                        }
                        index.back()++;
                    }
                }

                index.pop_back();
                index.back()++;
            }

            index.pop_back();
        }
    }

    /***********************************************************************************\

    Function:
        GetPerformanceGraphColumns

    Description:
        Enumerate performance graph columns.

    \***********************************************************************************/
    void ProfilerOverlayOutput::GetPerformanceGraphColumns(
        const DeviceProfilerPipelineData& data,
        FrameBrowserTreeNodeIndex& index,
        std::vector<PerformanceGraphColumn>& columns ) const
    {
        if( (m_HistogramGroupMode <= HistogramGroupMode::ePipeline) &&
            ((data.m_ShaderTuple.m_Hash & 0xFFFF) != 0) &&
            (data.m_Handle != VK_NULL_HANDLE) )
        {
            const float cycleCount = static_cast<float>(GetDuration( data ));

            PerformanceGraphColumn column = {};
            column.x = cycleCount;
            column.y = cycleCount;
            column.userData = &data;
            column.groupMode = HistogramGroupMode::ePipeline;
            column.nodeIndex = index;

            switch( data.m_BindPoint )
            {
            case VK_PIPELINE_BIND_POINT_GRAPHICS:
                column.color = m_GraphicsPipelineColumnColor;
                break;

            case VK_PIPELINE_BIND_POINT_COMPUTE:
                column.color = m_ComputePipelineColumnColor;
                break;

            case VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR:
                column.color = m_RayTracingPipelineColumnColor;
                break;

            default:
                assert( !"Unsupported pipeline type" );
                break;
            }

            // Insert pipeline cycle count to histogram
            columns.push_back( column );
        }
        else
        {
            index.emplace_back( 0 );

            // Enumerate drawcalls in pipeline
            for( const auto& drawcall : data.m_Drawcalls )
            {
                GetPerformanceGraphColumns( drawcall, index, columns );
                index.back()++;
            }

            index.pop_back();
        }
    }

    /***********************************************************************************\

    Function:
        GetPerformanceGraphColumns

    Description:
        Enumerate performance graph columns.

    \***********************************************************************************/
    void ProfilerOverlayOutput::GetPerformanceGraphColumns(
        const DeviceProfilerDrawcall& data,
        FrameBrowserTreeNodeIndex& index,
        std::vector<PerformanceGraphColumn>& columns ) const
    {
        const float cycleCount = static_cast<float>(GetDuration( data ));

        PerformanceGraphColumn column = {};
        column.x = cycleCount;
        column.y = cycleCount;
        column.userData = &data;
        column.groupMode = HistogramGroupMode::eDrawcall;
        column.nodeIndex = index;

        switch( data.GetPipelineType() )
        {
        case DeviceProfilerPipelineType::eGraphics:
            column.color = m_GraphicsPipelineColumnColor;
            break;

        case DeviceProfilerPipelineType::eCompute:
            column.color = m_ComputePipelineColumnColor;
            break;

        default:
            column.color = m_InternalPipelineColumnColor;
            break;
        }

        // Insert drawcall cycle count to histogram
        columns.push_back( column );
    }

    /***********************************************************************************\

    Function:
        DrawPerformanceGraphLabel

    Description:
        Draw label for hovered column.

    \***********************************************************************************/
    void ProfilerOverlayOutput::DrawPerformanceGraphLabel( const ImGuiX::HistogramColumnData& data_ )
    {
        const PerformanceGraphColumn& data = reinterpret_cast<const PerformanceGraphColumn&>(data_);

        std::string regionName = "";
        uint64_t regionCycleCount = 0;

        switch( data.groupMode )
        {
        case HistogramGroupMode::eRenderPass:
        {
            const DeviceProfilerRenderPassData& renderPassData =
                *reinterpret_cast<const DeviceProfilerRenderPassData*>(data.userData);

            regionName = m_pStringSerializer->GetName( renderPassData );
            regionCycleCount = GetDuration( renderPassData );
            break;
        }

        case HistogramGroupMode::ePipeline:
        {
            const DeviceProfilerPipelineData& pipelineData =
                *reinterpret_cast<const DeviceProfilerPipelineData*>(data.userData);

            regionName = m_pStringSerializer->GetName( pipelineData );
            regionCycleCount = GetDuration( pipelineData );
            break;
        }

        case HistogramGroupMode::eDrawcall:
        {
            const DeviceProfilerDrawcall& pipelineData =
                *reinterpret_cast<const DeviceProfilerDrawcall*>(data.userData);

            regionName = m_pStringSerializer->GetName( pipelineData );
            regionCycleCount = GetDuration( pipelineData );
            break;
        }
        }

        ImGui::SetTooltip( "%s\n%.2f ms",
            regionName.c_str(),
            regionCycleCount * m_TimestampPeriod.count() );
    }

    /***********************************************************************************\

    Function:
        SelectPerformanceGraphColumn

    Description:
        Scroll frame browser to node selected in performance graph.

    \***********************************************************************************/
    void ProfilerOverlayOutput::SelectPerformanceGraphColumn( const ImGuiX::HistogramColumnData& data_ )
    {
        const PerformanceGraphColumn& data = reinterpret_cast<const PerformanceGraphColumn&>(data_);

        m_SelectedFrameBrowserNodeIndex = data.nodeIndex;
        m_ScrollToSelectedFrameBrowserNode = true;

        m_SelectionUpdateTimestamp = std::chrono::high_resolution_clock::now();
    }

    /***********************************************************************************\

    Function:
        ScrollToSelectedFrameBrowserNode

    Description:
        Checks if the frame browser should scroll to the node at the given index
        (or its child).

    \***********************************************************************************/
    bool ProfilerOverlayOutput::ScrollToSelectedFrameBrowserNode( const FrameBrowserTreeNodeIndex& index ) const
    {
        if( !m_ScrollToSelectedFrameBrowserNode )
        {
            return false;
        }

        if( m_SelectedFrameBrowserNodeIndex.size() < index.size() )
        {
            return false;
        }

        return memcmp( m_SelectedFrameBrowserNodeIndex.data(),
            index.data(),
            index.size() * sizeof( FrameBrowserTreeNodeIndex::value_type ) ) == 0;
    }

    /***********************************************************************************\

    Function:
        GetFrameBrowserNodeIndexStr

    Description:
        Returns pointer to a string representation of the index.
        The pointer remains valid until the next call to this function.

    \***********************************************************************************/
    const char* ProfilerOverlayOutput::GetFrameBrowserNodeIndexStr( const FrameBrowserTreeNodeIndex& index )
    {
        // Allocate size for the string.
        m_FrameBrowserNodeIndexStr.resize(
            (index.size() * sizeof( FrameBrowserTreeNodeIndex::value_type ) * 2) + 1 );

        ProfilerStringFunctions::Hex(
            m_FrameBrowserNodeIndexStr.data(),
            index.data(),
            index.size() );

        return m_FrameBrowserNodeIndexStr.data();
    }

    /***********************************************************************************\

    Function:
        SaveTrace

    Description:
        Saves current frame trace to a file.

    \***********************************************************************************/
    void ProfilerOverlayOutput::SaveTrace()
    {
        DeviceProfilerTraceSerializer serializer( m_pStringSerializer, m_TimestampPeriod );
        DeviceProfilerTraceSerializationResult result = serializer.Serialize( m_Data );

        m_SerializationSucceeded = result.m_Succeeded;
        m_SerializationMessage = result.m_Message;

        // Display message box
        m_SerializationFinishTimestamp = std::chrono::high_resolution_clock::now();
        m_SerializationOutputWindowSize = { 0, 0 };
        m_SerializationWindowVisible = false;
    }

    /***********************************************************************************\

    Function:
        DrawTraceSerializationOutputWindow

    Description:
        Display window with serialization output.

    \***********************************************************************************/
    void ProfilerOverlayOutput::DrawTraceSerializationOutputWindow()
    {
        using namespace std::chrono;
        using namespace std::chrono_literals;

        const auto& now = std::chrono::high_resolution_clock::now();

        if( (now - m_SerializationFinishTimestamp) < 4s )
        {
            const ImVec2 windowPos = {
                static_cast<float>(m_RenderArea.width - m_SerializationOutputWindowSize.width),
                static_cast<float>(m_RenderArea.height - m_SerializationOutputWindowSize.height) };

            const float fadeOutStep =
                1.f - std::max( 0.f, std::min( 1.f,
                                         duration_cast<milliseconds>(now - (m_SerializationFinishTimestamp + 3s)).count() / 1000.f ) );

            ImGui::PushStyleVar( ImGuiStyleVar_Alpha, fadeOutStep );

            if( !m_SerializationSucceeded )
            {
                ImGui::PushStyleColor( ImGuiCol_WindowBg, { 1, 0, 0, 1 } );
            }

            ImGui::SetNextWindowPos( windowPos );
            ImGui::Begin( "Trace Export", nullptr,
                ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoTitleBar |
                    ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoDocking |
                    ImGuiWindowFlags_NoFocusOnAppearing |
                    ImGuiWindowFlags_NoSavedSettings |
                    ImGuiWindowFlags_AlwaysAutoResize );

            ImGui::Text( "%s", m_SerializationMessage.c_str() );

            // Save final size of the window
            if ( m_SerializationWindowVisible &&
                (m_SerializationOutputWindowSize.width == 0) )
            {
                const ImVec2 windowSize = ImGui::GetWindowSize();
                m_SerializationOutputWindowSize.width = static_cast<uint32_t>(windowSize.x);
                m_SerializationOutputWindowSize.height = static_cast<uint32_t>(windowSize.y);
            }

            ImGui::End();
            ImGui::PopStyleVar();

            if( !m_SerializationSucceeded )
            {
                ImGui::PopStyleColor();
            }

            m_SerializationWindowVisible = true;
        }
    }

    /***********************************************************************************\

    Function:
        PrintCommandBuffer

    Description:
        Writes command buffer data to the overlay.

    \***********************************************************************************/
    void ProfilerOverlayOutput::PrintCommandBuffer( const DeviceProfilerCommandBufferData& cmdBuffer, FrameBrowserTreeNodeIndex& index )
    {
        const uint64_t commandBufferTicks = GetDuration( cmdBuffer );

        // Mark hotspots with color
        DrawSignificanceRect( (float)commandBufferTicks / m_Data.m_Ticks, index );

        if( ScrollToSelectedFrameBrowserNode( index ) )
        {
            // Tree contains selected node
            ImGui::SetNextItemOpen( true );
            ImGui::SetScrollHereY();
        }

        const char* indexStr = GetFrameBrowserNodeIndexStr( index );
        if( ImGui::TreeNode( indexStr, "%s", m_pStringSerializer->GetName( cmdBuffer.m_Handle ).c_str() ) )
        {
            // Command buffer opened
            PrintDuration( cmdBuffer );

            // Sort frame browser data
            std::list<const DeviceProfilerRenderPassData*> pRenderPasses =
                SortFrameBrowserData( cmdBuffer.m_RenderPasses );

            index.emplace_back( 0 );

            // Enumerate render passes in command buffer
            for( const DeviceProfilerRenderPassData* pRenderPass : pRenderPasses )
            {
                PrintRenderPass( *pRenderPass, index );
                index.back()++;
            }

            index.pop_back();
            ImGui::TreePop();
        }
        else
        {
            // Command buffer collapsed
            PrintDuration( cmdBuffer );
        }
    }

    /***********************************************************************************\

    Function:
        PrintRenderPassCommand

    Description:
        Writes render pass command data to the overlay.
        Render pass commands include vkCmdBeginRenderPass, vkCmdEndRenderPass, as well as
        dynamic rendering counterparts: vkCmdBeginRendering, etc.

    \***********************************************************************************/
    template<typename Data>
    void ProfilerOverlayOutput::PrintRenderPassCommand( const Data& data, bool dynamic, FrameBrowserTreeNodeIndex& index, uint32_t drawcallIndex )
    {
        const uint64_t commandTicks = GetDuration( data );

        index.emplace_back( drawcallIndex );

        if( (m_ScrollToSelectedFrameBrowserNode) &&
            (m_SelectedFrameBrowserNodeIndex == index) )
        {
            ImGui::SetScrollHereY();
        }

        // Mark hotspots with color
        DrawSignificanceRect( (float)commandTicks / m_Data.m_Ticks, index );

        index.pop_back();

        // Print command's name
        ImGui::TextUnformatted( m_pStringSerializer->GetName( data, dynamic ).c_str() );

        PrintDuration( data );
    }

    /***********************************************************************************\

    Function:
        PrintRenderPass

    Description:
        Writes render pass data to the overlay.

    \***********************************************************************************/
    void ProfilerOverlayOutput::PrintRenderPass( const DeviceProfilerRenderPassData& renderPass, FrameBrowserTreeNodeIndex& index )
    {
        const bool isValidRenderPass = (renderPass.m_Type != DeviceProfilerRenderPassType::eNone);

        if( isValidRenderPass )
        {
            const uint64_t renderPassTicks = GetDuration( renderPass );

            // Mark hotspots with color
            DrawSignificanceRect( (float)renderPassTicks / m_Data.m_Ticks, index );
        }

        // At least one subpass must be present
        assert( !renderPass.m_Subpasses.empty() );

        if( ScrollToSelectedFrameBrowserNode( index ) )
        {
            // Tree contains selected node
            ImGui::SetNextItemOpen( true );
            ImGui::SetScrollHereY();
        }

        bool inRenderPassSubtree;
        if( isValidRenderPass )
        {
            const char* indexStr = GetFrameBrowserNodeIndexStr( index );
            inRenderPassSubtree = ImGui::TreeNode( indexStr, "%s",
                m_pStringSerializer->GetName( renderPass ).c_str() );
        }
        else
        {
            // Print render pass inline.
            inRenderPassSubtree = true;
        }

        if( inRenderPassSubtree )
        {
            index.emplace_back( 0 );

            // Render pass subtree opened
            if( isValidRenderPass )
            {
                PrintDuration( renderPass );

                if( renderPass.HasBeginCommand() )
                {
                    PrintRenderPassCommand( renderPass.m_Begin, renderPass.m_Dynamic, index, 0 );
                    index.back()++;
                }
            }

            // Sort frame browser data
            std::list<const DeviceProfilerSubpassData*> pSubpasses =
                SortFrameBrowserData( renderPass.m_Subpasses );

            // Enumerate subpasses
            for( const DeviceProfilerSubpassData* pSubpass : pSubpasses )
            {
                PrintSubpass( *pSubpass, index, (pSubpasses.size() == 1) );
                index.back()++;
            }

            if( isValidRenderPass )
            {
                if( renderPass.HasEndCommand() )
                {
                    PrintRenderPassCommand( renderPass.m_End, renderPass.m_Dynamic, index, 1 );
                }

                ImGui::TreePop();
            }

            index.pop_back();
        }

        if( isValidRenderPass && !inRenderPassSubtree )
        {
            // Render pass collapsed
            PrintDuration( renderPass );
        }
    }

    /***********************************************************************************\

    Function:
        PrintSubpass

    Description:
        Writes subpass data to the overlay.

    \***********************************************************************************/
    void ProfilerOverlayOutput::PrintSubpass( const DeviceProfilerSubpassData& subpass, FrameBrowserTreeNodeIndex& index, bool isOnlySubpass )
    {
        const uint64_t subpassTicks = GetDuration( subpass );
        bool inSubpassSubtree = false;

        if( !isOnlySubpass )
        {
            // Mark hotspots with color
            DrawSignificanceRect( (float)subpassTicks / m_Data.m_Ticks, index );

            if( ScrollToSelectedFrameBrowserNode( index ) )
            {
                // Tree contains selected node
                ImGui::SetNextItemOpen( true );
                ImGui::SetScrollHereY();
            }

            const char* indexStr = GetFrameBrowserNodeIndexStr( index );
            inSubpassSubtree =
                (subpass.m_Index != -1) &&
                (ImGui::TreeNode( indexStr, "Subpass #%u", subpass.m_Index ));
        }

        if( inSubpassSubtree )
        {
            // Subpass subtree opened
            PrintDuration( subpass );
        }

        if( inSubpassSubtree ||
            isOnlySubpass ||
            (subpass.m_Index == -1) )
        {
            index.emplace_back( 0 );

            // Treat data as pipelines if subpass contents are inline-only.
            if( subpass.m_Contents == VK_SUBPASS_CONTENTS_INLINE )
            {
                // Sort frame browser data
                std::list<const DeviceProfilerSubpassData::Data*> pDataSorted =
                    SortFrameBrowserData<DeviceProfilerPipelineData>( subpass.m_Data );

                for( const DeviceProfilerSubpassData::Data* pData : pDataSorted )
                {
                    PrintPipeline( std::get<DeviceProfilerPipelineData>( *pData ), index );
                    index.back()++;
                }
            }

            // Treat data as secondary command buffers if subpass contents are secondary command buffers only.
            else if( subpass.m_Contents == VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS )
            {
                // Sort frame browser data
                std::list<const DeviceProfilerSubpassData::Data*> pDataSorted =
                    SortFrameBrowserData<DeviceProfilerCommandBufferData>( subpass.m_Data );

                for( const DeviceProfilerSubpassData::Data* pData : pDataSorted )
                {
                    PrintCommandBuffer( std::get<DeviceProfilerCommandBufferData>( *pData ), index );
                    index.back()++;
                }
            }

            // With VK_EXT_nested_command_buffer, it is possible to insert both command buffers and inline commands in the same subpass.
            else if( subpass.m_Contents == VK_SUBPASS_CONTENTS_INLINE_AND_SECONDARY_COMMAND_BUFFERS_EXT )
            {
                // Sort frame browser data
                std::list<const DeviceProfilerSubpassData::Data*> pDataSorted =
                    SortFrameBrowserData( subpass.m_Data );

                for( const DeviceProfilerSubpassData::Data* pData : pDataSorted )
                {
                    switch( pData->GetType() )
                    {
                    case DeviceProfilerSubpassDataType::ePipeline:
                        PrintPipeline( std::get<DeviceProfilerPipelineData>( *pData ), index );
                        break;

                    case DeviceProfilerSubpassDataType::eCommandBuffer:
                        PrintCommandBuffer( std::get<DeviceProfilerCommandBufferData>( *pData ), index );
                        break;
                    }
                    index.back()++;
                }
            }

            index.pop_back();
        }

        if( inSubpassSubtree )
        {
            // Finish subpass tree
            ImGui::TreePop();
        }

        if( !inSubpassSubtree && !isOnlySubpass && (subpass.m_Index != -1) )
        {
            // Subpass collapsed
            PrintDuration( subpass );
        }
    }

    /***********************************************************************************\

    Function:
        PrintPipeline

    Description:
        Writes pipeline data to the overlay.

    \***********************************************************************************/
    void ProfilerOverlayOutput::PrintPipeline( const DeviceProfilerPipelineData& pipeline, FrameBrowserTreeNodeIndex& index )
    {
        const uint64_t pipelineTicks = GetDuration( pipeline );

        const bool printPipelineInline =
            ((pipeline.m_Handle == VK_NULL_HANDLE) &&
                !pipeline.m_UsesShaderObjects) ||
            ((pipeline.m_ShaderTuple.m_Hash & 0xFFFF) == 0);

        bool inPipelineSubtree = false;

        if( !printPipelineInline )
        {
            // Mark hotspots with color
            DrawSignificanceRect( (float)pipelineTicks / m_Data.m_Ticks, index );

            if( ScrollToSelectedFrameBrowserNode( index ) )
            {
                // Tree contains selected node
                ImGui::SetNextItemOpen( true );
                ImGui::SetScrollHereY();
            }

            const char* indexStr = GetFrameBrowserNodeIndexStr( index );
            inPipelineSubtree =
                (ImGui::TreeNode( indexStr, "%s", m_pStringSerializer->GetName( pipeline ).c_str() ));

            if( ImGui::BeginPopupContextItem() )
            {
                if( ImGui::MenuItem( Lang::Inspect, nullptr, nullptr ) )
                {
                    Inspect( pipeline );
                }

                ImGui::EndPopup();
            }
        }

        if( m_ShowShaderCapabilities )
        {
            if( pipeline.m_UsesShaderObjects )
            {
                static ImU32 shaderObjectsColor = IM_COL32( 104, 25, 133, 255 );
                DrawBadge( shaderObjectsColor, "SO", Lang::ShaderObjectsTooltip );
            }
            if( pipeline.m_UsesRayQuery )
            {
                static ImU32 rayQueryCapabilityColor = IM_COL32( 133, 82, 25, 255 );
                DrawBadge( rayQueryCapabilityColor, "RQ", Lang::ShaderCapabilityTooltipFmt, "Ray Query" );
            }
            if( pipeline.m_UsesRayTracing )
            {
                static ImU32 rayTracingCapabilityColor = IM_COL32( 25, 110, 133, 255 );
                DrawBadge( rayTracingCapabilityColor, "RT", Lang::ShaderCapabilityTooltipFmt, "Ray Tracing" );
            }
        }

        if( inPipelineSubtree )
        {
            // Pipeline subtree opened
            PrintDuration( pipeline );
        }

        if( inPipelineSubtree || printPipelineInline )
        {
            // Sort frame browser data
            std::list<const DeviceProfilerDrawcall*> pDrawcalls =
                SortFrameBrowserData( pipeline.m_Drawcalls );

            index.emplace_back( 0 );

            // Enumerate drawcalls in pipeline
            for( const DeviceProfilerDrawcall* pDrawcall : pDrawcalls )
            {
                PrintDrawcall( *pDrawcall, index );
                index.back()++;
            }

            index.pop_back();
        }

        if( inPipelineSubtree )
        {
            // Finish pipeline subtree
            ImGui::TreePop();
        }

        if( !inPipelineSubtree && !printPipelineInline )
        {
            // Pipeline collapsed
            PrintDuration( pipeline );
        }
    }

    /***********************************************************************************\

    Function:
        PrintDrawcall

    Description:
        Writes drawcall data to the overlay.

    \***********************************************************************************/
    void ProfilerOverlayOutput::PrintDrawcall( const DeviceProfilerDrawcall& drawcall, FrameBrowserTreeNodeIndex& index )
    {
        if( drawcall.GetPipelineType() != DeviceProfilerPipelineType::eDebug )
        {
            const uint64_t drawcallTicks = GetDuration( drawcall );

            if( ScrollToSelectedFrameBrowserNode( index ) )
            {
                ImGui::SetScrollHereY();
            }

            // Mark hotspots with color
            DrawSignificanceRect( (float)drawcallTicks / m_Data.m_Ticks, index );

            const std::string drawcallString = m_pStringSerializer->GetName( drawcall );
            ImGui::TextUnformatted( drawcallString.c_str() );

            PrintDuration( drawcall );
        }
        else
        {
            // Draw debug label
            PrintDebugLabel( drawcall.m_Payload.m_DebugLabel.m_pName, drawcall.m_Payload.m_DebugLabel.m_Color );
        }
    }

    /***********************************************************************************\

    Function:
        DrawSignificanceRect

    Description:

    \***********************************************************************************/
    void ProfilerOverlayOutput::DrawSignificanceRect( float significance, const FrameBrowserTreeNodeIndex& index )
    {
        ImVec2 cursorPosition = ImGui::GetCursorScreenPos();
        ImVec2 rectSize;

        cursorPosition.x = ImGui::GetWindowPos().x;

        rectSize.x = cursorPosition.x + ImGui::GetWindowSize().x;
        rectSize.y = cursorPosition.y + ImGui::GetTextLineHeight();

        ImU32 color = ImGui::GetColorU32( { 1, 0, 0, significance } );

        if( index == m_SelectedFrameBrowserNodeIndex )
        {
            using namespace std::chrono;
            using namespace std::chrono_literals;

            // Node is selected
            ImU32 selectionColor = ImGui::GetColorU32( ImGuiCol_TabHovered );

            // Interpolate color
            auto now = std::chrono::high_resolution_clock::now();
            float step = std::max( 0.0f, std::min( 1.0f,
                                             duration_cast<Milliseconds>((now - m_SelectionUpdateTimestamp) - 0.3s).count() / 1000.0f ) );

            // Linear interpolation
            color = ImGuiX::ColorLerp( selectionColor, color, step );
        }

        ImDrawList* pDrawList = ImGui::GetWindowDrawList();
        pDrawList->AddRectFilled( cursorPosition, rectSize, color );
    }

    /***********************************************************************************\

    Function:
        DrawSignificanceRect

    Description:

    \***********************************************************************************/
    void ProfilerOverlayOutput::DrawBadge( uint32_t color, const char* shortName, const char* fmt, ... )
    {
        assert( m_ShowShaderCapabilities );

        ImGui::SameLine();
        ImGuiX::BadgeUnformatted( color, 5.f, shortName );

        if (ImGui::IsItemHovered())
        {
            va_list args;
            va_start( args, fmt );

            ImGui::BeginTooltip();
            ImGui::TextV( fmt, args );
            ImGui::EndTooltip();

            va_end( args );
        }
    }

    /***********************************************************************************\

    Function:
        DrawDebugLabel

    Description:

    \***********************************************************************************/
    void ProfilerOverlayOutput::PrintDebugLabel( const char* pName, const float pColor[ 4 ] )
    {
        if( !(m_ShowDebugLabels) ||
            (m_FrameBrowserSortMode != FrameBrowserSortMode::eSubmissionOrder) ||
            !(pName) )
        {
            // Don't print debug labels if frame browser is sorted out of submission order
            return;
        }

        ImVec2 cursorPosition = ImGui::GetCursorScreenPos();
        ImVec2 rectSize;

        rectSize.x = cursorPosition.x + 8;
        rectSize.y = cursorPosition.y + ImGui::GetTextLineHeight();

        // Resolve debug label color
        ImU32 color = ImGui::GetColorU32( *reinterpret_cast<const ImVec4*>(pColor) );

        ImDrawList* pDrawList = ImGui::GetWindowDrawList();
        pDrawList->AddRectFilled( cursorPosition, rectSize, color );
        pDrawList->AddRect( cursorPosition, rectSize, ImGui::GetColorU32( ImGuiCol_Border ) );

        cursorPosition.x += 12;
        ImGui::SetCursorScreenPos( cursorPosition );

        ImGui::TextUnformatted( pName );
    }

    /***********************************************************************************\

    Function:
        PrintDuration

    Description:

    \***********************************************************************************/
    template <typename Data>
    void ProfilerOverlayOutput::PrintDuration( const Data& data )
    {
        if( ( data.m_BeginTimestamp.m_Value != UINT64_MAX ) && ( data.m_EndTimestamp.m_Value != UINT64_MAX ) )
        {
            const uint64_t ticks = GetDuration( data );

            // Print the duration
            ImGuiX::TextAlignRight( "%.2f %s",
                m_TimestampDisplayUnit * ticks * m_TimestampPeriod.count(),
                m_pTimestampDisplayUnitStr );
        }
        else
        {
            // No data collected in this mode
            ImGuiX::TextAlignRight( "- %s",
                m_pTimestampDisplayUnitStr );
        }
    }
}

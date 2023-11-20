// Copyright (c) 2019-2023 Lukasz Stalmirski
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
#include "profiler_trace/profiler_trace.h"
#include "profiler_helpers/profiler_data_helpers.h"

#include "imgui_impl_vulkan_layer.h"
#include <string>
#include <sstream>
#include <stack>
#include <fstream>
#include <regex>

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
#ifdef WIN32
#include <ShlObj.h> // SHGetKnownFolderPath
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
    std::mutex ProfilerOverlayOutput::s_ImGuiMutex;

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
        , m_pImGuiDefaultFont(nullptr)
        , m_pImGuiCodeFont(nullptr)
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
        , m_SelectedFrameBrowserNodeIndex( { 0xFFFF } )
        , m_ScrollToSelectedFrameBrowserNode( false )
        , m_SelectionUpdateTimestamp( std::chrono::high_resolution_clock::duration::zero() )
        , m_SerializationFinishTimestamp( std::chrono::high_resolution_clock::duration::zero() )
        , m_PerformanceQueryCommandBufferFilter( VK_NULL_HANDLE )
        , m_PerformanceQueryCommandBufferFilterName( "Frame" )
        , m_SerializationSucceeded( false )
        , m_SerializationMessage()
        , m_SerializationOutputWindowSize( { 0, 0 } )
        , m_SerializationOutputWindowDuration( std::chrono::seconds( 4 ) )
        , m_SerializationOutputWindowFadeOutDuration( std::chrono::seconds( 1 ) )
        , m_RenderPassColumnColor( 0 )
        , m_GraphicsPipelineColumnColor( 0 )
        , m_ComputePipelineColumnColor( 0 )
        , m_RayTracingPipelineColumnColor( 0 )
        , m_InternalPipelineColumnColor( 0 )
        , m_PipelineInspectorTabOpen( false )
        , m_pStringSerializer( nullptr )
        , m_pSelectedPipeline( nullptr )
        , m_pSelectedPipelineShaderStageNames( 0 )
        , m_pSelectedPipelineShaderStageInspectors( 0 )
        , m_SelectedPipelineShaderStageIndex( 0 )
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

            ImGuiIO& io = ImGui::GetIO();
            io.DisplaySize = { (float)m_RenderArea.width, (float)m_RenderArea.height };
            io.DeltaTime = 1.0f / 60.0f;
            io.IniFilename = "VK_LAYER_profiler_imgui.ini";
            io.ConfigFlags = ImGuiConfigFlags_None;

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

        // Initialize serializer
        if( result == VK_SUCCESS )
        {
            result = (m_pStringSerializer = new (std::nothrow) DeviceProfilerStringSerializer( device ))
                         ? VK_SUCCESS
                         : VK_ERROR_OUT_OF_HOST_MEMORY;
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
            m_pImGuiVulkanContext->RenderDrawData( ImGui::GetDrawData(), commandBuffer );

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
        std::scoped_lock lk( s_ImGuiMutex );
        ImGui::SetCurrentContext( m_pImGuiContext );

        m_pImGuiVulkanContext->NewFrame();

        m_pImGuiWindowContext->NewFrame();

        ImGui::NewFrame();
        ImGui::Begin( Lang::WindowName );

        // Update input clipping rect
        m_pImGuiWindowContext->UpdateWindowRect();

        // GPU properties
        ImGui::Text( "%s: %s", Lang::Device, m_pDevice->pPhysicalDevice->Properties.deviceName );

        ImGuiX::TextAlignRight( "Vulkan %u.%u",
            VK_VERSION_MAJOR( m_pDevice->pInstance->ApplicationInfo.apiVersion ),
            VK_VERSION_MINOR( m_pDevice->pInstance->ApplicationInfo.apiVersion ) );

        // Save results to file
        if( ImGui::Button( Lang::Save ) )
        {
            DeviceProfilerTraceSerializer serializer( m_pStringSerializer, m_TimestampPeriod );
            DeviceProfilerTraceSerializationResult result = serializer.Serialize( data );

            m_SerializationSucceeded = result.m_Succeeded;
            m_SerializationMessage = result.m_Message;

            // Display message box
            m_SerializationFinishTimestamp = std::chrono::high_resolution_clock::now();
            m_SerializationOutputWindowSize = { 0, 0 };
            m_SerializationWindowVisible = false;
        }

        // Keep results
        ImGui::SameLine();
        ImGui::Checkbox( Lang::Pause, &m_Pause );

        if( !m_Pause )
        {
            // Update data
            m_Data = data;
        }

        ImGui::BeginTabBar( "##tabs" );

        if( ImGui::BeginTabItem( Lang::Performance ) )
        {
            UpdatePerformanceTab();
            ImGui::EndTabItem();
        }
        if( ImGui::BeginTabItem( Lang::Memory ) )
        {
            UpdateMemoryTab();
            ImGui::EndTabItem();
        }
        if( ImGui::BeginTabItem( Lang::Statistics ) )
        {
            UpdateStatisticsTab();
            ImGui::EndTabItem();
        }
        if( ImGui::BeginTabItem( Lang::Settings ) )
        {
            UpdateSettingsTab();
            ImGui::EndTabItem();
        }

        ImGuiTabItemFlags inspectorTabFlags = 0;
        if( m_SwitchToPipelineInspectorTab )
        {
            inspectorTabFlags |= ImGuiTabItemFlags_SetSelected;
            m_SwitchToPipelineInspectorTab = false;
        }
        if( ImGui::BeginTabItem( Lang::Inspector, &m_PipelineInspectorTabOpen, inspectorTabFlags ) )
        {
            UpdateInspectorTab();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();

        // Draw other windows
        DrawTraceSerializationOutputWindow();

        ImGui::End();
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
        ImGuiIO& io = ImGui::GetIO();

        // Absolute path to the selected font
        std::filesystem::path defaultFontPath;
        std::filesystem::path codeFontPath;

#ifdef WIN32
        {
            // Locate system fonts directory
            std::filesystem::path fontsPath;

            PWSTR pFontsDirectoryPath = nullptr;

            if( SUCCEEDED( SHGetKnownFolderPath( FOLDERID_Fonts, KF_FLAG_DEFAULT, nullptr, &pFontsDirectoryPath ) ) )
            {
                fontsPath = pFontsDirectoryPath;
                CoTaskMemFree( pFontsDirectoryPath );
            }

            // List of fonts to use (in this order)
            const char* defaultFonts[] = {
                "segoeui.ttf",
                "tahoma.ttf" };

            const char* codeFonts[] = {
                "consolas.ttf",
                "cour.ttf" };

            for( const char* font : defaultFonts )
            {
                defaultFontPath = fontsPath / font;
                if( std::filesystem::exists( defaultFontPath ) )
                    break;
                else defaultFontPath = "";
            }

            for( const char* font : codeFonts )
            {
                codeFontPath = fontsPath / font;
                if( std::filesystem::exists( codeFontPath ) )
                    break;
                else codeFontPath = "";
            }
        }
#endif
#ifdef __linux__
        {
            // Linux distros use multiple font directories (or X server, TODO)
            std::vector<std::filesystem::path> fontDirectories = {
                "/usr/share/fonts",
                "/usr/local/share/fonts",
                "~/.fonts" };

            // Some systems may have these directories specified in conf file
            // https://stackoverflow.com/questions/3954223/platform-independent-way-to-get-font-directory
            const char* fontConfigurationFiles[] = {
                "/etc/fonts/fonts.conf",
                "/etc/fonts/local.conf" };

            std::vector<std::filesystem::path> configurationDirectories = {};

            for( const char* fontConfigurationFile : fontConfigurationFiles )
            {
                if( std::filesystem::exists( fontConfigurationFile ) )
                {
                    // Try to open configuration file for reading
                    std::ifstream conf( fontConfigurationFile );

                    if( conf.is_open() )
                    {
                        std::string line;

                        // conf is XML file, read line by line and find <dir> tag
                        while( std::getline( conf, line ) )
                        {
                            const size_t dirTagOpen = line.find( "<dir>" );
                            const size_t dirTagClose = line.find( "</dir>" );

                            // TODO: tags can be in different lines
                            if( (dirTagOpen != std::string::npos) && (dirTagClose != std::string::npos) )
                            {
                                configurationDirectories.push_back( line.substr( dirTagOpen + 5, dirTagClose - dirTagOpen - 5 ) );
                            }
                        }
                    }
                }
            }

            if( !configurationDirectories.empty() )
            {
                // Override predefined font directories
                fontDirectories = configurationDirectories;
            }

            // List of fonts to use (in this order)
            const char* defaultFonts[] = {
                "Ubuntu-R.ttf",
                "LiberationSans-Regural.ttf",
                "DejaVuSans.ttf" };

            const char* codeFonts[] = {
                "UbuntuMono-R.ttf",
                "DejaVuSansMono.ttf" };

            for( const char* font : defaultFonts )
            {
                for( const std::filesystem::path& fontDirectory : fontDirectories )
                {
                    defaultFontPath = ProfilerPlatformFunctions::FindFile( fontDirectory, font );
                    if( !defaultFontPath.empty() )
                        break;
                }
                if( !defaultFontPath.empty() )
                    break;
            }

            for( const char* font : codeFonts )
            {
                for( const std::filesystem::path& fontDirectory : fontDirectories )
                {
                    codeFontPath = ProfilerPlatformFunctions::FindFile( fontDirectory, font );
                    if( !codeFontPath.empty() )
                        break;
                }
                if( !codeFontPath.empty() )
                    break;
            }
        }
#endif

        // Include all glyphs in the font to support non-latin letters
        const ImWchar range[] = { 0x20, 0xFFFF, 0 };

        if( !defaultFontPath.empty() )
        {
            m_pImGuiDefaultFont = io.Fonts->AddFontFromFileTTF( defaultFontPath.string().c_str(), 16.f, nullptr, range );
        }

        if( !codeFontPath.empty() )
        {
            m_pImGuiCodeFont = io.Fonts->AddFontFromFileTTF( codeFontPath.string().c_str(), 16.f, nullptr, range );
        }
        else
        {
            m_pImGuiCodeFont = io.Fonts->AddFontDefault();
        }

        // Build atlas
        unsigned char* tex_pixels = NULL;
        int tex_w, tex_h;
        io.Fonts->GetTexDataAsRGBA32( &tex_pixels, &tex_w, &tex_h );
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

        // Top pipelines
        if( ImGui::CollapsingHeader( Lang::TopPipelines ) )
        {
            uint32_t i = 0;

            for( const auto& pipeline : m_Data.m_TopPipelines )
            {
                if( pipeline.m_Handle != VK_NULL_HANDLE )
                {
                    const uint64_t pipelineTicks = (pipeline.m_EndTimestamp.m_Value - pipeline.m_BeginTimestamp.m_Value);

                    ImGui::Text( "%2u. %s", i + 1, m_pStringSerializer->GetName( pipeline ).c_str() );
                    ImGuiX::TextAlignRight( "(%.1f %%) %.2f ms",
                        pipelineTicks * 100.f / m_Data.m_Ticks,
                        pipelineTicks * m_TimestampPeriod.count() );

                    // Print up to 10 top pipelines
                    if( (++i) == 10 ) break;
                }
            }
        }

        // Vendor-specific
        if( !m_Data.m_VendorMetrics.empty() &&
            ImGui::CollapsingHeader( Lang::PerformanceCounters ) )
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
            ImGui::Text( "Range" );
            ImGui::SameLine( 100.f );
            if( ImGui::BeginCombo( "PerformanceQueryFilter", m_PerformanceQueryCommandBufferFilterName.c_str() ) )
            {
                if( ImGuiX::TSelectable( "Frame", m_PerformanceQueryCommandBufferFilter, VkCommandBuffer() ) )
                {
                    // Selection changed.
                    m_PerformanceQueryCommandBufferFilterName = "Frame";
                }

                // Enumerate command buffers.
                for( VkCommandBuffer commandBuffer : uniqueCommandBuffers )
                {
                    std::string commandBufferName = m_pStringSerializer->GetName(commandBuffer);

                    if( ImGuiX::TSelectable( commandBufferName.c_str(), m_PerformanceQueryCommandBufferFilter, commandBuffer ) )
                    {
                        // Selection changed.
                        m_PerformanceQueryCommandBufferFilterName = std::move( commandBufferName );
                    }
                }

                ImGui::EndCombo();
            }

            // Show a combo box that allows the user to change the active metrics set.
            ImGui::Text( "Metrics set" );
            ImGui::SameLine( 100.f );
            if( ImGui::BeginCombo( "PerformanceQueryMetricsSet", m_VendorMetricsSets[ m_ActiveMetricsSetIndex ].m_Properties.name ) )
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
            ImGui::Text( "Filter" );
            ImGui::SameLine( 100.f );
            if( ImGui::InputText( "PerformanceQueryMetricsFilter", m_VendorMetricFilter, std::extent_v<decltype( m_VendorMetricFilter )> ) )
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
                ImGui::Text( "Performance metrics are not available for the selected command buffer." );
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
                        ImGuiX::TooltipUnformatted( metricProperties.description );
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

        // Force frame browser open
        if( m_ScrollToSelectedFrameBrowserNode )
        {
            ImGui::SetNextItemOpen( true );
        }

        // Frame browser
        if( ImGui::CollapsingHeader( Lang::FrameBrowser ) )
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

            FrameBrowserTreeNodeIndex index = {
                0x0,
                0xFFFF,
                0xFFFF,
                0xFFFF,
                0xFFFF,
                0xFFFF,
                0xFFFF,
                0xFFFF };

            // Enumerate submits in frame
            for( const auto& submitBatch : m_Data.m_Submits )
            {
                const std::string queueName = m_pStringSerializer->GetName( submitBatch.m_Handle );

                index.SubmitIndex = 0;
                index.PrimaryCommandBufferIndex = 0;

                char indexStr[ 2 * sizeof( index ) + 1 ] = {};
                structtohex( indexStr, index );

                if( m_ScrollToSelectedFrameBrowserNode &&
                    (m_SelectedFrameBrowserNodeIndex.SubmitBatchIndex == index.SubmitBatchIndex) )
                {
                    ImGui::SetNextItemOpen( true );
                }

                if( ImGui::TreeNode( indexStr, "vkQueueSubmit(%s, %u)",
                        queueName.c_str(),
                        static_cast<uint32_t>(submitBatch.m_Submits.size()) ) )
                {
                    for( const auto& submit : submitBatch.m_Submits )
                    {
                        structtohex( indexStr, index );

                        if( m_ScrollToSelectedFrameBrowserNode &&
                            (m_SelectedFrameBrowserNodeIndex.SubmitBatchIndex == index.SubmitBatchIndex) &&
                            (m_SelectedFrameBrowserNodeIndex.SubmitIndex == index.SubmitIndex) )
                        {
                            ImGui::SetNextItemOpen( true );
                        }

                        const bool inSubmitSubtree =
                            (submitBatch.m_Submits.size() > 1) &&
                            (ImGui::TreeNode( indexStr, "VkSubmitInfo #%u", index.SubmitIndex ));

                        if( (inSubmitSubtree) || (submitBatch.m_Submits.size() == 1) )
                        {
                            index.PrimaryCommandBufferIndex = 0;

                            // Sort frame browser data
                            std::list<const DeviceProfilerCommandBufferData*> pCommandBuffers =
                                SortFrameBrowserData( submit.m_CommandBuffers );

                            // Enumerate command buffers in submit
                            for( const auto* pCommandBuffer : pCommandBuffers )
                            {
                                PrintCommandBuffer( *pCommandBuffer, index );
                                index.PrimaryCommandBufferIndex++;
                            }

                            // Invalidate command buffer index
                            index.PrimaryCommandBufferIndex = 0xFFFF;
                        }

                        if( inSubmitSubtree )
                        {
                            // Finish submit subtree
                            ImGui::TreePop();
                        }

                        index.SubmitIndex++;
                    }

                    // Finish submit batch subtree
                    ImGui::TreePop();

                    // Invalidate submit index
                    index.SubmitIndex = 0xFFFF;
                }

                index.SubmitBatchIndex++;
            }
        }

        m_ScrollToSelectedFrameBrowserNode = false;
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
        PrintPipelineState

    Description:
        Helper function for writing pipeline states.

    \***********************************************************************************/
    template<int headerColumnSize = 250, typename... Args>
    PROFILER_FORCE_INLINE
    void PrintPipelineState(
        const char* pStateName,
        const char* pFormat,
        Args&&... args )
    {
        ImGui::TextUnformatted( pStateName );
        ImGui::SameLine();
        ImGui::SetCursorPosX( headerColumnSize );
        ImGui::Text( pFormat, std::forward<Args>( args )... );
    };

    /***********************************************************************************\

    Function:
        UpdateInspectorTab

    Description:
        Updates "Inspector" tab.

    \***********************************************************************************/
    void ProfilerOverlayOutput::UpdateInspectorTab()
    {
        if( m_pSelectedPipeline )
        {
            ImGui::TextUnformatted( "Pipeline" );
            ImGui::SameLine();
            ImGui::SetCursorPosX( 100 );
            ImGui::TextUnformatted( m_pStringSerializer->GetName( *m_pSelectedPipeline ).c_str() );

            if( m_ShowShaderCapabilities )
            {
                DrawShaderCapabilities( *m_pSelectedPipeline );
            }

            ImGui::Dummy( ImVec2( 1, 5 ) );

#if 0
            // Print all pipeline shader stages.
            const auto& stages = m_pSelectedPipeline->m_ShaderTuple.m_Shaders;
            const size_t stageCount = stages.size();

            for( uint32_t i = 0; i < stageCount; ++i )
            {
                const auto& shader = stages[ i ];

                if (shader.m_pShaderModule)
                {
                    PrintPipelineState( m_pStringSerializer->GetShaderStageName( shader.m_Stage ).c_str(), "%08X", shader.m_Hash );
                }
            }

            ImGui::Dummy( ImVec2( 1, 5 ) );
#endif

            // Print graphics pipeline state.
            if( m_pSelectedPipeline->m_BindPoint == VK_PIPELINE_BIND_POINT_GRAPHICS )
            {
                assert( m_pSelectedPipeline->m_pGraphicsState );
                auto& graphicsState = *m_pSelectedPipeline->m_pGraphicsState;

                if( graphicsState.m_InputAssemblyState.sType && ImGui::CollapsingHeader( "Input Assembly State" ) )
                {
                    PrintPipelineState( "Primitive topology", "%s", m_pStringSerializer->GetTopologyName( graphicsState.m_InputAssemblyState.topology ).c_str() );
                    PrintPipelineState( "Primitive restart enabled", "%s", m_pStringSerializer->GetBoolean( graphicsState.m_InputAssemblyState.primitiveRestartEnable ) );
                    ImGui::Dummy( ImVec2( 1, 5 ) );
                }

                if( graphicsState.m_TessellationState.sType && ImGui::CollapsingHeader( "Tessellation State" ) )
                {
                    PrintPipelineState( "Primitive restart enabled", "%u", graphicsState.m_TessellationState.patchControlPoints );
                    ImGui::Dummy( ImVec2( 1, 5 ) );
                }

                if( graphicsState.m_RasterizationState.sType && ImGui::CollapsingHeader( "Rasterization State" ) )
                {
                    PrintPipelineState( "Polygon mode", "%s", m_pStringSerializer->GetPolygonModeName( graphicsState.m_RasterizationState.polygonMode ).c_str() );
                    PrintPipelineState( "Cull mode", "%s", m_pStringSerializer->GetCullModeName( graphicsState.m_RasterizationState.cullMode ).c_str() );
                    PrintPipelineState( "Front face", "%s", m_pStringSerializer->GetFrontFaceName( graphicsState.m_RasterizationState.frontFace ).c_str() );
                    PrintPipelineState( "Rasterizer discard enabled", "%s", m_pStringSerializer->GetBoolean( graphicsState.m_RasterizationState.rasterizerDiscardEnable ) );
                    PrintPipelineState( "Depth bias enabled", "%s", m_pStringSerializer->GetBoolean( graphicsState.m_RasterizationState.depthBiasEnable ) );
                    PrintPipelineState( "Depth bias constant factor", "%f", graphicsState.m_RasterizationState.depthBiasConstantFactor );
                    PrintPipelineState( "Depth bias clamp", "%f", graphicsState.m_RasterizationState.depthBiasClamp );
                    PrintPipelineState( "Depth bias slope factor", "%f", graphicsState.m_RasterizationState.depthBiasSlopeFactor );
                    PrintPipelineState( "Line width", "%f", graphicsState.m_RasterizationState.lineWidth );
                    ImGui::Dummy( ImVec2( 1, 5 ) );
                }

                if( graphicsState.m_MultisampleState.sType && ImGui::CollapsingHeader( "Multisample State" ) )
                {
                    PrintPipelineState( "Rasterization samples", "%u", u32log2( graphicsState.m_MultisampleState.rasterizationSamples ) );
                    PrintPipelineState( "Sample shading enabled", "%s", m_pStringSerializer->GetBoolean( graphicsState.m_MultisampleState.sampleShadingEnable ) );
                    PrintPipelineState( "Min sample shading", "%f", graphicsState.m_MultisampleState.minSampleShading );
                    PrintPipelineState( "Sample mask", "%08X", graphicsState.m_MultisampleState.pSampleMask ? *graphicsState.m_MultisampleState.pSampleMask : 0xFFFFFFFF );
                    PrintPipelineState( "Alpha to coverage enabled", "%s", m_pStringSerializer->GetBoolean( graphicsState.m_MultisampleState.alphaToCoverageEnable ) );
                    PrintPipelineState( "Alpha to one enabled", "%s", m_pStringSerializer->GetBoolean( graphicsState.m_MultisampleState.alphaToOneEnable ) );
                    ImGui::Dummy( ImVec2( 1, 5 ) );
                }

                if( graphicsState.m_DepthStencilState.sType && ImGui::CollapsingHeader( "Depth-Stencil State" ) )
                {
                    PrintPipelineState( "Depth test enabled", "%s", m_pStringSerializer->GetBoolean( graphicsState.m_DepthStencilState.depthTestEnable ) );
                    PrintPipelineState( "Depth write enabled", "%s", m_pStringSerializer->GetBoolean( graphicsState.m_DepthStencilState.depthWriteEnable ) );
                    PrintPipelineState( "Depth compare op", "%s", m_pStringSerializer->GetCompareOpName( graphicsState.m_DepthStencilState.depthCompareOp ).c_str() );
                    PrintPipelineState( "Depth bounds test enabled", "%s", m_pStringSerializer->GetBoolean( graphicsState.m_DepthStencilState.depthBoundsTestEnable ) );
                    PrintPipelineState( "Stencil test enabled", "%s", m_pStringSerializer->GetBoolean( graphicsState.m_DepthStencilState.stencilTestEnable ) );
                    PrintPipelineState( "Min depth bounds", "%f", graphicsState.m_DepthStencilState.minDepthBounds );
                    PrintPipelineState( "Max depth bounds", "%f", graphicsState.m_DepthStencilState.maxDepthBounds );
                    ImGui::Dummy( ImVec2( 1, 5 ) );
                }

                if( graphicsState.m_ColorBlendState.sType && ImGui::CollapsingHeader( "Color Blend State" ) )
                {
                    PrintPipelineState( "Blend constants", "[ %.2f, %.2f, %.2f, %.2f ]",
                        graphicsState.m_ColorBlendState.blendConstants[ 0 ],
                        graphicsState.m_ColorBlendState.blendConstants[ 1 ],
                        graphicsState.m_ColorBlendState.blendConstants[ 2 ],
                        graphicsState.m_ColorBlendState.blendConstants[ 3 ] );

                    PrintPipelineState( "Logic op", "%s",
                        graphicsState.m_ColorBlendState.logicOpEnable
                            ? m_pStringSerializer->GetLogicOpName( graphicsState.m_ColorBlendState.logicOp ).c_str()
                            : "Disabled" );
                    
                    ImGui::Dummy( ImVec2( 1, 5 ) );
                    ImGui::BeginTable(
                        "Color blend attachment states",
                        /* columns_count */ 9,
                        ImGuiTableFlags_Borders & ~ImGuiTableFlags_BordersInnerV );

                    // Headers
                    ImGui::TableSetupColumn( "Attachment" );
                    ImGui::TableSetupColumn( "Enabled" );
                    ImGui::TableSetupColumn( "Color Op" );
                    ImGui::TableSetupColumn( "Color Src" );
                    ImGui::TableSetupColumn( "Color Dst" );
                    ImGui::TableSetupColumn( "Alpha Op" );
                    ImGui::TableSetupColumn( "Alpha Src" );
                    ImGui::TableSetupColumn( "Alpha Dst");
                    ImGui::TableSetupColumn( "Mask" );
                    ImGui::TableHeadersRow();

                    for( uint32_t i = 0; i < graphicsState.m_ColorBlendState.attachmentCount; ++i )
                    {
                        const auto& blendState = graphicsState.m_ColorBlendState.pAttachments[ i ];

                        ImGui::TableNextColumn();
                        ImGui::Text( "%d", i );
                        ImGui::TableNextColumn();
                        ImGui::Text( "%s", m_pStringSerializer->GetBoolean( blendState.blendEnable ) );
                        ImGui::TableNextColumn();
                        ImGui::Text( "%s", m_pStringSerializer->GetBlendOpName( blendState.colorBlendOp ).c_str() );
                        ImGui::TableNextColumn();
                        ImGui::Text( "%s", m_pStringSerializer->GetBlendFactorName( blendState.srcColorBlendFactor ).c_str() );
                        ImGui::TableNextColumn();
                        ImGui::Text( "%s", m_pStringSerializer->GetBlendFactorName( blendState.dstColorBlendFactor ).c_str() );
                        ImGui::TableNextColumn();
                        ImGui::Text( "%s", m_pStringSerializer->GetBlendOpName( blendState.alphaBlendOp ).c_str() );
                        ImGui::TableNextColumn();
                        ImGui::Text( "%s", m_pStringSerializer->GetBlendFactorName( blendState.srcAlphaBlendFactor ).c_str() );
                        ImGui::TableNextColumn();
                        ImGui::Text( "%s", m_pStringSerializer->GetBlendFactorName( blendState.dstAlphaBlendFactor ).c_str() );
                        ImGui::TableNextColumn();
                        int maskIdx = 0;
                        char mask[ 5 ];
                        if( blendState.colorWriteMask & VK_COLOR_COMPONENT_R_BIT ) mask[ maskIdx++ ] = 'R';
                        if( blendState.colorWriteMask & VK_COLOR_COMPONENT_G_BIT ) mask[ maskIdx++ ] = 'G';
                        if( blendState.colorWriteMask & VK_COLOR_COMPONENT_B_BIT ) mask[ maskIdx++ ] = 'B';
                        if( blendState.colorWriteMask & VK_COLOR_COMPONENT_A_BIT ) mask[ maskIdx++ ] = 'A';
                        mask[ maskIdx ] = 0;
                        ImGui::TextUnformatted( mask );
                    }

                    ImGui::EndTable();
                    ImGui::Dummy(ImVec2(1, 5));
                }
            }

            // Print ray-tracing pipeline state.
            if( m_pSelectedPipeline->m_BindPoint == VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR )
            {
                assert(m_pSelectedPipeline->m_pRayTracingState);
                auto& rayTracingState = *m_pSelectedPipeline->m_pRayTracingState;

                PrintPipelineState( "Max ray recursion depth", "%u", rayTracingState.m_MaxRecursionDepth );

                ImGui::Dummy( ImVec2( 1, 5 ) );

                if( ImGui::CollapsingHeader( "Groups" ) )
                {
                    const auto& shaderTuple = m_pSelectedPipeline->m_ShaderTuple;
                    const size_t shaderGroupCount = rayTracingState.m_ShaderGroups.size();

                    // Enumerate all shader groups in this pipeline.
                    for( uint32_t i = 0; i < shaderGroupCount; ++i )
                    {
                        ImGui::Text( "Shader group %u", i );
                        const auto& shaderGroup = rayTracingState.m_ShaderGroups[ i ];

                        if( auto* pGeneralShader = shaderTuple.GetShaderAtIndex( shaderGroup.generalShader ) )
                            PrintPipelineState<175>( "  General shader", "%s", m_pStringSerializer->GetName( *pGeneralShader ).c_str() );

                        if( auto* pClosestHitShader = shaderTuple.GetShaderAtIndex( shaderGroup.closestHitShader ) )
                            PrintPipelineState<175>( "  Closest-hit shader", "%s", m_pStringSerializer->GetName( *pClosestHitShader ).c_str() );

                        if( auto* pAnyHitShader = shaderTuple.GetShaderAtIndex( shaderGroup.anyHitShader ) )
                            PrintPipelineState<175>( "  Any-hit shader", "%s", m_pStringSerializer->GetName( *pAnyHitShader ).c_str() );

                        if( auto* pIntersectionShader = shaderTuple.GetShaderAtIndex( shaderGroup.intersectionShader ) )
                            PrintPipelineState<175>( "  Intersection shader", "%s", m_pStringSerializer->GetName( *pIntersectionShader ).c_str() );

                        ImGui::Dummy( ImVec2( 1, 2 ) );
                    }

                    ImGui::Dummy( ImVec2( 1, 3 ) );
                }
            }

            // Print shader stages.
            if( ImGui::CollapsingHeader( "Shaders" ) )
            {
                // Draw combo box with all shaders in the pipeline.
                ImGui::SetNextItemWidth( 350 );

                const char* pSelectedStage = m_pSelectedPipelineShaderStageNames[ m_SelectedPipelineShaderStageIndex ].c_str();
                if( ImGui::BeginCombo( "##ShaderStageCombo", pSelectedStage ) )
                {
                    const size_t stageCount = m_pSelectedPipelineShaderStageNames.size();
                    for( size_t i = 0; i < stageCount; ++i )
                    {
                        if( ImGuiX::TSelectable(
                                m_pSelectedPipelineShaderStageNames[ i ].c_str(),
                                pSelectedStage,
                                m_pSelectedPipelineShaderStageNames[ i ].c_str() ) )
                        {
                            // Selection changed
                            m_SelectedPipelineShaderStageIndex = i;
                        }
                    }

                    ImGui::EndCombo();
                }

                // Draw font size adjustment box.
                ImGui::SameLine();
                ImGui::SetCursorPosX( 365 );
                ImGui::SetNextItemWidth( 50 );

                static char fontSize[ 32 ] = "16";
                if( ImGui::InputText( "Font size", fontSize, 32, ImGuiInputTextFlags_CharsDecimal ) )
                {
                    float size = atof( fontSize );
                    if( size > 6.0f )
                        m_pImGuiCodeFont->Scale = size / 16.0f;
                }

                // Draw the selected shader stage.
                m_pSelectedPipelineShaderStageInspectors[ m_SelectedPipelineShaderStageIndex ]->Draw();
            }
        }
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
            ImGui::TextUnformatted( Lang::DrawCalls );
            ImGuiX::TextAlignRight( "%u", m_Data.m_Stats.m_DrawCount );

            ImGui::TextUnformatted( Lang::DrawCallsIndirect );
            ImGuiX::TextAlignRight( "%u", m_Data.m_Stats.m_DrawIndirectCount );

            ImGui::TextUnformatted( Lang::DispatchCalls );
            ImGuiX::TextAlignRight( "%u", m_Data.m_Stats.m_DispatchCount );

            ImGui::TextUnformatted( Lang::DispatchCallsIndirect );
            ImGuiX::TextAlignRight( "%u", m_Data.m_Stats.m_DispatchIndirectCount );
            
            ImGui::TextUnformatted( Lang::TraceRaysCalls );
            ImGuiX::TextAlignRight( "%u", m_Data.m_Stats.m_TraceRaysCount );

            ImGui::TextUnformatted( Lang::TraceRaysIndirectCalls );
            ImGuiX::TextAlignRight( "%u", m_Data.m_Stats.m_TraceRaysIndirectCount );

            ImGui::TextUnformatted( Lang::CopyBufferCalls );
            ImGuiX::TextAlignRight( "%u", m_Data.m_Stats.m_CopyBufferCount );

            ImGui::TextUnformatted( Lang::CopyBufferToImageCalls );
            ImGuiX::TextAlignRight( "%u", m_Data.m_Stats.m_CopyBufferToImageCount );

            ImGui::TextUnformatted( Lang::CopyImageCalls );
            ImGuiX::TextAlignRight( "%u", m_Data.m_Stats.m_CopyImageCount );

            ImGui::TextUnformatted( Lang::CopyImageToBufferCalls );
            ImGuiX::TextAlignRight( "%u", m_Data.m_Stats.m_CopyImageToBufferCount );

            ImGui::TextUnformatted( Lang::PipelineBarriers );
            ImGuiX::TextAlignRight( "%u", m_Data.m_Stats.m_PipelineBarrierCount );

            ImGui::TextUnformatted( Lang::ColorClearCalls );
            ImGuiX::TextAlignRight( "%u", m_Data.m_Stats.m_ClearColorCount );

            ImGui::TextUnformatted( Lang::DepthStencilClearCalls );
            ImGuiX::TextAlignRight( "%u", m_Data.m_Stats.m_ClearDepthStencilCount );

            ImGui::TextUnformatted( Lang::ResolveCalls );
            ImGuiX::TextAlignRight( "%u", m_Data.m_Stats.m_ResolveCount );

            ImGui::TextUnformatted( Lang::BlitCalls );
            ImGuiX::TextAlignRight( "%u", m_Data.m_Stats.m_BlitImageCount );

            ImGui::TextUnformatted( Lang::FillBufferCalls );
            ImGuiX::TextAlignRight( "%u", m_Data.m_Stats.m_FillBufferCount );

            ImGui::TextUnformatted( Lang::UpdateBufferCalls );
            ImGuiX::TextAlignRight( "%u", m_Data.m_Stats.m_UpdateBufferCount );
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
        if( ImGui::InputFloat( "Interface scale", &interfaceScale ) )
        {
            ImGui::GetIO().FontGlobalScale = std::clamp( interfaceScale, 0.25f, 4.0f );
        }

        // Select synchronization mode
        {
            static const char* syncGroupOptions[] = {
                Lang::Present,
                Lang::Submit };

            static int syncModeSelectedOption = 0;
            int previousSyncModeSelectedOption = syncModeSelectedOption;

            ImGui::Combo( Lang::SyncMode, &syncModeSelectedOption, syncGroupOptions, 2 );

            if( syncModeSelectedOption != previousSyncModeSelectedOption )
            {
                vkSetProfilerSyncModeEXT( m_pDevice->Handle, (VkProfilerSyncModeEXT)syncModeSelectedOption );
            }
        }

        // Select time display unit.
        {
            static const char* timeUnitGroupOptions[] = {
                Lang::Milliseconds,
                Lang::Microseconds,
                Lang::Nanoseconds };

            static int timeUnitSelectedOption = 0;
            int previousTimeUnitSelectedOption = timeUnitSelectedOption;

            ImGui::Combo( Lang::TimeUnit, &timeUnitSelectedOption, timeUnitGroupOptions, 3 );

            if( timeUnitSelectedOption != previousTimeUnitSelectedOption )
            {
                static float timeUnitFactors[] = {
                    1.0f,
                    1'000.0f,
                    1'000'000.0f
                };

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
        FrameBrowserTreeNodeIndex index = {
            0x0,
            0xFFFF,
            0xFFFF,
            0xFFFF,
            0xFFFF,
            0xFFFF,
            0xFFFF,
            0xFFFF };

        // Enumerate submits batches in frame
        for( const auto& submitBatch : m_Data.m_Submits )
        {
            index.SubmitIndex = 0;

            // Enumerate submits in submit batch
            for( const auto& submit : submitBatch.m_Submits )
            {
                index.PrimaryCommandBufferIndex = 0;

                // Enumerate command buffers in submit
                for( const auto& commandBuffer : submit.m_CommandBuffers )
                {
                    GetPerformanceGraphColumns( commandBuffer, index, columns );
                    index.PrimaryCommandBufferIndex++;
                }

                index.PrimaryCommandBufferIndex = 0xFFFF;
                index.SubmitIndex++;
            }

            index.SubmitIndex = 0xFFFF;
            index.SubmitBatchIndex++;
        }
    }

    /***********************************************************************************\

    Function:
        GetPerformanceGraphColumns

    Description:
        Enumerate performance graph columns.

    \***********************************************************************************/
    void ProfilerOverlayOutput::GetPerformanceGraphColumns(
        const DeviceProfilerCommandBufferData& data,
        FrameBrowserTreeNodeIndex index,
        std::vector<PerformanceGraphColumn>& columns ) const
    {
        // RenderPassIndex may be already set if we're processing secondary command buffer with RENDER_PASS_CONTINUE_BIT set.
        const bool renderPassContinue = (index.RenderPassIndex != 0xFFFF);

        if( !renderPassContinue )
        {
            index.RenderPassIndex = 0;
        }

        // Enumerate render passes in command buffer
        for( const auto& renderPass : data.m_RenderPasses )
        {
            GetPerformanceGraphColumns( renderPass, index, columns );
            index.RenderPassIndex++;
        }
    }

    /***********************************************************************************\

    Function:
        GetPerformanceGraphColumns

    Description:
        Enumerate performance graph columns.

    \***********************************************************************************/
    void ProfilerOverlayOutput::GetPerformanceGraphColumns(
        const DeviceProfilerRenderPassData& data,
        FrameBrowserTreeNodeIndex index,
        std::vector<PerformanceGraphColumn>& columns ) const
    {
        // RenderPassIndex may be already set if we're processing secondary command buffer with RENDER_PASS_CONTINUE_BIT set.
        const bool renderPassContinue = (index.SubpassIndex != 0xFFFF);

        if( (m_HistogramGroupMode <= HistogramGroupMode::eRenderPass) &&
            (data.m_Handle != VK_NULL_HANDLE || data.m_Dynamic) )
        {
            const float cycleCount = static_cast<float>( data.m_EndTimestamp.m_Value - data.m_BeginTimestamp.m_Value );

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
            if( !renderPassContinue )
            {
                index.SubpassIndex = 0;
            }

            // Enumerate subpasses in render pass
            for( const auto& subpass : data.m_Subpasses )
            {
                if( subpass.m_Contents == VK_SUBPASS_CONTENTS_INLINE )
                {
                    index.PipelineIndex = 0;

                    // Enumerate pipelines in subpass
                    for( const auto& pipeline : subpass.m_Pipelines )
                    {
                        GetPerformanceGraphColumns( pipeline, index, columns );
                        index.PipelineIndex++;
                    }

                    index.PipelineIndex = 0xFFFF;
                }

                else if( subpass.m_Contents == VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS )
                {
                    index.SecondaryCommandBufferIndex = 0;

                    // Enumerate secondary command buffers
                    for( const auto& commandBuffer : subpass.m_SecondaryCommandBuffers )
                    {
                        GetPerformanceGraphColumns( commandBuffer, index, columns );
                        index.SecondaryCommandBufferIndex++;
                    }

                    index.SecondaryCommandBufferIndex = 0xFFFF;
                }

                index.SubpassIndex++;
            }
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
        FrameBrowserTreeNodeIndex index,
        std::vector<PerformanceGraphColumn>& columns ) const
    {
        if( (m_HistogramGroupMode <= HistogramGroupMode::ePipeline) &&
            ((data.m_ShaderTuple.m_Hash & 0xFFFF) != 0) &&
            (data.m_Handle != VK_NULL_HANDLE) )
        {
            const float cycleCount = static_cast<float>( data.m_EndTimestamp.m_Value - data.m_BeginTimestamp.m_Value );

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
            index.DrawcallIndex = 0;

            // Enumerate drawcalls in pipeline
            for( const auto& drawcall : data.m_Drawcalls )
            {
                GetPerformanceGraphColumns( drawcall, index, columns );
                index.DrawcallIndex++;
            }
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
        FrameBrowserTreeNodeIndex index,
        std::vector<PerformanceGraphColumn>& columns ) const
    {
        const float cycleCount = static_cast<float>( data.m_EndTimestamp.m_Value - data.m_BeginTimestamp.m_Value );

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
            regionCycleCount = renderPassData.m_EndTimestamp.m_Value - renderPassData.m_BeginTimestamp.m_Value;
            break;
        }

        case HistogramGroupMode::ePipeline:
        {
            const DeviceProfilerPipelineData& pipelineData =
                *reinterpret_cast<const DeviceProfilerPipelineData*>(data.userData);

            regionName = m_pStringSerializer->GetName( pipelineData );
            regionCycleCount = pipelineData.m_EndTimestamp.m_Value - pipelineData.m_BeginTimestamp.m_Value;
            break;
        }

        case HistogramGroupMode::eDrawcall:
        {
            const DeviceProfilerDrawcall& pipelineData =
                *reinterpret_cast<const DeviceProfilerDrawcall*>(data.userData);

            regionName = m_pStringSerializer->GetName( pipelineData );
            regionCycleCount = pipelineData.m_EndTimestamp.m_Value - pipelineData.m_BeginTimestamp.m_Value;
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
    void ProfilerOverlayOutput::PrintCommandBuffer( const DeviceProfilerCommandBufferData& cmdBuffer, FrameBrowserTreeNodeIndex index )
    {
        const uint64_t commandBufferTicks = (cmdBuffer.m_EndTimestamp.m_Value - cmdBuffer.m_BeginTimestamp.m_Value);

        // Mark hotspots with color
        DrawSignificanceRect( (float)commandBufferTicks / m_Data.m_Ticks, index );

        char indexStr[ 2 * sizeof( index ) + 1 ] = {};
        structtohex( indexStr, index );

        if( (m_ScrollToSelectedFrameBrowserNode) &&
            (m_SelectedFrameBrowserNodeIndex.SubmitBatchIndex == index.SubmitBatchIndex) &&
            (m_SelectedFrameBrowserNodeIndex.SubmitIndex == index.SubmitIndex) &&
            (((cmdBuffer.m_Level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) &&
                  (m_SelectedFrameBrowserNodeIndex.PrimaryCommandBufferIndex == index.PrimaryCommandBufferIndex)) ||
                ((cmdBuffer.m_Level == VK_COMMAND_BUFFER_LEVEL_SECONDARY) &&
                    (m_SelectedFrameBrowserNodeIndex.PrimaryCommandBufferIndex == index.PrimaryCommandBufferIndex) &&
                    (m_SelectedFrameBrowserNodeIndex.RenderPassIndex == index.RenderPassIndex) &&
                    (m_SelectedFrameBrowserNodeIndex.SubpassIndex == index.SubpassIndex) &&
                    (m_SelectedFrameBrowserNodeIndex.SecondaryCommandBufferIndex == index.SecondaryCommandBufferIndex))) )
        {
            // Tree contains selected node
            ImGui::SetNextItemOpen( true );
            ImGui::SetScrollHereY();
        }

        if( ImGui::TreeNode( indexStr, "%s", m_pStringSerializer->GetName( cmdBuffer.m_Handle ).c_str() ) )
        {
            // Command buffer opened
            PrintDuration( cmdBuffer );

            // Sort frame browser data
            std::list<const DeviceProfilerRenderPassData*> pRenderPasses =
                SortFrameBrowserData( cmdBuffer.m_RenderPasses );

            // RenderPassIndex may be already set if we're processing secondary command buffer with RENDER_PASS_CONTINUE_BIT set.
            const bool renderPassContinue = (index.RenderPassIndex != 0xFFFF);

            if( !renderPassContinue )
            {
                index.RenderPassIndex = 0;
            }

            // Enumerate render passes in command buffer
            for( const DeviceProfilerRenderPassData* pRenderPass : pRenderPasses )
            {
                PrintRenderPass( *pRenderPass, index );
                index.RenderPassIndex++;
            }

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
        const uint64_t commandTicks = (data.m_EndTimestamp.m_Value - data.m_BeginTimestamp.m_Value);

        index.DrawcallIndex = drawcallIndex;

        if( (m_ScrollToSelectedFrameBrowserNode) &&
            (m_SelectedFrameBrowserNodeIndex == index) )
        {
            ImGui::SetScrollHereY();
        }

        // Mark hotspots with color
        DrawSignificanceRect( (float)commandTicks / m_Data.m_Ticks, index );

        index.DrawcallIndex = 0xFFFF;

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
    void ProfilerOverlayOutput::PrintRenderPass( const DeviceProfilerRenderPassData& renderPass, FrameBrowserTreeNodeIndex index )
    {
        const bool isValidRenderPass = (renderPass.m_Type != DeviceProfilerRenderPassType::eNone);

        if( isValidRenderPass )
        {
            const uint64_t renderPassTicks = (renderPass.m_EndTimestamp.m_Value - renderPass.m_BeginTimestamp.m_Value);

            // Mark hotspots with color
            DrawSignificanceRect( (float)renderPassTicks / m_Data.m_Ticks, index );
        }

        char indexStr[ 2 * sizeof( index ) + 1 ] = {};
        structtohex( indexStr, index );

        // At least one subpass must be present
        assert( !renderPass.m_Subpasses.empty() );

        if( (m_ScrollToSelectedFrameBrowserNode) &&
            (m_SelectedFrameBrowserNodeIndex.SubmitBatchIndex == index.SubmitBatchIndex) &&
            (m_SelectedFrameBrowserNodeIndex.SubmitIndex == index.SubmitIndex) &&
            (m_SelectedFrameBrowserNodeIndex.PrimaryCommandBufferIndex == index.PrimaryCommandBufferIndex) &&
            (m_SelectedFrameBrowserNodeIndex.RenderPassIndex == index.RenderPassIndex) &&
            ((index.SecondaryCommandBufferIndex == 0xFFFF) ||
                (m_SelectedFrameBrowserNodeIndex.SecondaryCommandBufferIndex == index.SecondaryCommandBufferIndex)) )
        {
            // Tree contains selected node
            ImGui::SetNextItemOpen( true );
            ImGui::SetScrollHereY();
        }

        bool inRenderPassSubtree;
        if( isValidRenderPass )
        {
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
            // Render pass subtree opened
            if( isValidRenderPass )
            {
                PrintDuration( renderPass );

                if( renderPass.HasBeginCommand() )
                {
                    PrintRenderPassCommand( renderPass.m_Begin, renderPass.m_Dynamic, index, 0 );
                }
            }

            // Sort frame browser data
            std::list<const DeviceProfilerSubpassData*> pSubpasses =
                SortFrameBrowserData( renderPass.m_Subpasses );

            // SubpassIndex may be already set if we're processing secondary command buffer with RENDER_PASS_CONTINUE_BIT set.
            const bool renderPassContinue = (index.SubpassIndex != 0xFFFF);

            if( !renderPassContinue )
            {
                index.SubpassIndex = 0;
            }

            // Enumerate subpasses
            for( const DeviceProfilerSubpassData* pSubpass : pSubpasses )
            {
                PrintSubpass( *pSubpass, index, (pSubpasses.size() == 1) );
                index.SubpassIndex++;
            }

            if( !renderPassContinue )
            {
                index.SubpassIndex = 0xFFFF;
            }

            if( isValidRenderPass )
            {
                if( renderPass.HasEndCommand() )
                {
                    PrintRenderPassCommand( renderPass.m_End, renderPass.m_Dynamic, index, 1 );
                }

                ImGui::TreePop();
            }
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
    void ProfilerOverlayOutput::PrintSubpass( const DeviceProfilerSubpassData& subpass, FrameBrowserTreeNodeIndex index, bool isOnlySubpass )
    {
        const uint64_t subpassTicks = (subpass.m_EndTimestamp.m_Value - subpass.m_BeginTimestamp.m_Value);
        bool inSubpassSubtree = false;

        if( !isOnlySubpass )
        {
            // Mark hotspots with color
            DrawSignificanceRect( (float)subpassTicks / m_Data.m_Ticks, index );

            char indexStr[ 2 * sizeof( index ) + 1 ] = {};
            structtohex( indexStr, index );

            if( (m_ScrollToSelectedFrameBrowserNode) &&
                (m_SelectedFrameBrowserNodeIndex.SubmitBatchIndex == index.SubmitBatchIndex) &&
                (m_SelectedFrameBrowserNodeIndex.SubmitIndex == index.SubmitIndex) &&
                (m_SelectedFrameBrowserNodeIndex.PrimaryCommandBufferIndex == index.PrimaryCommandBufferIndex) &&
                (m_SelectedFrameBrowserNodeIndex.SecondaryCommandBufferIndex == index.SecondaryCommandBufferIndex) &&
                (m_SelectedFrameBrowserNodeIndex.RenderPassIndex == index.RenderPassIndex) &&
                (m_SelectedFrameBrowserNodeIndex.SubpassIndex == index.SubpassIndex) )
            {
                // Tree contains selected node
                ImGui::SetNextItemOpen( true );
                ImGui::SetScrollHereY();
            }

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
            if( subpass.m_Contents == VK_SUBPASS_CONTENTS_INLINE )
            {
                // Sort frame browser data
                std::list<const DeviceProfilerPipelineData*> pPipelines =
                    SortFrameBrowserData( subpass.m_Pipelines );

                index.PipelineIndex = 0;

                // Enumerate pipelines in subpass
                for( const DeviceProfilerPipelineData* pPipeline : pPipelines )
                {
                    PrintPipeline( *pPipeline, index );
                    index.PipelineIndex++;
                }
            }

            else if( subpass.m_Contents == VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS )
            {
                // Sort command buffers
                std::list<const DeviceProfilerCommandBufferData*> pCommandBuffers =
                    SortFrameBrowserData( subpass.m_SecondaryCommandBuffers );

                index.SecondaryCommandBufferIndex = 0;

                // Enumerate command buffers in subpass
                for( const DeviceProfilerCommandBufferData* pCommandBuffer : pCommandBuffers )
                {
                    PrintCommandBuffer( *pCommandBuffer, index );
                    index.SecondaryCommandBufferIndex++;
                }
            }
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
    void ProfilerOverlayOutput::PrintPipeline( const DeviceProfilerPipelineData& pipeline, FrameBrowserTreeNodeIndex index )
    {
        const uint64_t pipelineTicks = (pipeline.m_EndTimestamp.m_Value - pipeline.m_BeginTimestamp.m_Value);

        const bool printPipelineInline =
            (pipeline.m_Handle == VK_NULL_HANDLE) ||
            ((pipeline.m_ShaderTuple.m_Hash & 0xFFFF) == 0);

        bool inPipelineSubtree = false;

        if( !printPipelineInline )
        {
            // Mark hotspots with color
            DrawSignificanceRect( (float)pipelineTicks / m_Data.m_Ticks, index );

            char indexStr[ 2 * sizeof( index ) + 1 ] = {};
            structtohex( indexStr, index );

            if( (m_ScrollToSelectedFrameBrowserNode) &&
                (m_SelectedFrameBrowserNodeIndex.SubmitBatchIndex == index.SubmitBatchIndex) &&
                (m_SelectedFrameBrowserNodeIndex.SubmitIndex == index.SubmitIndex) &&
                (m_SelectedFrameBrowserNodeIndex.PrimaryCommandBufferIndex == index.PrimaryCommandBufferIndex) &&
                (m_SelectedFrameBrowserNodeIndex.SecondaryCommandBufferIndex == index.SecondaryCommandBufferIndex) &&
                (m_SelectedFrameBrowserNodeIndex.RenderPassIndex == index.RenderPassIndex) &&
                (m_SelectedFrameBrowserNodeIndex.SubpassIndex == index.SubpassIndex) &&
                (m_SelectedFrameBrowserNodeIndex.PipelineIndex == index.PipelineIndex) )
            {
                // Tree contains selected node
                ImGui::SetNextItemOpen( true );
                ImGui::SetScrollHereY();
            }

            inPipelineSubtree =
                (ImGui::TreeNode( indexStr, "%s", m_pStringSerializer->GetName( pipeline ).c_str() ));

            if( ImGui::BeginPopupContextItem() )
            {
                if( ImGui::MenuItem( "Inspect", nullptr, nullptr ) )
                {
                    InspectPipeline( pipeline );
                }

                ImGui::EndPopup();
            }
        }

        if( m_ShowShaderCapabilities )
        {
            DrawShaderCapabilities( pipeline );
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

            index.DrawcallIndex = 0;

            // Enumerate drawcalls in pipeline
            for( const DeviceProfilerDrawcall* pDrawcall : pDrawcalls )
            {
                PrintDrawcall( *pDrawcall, index );
                index.DrawcallIndex++;
            }
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
    void ProfilerOverlayOutput::PrintDrawcall( const DeviceProfilerDrawcall& drawcall, FrameBrowserTreeNodeIndex index )
    {
        if( drawcall.GetPipelineType() != DeviceProfilerPipelineType::eDebug )
        {
            const uint64_t drawcallTicks = (drawcall.m_EndTimestamp.m_Value - drawcall.m_BeginTimestamp.m_Value);

            if( (m_ScrollToSelectedFrameBrowserNode) &&
                (m_SelectedFrameBrowserNodeIndex == index) )
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
        DrawShaderCapabilities

    Description:

    \***********************************************************************************/
    void ProfilerOverlayOutput::DrawShaderCapabilities( const DeviceProfilerPipelineData& pipeline )
    {
        if( pipeline.m_UsesRayQuery )
        {
            static ImU32 rayQueryCapabilityColor = ImGui::GetColorU32({ 0.52f, 0.32f, 0.1f, 1.f });
            DrawShaderCapabilityBadge( rayQueryCapabilityColor, "RQ", "Ray Query" );
        }
        if( pipeline.m_UsesRayTracing )
        {
            static ImU32 rayTracingCapabilityColor = ImGui::GetColorU32({ 0.1f, 0.43f, 0.52f, 1.0f });
            DrawShaderCapabilityBadge( rayTracingCapabilityColor, "RT", "Ray Tracing" );
        }
    }

    /***********************************************************************************\

    Function:
        DrawShaderCapabilityBadge

    Description:

    \***********************************************************************************/
    void ProfilerOverlayOutput::DrawShaderCapabilityBadge( uint32_t color, const char* shortName, const char* longName )
    {
        assert( m_ShowShaderCapabilities );
        
        ImGui::SameLine();
        ImGuiX::BadgeUnformatted( color, 5.f, shortName );

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text( Lang::ShaderCapabilityTooltipFmt, longName );
            ImGui::EndTooltip();
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
            const uint64_t ticks = data.m_EndTimestamp.m_Value - data.m_BeginTimestamp.m_Value;

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

    /***********************************************************************************\

    Function:
        InspectPipeline

    Description:

    \***********************************************************************************/
    void ProfilerOverlayOutput::InspectPipeline( const DeviceProfilerPipelineData& pipeline )
    {
        // Don't do anything if we're already inspecting this pipeline.
        if( !m_pSelectedPipeline || m_pSelectedPipeline->m_Handle != pipeline.m_Handle )
        {
            m_pSelectedPipeline = &pipeline;
            m_pSelectedPipelineShaderStageNames.clear();
            m_pSelectedPipelineShaderStageInspectors.clear();
            m_SelectedPipelineShaderStageIndex = 0;

            // Create an inspector tab for each stage in the pipeline.
            for( size_t i = 0; i < pipeline.m_ShaderTuple.m_Shaders.size(); ++i )
            {
                const DeviceProfilerPipelineShader& shader = pipeline.m_ShaderTuple.m_Shaders[i];
                if( shader.m_pShaderModule )
                {
                    m_pSelectedPipelineShaderStageNames.emplace_back( m_pStringSerializer->GetName( shader ) );
                    m_pSelectedPipelineShaderStageInspectors.emplace_back(
                        std::make_unique<DeviceProfilerShaderInspectorTab>( *m_pDevice, pipeline, shader, m_pImGuiCodeFont ) );
                }
            }
        }

        m_PipelineInspectorTabOpen = true;
        m_SwitchToPipelineInspectorTab = true;
    }
}

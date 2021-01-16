// Copyright (c) 2019-2021 Lukasz Stalmirski
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

#include "profiler.h"
#include "profiler_command_buffer.h"
#include "profiler_helpers.h"
#include "farmhash/src/farmhash.h"
#include <sstream>
#include <fstream>

namespace
{
    static inline VkImageAspectFlags GetImageAspectFlagsForFormat( VkFormat format )
    {
        // Assume color aspect except for depth-stencil formats
        switch( format )
        {
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_D32_SFLOAT:
            return VK_IMAGE_ASPECT_DEPTH_BIT;

        case VK_FORMAT_D16_UNORM_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

        case VK_FORMAT_S8_UINT:
            return VK_IMAGE_ASPECT_STENCIL_BIT;
        }

        return VK_IMAGE_ASPECT_COLOR_BIT;
    }

    template<typename RenderPassCreateInfo>
    static inline void CountRenderPassAttachmentClears(
        Profiler::DeviceProfilerRenderPass& renderPass,
        const RenderPassCreateInfo* pCreateInfo )
    {
        for( uint32_t attachmentIndex = 0; attachmentIndex < pCreateInfo->attachmentCount; ++attachmentIndex )
        {
            const auto& attachment = pCreateInfo->pAttachments[ attachmentIndex ];
            const auto imageFormatAspectFlags = GetImageAspectFlagsForFormat( attachment.format );

            // Color attachment clear
            if( (imageFormatAspectFlags & VK_IMAGE_ASPECT_COLOR_BIT) &&
                (attachment.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR) )
            {
                renderPass.m_ClearColorAttachmentCount++;
            }

            bool hasDepthClear = false;

            // Depth attachment clear
            if( (imageFormatAspectFlags & VK_IMAGE_ASPECT_DEPTH_BIT) &&
                (attachment.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR) )
            {
                hasDepthClear = true;
                renderPass.m_ClearDepthStencilAttachmentCount++;
            }

            // Stencil attachment clear
            if( (imageFormatAspectFlags & VK_IMAGE_ASPECT_STENCIL_BIT) &&
                (attachment.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_CLEAR) )
            {
                // Treat depth-stencil clear as one (just like vkCmdClearDepthStencilImage call)
                if( !(hasDepthClear) )
                {
                    renderPass.m_ClearDepthStencilAttachmentCount++;
                }
            }
        }
    }

    template<typename SubpassDescription>
    static inline void CountSubpassAttachmentResolves(
        Profiler::DeviceProfilerSubpass& subpass,
        const SubpassDescription& subpassDescription )
    {
        if( subpassDescription.pResolveAttachments )
        {
            for( uint32_t attachmentIndex = 0; attachmentIndex < subpassDescription.colorAttachmentCount; ++attachmentIndex )
            {
                // Attachments which are not resolved have VK_ATTACHMENT_UNUSED set
                if( subpassDescription.pResolveAttachments[ attachmentIndex ].attachment != VK_ATTACHMENT_UNUSED )
                {
                    subpass.m_ResolveCount++;
                }
            }
        }
    }
}

namespace Profiler
{
    /***********************************************************************************\

    Function:
        DeviceProfiler

    Description:
        Constructor

    \***********************************************************************************/
    DeviceProfiler::DeviceProfiler()
        : m_pDevice( nullptr )
        , m_Config()
        , m_PresentMutex()
        , m_SubmitMutex()
        , m_Data()
        , m_DataAggregator()
        , m_CurrentFrame( 0 )
        , m_CpuTimestampCounter()
        , m_CpuFpsCounter()
        , m_Allocations()
        , m_CommandBuffers()
        , m_CommandPools()
        , m_PerformanceConfigurationINTEL( VK_NULL_HANDLE )
    {
    }

    /***********************************************************************************\

    Function:
        EnumerateOptionalDeviceExtensions

    Description:
        Get list of optional device extensions that may be utilized by the profiler.

    \***********************************************************************************/
    std::unordered_set<std::string> DeviceProfiler::EnumerateOptionalDeviceExtensions()
    {
        return {
            VK_INTEL_PERFORMANCE_QUERY_EXTENSION_NAME,
            VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME,
            VK_EXT_DEBUG_MARKER_EXTENSION_NAME
        };
    }

    /***********************************************************************************\

    Function:
        EnumerateOptionalInstanceExtensions

    Description:
        Get list of optional instance extensions that may be utilized by the profiler.

    \***********************************************************************************/
    std::unordered_set<std::string> DeviceProfiler::EnumerateOptionalInstanceExtensions()
    {
        return {
            VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME
        };
    }

    /***********************************************************************************\

    Function:
        Initialize

    Description:
        Initializes profiler resources.

    \***********************************************************************************/
    VkResult DeviceProfiler::Initialize( VkDevice_Object* pDevice, const VkProfilerCreateInfoEXT* pCreateInfo )
    {
        m_pDevice = pDevice;
        m_CurrentFrame = 0;

        std::memset( &m_Config, 0, sizeof( m_Config ) );

        // Check if application provided create info
        if( pCreateInfo )
        {
            m_Config.m_Flags = pCreateInfo->flags;
        }

        // Check if preemption is enabled
        // It may break the results
        if( ProfilerPlatformFunctions::IsPreemptionEnabled() )
        {
            // Sample per drawcall to avoid DMA packet splits between timestamps
            //m_Config.m_SamplingMode = ProfilerMode::ePerDrawcall;
            #if 0
            m_Output.Summary.Message = "Preemption enabled. Profiler will collect results per drawcall.";
            m_Overlay.Summary.Message = "Preemption enabled. Results may be unstable.";
            #endif
        }

        // Create submit fence
        VkFenceCreateInfo fenceCreateInfo;
        ClearStructure( &fenceCreateInfo, VK_STRUCTURE_TYPE_FENCE_CREATE_INFO );

        VkResult result = m_pDevice->Callbacks.CreateFence(
            m_pDevice->Handle, &fenceCreateInfo, nullptr, &m_SubmitFence );

        // Prepare for memory usage tracking
        m_MemoryData.m_Heaps.resize( m_pDevice->pPhysicalDevice->MemoryProperties.memoryHeapCount );
        m_MemoryData.m_Types.resize( m_pDevice->pPhysicalDevice->MemoryProperties.memoryTypeCount );

        if( result != VK_SUCCESS )
        {
            // Fence creation failed
            Destroy();
            return result;
        }

        // Enable vendor-specific extensions
        if( m_pDevice->EnabledExtensions.count( VK_INTEL_PERFORMANCE_QUERY_EXTENSION_NAME ) )
        {
            InitializeINTEL();
        }

        // Initialize aggregator
        m_DataAggregator.Initialize( this );

        // Initialize internal pipelines
        CreateInternalPipeline( DeviceProfilerPipelineType::eCopyBuffer, "CopyBuffer" );
        CreateInternalPipeline( DeviceProfilerPipelineType::eCopyBufferToImage, "CopyBufferToImage" );
        CreateInternalPipeline( DeviceProfilerPipelineType::eCopyImage, "CopyImage" );
        CreateInternalPipeline( DeviceProfilerPipelineType::eCopyImageToBuffer, "CopyImageToBuffer" );
        CreateInternalPipeline( DeviceProfilerPipelineType::eClearAttachments, "ClearAttachments" );
        CreateInternalPipeline( DeviceProfilerPipelineType::eClearColorImage, "ClearColorImage" );
        CreateInternalPipeline( DeviceProfilerPipelineType::eClearDepthStencilImage, "ClearDepthStencilImage" );
        CreateInternalPipeline( DeviceProfilerPipelineType::eResolveImage, "ResolveImage" );
        CreateInternalPipeline( DeviceProfilerPipelineType::eBlitImage, "BlitImage" );
        CreateInternalPipeline( DeviceProfilerPipelineType::eFillBuffer, "FillBuffer" );
        CreateInternalPipeline( DeviceProfilerPipelineType::eUpdateBuffer, "UpdateBuffer" );
        CreateInternalPipeline( DeviceProfilerPipelineType::eBeginRenderPass, "BeginRenderPass" );
        CreateInternalPipeline( DeviceProfilerPipelineType::eEndRenderPass, "EndRenderPass" );

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        InitializeINTEL

    Description:
        Initializes INTEL-specific profiler resources.

    \***********************************************************************************/
    VkResult DeviceProfiler::InitializeINTEL()
    {
        // Load MDAPI
        VkResult result = m_MetricsApiINTEL.Initialize();

        if( result != VK_SUCCESS ||
            m_MetricsApiINTEL.IsAvailable() == false )
        {
            return result;
        }

        // Import extension functions
        if( m_pDevice->Callbacks.InitializePerformanceApiINTEL == nullptr )
        {
            auto gpa = m_pDevice->Callbacks.GetDeviceProcAddr;

            #define GPA( PROC ) (PFN_vk##PROC)gpa( m_pDevice->Handle, "vk" #PROC ); \
            assert( m_pDevice->Callbacks.PROC )

            m_pDevice->Callbacks.AcquirePerformanceConfigurationINTEL = GPA( AcquirePerformanceConfigurationINTEL );
            m_pDevice->Callbacks.CmdSetPerformanceMarkerINTEL = GPA( CmdSetPerformanceMarkerINTEL );
            m_pDevice->Callbacks.CmdSetPerformanceOverrideINTEL = GPA( CmdSetPerformanceOverrideINTEL );
            m_pDevice->Callbacks.CmdSetPerformanceStreamMarkerINTEL = GPA( CmdSetPerformanceStreamMarkerINTEL );
            m_pDevice->Callbacks.GetPerformanceParameterINTEL = GPA( GetPerformanceParameterINTEL );
            m_pDevice->Callbacks.InitializePerformanceApiINTEL = GPA( InitializePerformanceApiINTEL );
            m_pDevice->Callbacks.QueueSetPerformanceConfigurationINTEL = GPA( QueueSetPerformanceConfigurationINTEL );
            m_pDevice->Callbacks.ReleasePerformanceConfigurationINTEL = GPA( ReleasePerformanceConfigurationINTEL );
            m_pDevice->Callbacks.UninitializePerformanceApiINTEL = GPA( UninitializePerformanceApiINTEL );
        }

        // Initialize performance API
        {
            VkInitializePerformanceApiInfoINTEL initInfo = {};
            initInfo.sType = VK_STRUCTURE_TYPE_INITIALIZE_PERFORMANCE_API_INFO_INTEL;

            result = m_pDevice->Callbacks.InitializePerformanceApiINTEL(
                m_pDevice->Handle, &initInfo );

            if( result != VK_SUCCESS )
            {
                m_MetricsApiINTEL.Destroy();
                return result;
            }
        }

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:
        Frees resources allocated by the profiler.

    \***********************************************************************************/
    void DeviceProfiler::Destroy()
    {
        m_CommandBuffers.clear();

        m_Allocations.clear();

        if( m_SubmitFence != VK_NULL_HANDLE )
        {
            m_pDevice->Callbacks.DestroyFence( m_pDevice->Handle, m_SubmitFence, nullptr );
            m_SubmitFence = VK_NULL_HANDLE;
        }

        m_CurrentFrame = 0;
        m_pDevice = nullptr;
    }

    /***********************************************************************************\

    Function:
        IsAvailable

    Description:
        Check if profiler has been initialized for this device.

    \***********************************************************************************/
    bool DeviceProfiler::IsAvailable() const
    {
        return m_pDevice != nullptr;
    }

    /***********************************************************************************\

    \***********************************************************************************/
    VkResult DeviceProfiler::SetMode( VkProfilerModeEXT mode )
    {
        // TODO: Invalidate all command buffers
        m_Config.m_Mode = mode;

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        SetSyncMode

    Description:
        Set synchronization mode used to wait for data from the GPU.
        VK_PROFILER_SYNC_MODE_PRESENT_EXT - Wait on vkQueuePresentKHR
        VK_PROFILER_SYNC_MODE_SUBMIT_EXT - Wait on vkQueueSumit

    \***********************************************************************************/
    VkResult DeviceProfiler::SetSyncMode( VkProfilerSyncModeEXT syncMode )
    {
        // Check if synchronization mode is supported by current implementation
        if( syncMode != VK_PROFILER_SYNC_MODE_PRESENT_EXT &&
            syncMode != VK_PROFILER_SYNC_MODE_SUBMIT_EXT )
        {
            return VK_ERROR_VALIDATION_FAILED_EXT;
        }

        m_Config.m_SyncMode = syncMode;

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    \***********************************************************************************/
    DeviceProfilerFrameData DeviceProfiler::GetData() const
    {
        // Hold aggregator updates to keep m_Data consistent
        std::scoped_lock lk( m_DataMutex );
        return m_Data;
    }

    /***********************************************************************************\
    \***********************************************************************************/
    ProfilerCommandBuffer& DeviceProfiler::GetCommandBuffer( VkCommandBuffer commandBuffer )
    {
        return m_CommandBuffers.at( commandBuffer );
    }

    /***********************************************************************************\
    \***********************************************************************************/
    DeviceProfilerCommandPool& DeviceProfiler::GetCommandPool( VkCommandPool commandPool )
    {
        return m_CommandPools.at( commandPool );
    }

    /***********************************************************************************\
    \***********************************************************************************/
    DeviceProfilerPipeline& DeviceProfiler::GetPipeline( VkPipeline pipeline )
    {
        return m_Pipelines.at( pipeline );
    }

    /***********************************************************************************\
    \***********************************************************************************/
    DeviceProfilerRenderPass& DeviceProfiler::GetRenderPass( VkRenderPass renderPass )
    {
        return m_RenderPasses.at( renderPass );
    }

    /***********************************************************************************\

    Function:
        CreateCommandPool

    Description:
        Create wrapper for VkCommandPool object.

    \***********************************************************************************/
    void DeviceProfiler::CreateCommandPool( VkCommandPool commandPool, const VkCommandPoolCreateInfo* pCreateInfo )
    {
        m_CommandPools.insert( commandPool,
            DeviceProfilerCommandPool( *this, commandPool, *pCreateInfo ) );
    }

    /***********************************************************************************\

    Function:
        DestroyCommandPool

    Description:
        Destroy wrapper for VkCommandPool object and all command buffers allocated from
        that pool.

    \***********************************************************************************/
    void DeviceProfiler::DestroyCommandPool( VkCommandPool commandPool )
    {
        std::scoped_lock lk( m_SubmitMutex, m_PresentMutex, m_CommandBuffers );

        for( auto it = m_CommandBuffers.begin(); it != m_CommandBuffers.end(); )
        {
            it = (it->second.GetCommandPool().GetHandle() == commandPool)
                ? FreeCommandBuffer( it )
                : std::next( it );
        }

        m_CommandPools.remove( commandPool );
    }

    /***********************************************************************************\

    Function:
        RegisterCommandBuffers

    Description:
        Create wrappers for VkCommandBuffer objects.

    \***********************************************************************************/
    void DeviceProfiler::AllocateCommandBuffers( VkCommandPool commandPool, VkCommandBufferLevel level, uint32_t count, VkCommandBuffer* pCommandBuffers )
    {
        std::scoped_lock lk( m_SubmitMutex, m_PresentMutex, m_CommandBuffers );

        DeviceProfilerCommandPool& profilerCommandPool = GetCommandPool( commandPool );

        for( uint32_t i = 0; i < count; ++i )
        {
            VkCommandBuffer commandBuffer = pCommandBuffers[ i ];

            SetDefaultObjectName( commandBuffer );

            m_CommandBuffers.unsafe_insert( commandBuffer,
                ProfilerCommandBuffer( *this, profilerCommandPool, commandBuffer, level ) );
        }
    }

    /***********************************************************************************\

    Function:
        UnregisterCommandBuffers

    Description:
        Destroy wrappers for VkCommandBuffer objects.

    \***********************************************************************************/
    void DeviceProfiler::FreeCommandBuffers( uint32_t count, const VkCommandBuffer* pCommandBuffers )
    {
        std::scoped_lock lk( m_SubmitMutex, m_PresentMutex, m_CommandBuffers );

        for( uint32_t i = 0; i < count; ++i )
        {
            FreeCommandBuffer( pCommandBuffers[ i ] );
        }
    }

    /***********************************************************************************\

    Function:
        CreatePipelines

    Description:
        Register graphics pipelines.

    \***********************************************************************************/
    void DeviceProfiler::CreatePipelines( uint32_t pipelineCount, const VkGraphicsPipelineCreateInfo* pCreateInfos, VkPipeline* pPipelines )
    {
        for( uint32_t i = 0; i < pipelineCount; ++i )
        {
            DeviceProfilerPipeline profilerPipeline;
            profilerPipeline.m_Handle = pPipelines[i];
            profilerPipeline.m_ShaderTuple = CreateShaderTuple( pCreateInfos[i] );
            profilerPipeline.m_BindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

            SetDefaultObjectName( profilerPipeline );

            m_Pipelines.insert( pPipelines[i], profilerPipeline );
        }
    }

    /***********************************************************************************\

    Function:
        CreatePipelines

    Description:
        Register compute pipelines.

    \***********************************************************************************/
    void DeviceProfiler::CreatePipelines( uint32_t pipelineCount, const VkComputePipelineCreateInfo* pCreateInfos, VkPipeline* pPipelines )
    {
        for( uint32_t i = 0; i < pipelineCount; ++i )
        {
            DeviceProfilerPipeline profilerPipeline;
            profilerPipeline.m_Handle = pPipelines[ i ];
            profilerPipeline.m_ShaderTuple = CreateShaderTuple( pCreateInfos[ i ] );
            profilerPipeline.m_BindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;

            SetDefaultObjectName( profilerPipeline );

            m_Pipelines.insert( pPipelines[ i ], profilerPipeline );
        }
    }

    /***********************************************************************************\

    Function:
        DestroyPipeline

    Description:

    \***********************************************************************************/
    void DeviceProfiler::DestroyPipeline( VkPipeline pipeline )
    {
        m_Pipelines.remove( pipeline );
    }

    /***********************************************************************************\

    Function:
        CreateShaderModule

    Description:

    \***********************************************************************************/
    void DeviceProfiler::CreateShaderModule( VkShaderModule module, const VkShaderModuleCreateInfo* pCreateInfo )
    {
        // Compute shader code hash to use later
        const uint32_t hash = Hash::Fingerprint32( reinterpret_cast<const char*>(pCreateInfo->pCode), pCreateInfo->codeSize );

        m_ShaderModuleHashes.insert( module, hash );
    }

    /***********************************************************************************\

    Function:
        DestroyShaderModule

    Description:

    \***********************************************************************************/
    void DeviceProfiler::DestroyShaderModule( VkShaderModule module )
    {
        m_ShaderModuleHashes.remove( module );
    }

    /***********************************************************************************\

    Function:
        CreateRenderPass

    Description:

    \***********************************************************************************/
    void DeviceProfiler::CreateRenderPass( VkRenderPass renderPass, const VkRenderPassCreateInfo* pCreateInfo )
    {
        DeviceProfilerRenderPass deviceProfilerRenderPass;
        deviceProfilerRenderPass.m_Handle = renderPass;
        
        for( uint32_t subpassIndex = 0; subpassIndex < pCreateInfo->subpassCount; ++subpassIndex )
        {
            const VkSubpassDescription& subpass = pCreateInfo->pSubpasses[ subpassIndex ];

            DeviceProfilerSubpass deviceProfilerSubpass;
            deviceProfilerSubpass.m_Index = subpassIndex;
            
            // Check if this subpass resolves any attachments at the end
            CountSubpassAttachmentResolves( deviceProfilerSubpass, subpass );

            deviceProfilerRenderPass.m_Subpasses.push_back( deviceProfilerSubpass );
        }

        // Count clear attachments
        CountRenderPassAttachmentClears( deviceProfilerRenderPass, pCreateInfo );

        // Assign default debug name for the render pass
        SetDefaultObjectName( renderPass );

        // Store render pass
        m_RenderPasses.insert( renderPass, deviceProfilerRenderPass );
    }

    /***********************************************************************************\

    Function:
        CreateRenderPass

    Description:

    \***********************************************************************************/
    void DeviceProfiler::CreateRenderPass( VkRenderPass renderPass, const VkRenderPassCreateInfo2* pCreateInfo )
    {
        DeviceProfilerRenderPass deviceProfilerRenderPass;
        deviceProfilerRenderPass.m_Handle = renderPass;

        for( uint32_t subpassIndex = 0; subpassIndex < pCreateInfo->subpassCount; ++subpassIndex )
        {
            const VkSubpassDescription2& subpass = pCreateInfo->pSubpasses[ subpassIndex ];

            DeviceProfilerSubpass deviceProfilerSubpass;
            deviceProfilerSubpass.m_Index = subpassIndex;

            // Check if this subpass resolves any attachments at the end
            CountSubpassAttachmentResolves( deviceProfilerSubpass, subpass );

            // Check if this subpass resolves depth-stencil attachment
            for( const auto& it : PNextIterator( subpass.pNext ) )
            {
                if( it.sType == VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE )
                {
                    const VkSubpassDescriptionDepthStencilResolve& depthStencilResolve =
                        reinterpret_cast<const VkSubpassDescriptionDepthStencilResolve&>(it);

                    // Check if depth-stencil resolve is actually enabled for this subpass
                    if( (depthStencilResolve.pDepthStencilResolveAttachment) &&
                        (depthStencilResolve.pDepthStencilResolveAttachment->attachment != VK_ATTACHMENT_UNUSED) )
                    {
                        if( (depthStencilResolve.depthResolveMode != VK_RESOLVE_MODE_NONE) ||
                            (depthStencilResolve.stencilResolveMode != VK_RESOLVE_MODE_NONE) )
                        {
                            deviceProfilerSubpass.m_ResolveCount++;
                        }

                        // Check if independent resolve is used - it will count as 2 resolves
                        if( (depthStencilResolve.depthResolveMode != VK_RESOLVE_MODE_NONE) &&
                            (depthStencilResolve.stencilResolveMode != VK_RESOLVE_MODE_NONE) &&
                            (depthStencilResolve.stencilResolveMode != depthStencilResolve.depthResolveMode) )
                        {
                            deviceProfilerSubpass.m_ResolveCount++;
                        }
                    }
                }
            }

            deviceProfilerRenderPass.m_Subpasses.push_back( deviceProfilerSubpass );
        }

        // Count clear attachments
        CountRenderPassAttachmentClears( deviceProfilerRenderPass, pCreateInfo );

        // Assign default debug name for the render pass
        SetDefaultObjectName( renderPass );

        // Store render pass
        m_RenderPasses.insert( renderPass, deviceProfilerRenderPass );
    }

    /***********************************************************************************\

    Function:
        DestroyRenderPass

    Description:

    \***********************************************************************************/
    void DeviceProfiler::DestroyRenderPass( VkRenderPass renderPass )
    {
        m_RenderPasses.remove( renderPass );
    }

    /***********************************************************************************\

    Function:
        PreSubmitCommandBuffers

    Description:

    \***********************************************************************************/
    void DeviceProfiler::PreSubmitCommandBuffers( VkQueue queue, uint32_t, const VkSubmitInfo*, VkFence )
    {
        assert( m_PerformanceConfigurationINTEL == VK_NULL_HANDLE );

        if( m_MetricsApiINTEL.IsAvailable() )
        {
            VkResult result;

            // Acquire performance configuration
            {
                VkPerformanceConfigurationAcquireInfoINTEL acquireInfo = {};
                acquireInfo.sType = VK_STRUCTURE_TYPE_PERFORMANCE_CONFIGURATION_ACQUIRE_INFO_INTEL;
                acquireInfo.type = VK_PERFORMANCE_CONFIGURATION_TYPE_COMMAND_QUEUE_METRICS_DISCOVERY_ACTIVATED_INTEL;

                result = m_pDevice->Callbacks.AcquirePerformanceConfigurationINTEL(
                    m_pDevice->Handle,
                    &acquireInfo,
                    &m_PerformanceConfigurationINTEL );
            }

            // Set performance configuration for the queue
            if( result == VK_SUCCESS )
            {
                result = m_pDevice->Callbacks.QueueSetPerformanceConfigurationINTEL(
                    queue, m_PerformanceConfigurationINTEL );
            }

            assert( result == VK_SUCCESS );
        }
    }

    /***********************************************************************************\

    Function:
        PostSubmitCommandBuffers

    Description:

    \***********************************************************************************/
    void DeviceProfiler::PostSubmitCommandBuffers( VkQueue queue, uint32_t count, const VkSubmitInfo* pSubmitInfo, VkFence fence )
    {
        #if PROFILER_DISABLE_CRITICAL_SECTION_OPTIMIZATION
        std::scoped_lock lk( m_CommandBuffers );
        #else
        std::scoped_lock lk( m_SubmitMutex );
        #endif

        // Wait for the submitted command buffers to execute
        if( m_Config.m_SyncMode == VK_PROFILER_SYNC_MODE_SUBMIT_EXT )
        {
            m_pDevice->Callbacks.QueueSubmit( queue, 0, nullptr, m_SubmitFence );
            m_pDevice->Callbacks.WaitForFences( m_pDevice->Handle, 1, &m_SubmitFence, true, std::numeric_limits<uint64_t>::max() );
            m_pDevice->Callbacks.ResetFences( m_pDevice->Handle, 1, &m_SubmitFence );
        }

        // Store submitted command buffers and get results
        DeviceProfilerSubmitBatch submitBatch;
        submitBatch.m_Handle = queue;
        submitBatch.m_Timestamp = m_CpuTimestampCounter.GetCurrentValue();
        submitBatch.m_ThreadId = ProfilerPlatformFunctions::GetCurrentThreadId();

        for( uint32_t submitIdx = 0; submitIdx < count; ++submitIdx )
        {
            const VkSubmitInfo& submitInfo = pSubmitInfo[submitIdx];

            // Wrap submit info into our structure
            DeviceProfilerSubmit submit;

            for( uint32_t commandBufferIdx = 0; commandBufferIdx < submitInfo.commandBufferCount; ++commandBufferIdx )
            {
                // Get command buffer handle
                VkCommandBuffer commandBuffer = submitInfo.pCommandBuffers[commandBufferIdx];
                auto& profilerCommandBuffer = GetCommandBuffer( commandBuffer );

                // Dirty command buffer profiling data
                profilerCommandBuffer.Submit();

                submit.m_pCommandBuffers.push_back( &profilerCommandBuffer );
            }

            // Store the submit wrapper
            submitBatch.m_Submits.push_back( submit );
        }

        m_DataAggregator.AppendSubmit( submitBatch );

        // Release performance configuration
        if( m_PerformanceConfigurationINTEL )
        {
            assert( m_pDevice->Callbacks.ReleasePerformanceConfigurationINTEL );

            VkResult result = m_pDevice->Callbacks.ReleasePerformanceConfigurationINTEL(
                m_pDevice->Handle, m_PerformanceConfigurationINTEL );

            assert( result == VK_SUCCESS );

            // Reset object handle for the next submit
            m_PerformanceConfigurationINTEL = VK_NULL_HANDLE;
        }

        if( m_Config.m_SyncMode == VK_PROFILER_SYNC_MODE_SUBMIT_EXT )
        {
            // Collect data from the submitted command buffers
            m_DataAggregator.Aggregate();
        }
    }

    /***********************************************************************************\

    Function:
        FinishFrame

    Description:

    \***********************************************************************************/
    void DeviceProfiler::FinishFrame()
    {
        std::scoped_lock lk( m_PresentMutex );

        // Update FPS counter
        const bool updatePerfCounters = m_CpuFpsCounter.Update();

        m_CurrentFrame++;

        if( m_Config.m_SyncMode == VK_PROFILER_SYNC_MODE_PRESENT_EXT )
        {
            // Doesn't introduce in-frame CPU overhead but may cause some image-count-related issues disappear
            m_pDevice->Callbacks.DeviceWaitIdle( m_pDevice->Handle );

            // Collect data from the submitted command buffers
            m_DataAggregator.Aggregate();
        }

        {
            std::scoped_lock lk2( m_DataMutex );

            // Get data captured during the last frame
            m_Data = m_DataAggregator.GetAggregatedData();
        }

        // TODO: Move to memory tracker
        m_Data.m_Memory = m_MemoryData;

        m_CpuTimestampCounter.End();

        // TODO: Move to CPU tracker
        m_Data.m_CPU.m_BeginTimestamp = m_CpuTimestampCounter.GetBeginValue();
        m_Data.m_CPU.m_EndTimestamp = m_CpuTimestampCounter.GetCurrentValue();
        m_Data.m_CPU.m_FramesPerSec = m_CpuFpsCounter.GetValue();
        m_Data.m_CPU.m_ThreadId = ProfilerPlatformFunctions::GetCurrentThreadId();

        m_CpuTimestampCounter.Begin();

        // Prepare aggregator for the next frame
        m_DataAggregator.Reset();
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:

    \***********************************************************************************/
    void DeviceProfiler::AllocateMemory( VkDeviceMemory allocatedMemory, const VkMemoryAllocateInfo* pAllocateInfo )
    {
        std::scoped_lock lk( m_Allocations );

        // Insert allocation info to the map, it will be needed during deallocation.
        m_Allocations.unsafe_insert( allocatedMemory, *pAllocateInfo );

        const VkMemoryType& memoryType =
            m_pDevice->pPhysicalDevice->MemoryProperties.memoryTypes[ pAllocateInfo->memoryTypeIndex ];

        auto& heap = m_MemoryData.m_Heaps[ memoryType.heapIndex ];
        heap.m_AllocationCount++;
        heap.m_AllocationSize += pAllocateInfo->allocationSize;

        auto& type = m_MemoryData.m_Types[ pAllocateInfo->memoryTypeIndex ];
        type.m_AllocationCount++;
        type.m_AllocationSize += pAllocateInfo->allocationSize;

        m_MemoryData.m_TotalAllocationCount++;
        m_MemoryData.m_TotalAllocationSize += pAllocateInfo->allocationSize;
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:

    \***********************************************************************************/
    void DeviceProfiler::FreeMemory( VkDeviceMemory allocatedMemory )
    {
        std::scoped_lock lk( m_Allocations );

        auto it = m_Allocations.unsafe_find( allocatedMemory );
        if( it != m_Allocations.end() )
        {
            const VkMemoryType& memoryType =
                m_pDevice->pPhysicalDevice->MemoryProperties.memoryTypes[ it->second.memoryTypeIndex ];

            auto& heap = m_MemoryData.m_Heaps[ memoryType.heapIndex ];
            heap.m_AllocationCount--;
            heap.m_AllocationSize -= it->second.allocationSize;

            auto& type = m_MemoryData.m_Types[ it->second.memoryTypeIndex ];
            type.m_AllocationCount--;
            type.m_AllocationSize -= it->second.allocationSize;

            m_MemoryData.m_TotalAllocationCount--;
            m_MemoryData.m_TotalAllocationSize -= it->second.allocationSize;

            // Remove allocation entry from the map
            m_Allocations.unsafe_remove( it );
        }
    }

    /***********************************************************************************\

    Function:
        CreateShaderTuple

    Description:

    \***********************************************************************************/
    ProfilerShaderTuple DeviceProfiler::CreateShaderTuple( const VkGraphicsPipelineCreateInfo& createInfo )
    {
        ProfilerShaderTuple tuple;

        for( uint32_t i = 0; i < createInfo.stageCount; ++i )
        {
            // VkShaderModule entry should already be in the map
            uint32_t hash = m_ShaderModuleHashes.at( createInfo.pStages[i].module );

            const char* entrypoint = createInfo.pStages[i].pName;

            // Hash the entrypoint and append it to the final hash
            hash ^= Hash::Fingerprint32( entrypoint, std::strlen( entrypoint ) );

            switch( createInfo.pStages[i].stage )
            {
            case VK_SHADER_STAGE_VERTEX_BIT: tuple.m_Vert = hash; break;
            case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT: tuple.m_Tesc = hash; break;
            case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: tuple.m_Tese = hash; break;
            case VK_SHADER_STAGE_GEOMETRY_BIT: tuple.m_Geom = hash; break;
            case VK_SHADER_STAGE_FRAGMENT_BIT: tuple.m_Frag = hash; break;

            default:
            {
                // Break in debug builds
                assert( !"Usupported graphics shader stage" );
            }
            }
        }

        // Compute aggregated tuple hash for fast comparison
        tuple.m_Hash = Hash::Fingerprint32( reinterpret_cast<const char*>(&tuple), sizeof( tuple ) );

        return tuple;
    }

    /***********************************************************************************\

    Function:
        CreateShaderTuple

    Description:

    \***********************************************************************************/
    ProfilerShaderTuple DeviceProfiler::CreateShaderTuple( const VkComputePipelineCreateInfo& createInfo )
    {
        ProfilerShaderTuple tuple;

        // VkShaderModule entry should already be in the map
        uint32_t hash = m_ShaderModuleHashes.at( createInfo.stage.module );

        const char* entrypoint = createInfo.stage.pName;

        // Hash the entrypoint and append it to the final hash
        hash ^= Hash::Fingerprint32( entrypoint, std::strlen( entrypoint ) );

        // This should be checked in validation layers
        assert( createInfo.stage.stage == VK_SHADER_STAGE_COMPUTE_BIT );

        tuple.m_Comp = hash;

        // Aggregated tuple hash for fast comparison
        tuple.m_Hash = hash;

        return tuple;
    }

    /***********************************************************************************\

    Function:
        SetObjectName

    Description:
        Set custom object name.

    \***********************************************************************************/
    void DeviceProfiler::SetObjectName( VkObject object, const char* pName )
    {
        m_pDevice->Debug.ObjectNames.insert_or_assign( object, pName );
    }

    /***********************************************************************************\

    Function:
        SetDefaultObjectName

    Description:
        Set default object name.

    \***********************************************************************************/
    void DeviceProfiler::SetDefaultObjectName( VkObject object )
    {
        // There is special function for VkPipeline objects
        if( object.m_Type == VK_OBJECT_TYPE_PIPELINE )
        {
            SetDefaultObjectName( VkObject_Traits<VkPipeline>::GetObjectHandleAsVulkanHandle( object.m_Handle ) );
        }

        char pObjectDebugName[ 64 ] = {};
        sprintf_s( pObjectDebugName, "%s 0x%016llx", object.m_pTypeName, object.m_Handle );

        m_pDevice->Debug.ObjectNames.insert_or_assign( object, pObjectDebugName );
    }

    /***********************************************************************************\

    Function:
        SetDefaultObjectName

    Description:
        Set default pipeline name consisting of shader tuple hashes.

    \***********************************************************************************/
    void DeviceProfiler::SetDefaultObjectName( VkPipeline object )
    {
        SetDefaultObjectName( GetPipeline( object ) );
    }

    /***********************************************************************************\

    Function:
        SetDefaultObjectName

    Description:
        Set default pipeline name consisting of shader tuple hashes.

    \***********************************************************************************/
    void DeviceProfiler::SetDefaultObjectName( const DeviceProfilerPipeline& pipeline )
    {
        if( pipeline.m_BindPoint == VK_PIPELINE_BIND_POINT_GRAPHICS )
        {
            // Vertex and pixel shader hashes
            char pPipelineDebugName[ 25 ] = "VS=XXXXXXXX, PS=XXXXXXXX";
            u32tohex( pPipelineDebugName + 3, pipeline.m_ShaderTuple.m_Vert );
            u32tohex( pPipelineDebugName + 16, pipeline.m_ShaderTuple.m_Frag );

            m_pDevice->Debug.ObjectNames.insert_or_assign( pipeline.m_Handle, pPipelineDebugName );
        }

        if( pipeline.m_BindPoint == VK_PIPELINE_BIND_POINT_COMPUTE )
        {
            // Compute shader hash
            char pPipelineDebugName[ 12 ] = "CS=XXXXXXXX";
            u32tohex( pPipelineDebugName + 3, pipeline.m_ShaderTuple.m_Comp );

            m_pDevice->Debug.ObjectNames.insert_or_assign( pipeline.m_Handle, pPipelineDebugName );
        }
    }

    /***********************************************************************************\

    Function:
        CreateInternalPipeline

    Description:
        Create internal pipeline to track drawcalls which don't require any user-provided
        pipelines but execude some tasks on the GPU.

    \***********************************************************************************/
    void DeviceProfiler::CreateInternalPipeline( DeviceProfilerPipelineType type, const char* pName )
    {
        DeviceProfilerPipeline internalPipeline;
        internalPipeline.m_Handle = (VkPipeline)type;
        internalPipeline.m_ShaderTuple.m_Hash = (uint32_t)type;

        // Assign name for the internal pipeline
        SetObjectName( internalPipeline.m_Handle, pName );

        m_Pipelines.insert( internalPipeline.m_Handle, internalPipeline );
    }

    /***********************************************************************************\

    Function:
        FreeCommandBuffer

    Description:

    \***********************************************************************************/
    decltype(DeviceProfiler::m_CommandBuffers)::iterator DeviceProfiler::FreeCommandBuffer( VkCommandBuffer commandBuffer )
    {
        // Assume m_CommandBuffers map is already locked
        assert( !m_CommandBuffers.try_lock() );

        auto it = m_CommandBuffers.unsafe_find( commandBuffer );

        // Collect command buffer data now, command buffer won't be available later
        m_DataAggregator.AppendData( &it->second, it->second.GetData() );

        return m_CommandBuffers.unsafe_remove( it );
    }

    /***********************************************************************************\

    Function:
        FreeCommandBuffer

    Description:

    \***********************************************************************************/
    decltype(DeviceProfiler::m_CommandBuffers)::iterator DeviceProfiler::FreeCommandBuffer( decltype(m_CommandBuffers)::iterator it )
    {
        // Assume m_CommandBuffers map is already locked
        assert( !m_CommandBuffers.try_lock() );

        // Collect command buffer data now, command buffer won't be available later
        m_DataAggregator.AppendData( &it->second, it->second.GetData() );

        return m_CommandBuffers.unsafe_remove( it );
    }
}

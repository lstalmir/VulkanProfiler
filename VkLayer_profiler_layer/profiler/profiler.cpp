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
        , m_DataMutex()
        , m_Data()
        , m_DataAggregator()
        , m_CurrentFrame( 0 )
        , m_CpuTimestampCounter()
        , m_CpuFpsCounter()
        , m_Allocations()
        , m_DeviceLocalAllocatedMemorySize( 0 )
        , m_DeviceLocalAllocationCount( 0 )
        , m_HostVisibleAllocatedMemorySize( 0 )
        , m_HostVisibleAllocationCount( 0 )
        , m_CommandBuffers()
        , m_TimestampPeriod( 0.0f )
        , m_PerformanceConfigurationINTEL( VK_NULL_HANDLE )
    {
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

        if( result != VK_SUCCESS )
        {
            // Fence creation failed
            Destroy();
            return result;
        }

        // Get GPU timestamp period
        m_pDevice->pInstance->Callbacks.GetPhysicalDeviceProperties(
            m_pDevice->PhysicalDevice, &m_DeviceProperties );

        m_TimestampPeriod = m_DeviceProperties.limits.timestampPeriod;

        // Enable vendor-specific extensions
        switch( pDevice->VendorID )
        {
        case VkDevice_Vendor_ID::eINTEL:
        {
            InitializeINTEL();
            break;
        }
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
        CreateInternalPipeline( DeviceProfilerPipelineType::eUpdatBuffer, "UpdateBuffer" );
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

    Function:
        RegisterCommandBuffers

    Description:
        Create wrappers for VkCommandBuffer objects.

    \***********************************************************************************/
    void DeviceProfiler::AllocateCommandBuffers( VkCommandPool commandPool, VkCommandBufferLevel level, uint32_t count, VkCommandBuffer* pCommandBuffers )
    {
        std::scoped_lock lk( m_CommandBuffers );

        for( uint32_t i = 0; i < count; ++i )
        {
            VkCommandBuffer commandBuffer = pCommandBuffers[ i ];

            m_CommandBuffers.try_emplace( commandBuffer,
                std::ref( *this ),
                commandPool,
                commandBuffer,
                level );
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
        std::scoped_lock lk( m_CommandBuffers );

        for( uint32_t i = 0; i < count; ++i )
        {
            FreeCommandBuffer( pCommandBuffers[ i ] );
        }
    }

    /***********************************************************************************\

    Function:
        UnregisterCommandBuffers

    Description:
        Destroy all command buffer wrappers allocated in the commandPool.

    \***********************************************************************************/
    void DeviceProfiler::FreeCommandBuffers( VkCommandPool commandPool )
    {
        std::scoped_lock lk( m_CommandBuffers );

        for( auto it = m_CommandBuffers.begin(); it != m_CommandBuffers.end(); )
        {
            it = (it->second.GetCommandPool() == commandPool)
                ? FreeCommandBuffer( it )
                : std::next( it );
        }
    }

    /***********************************************************************************\
    \***********************************************************************************/
    ProfilerCommandBuffer& DeviceProfiler::GetCommandBuffer( VkCommandBuffer commandBuffer )
    {
        CpuScopedTimestampCounter<std::chrono::nanoseconds> counter( m_CommandBufferLookupTimeNs );
        return m_CommandBuffers.interlocked_at( commandBuffer );
    }

    /***********************************************************************************\
    \***********************************************************************************/
    DeviceProfilerPipeline& DeviceProfiler::GetPipeline( VkPipeline pipeline )
    {
        CpuScopedTimestampCounter<std::chrono::nanoseconds> counter( m_PipelineLookupTimeNs );
        return m_Pipelines.interlocked_at( pipeline );
    }

    /***********************************************************************************\
    \***********************************************************************************/
    DeviceProfilerRenderPass& DeviceProfiler::GetRenderPass( VkRenderPass renderPass )
    {
        CpuScopedTimestampCounter<std::chrono::nanoseconds> counter( m_RenderPassLookupTimeNs );
        return m_RenderPasses.interlocked_at( renderPass );
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

            SetDefaultPipelineObjectName( profilerPipeline );

            m_Pipelines.interlocked_emplace( pPipelines[i], profilerPipeline );
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

            SetDefaultPipelineObjectName( profilerPipeline );

            m_Pipelines.interlocked_emplace( pPipelines[ i ], profilerPipeline );
        }
    }

    /***********************************************************************************\

    Function:
        DestroyPipeline

    Description:

    \***********************************************************************************/
    void DeviceProfiler::DestroyPipeline( VkPipeline pipeline )
    {
        m_Pipelines.interlocked_erase( pipeline );
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

        m_ShaderModuleHashes.interlocked_emplace( module, hash );
    }

    /***********************************************************************************\

    Function:
        DestroyShaderModule

    Description:

    \***********************************************************************************/
    void DeviceProfiler::DestroyShaderModule( VkShaderModule module )
    {
        m_ShaderModuleHashes.interlocked_erase( module );
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

        // Store render pass
        m_RenderPasses.interlocked_emplace( renderPass, deviceProfilerRenderPass );
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

        // Store render pass
        m_RenderPasses.interlocked_emplace( renderPass, deviceProfilerRenderPass );
    }

    /***********************************************************************************\

    Function:
        DestroyRenderPass

    Description:

    \***********************************************************************************/
    void DeviceProfiler::DestroyRenderPass( VkRenderPass renderPass )
    {
        m_RenderPasses.interlocked_erase( renderPass );
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
        CpuTimestampCounter commandBufferLookupTimeCounter;

        // Block access from other threads
        std::scoped_lock lk( m_CommandBuffers );

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

        for( uint32_t submitIdx = 0; submitIdx < count; ++submitIdx )
        {
            const VkSubmitInfo& submitInfo = pSubmitInfo[submitIdx];

            // Wrap submit info into our structure
            DeviceProfilerSubmit submit;

            for( uint32_t commandBufferIdx = 0; commandBufferIdx < submitInfo.commandBufferCount; ++commandBufferIdx )
            {
                // Get command buffer handle
                VkCommandBuffer commandBuffer = submitInfo.pCommandBuffers[commandBufferIdx];

                commandBufferLookupTimeCounter.Begin();
                auto& profilerCommandBuffer = m_CommandBuffers.at( commandBuffer );
                commandBufferLookupTimeCounter.End();

                // Aggregate command buffer lookup time
                m_CommandBufferLookupTimeNs +=
                    commandBufferLookupTimeCounter.GetValue<std::chrono::nanoseconds>().count();

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
    }

    /***********************************************************************************\

    Function:
        PostPresent

    Description:

    \***********************************************************************************/
    void DeviceProfiler::Present( const VkQueue_Object& queue, VkPresentInfoKHR* pPresentInfo )
    {
        m_CpuTimestampCounter.End();

        // Update FPS counter
        m_CpuFpsCounter.Update();

        m_CurrentFrame++;

        if( m_Config.m_SyncMode == VK_PROFILER_SYNC_MODE_PRESENT_EXT )
        {
            // Doesn't introduce in-frame CPU overhead but may cause some image-count-related issues disappear
            m_pDevice->Callbacks.DeviceWaitIdle( m_pDevice->Handle );
        }

        // TMP
        std::scoped_lock lk( m_DataMutex );
        m_Data = m_DataAggregator.GetAggregatedData();

        // TODO: Move to CPU tracker
        m_Data.m_CPU.m_TimeNs = m_CpuTimestampCounter.GetValue<std::chrono::nanoseconds>().count();
        m_Data.m_CPU.m_FramesPerSec = m_CpuFpsCounter.GetValue();

        // TODO: Move to memory tracker
        m_Data.m_Memory.m_TotalAllocationCount = m_TotalAllocationCount;
        m_Data.m_Memory.m_TotalAllocationSize = m_TotalAllocatedMemorySize;
        m_Data.m_Memory.m_DeviceLocalAllocationSize = m_DeviceLocalAllocatedMemorySize;
        m_Data.m_Memory.m_HostVisibleAllocationSize = m_HostVisibleAllocatedMemorySize;

        // TODO: Move to memory tracker
        //ClearStructure( &m_MemoryProperties2, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2 );
        //m_pDevice->pInstance->Callbacks.GetPhysicalDeviceMemoryProperties2KHR(
        //    m_pDevice->PhysicalDevice,
        //    &m_MemoryProperties2 );

        m_Data.m_CPU.m_CommandBufferLookupTimeNs += m_CommandBufferLookupTimeNs;
        m_Data.m_CPU.m_PipelineLookupTimeNs += m_PipelineLookupTimeNs;
        m_Data.m_CPU.m_RenderPassLookupTimeNs += m_RenderPassLookupTimeNs;
        
        // Reset self data for the next frame
        m_CommandBufferLookupTimeNs = 0;
        m_PipelineLookupTimeNs = 0;
        m_RenderPassLookupTimeNs = 0;

        // TMP
        m_DataAggregator.Reset();

        m_CpuTimestampCounter.Begin();
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:

    \***********************************************************************************/
    void DeviceProfiler::OnAllocateMemory( VkDeviceMemory allocatedMemory, const VkMemoryAllocateInfo* pAllocateInfo )
    {
        // Insert allocation info to the map, it will be needed during deallocation.
        m_Allocations.emplace( allocatedMemory, *pAllocateInfo );

        const VkMemoryType& memoryType =
            m_MemoryProperties.memoryTypes[ pAllocateInfo->memoryTypeIndex ];

        if( memoryType.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT )
        {
            m_DeviceLocalAllocationCount++;
            m_DeviceLocalAllocatedMemorySize += pAllocateInfo->allocationSize;
        }

        if( memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT )
        {
            m_HostVisibleAllocationCount++;
            m_HostVisibleAllocatedMemorySize += pAllocateInfo->allocationSize;
        }
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:

    \***********************************************************************************/
    void DeviceProfiler::OnFreeMemory( VkDeviceMemory allocatedMemory )
    {
        auto it = m_Allocations.find( allocatedMemory );
        if( it != m_Allocations.end() )
        {
            const VkMemoryType& memoryType =
                m_MemoryProperties.memoryTypes[ it->second.memoryTypeIndex ];

            if( memoryType.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT )
            {
                m_DeviceLocalAllocationCount--;
                m_DeviceLocalAllocatedMemorySize -= it->second.allocationSize;
            }

            if( memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT )
            {
                m_HostVisibleAllocationCount--;
                m_HostVisibleAllocatedMemorySize -= it->second.allocationSize;
            }

            // Remove allocation entry from the map
            m_Allocations.erase( it );
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
            uint32_t hash = m_ShaderModuleHashes.interlocked_at( createInfo.pStages[i].module );

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
        uint32_t hash = m_ShaderModuleHashes.interlocked_at( createInfo.stage.module );

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
        SetDefaultPipelineObjectName

    Description:
        Set default pipeline name consisting of shader tuple hashes.

    \***********************************************************************************/
    void DeviceProfiler::SetDefaultPipelineObjectName( const DeviceProfilerPipeline& pipeline )
    {
        if( pipeline.m_BindPoint == VK_PIPELINE_BIND_POINT_GRAPHICS )
        {
            // Vertex and pixel shader hashes
            char pPipelineDebugName[ 24 ] = "VS=XXXXXXXX,PS=XXXXXXXX";
            u32tohex( pPipelineDebugName + 3, pipeline.m_ShaderTuple.m_Vert );
            u32tohex( pPipelineDebugName + 15, pipeline.m_ShaderTuple.m_Frag );

            m_pDevice->Debug.ObjectNames.emplace( (uint64_t)pipeline.m_Handle, pPipelineDebugName );
        }

        if( pipeline.m_BindPoint == VK_PIPELINE_BIND_POINT_COMPUTE )
        {
            // Compute shader hash
            char pPipelineDebugName[ 12 ] = "CS=XXXXXXXX";
            u32tohex( pPipelineDebugName + 3, pipeline.m_ShaderTuple.m_Comp );

            m_pDevice->Debug.ObjectNames.emplace( (uint64_t)pipeline.m_Handle, pPipelineDebugName );
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

        [[maybe_unused]]
        auto result = m_pDevice->Debug.ObjectNames.emplace( (uint64_t)internalPipeline.m_Handle, pName );

        // Check if new value has been created
        assert( result.second && "Multiple initialization of internal pipeline - possible hash conflict" );

        m_Pipelines.interlocked_emplace( internalPipeline.m_Handle, internalPipeline );
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

        auto it = m_CommandBuffers.find( commandBuffer );
        if( it != m_CommandBuffers.end() )
        {
            // Collect command buffer data now, command buffer won't be available later
            m_DataAggregator.AppendData( &it->second, it->second.GetData() );
        }

        return m_CommandBuffers.erase( it );
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

        return m_CommandBuffers.erase( it );
    }
}

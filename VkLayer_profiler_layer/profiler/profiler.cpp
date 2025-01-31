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

#include "profiler.h"
#include "profiler_command_buffer.h"
#include "profiler_helpers.h"
#include <farmhash.h>
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

    template<typename SubmitInfo>
    struct SubmitInfoTraits;

    template<>
    struct SubmitInfoTraits<VkSubmitInfo>
    {
        PROFILER_FORCE_INLINE static uint32_t CommandBufferCount( const VkSubmitInfo& info ) { return info.commandBufferCount; }
        PROFILER_FORCE_INLINE static uint32_t SignalSemaphoreCount( const VkSubmitInfo& info ) { return info.signalSemaphoreCount; }
        PROFILER_FORCE_INLINE static uint32_t WaitSemaphoreCount( const VkSubmitInfo& info ) { return info.waitSemaphoreCount; }
        PROFILER_FORCE_INLINE static VkCommandBuffer CommandBuffer( const VkSubmitInfo& info, uint32_t i ) { return info.pCommandBuffers[ i ]; }
        PROFILER_FORCE_INLINE static VkSemaphore SignalSemaphore( const VkSubmitInfo& info, uint32_t i ) { return info.pSignalSemaphores[ i ]; }
        PROFILER_FORCE_INLINE static VkSemaphore WaitSemaphore( const VkSubmitInfo& info, uint32_t i ) { return info.pWaitSemaphores[ i ]; }
    };

    template<>
    struct SubmitInfoTraits<VkSubmitInfo2>
    {
        PROFILER_FORCE_INLINE static uint32_t CommandBufferCount( const VkSubmitInfo2& info ) { return info.commandBufferInfoCount; }
        PROFILER_FORCE_INLINE static uint32_t SignalSemaphoreCount( const VkSubmitInfo2& info ) { return info.signalSemaphoreInfoCount; }
        PROFILER_FORCE_INLINE static uint32_t WaitSemaphoreCount( const VkSubmitInfo2& info ) { return info.waitSemaphoreInfoCount; }
        PROFILER_FORCE_INLINE static VkCommandBuffer CommandBuffer( const VkSubmitInfo2& info, uint32_t i ) { return info.pCommandBufferInfos[ i ].commandBuffer; }
        PROFILER_FORCE_INLINE static VkSemaphore SignalSemaphore( const VkSubmitInfo2& info, uint32_t i ) { return info.pSignalSemaphoreInfos[ i ].semaphore; }
        PROFILER_FORCE_INLINE static VkSemaphore WaitSemaphore( const VkSubmitInfo2& info, uint32_t i ) { return info.pWaitSemaphoreInfos[ i ].semaphore; }
    };
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
        , m_pData( nullptr )
        , m_MemoryManager()
        , m_DataAggregator()
        , m_NextFrameIndex( 0 )
        , m_LastFrameBeginTimestamp( 0 )
        , m_CpuTimestampCounter()
        , m_CpuFpsCounter()
        , m_Allocations()
        , m_pCommandBuffers()
        , m_pCommandPools()
        , m_SubmitFence( VK_NULL_HANDLE )
        , m_PerformanceConfigurationINTEL( VK_NULL_HANDLE )
        , m_PipelineExecutablePropertiesEnabled( false )
        , m_pStablePowerStateHandle( nullptr )
    {
    }

    /***********************************************************************************\

    Function:
        EnumerateOptionalDeviceExtensions

    Description:
        Get list of optional device extensions that may be utilized by the profiler.

    \***********************************************************************************/
    std::unordered_set<std::string> DeviceProfiler::EnumerateOptionalDeviceExtensions( const ProfilerLayerSettings& settings, const VkProfilerCreateInfoEXT* pCreateInfo )
    {
        std::unordered_set<std::string> deviceExtensions = {
            VK_KHR_CALIBRATED_TIMESTAMPS_EXTENSION_NAME,
            VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME,
            VK_EXT_SHADER_MODULE_IDENTIFIER_EXTENSION_NAME
        };

        // Load configuration that will be used by the profiler.
        DeviceProfilerConfig config;
        DeviceProfiler::LoadConfiguration( settings, pCreateInfo, &config );

        if( config.m_EnablePerformanceQueryExt )
        {
            // Enable MDAPI data collection on Intel GPUs
            deviceExtensions.insert( VK_INTEL_PERFORMANCE_QUERY_EXTENSION_NAME );
        }

        if( config.m_EnablePipelineExecutablePropertiesExt )
        {
            // Enable pipeline executable properties capture
            deviceExtensions.insert( VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME );
        }

        return deviceExtensions;
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
        LoadConfiguration

    Description:
        Loads the configuration structure from all available sources.

    \***********************************************************************************/
    void DeviceProfiler::LoadConfiguration( const ProfilerLayerSettings& settings, const VkProfilerCreateInfoEXT* pCreateInfo, DeviceProfilerConfig* pConfig )
    {
        *pConfig = DeviceProfilerConfig( settings );

        // Load configuration from file (if exists).
        const std::filesystem::path& configFilename =
            ProfilerPlatformFunctions::GetApplicationDir() / "VK_LAYER_profiler_config.ini";

        if( std::filesystem::exists( configFilename ) )
        {
            pConfig->LoadFromFile( configFilename );
        }

        // Check if application provided create info
        if( pCreateInfo )
        {
            pConfig->LoadFromCreateInfo( pCreateInfo );
        }

        // Configure the profiler from the environment.
        pConfig->LoadFromEnvironment();
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
        m_NextFrameIndex = 0;

        // Configure the profiler.
        DeviceProfiler::LoadConfiguration( pDevice->pInstance->LayerSettings, pCreateInfo, &m_Config );

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

        DESTROYANDRETURNONFAIL( m_pDevice->Callbacks.CreateFence(
            m_pDevice->Handle, &fenceCreateInfo, nullptr, &m_SubmitFence ) );

        // Prepare for memory usage tracking
        m_MemoryData.m_Heaps.resize( m_pDevice->pPhysicalDevice->MemoryProperties.memoryHeapCount );
        m_MemoryData.m_Types.resize( m_pDevice->pPhysicalDevice->MemoryProperties.memoryTypeCount );

        // Enable vendor-specific extensions
        if( m_pDevice->EnabledExtensions.count( VK_INTEL_PERFORMANCE_QUERY_EXTENSION_NAME ) )
        {
            InitializeINTEL();
        }

        // Capture pipeline statistics and internal representations for debugging
        m_PipelineExecutablePropertiesEnabled =
            m_Config.m_EnablePipelineExecutablePropertiesExt &&
            m_pDevice->EnabledExtensions.count( VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME );

        // Initialize synchroniation manager
        DESTROYANDRETURNONFAIL( m_Synchronization.Initialize( m_pDevice ) );

        VkTimeDomainEXT hostTimeDomain = m_Synchronization.GetHostTimeDomain();
        m_CpuTimestampCounter.SetTimeDomain( hostTimeDomain );
        m_CpuFpsCounter.SetTimeDomain( hostTimeDomain );

        m_pDevice->TIP.SetTimeDomain( hostTimeDomain );

        // Initialize memory manager
        DESTROYANDRETURNONFAIL( m_MemoryManager.Initialize( m_pDevice ) );

        // Initialize aggregator
        DESTROYANDRETURNONFAIL( m_DataAggregator.Initialize( this ) );

        m_pData = m_DataAggregator.GetAggregatedData();
        assert( m_pData );

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
        CreateInternalPipeline( DeviceProfilerPipelineType::eBuildAccelerationStructuresKHR, "BuildAccelerationStructuresKHR" );
        CreateInternalPipeline( DeviceProfilerPipelineType::eCopyAccelerationStructureKHR, "CopyAccelerationStructureKHR" );
        CreateInternalPipeline( DeviceProfilerPipelineType::eCopyAccelerationStructureToMemoryKHR, "CopyAccelerationStructureToMemoryKHR" );
        CreateInternalPipeline( DeviceProfilerPipelineType::eCopyMemoryToAccelerationStructureKHR, "CopyMemoryToAccelerationStructureKHR" );

        if( m_Config.m_SetStablePowerState )
        {
            // Set stable power state.
            ProfilerPlatformFunctions::SetStablePowerState( m_pDevice, &m_pStablePowerStateHandle );
        }

        // Begin profiling of the first frame.
        BeginNextFrame();

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
        VkResult result = m_MetricsApiINTEL.Initialize( m_pDevice );

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
        AcquirePerformanceConfigurationINTEL

    Description:

    \***********************************************************************************/
    void DeviceProfiler::AcquirePerformanceConfigurationINTEL( VkQueue queue )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        assert( m_MetricsApiINTEL.IsAvailable() );
        assert( m_PerformanceConfigurationINTEL == VK_NULL_HANDLE );

        VkResult result;

        // Acquire performance configuration
        {
            VkPerformanceConfigurationAcquireInfoINTEL acquireInfo = {};
            acquireInfo.sType = VK_STRUCTURE_TYPE_PERFORMANCE_CONFIGURATION_ACQUIRE_INFO_INTEL;
            acquireInfo.type = VK_PERFORMANCE_CONFIGURATION_TYPE_COMMAND_QUEUE_METRICS_DISCOVERY_ACTIVATED_INTEL;

            assert( m_pDevice->Callbacks.AcquirePerformanceConfigurationINTEL );
            result = m_pDevice->Callbacks.AcquirePerformanceConfigurationINTEL(
                m_pDevice->Handle,
                &acquireInfo,
                &m_PerformanceConfigurationINTEL );
        }

        // Set performance configuration for the queue
        if( result == VK_SUCCESS )
        {
            assert( m_pDevice->Callbacks.QueueSetPerformanceConfigurationINTEL );
            result = m_pDevice->Callbacks.QueueSetPerformanceConfigurationINTEL(
                queue, m_PerformanceConfigurationINTEL );
        }

        assert( result == VK_SUCCESS );
    }

    /***********************************************************************************\

    Function:
        ReleasePerformanceConfigurationINTEL

    Description:

    \***********************************************************************************/
    void DeviceProfiler::ReleasePerformanceConfigurationINTEL()
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        assert( m_MetricsApiINTEL.IsAvailable() );

        if( m_PerformanceConfigurationINTEL )
        {
            assert( m_pDevice->Callbacks.ReleasePerformanceConfigurationINTEL );
            VkResult result = m_pDevice->Callbacks.ReleasePerformanceConfigurationINTEL(
                m_pDevice->Handle, m_PerformanceConfigurationINTEL );

            // Reset object handle for the next submit
            m_PerformanceConfigurationINTEL = VK_NULL_HANDLE;

            assert( result == VK_SUCCESS );
        }
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:
        Frees resources allocated by the profiler.

    \***********************************************************************************/
    void DeviceProfiler::Destroy()
    {
        m_DeferredOperationCallbacks.clear();

        m_pCommandBuffers.clear();
        m_pCommandPools.clear();

        m_Allocations.clear();

        m_Synchronization.Destroy();
        m_MemoryManager.Destroy();

        m_DataAggregator.Destroy();

        if( m_SubmitFence != VK_NULL_HANDLE )
        {
            m_pDevice->Callbacks.DestroyFence( m_pDevice->Handle, m_SubmitFence, nullptr );
            m_SubmitFence = VK_NULL_HANDLE;
        }

        if( m_pStablePowerStateHandle != nullptr )
        {
            ProfilerPlatformFunctions::ResetStablePowerState( m_pStablePowerStateHandle );
        }

        m_NextFrameIndex = 0;
        m_pDevice = nullptr;
    }

    /***********************************************************************************\

    \***********************************************************************************/
    VkResult DeviceProfiler::SetMode( VkProfilerModeEXT mode )
    {
        // TODO: Invalidate all command buffers
        m_Config.m_SamplingMode = mode;

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
    std::shared_ptr<DeviceProfilerFrameData> DeviceProfiler::GetData() const
    {
        return m_pData;
    }

    /***********************************************************************************\
    \***********************************************************************************/
    ProfilerCommandBuffer& DeviceProfiler::GetCommandBuffer( VkCommandBuffer commandBuffer )
    {
        return *m_pCommandBuffers.at( commandBuffer );
    }

    /***********************************************************************************\
    \***********************************************************************************/
    DeviceProfilerCommandPool& DeviceProfiler::GetCommandPool( VkCommandPool commandPool )
    {
        return *m_pCommandPools.at( commandPool );
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
    \***********************************************************************************/
    ProfilerShader& DeviceProfiler::GetShader( VkShaderEXT handle )
    {
        return m_Shaders.at( handle );
    }

    /***********************************************************************************\

    Function:
        ShouldCapturePipelineExecutableProperties

    Description:
        Checks whether pipeline executable properties should be captured.
        The feature is enabled only if VK_KHR_pipeline_executable_properties extension
        is enabled and it is not disabled in the configuration.

    \***********************************************************************************/
    bool DeviceProfiler::ShouldCapturePipelineExecutableProperties() const
    {
        return m_PipelineExecutablePropertiesEnabled;
    }

    /***********************************************************************************\

    Function:
        CreateCommandPool

    Description:
        Create wrapper for VkCommandPool object.

    \***********************************************************************************/
    void DeviceProfiler::CreateCommandPool( VkCommandPool commandPool, const VkCommandPoolCreateInfo* pCreateInfo )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        m_pCommandPools.insert( commandPool,
            std::make_unique<DeviceProfilerCommandPool>( *this, commandPool, *pCreateInfo ) );
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
        TipGuard tip( m_pDevice->TIP, __func__ );

        std::scoped_lock lk( m_SubmitMutex, m_PresentMutex, m_pCommandBuffers );

        for( auto it = m_pCommandBuffers.begin(); it != m_pCommandBuffers.end(); )
        {
            it = (it->second->GetCommandPool().GetHandle() == commandPool)
                ? FreeCommandBuffer( it )
                : std::next( it );
        }

        m_pCommandPools.remove( commandPool );
    }

    /***********************************************************************************\

    Function:
        RegisterCommandBuffers

    Description:
        Create wrappers for VkCommandBuffer objects.

    \***********************************************************************************/
    void DeviceProfiler::AllocateCommandBuffers( VkCommandPool commandPool, VkCommandBufferLevel level, uint32_t count, VkCommandBuffer* pCommandBuffers )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        std::scoped_lock lk( m_SubmitMutex, m_PresentMutex, m_pCommandBuffers );

        DeviceProfilerCommandPool& profilerCommandPool = GetCommandPool( commandPool );

        for( uint32_t i = 0; i < count; ++i )
        {
            VkCommandBuffer commandBuffer = pCommandBuffers[ i ];

            SetDefaultObjectName( commandBuffer );

            m_pCommandBuffers.unsafe_insert( commandBuffer,
                std::make_unique<ProfilerCommandBuffer>( *this, profilerCommandPool, commandBuffer, level ) );
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
        TipGuard tip( m_pDevice->TIP, __func__ );

        std::scoped_lock lk( m_SubmitMutex, m_PresentMutex, m_pCommandBuffers );

        for( uint32_t i = 0; i < count; ++i )
        {
            FreeCommandBuffer( pCommandBuffers[ i ] );
        }
    }

    /***********************************************************************************\

    Function:
        CreateDeferredOperation

    Description:
        Register deferred host operation.

    \***********************************************************************************/
    void DeviceProfiler::CreateDeferredOperation( VkDeferredOperationKHR deferredOperation )
    {
        m_DeferredOperationCallbacks.insert( deferredOperation, nullptr );
    }

    /***********************************************************************************\

    Function:
        DestroyDeferredOperation

    Description:
        Unregister deferred host operation.

    \***********************************************************************************/
    void DeviceProfiler::DestroyDeferredOperation( VkDeferredOperationKHR deferredOperation )
    {
        m_DeferredOperationCallbacks.remove( deferredOperation );
    }

    /***********************************************************************************\

    Function:
        SetDeferredOperationCallback

    Description:
        Associate an action with a deferred host operation. The action will be executed
        when the deferred operation is joined.

    \***********************************************************************************/
    void DeviceProfiler::SetDeferredOperationCallback( VkDeferredOperationKHR deferredOperation, DeferredOperationCallback callback )
    {
        m_DeferredOperationCallbacks.at( deferredOperation ) = std::move( callback );
    }

    /***********************************************************************************\

    Function:
        ExecuteDeferredOperationCallback

    Description:
        Execute an action associated with the deferred host operation.

    \***********************************************************************************/
    void DeviceProfiler::ExecuteDeferredOperationCallback( VkDeferredOperationKHR deferredOperation )
    {
        auto& callback = m_DeferredOperationCallbacks.at( deferredOperation );
        if( callback )
        {
            // Execute the custom action.
            callback( deferredOperation );

            // Clear the callback once it is executed.
            // Note that callback is a reference to the element in the m_DeferredOperationCallbacks map.
            callback = nullptr;
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
        TipGuard tip( m_pDevice->TIP, __func__ );

        for( uint32_t i = 0; i < pipelineCount; ++i )
        {
            DeviceProfilerPipeline profilerPipeline;
            profilerPipeline.m_Handle = pPipelines[i];
            profilerPipeline.m_BindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            profilerPipeline.m_Type = DeviceProfilerPipelineType::eGraphics;

            const VkGraphicsPipelineCreateInfo& createInfo = pCreateInfos[i];

            SetPipelineShaderProperties( profilerPipeline, createInfo.stageCount, createInfo.pStages );
            SetDefaultPipelineName( profilerPipeline );

            profilerPipeline.m_pCreateInfo = DeviceProfilerPipeline::CopyPipelineCreateInfo( &createInfo );

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
        TipGuard tip( m_pDevice->TIP, __func__ );

        for( uint32_t i = 0; i < pipelineCount; ++i )
        {
            DeviceProfilerPipeline profilerPipeline;
            profilerPipeline.m_Handle = pPipelines[ i ];
            profilerPipeline.m_BindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
            profilerPipeline.m_Type = DeviceProfilerPipelineType::eCompute;
            
            SetPipelineShaderProperties( profilerPipeline, 1, &pCreateInfos[i].stage );
            SetDefaultPipelineName( profilerPipeline );

            m_Pipelines.insert( pPipelines[ i ], profilerPipeline );
        }
    }

    /***********************************************************************************\

    Function:
        CreatePipelines

    Description:
        Register ray-tracing pipelines.

    \***********************************************************************************/
    void DeviceProfiler::CreatePipelines( uint32_t pipelineCount, const VkRayTracingPipelineCreateInfoKHR* pCreateInfos, VkPipeline* pPipelines, bool deferred )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        for( uint32_t i = 0; i < pipelineCount; ++i )
        {
            DeviceProfilerPipeline profilerPipeline;
            profilerPipeline.m_Handle = pPipelines[ i ];
            profilerPipeline.m_BindPoint = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
            profilerPipeline.m_Type = DeviceProfilerPipelineType::eRayTracingKHR;
            
            const VkRayTracingPipelineCreateInfoKHR& createInfo = pCreateInfos[i];

            SetPipelineShaderProperties( profilerPipeline, createInfo.stageCount, createInfo.pStages );
            SetDefaultPipelineName( profilerPipeline, deferred );

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
        TipGuard tip( m_pDevice->TIP, __func__ );

        m_Pipelines.remove( pipeline );
    }

    /***********************************************************************************\

    Function:
        CreateShaderModule

    Description:

    \***********************************************************************************/
    void DeviceProfiler::CreateShaderModule( VkShaderModule module, const VkShaderModuleCreateInfo* pCreateInfo )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        VkShaderModuleIdentifierEXT shaderModuleIdentifier = {};
        shaderModuleIdentifier.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_IDENTIFIER_EXT;

        if( m_pDevice->EnabledExtensions.count( VK_EXT_SHADER_MODULE_IDENTIFIER_EXTENSION_NAME ) &&
            m_pDevice->Callbacks.GetShaderModuleIdentifierEXT )
        {
            // Get shader module identifier.
            m_pDevice->Callbacks.GetShaderModuleIdentifierEXT( m_pDevice->Handle, module, &shaderModuleIdentifier );
        }

        m_pShaderModules.insert( module,
            std::make_shared<ProfilerShaderModule>(
                pCreateInfo->pCode,
                pCreateInfo->codeSize,
                shaderModuleIdentifier.identifier,
                shaderModuleIdentifier.identifierSize ) );
    }

    /***********************************************************************************\

    Function:
        DestroyShaderModule

    Description:

    \***********************************************************************************/
    void DeviceProfiler::DestroyShaderModule( VkShaderModule module )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        m_pShaderModules.remove( module );
    }

    /***********************************************************************************\

    Function:
        CreateShader

    Description:

    \***********************************************************************************/
    void DeviceProfiler::CreateShader( VkShaderEXT handle, const VkShaderCreateInfoEXT* pCreateInfo )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        ProfilerShader shader;
        shader.m_Index = UINT32_MAX;
        shader.m_Stage = pCreateInfo->stage;
        shader.m_EntryPoint = pCreateInfo->pName;

        if( pCreateInfo->codeType == VK_SHADER_CODE_TYPE_SPIRV_EXT )
        {
            VkShaderModuleIdentifierEXT shaderModuleIdentifier = {};
            shaderModuleIdentifier.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_IDENTIFIER_EXT;

            if( m_pDevice->EnabledExtensions.count( VK_EXT_SHADER_MODULE_IDENTIFIER_EXTENSION_NAME ) &&
                m_pDevice->Callbacks.GetShaderModuleCreateInfoIdentifierEXT )
            {
                // Get shader module identifier from a temporary shader module info structure based on the provided shader.
                VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
                shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
                shaderModuleCreateInfo.codeSize = pCreateInfo->codeSize;
                shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>( pCreateInfo->pCode );

                m_pDevice->Callbacks.GetShaderModuleCreateInfoIdentifierEXT(
                    m_pDevice->Handle,
                    &shaderModuleCreateInfo,
                    &shaderModuleIdentifier );
            }

            // Create a shader module for the shader.
            shader.m_pShaderModule = std::make_shared<ProfilerShaderModule>(
                reinterpret_cast<const uint32_t*>( pCreateInfo->pCode ),
                pCreateInfo->codeSize,
                shaderModuleIdentifier.identifier,
                shaderModuleIdentifier.identifierSize );

            shader.m_Hash = shader.m_pShaderModule->m_Hash;
        }

        // Hash the entrypoint and append it to the final hash
        shader.m_Hash ^= Farmhash::Fingerprint32( shader.m_EntryPoint.data(), shader.m_EntryPoint.length() );

        // Save the shader
        m_Shaders.insert( handle, std::move( shader ) );
    }

    /***********************************************************************************\

    Function:
        DestroyShader

    Description:

    \***********************************************************************************/
    void DeviceProfiler::DestroyShader( VkShaderEXT handle )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        m_Shaders.remove( handle );
    }

    /***********************************************************************************\

    Function:
        CreateRenderPass

    Description:

    \***********************************************************************************/
    void DeviceProfiler::CreateRenderPass( VkRenderPass renderPass, const VkRenderPassCreateInfo* pCreateInfo )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        DeviceProfilerRenderPass deviceProfilerRenderPass;
        deviceProfilerRenderPass.m_Handle = renderPass;
        deviceProfilerRenderPass.m_Type = DeviceProfilerRenderPassType::eGraphics;
        
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
        TipGuard tip( m_pDevice->TIP, __func__ );

        DeviceProfilerRenderPass deviceProfilerRenderPass;
        deviceProfilerRenderPass.m_Handle = renderPass;
        deviceProfilerRenderPass.m_Type = DeviceProfilerRenderPassType::eGraphics;

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
        TipGuard tip( m_pDevice->TIP, __func__ );

        m_RenderPasses.remove( renderPass );
    }

    /***********************************************************************************\

    Function:
        PreSubmitCommandBuffers

    Description:

    \***********************************************************************************/
    void DeviceProfiler::PreSubmitCommandBuffers( const DeviceProfilerSubmitBatch& submitBatch )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        if( m_MetricsApiINTEL.IsAvailable() )
        {
            AcquirePerformanceConfigurationINTEL( submitBatch.m_Handle );
        }
    }

    /***********************************************************************************\

    Function:
        PostSubmitCommandBuffers

    Description:

    \***********************************************************************************/
    void DeviceProfiler::PostSubmitCommandBuffers( const DeviceProfilerSubmitBatch& submitBatch )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        if( m_MetricsApiINTEL.IsAvailable() )
        {
            ReleasePerformanceConfigurationINTEL();
        }

        if( !m_DataAggregator.IsDataCollectionThreadRunning() )
        {
            m_DataAggregator.Aggregate();
        }

        // Append the submit batch for aggregation
        m_DataAggregator.AppendSubmit( submitBatch );
    }

    /***********************************************************************************\

    Function:
        CreateSubmitBatchInfoImpl

    Description:
        Structure-independent implementation of CreateSubmitBatchInfo.

    \***********************************************************************************/
    template<typename SubmitInfoT>
    void DeviceProfiler::CreateSubmitBatchInfoImpl( VkQueue queue, uint32_t count, const SubmitInfoT* pSubmitInfo, DeviceProfilerSubmitBatch* pSubmitBatch )
    {
        using T = SubmitInfoTraits<SubmitInfoT>;

        TipGuard tip( m_pDevice->TIP, __func__ );

        // Synchronize read access to m_pCommandBuffers
        std::shared_lock lk( m_pCommandBuffers );

        // Store submitted command buffers and get results
        pSubmitBatch->m_Handle = queue;
        pSubmitBatch->m_Timestamp = m_CpuTimestampCounter.GetCurrentValue();
        pSubmitBatch->m_ThreadId = ProfilerPlatformFunctions::GetCurrentThreadId();

        for( uint32_t submitIdx = 0; submitIdx < count; ++submitIdx )
        {
            const SubmitInfoT& submitInfo = pSubmitInfo[ submitIdx ];

            // Wrap submit info into our structure
            DeviceProfilerSubmit submit;
            submit.m_pCommandBuffers.reserve( T::CommandBufferCount( submitInfo ) );
            submit.m_SignalSemaphores.reserve( T::SignalSemaphoreCount( submitInfo ) );
            submit.m_WaitSemaphores.reserve( T::WaitSemaphoreCount( submitInfo ) );

            for( uint32_t commandBufferIdx = 0; commandBufferIdx < T::CommandBufferCount( submitInfo ); ++commandBufferIdx )
            {
                // Get command buffer handle
                VkCommandBuffer commandBuffer = T::CommandBuffer( submitInfo, commandBufferIdx );
                ProfilerCommandBuffer* pProfilerCommandBuffer = m_pCommandBuffers.unsafe_at( commandBuffer ).get();

                // Dirty command buffer profiling data
                pProfilerCommandBuffer->Submit();

                submit.m_pCommandBuffers.push_back( pProfilerCommandBuffer );
            }

            // Copy semaphores
            for( uint32_t semaphoreIdx = 0; semaphoreIdx < T::SignalSemaphoreCount( submitInfo ); ++semaphoreIdx )
            {
                submit.m_SignalSemaphores.push_back( T::SignalSemaphore( submitInfo, semaphoreIdx ) );
            }

            for( uint32_t semaphoreIdx = 0; semaphoreIdx < T::WaitSemaphoreCount( submitInfo ); ++semaphoreIdx )
            {
                submit.m_WaitSemaphores.push_back( T::WaitSemaphore( submitInfo, semaphoreIdx ) );
            }

            // Store the submit wrapper
            pSubmitBatch->m_Submits.push_back( std::move( submit ) );
        }
    }

    /***********************************************************************************\

    Function:
        CreateSubmitBatchInfo

    Description:

    \***********************************************************************************/
    void DeviceProfiler::CreateSubmitBatchInfo( VkQueue queue, uint32_t count, const VkSubmitInfo* pSubmitInfo, DeviceProfilerSubmitBatch* pSubmitBatch )
    {
        CreateSubmitBatchInfoImpl<VkSubmitInfo>( queue, count, pSubmitInfo, pSubmitBatch );
    }

    /***********************************************************************************\

    Function:
        CreateSubmitBatchInfo

    Description:

    \***********************************************************************************/
    void DeviceProfiler::CreateSubmitBatchInfo( VkQueue queue, uint32_t count, const VkSubmitInfo2* pSubmitInfo, DeviceProfilerSubmitBatch* pSubmitBatch )
    {
        CreateSubmitBatchInfoImpl<VkSubmitInfo2>( queue, count, pSubmitInfo, pSubmitBatch );
    }

    /***********************************************************************************\

    Function:
        FinishFrame

    Description:

    \***********************************************************************************/
    void DeviceProfiler::FinishFrame()
    {
        TipRangeId tip = m_pDevice->TIP.BeginFunction( __func__ );

        std::scoped_lock lk( m_PresentMutex );

        // Update FPS counter
        m_CpuFpsCounter.Update();

        BeginNextFrame();

        if( !m_DataAggregator.IsDataCollectionThreadRunning() )
        {
            // Collect data from the submitted command buffers
            m_DataAggregator.Aggregate();
        }

        // Get data captured during the last frame
        std::shared_ptr<DeviceProfilerFrameData> pData = m_DataAggregator.GetAggregatedData();
        assert( pData );

        // Check if new data is available
        if( pData != m_pData )
        {
            m_pData = std::move( pData );

            // TODO: Move to memory tracker
            m_pData->m_Memory = m_MemoryData;

            // Return TIP data
            m_pDevice->TIP.EndFunction( tip );
            m_pData->m_TIP = m_pDevice->TIP.GetData();
        }
    }

    /***********************************************************************************\

    Function:
        BeginNextFrame

    Description:

    \***********************************************************************************/
    void DeviceProfiler::BeginNextFrame()
    {
        // Recalibrate the timestamps for the next frame.
        m_Synchronization.SendSynchronizationTimestamps();

        // Prepare aggregator for the next frame.
        DeviceProfilerFrame frame = {};
        frame.m_FrameIndex = m_NextFrameIndex++;
        frame.m_ThreadId = ProfilerPlatformFunctions::GetCurrentThreadId();
        frame.m_Timestamp = m_CpuTimestampCounter.GetCurrentValue();
        frame.m_FramesPerSec = m_CpuFpsCounter.GetValue();
        frame.m_SyncTimestamps = m_Synchronization.GetSynchronizationTimestamps();

        m_DataAggregator.AppendFrame( frame );
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:

    \***********************************************************************************/
    void DeviceProfiler::AllocateMemory( VkDeviceMemory allocatedMemory, const VkMemoryAllocateInfo* pAllocateInfo )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

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
        TipGuard tip( m_pDevice->TIP, __func__ );

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
    void DeviceProfiler::SetPipelineShaderProperties( DeviceProfilerPipeline& pipeline, uint32_t stageCount, const VkPipelineShaderStageCreateInfo* pStages )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        // Capture pipeline executable properties
        if( ShouldCapturePipelineExecutableProperties() )
        {
            VkPipelineInfoKHR pipelineInfo = {};
            pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INFO_KHR;
            pipelineInfo.pipeline = pipeline.m_Handle;

            // Get number of executables collected for this pipeline
            uint32_t pipelineExecutablesCount = 0;
            VkResult result = m_pDevice->Callbacks.GetPipelineExecutablePropertiesKHR(
                m_pDevice->Handle,
                &pipelineInfo,
                &pipelineExecutablesCount,
                nullptr );

            std::vector<VkPipelineExecutablePropertiesKHR> pipelineExecutables( 0 );
            if( result == VK_SUCCESS && pipelineExecutablesCount > 0 )
            {
                pipelineExecutables.resize( pipelineExecutablesCount );
                m_pDevice->Callbacks.GetPipelineExecutablePropertiesKHR(
                    m_pDevice->Handle,
                    &pipelineInfo,
                    &pipelineExecutablesCount,
                    pipelineExecutables.data() );
            }

            // Preallocate space for the shader executables
            pipeline.m_ShaderTuple.m_ShaderExecutables.resize( pipelineExecutablesCount );

            std::vector<VkPipelineExecutableStatisticKHR> executableStatistics( 0 );
            std::vector<VkPipelineExecutableInternalRepresentationKHR> executableInternalRepresentations( 0 );

            for( uint32_t i = 0; i < pipelineExecutablesCount; ++i )
            {
                VkPipelineExecutableInfoKHR executableInfo = {};
                executableInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_INFO_KHR;
                executableInfo.executableIndex = i;
                executableInfo.pipeline = pipeline.m_Handle;

                // Enumerate shader statistics for the executable
                uint32_t executableStatisticsCount = 0;
                result = m_pDevice->Callbacks.GetPipelineExecutableStatisticsKHR(
                    m_pDevice->Handle,
                    &executableInfo,
                    &executableStatisticsCount,
                    nullptr );

                executableStatistics.clear();

                if( result == VK_SUCCESS && executableStatisticsCount > 0 )
                {
                    executableStatistics.resize( executableStatisticsCount );
                    result = m_pDevice->Callbacks.GetPipelineExecutableStatisticsKHR(
                        m_pDevice->Handle,
                        &executableInfo,
                        &executableStatisticsCount,
                        executableStatistics.data() );

                    if( result != VK_SUCCESS )
                    {
                        executableStatistics.clear();
                    }
                }

                // Enumerate shader internal representations
                uint32_t executableInternalRepresentationsCount = 0;
                result = m_pDevice->Callbacks.GetPipelineExecutableInternalRepresentationsKHR(
                    m_pDevice->Handle,
                    &executableInfo,
                    &executableInternalRepresentationsCount,
                    nullptr );

                executableInternalRepresentations.clear();

                if( result == VK_SUCCESS && executableInternalRepresentationsCount > 0 )
                {
                    executableInternalRepresentations.resize( executableInternalRepresentationsCount );
                    result = m_pDevice->Callbacks.GetPipelineExecutableInternalRepresentationsKHR(
                        m_pDevice->Handle,
                        &executableInfo,
                        &executableInternalRepresentationsCount,
                        executableInternalRepresentations.data() );

                    if( result != VK_SUCCESS )
                    {
                        executableInternalRepresentations.clear();
                    }
                }

                // Initialize the shader executable
                result = pipeline.m_ShaderTuple.m_ShaderExecutables[ i ].Initialize(
                    &pipelineExecutables[ i ],
                    executableStatisticsCount,
                    executableStatistics.data(),
                    executableInternalRepresentationsCount,
                    executableInternalRepresentations.data() );

                if( result == VK_INCOMPLETE )
                {
                    // Call vkGetPipelineExecutableInternalRepresentationsKHR to write the internal representations
                    // to the shader executable's internal memory.
                    m_pDevice->Callbacks.GetPipelineExecutableInternalRepresentationsKHR(
                        m_pDevice->Handle,
                        &executableInfo,
                        &executableInternalRepresentationsCount,
                        executableInternalRepresentations.data() );
                }
            }
        }

        // Preallocate memory for the pipeline shader stages
        pipeline.m_ShaderTuple.m_Shaders.resize( stageCount );

        for( uint32_t i = 0; i < stageCount; ++i )
        {
            std::shared_ptr<ProfilerShaderModule> pShaderModule = nullptr;

            // If module is VK_NULL_HANDLE, either the pNext chain contains a VkShaderModuleCreateInfo, or an identifier is provided.
            // In the latter case the bytecode may not be available if it is cached.
            if( pStages[i].module == VK_NULL_HANDLE )
            {
                for( const auto& it : PNextIterator( pStages[i].pNext ) )
                {
                    if( it.sType == VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO )
                    {
                        const VkShaderModuleCreateInfo& shaderModuleCreateInfo =
                            reinterpret_cast<const VkShaderModuleCreateInfo&>( it );

                        // Get shader identifier from the shader module create info.
                        VkShaderModuleIdentifierEXT shaderModuleIdentifier = {};
                        shaderModuleIdentifier.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_IDENTIFIER_EXT;

                        if( m_pDevice->EnabledExtensions.count( VK_EXT_SHADER_MODULE_IDENTIFIER_EXTENSION_NAME ) &&
                            m_pDevice->Callbacks.GetShaderModuleCreateInfoIdentifierEXT )
                        {
                            m_pDevice->Callbacks.GetShaderModuleCreateInfoIdentifierEXT(
                                m_pDevice->Handle,
                                &shaderModuleCreateInfo,
                                &shaderModuleIdentifier );
                        }

                        // Create shader object from the provided bytecode.
                        pShaderModule = std::make_shared<ProfilerShaderModule>(
                            reinterpret_cast<const uint32_t*>( shaderModuleCreateInfo.pCode ),
                            shaderModuleCreateInfo.codeSize,
                            shaderModuleIdentifier.identifier,
                            shaderModuleIdentifier.identifierSize );

                        break;
                    }

                    if( it.sType == VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_MODULE_IDENTIFIER_CREATE_INFO_EXT )
                    {
                        const VkPipelineShaderStageModuleIdentifierCreateInfoEXT& moduleIdentifierCreateInfo =
                            reinterpret_cast<const VkPipelineShaderStageModuleIdentifierCreateInfoEXT&>( it );

                        // Construct a shader module with no bytecode.
                        pShaderModule = std::make_shared<ProfilerShaderModule>(
                            nullptr, 0,
                            moduleIdentifierCreateInfo.pIdentifier,
                            moduleIdentifierCreateInfo.identifierSize );
                    }
                }
            }
            else
            {
                // VkShaderModule entry should already be in the map.
                pShaderModule = m_pShaderModules.at( pStages[i].module );
            }

            ProfilerShader& shader = pipeline.m_ShaderTuple.m_Shaders[ i ];
            shader.m_Hash = pShaderModule ? pShaderModule->m_Hash : 0;
            shader.m_Index = i;
            shader.m_Stage = pStages[ i ].stage;
            shader.m_EntryPoint = pStages[ i ].pName;
            shader.m_pShaderModule = std::move( pShaderModule );

            // Hash the entrypoint and append it to the final hash
            shader.m_Hash ^= Farmhash::Fingerprint32( shader.m_EntryPoint.data(), shader.m_EntryPoint.length() );
        }

        // Finalize pipeline creation
        pipeline.Finalize();
    }

    /***********************************************************************************\

    Function:
        SetObjectName

    Description:
        Set custom object name.

    \***********************************************************************************/
    void DeviceProfiler::SetObjectName( VkObject object, const char* pName )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

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
        TipGuard tip( m_pDevice->TIP, __func__ );

        // There is special function for VkPipeline objects
        if( object.m_Type == VK_OBJECT_TYPE_PIPELINE )
        {
            SetDefaultObjectName( VkObject_Traits<VkPipeline>::GetObjectHandleAsVulkanHandle( object.m_Handle ) );
            return;
        }

        char pObjectDebugName[ 64 ] = {};
        ProfilerStringFunctions::Format( pObjectDebugName, "%s 0x%016llx", object.m_pTypeName, object.m_Handle );

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
        SetDefaultPipelineName( GetPipeline( object ) );
    }

    /***********************************************************************************\

    Function:
        SetDefaultPipelineName

    Description:
        Set default pipeline name consisting of shader tuple hashes.

    \***********************************************************************************/
    void DeviceProfiler::SetDefaultPipelineName( const DeviceProfilerPipeline& pipeline, bool deferred )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        std::string pipelineName;

        if( pipeline.m_BindPoint == VK_PIPELINE_BIND_POINT_GRAPHICS )
        {
            pipelineName = pipeline.m_ShaderTuple.GetShaderStageHashesString(
                VK_SHADER_STAGE_VERTEX_BIT |
                VK_SHADER_STAGE_TASK_BIT_EXT |
                VK_SHADER_STAGE_MESH_BIT_EXT |
                VK_SHADER_STAGE_FRAGMENT_BIT,
                true /*skipEmptyStages*/ );
        }

        if( pipeline.m_BindPoint == VK_PIPELINE_BIND_POINT_COMPUTE )
        {
            pipelineName = pipeline.m_ShaderTuple.GetShaderStageHashesString(
                VK_SHADER_STAGE_COMPUTE_BIT );
        }

        if( pipeline.m_BindPoint == VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR )
        {
            pipelineName = pipeline.m_ShaderTuple.GetShaderStageHashesString(
                VK_SHADER_STAGE_RAYGEN_BIT_KHR |
                VK_SHADER_STAGE_ANY_HIT_BIT_KHR |
                VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR );
        }

        if( !pipelineName.empty() )
        {
            // When deferred operation is joined, the application may have already set a name for the pipeline.
            // Don't set the default in such case.
            if( deferred )
            {
                m_pDevice->Debug.ObjectNames.insert( pipeline.m_Handle, std::move( pipelineName ) );
            }
            else
            {
                m_pDevice->Debug.ObjectNames.insert_or_assign( pipeline.m_Handle, std::move( pipelineName ) );
            }
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
        TipGuard tip( m_pDevice->TIP, __func__ );

        DeviceProfilerPipeline internalPipeline;
        internalPipeline.m_Handle = (VkPipeline)type;
        internalPipeline.m_ShaderTuple.m_Hash = (uint32_t)type;
        internalPipeline.m_Type = type;

        // Assign name for the internal pipeline
        SetObjectName( internalPipeline.m_Handle, pName );

        m_Pipelines.insert( internalPipeline.m_Handle, internalPipeline );
    }

    /***********************************************************************************\

    Function:
        FreeCommandBuffer

    Description:

    \***********************************************************************************/
    decltype(DeviceProfiler::m_pCommandBuffers)::iterator DeviceProfiler::FreeCommandBuffer( VkCommandBuffer commandBuffer )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        // Assume m_CommandBuffers map is already locked
        assert( !m_pCommandBuffers.try_lock() );

        auto it = m_pCommandBuffers.unsafe_find( commandBuffer );

        // Collect command buffer data now, command buffer won't be available later
        m_DataAggregator.Aggregate( it->second.get() );

        return m_pCommandBuffers.unsafe_remove( it );
    }

    /***********************************************************************************\

    Function:
        FreeCommandBuffer

    Description:

    \***********************************************************************************/
    decltype(DeviceProfiler::m_pCommandBuffers)::iterator DeviceProfiler::FreeCommandBuffer( decltype(m_pCommandBuffers)::iterator it )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        // Assume m_CommandBuffers map is already locked
        assert( !m_pCommandBuffers.try_lock() );

        // Collect command buffer data now, command buffer won't be available later
        m_DataAggregator.Aggregate( it->second.get() );

        return m_pCommandBuffers.unsafe_remove( it );
    }
}

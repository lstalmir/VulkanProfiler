#include "profiler.h"
#include "profiler_helpers.h"
#include "farmhash/src/farmhash.h"
#include <sstream>
#include <fstream>

#undef max

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
        , m_Allocations()
        , m_DeviceLocalAllocatedMemorySize( 0 )
        , m_DeviceLocalAllocationCount( 0 )
        , m_HostVisibleAllocatedMemorySize( 0 )
        , m_HostVisibleAllocationCount( 0 )
        , m_ProfiledCommandBuffers()
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
    VkResult DeviceProfiler::Initialize( VkDevice_Object* pDevice )
    {
        m_pDevice = pDevice;
        m_CurrentFrame = 0;

        VkResult result;

        // Load config
        std::filesystem::path customConfigPath = ProfilerPlatformFunctions::GetCustomConfigPath();

        // Check if custom configuration file exists
        if( !customConfigPath.empty() &&
            !std::filesystem::exists( customConfigPath ) )
        {
            //m_Output.WriteLine( "ERROR: Custom config file %s not found",
            //    customConfigPath.c_str() );

            customConfigPath = "";
        }

        // Look for path relative to the app
        if( !customConfigPath.is_absolute() )
        {
            customConfigPath = ProfilerPlatformFunctions::GetApplicationDir() / customConfigPath;
        }

        customConfigPath /= "VkLayer_profiler_layer.conf";

        if( std::filesystem::exists( customConfigPath ) )
        {
            std::ifstream config( customConfigPath );

            // Check if configuration file was successfully opened
            while( !config.eof() )
            {
                std::string key; config >> key;
                uint32_t value; config >> value;

                if( key == "MODE" )
                    //m_Config.m_DisplayMode = static_cast<VkProfilerModeEXT>(value);
                    ;

                if( key == "NUM_QUERIES_PER_CMD_BUFFER" )
                    m_Config.m_NumQueriesPerCommandBuffer = value;

                if( key == "OUTPUT_UPDATE_INTERVAL" )
                    m_Config.m_OutputUpdateInterval = static_cast<std::chrono::milliseconds>(value);

                if( key == "OUTPUT_FLAGS" )
                    m_Config.m_OutputFlags = static_cast<VkProfilerOutputFlagsEXT>(value);
            }
        }

        // TODO: Remove
        m_Config.m_OutputFlags |= VK_PROFILER_OUTPUT_FLAG_OVERLAY_BIT_EXT;

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

        result = m_pDevice->Callbacks.CreateFence(
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
        m_DataAggregator.Initialize( pDevice );

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
        m_ProfiledCommandBuffers.clear();

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
        m_Config.m_DisplayMode = mode;

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    \***********************************************************************************/
    ProfilerAggregatedData DeviceProfiler::GetData() const
    {
        // Hold aggregator updates to keep m_Data consistent
        std::scoped_lock lk( m_DataMutex );
        return m_Data;
    }

    /***********************************************************************************\
    \***********************************************************************************/
    ProfilerCommandBuffer& DeviceProfiler::GetCommandBuffer( VkCommandBuffer commandBuffer )
    {
        // Create new wrapper if command buffer is not tracked yet
        // TODO: Move to AllocateCommandBuffers()
        auto emplaced = m_ProfiledCommandBuffers.interlocked_try_emplace(
            commandBuffer, std::ref( *this ), commandBuffer );

        return emplaced.first->second;
    }

    /***********************************************************************************\
    \***********************************************************************************/
    ProfilerPipeline& DeviceProfiler::GetPipeline( VkPipeline pipeline )
    {
        return m_ProfiledPipelines.interlocked_at( pipeline );
    }

    /***********************************************************************************\

    Function:
        PipelineBarrier

    Description:
        Collects barrier statistics.

    \***********************************************************************************/
    void DeviceProfiler::OnPipelineBarrier( VkCommandBuffer commandBuffer,
        uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers,
        uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers,
        uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers )
    {
        // ProfilerCommandBuffer object should already be in the map
        auto& profiledCommandBuffer = m_ProfiledCommandBuffers.interlocked_at( commandBuffer );

        profiledCommandBuffer.OnPipelineBarrier(
            memoryBarrierCount, pMemoryBarriers,
            bufferMemoryBarrierCount, pBufferMemoryBarriers,
            imageMemoryBarrierCount, pImageMemoryBarriers );

        // Transitions from undefined layout are slower than from other layouts
        // BUT are required in some cases (e.g., texture is used for the first time)
        for( uint32_t i = 0; i < imageMemoryBarrierCount; ++i )
        {
            const VkImageMemoryBarrier& barrier = pImageMemoryBarriers[ i ];

            if( barrier.oldLayout == VK_IMAGE_LAYOUT_UNDEFINED )
            {
                // TODO: Message
            }
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
            ProfilerPipeline profilerPipeline;
            profilerPipeline.m_Handle = pPipelines[i];
            profilerPipeline.m_ShaderTuple = CreateShaderTuple( pCreateInfos[i] );
            profilerPipeline.m_BindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

            char pPipelineDebugName[ 24 ] = "VS=XXXXXXXX,PS=XXXXXXXX";
            u32tohex( pPipelineDebugName + 3, profilerPipeline.m_ShaderTuple.m_Vert );
            u32tohex( pPipelineDebugName + 15, profilerPipeline.m_ShaderTuple.m_Frag );

            m_pDevice->Debug.ObjectNames.emplace( (uint64_t)profilerPipeline.m_Handle, pPipelineDebugName );

            m_ProfiledPipelines.interlocked_emplace( pPipelines[i], profilerPipeline );
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
            ProfilerPipeline profilerPipeline;
            profilerPipeline.m_Handle = pPipelines[ i ];
            profilerPipeline.m_ShaderTuple = CreateShaderTuple( pCreateInfos[ i ] );
            profilerPipeline.m_BindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;

            char pPipelineDebugName[ 12 ] = "CS=XXXXXXXX";
            u32tohex( pPipelineDebugName + 3, profilerPipeline.m_ShaderTuple.m_Comp );

            m_pDevice->Debug.ObjectNames.emplace( (uint64_t)profilerPipeline.m_Handle, pPipelineDebugName );

            m_ProfiledPipelines.interlocked_emplace( pPipelines[ i ], profilerPipeline );
        }
    }

    /***********************************************************************************\

    Function:
        DestroyPipeline

    Description:

    \***********************************************************************************/
    void DeviceProfiler::DestroyPipeline( VkPipeline pipeline )
    {
        m_ProfiledPipelines.interlocked_erase( pipeline );
    }

    /***********************************************************************************\

    Function:
        BindPipeline

    Description:

    \***********************************************************************************/
    void DeviceProfiler::BindPipeline( VkCommandBuffer commandBuffer, VkPipeline pipeline )
    {
        // ProfilerCommandBuffer object should already be in the map
        auto& profilerCommandBuffer = m_ProfiledCommandBuffers.interlocked_at( commandBuffer );

        // ProfilerPipeline object should already be in the map
        // TMP
        auto it = m_ProfiledPipelines.find( pipeline );
        if( it != m_ProfiledPipelines.end() )
        {
            profilerCommandBuffer.BindPipeline( it->second );
        }
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

        m_ProfiledShaderModules.interlocked_emplace( module, hash );
    }

    /***********************************************************************************\

    Function:
        DestroyShaderModule

    Description:

    \***********************************************************************************/
    void DeviceProfiler::DestroyShaderModule( VkShaderModule module )
    {
        m_ProfiledShaderModules.interlocked_erase( module );
    }

    /***********************************************************************************\

    Function:
        PreBeginRenderPass

    Description:

    \***********************************************************************************/
    void DeviceProfiler::PreBeginRenderPass( VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo* pBeginInfo )
    {
        // ProfilerCommandBuffer object should already be in the map
        auto& profilerCommandBuffer = m_ProfiledCommandBuffers.interlocked_at( commandBuffer );

        profilerCommandBuffer.PreBeginRenderPass( pBeginInfo );
    }

    /***********************************************************************************\

    Function:
        PostBeginRenderPass

    Description:

    \***********************************************************************************/
    void DeviceProfiler::PostBeginRenderPass( VkCommandBuffer commandBuffer )
    {
        // ProfilerCommandBuffer object should already be in the map
        auto& profilerCommandBuffer = m_ProfiledCommandBuffers.interlocked_at( commandBuffer );

        profilerCommandBuffer.PostBeginRenderPass();
    }

    /***********************************************************************************\

    Function:
        PreEndRenderPass

    Description:

    \***********************************************************************************/
    void DeviceProfiler::PreEndRenderPass( VkCommandBuffer commandBuffer )
    {
        // ProfilerCommandBuffer object should already be in the map
        auto& profilerCommandBuffer = m_ProfiledCommandBuffers.interlocked_at( commandBuffer );

        profilerCommandBuffer.PreEndRenderPass();
    }

    /***********************************************************************************\

    Function:
        PostEndRenderPass

    Description:

    \***********************************************************************************/
    void DeviceProfiler::PostEndRenderPass( VkCommandBuffer commandBuffer )
    {
        // ProfilerCommandBuffer object should already be in the map
        auto& profilerCommandBuffer = m_ProfiledCommandBuffers.interlocked_at( commandBuffer );

        profilerCommandBuffer.PostEndRenderPass();
    }

    /***********************************************************************************\

    \***********************************************************************************/
    void DeviceProfiler::NextSubpass( VkCommandBuffer commandBuffer, VkSubpassContents contents )
    {
        // ProfilerCommandBuffer object should already be in the map
        auto& profilerCommandBuffer = m_ProfiledCommandBuffers.interlocked_at( commandBuffer );

        profilerCommandBuffer.NextSubpass( contents );
    }

    /***********************************************************************************\

    Function:
        BeginCommandBuffer

    Description:

    \***********************************************************************************/
    void DeviceProfiler::BeginCommandBuffer( VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo* pBeginInfo )
    {
        auto emplaced = m_ProfiledCommandBuffers.interlocked_try_emplace( commandBuffer,
            std::ref( *this ), commandBuffer );

        // Grab reference to the ProfilerCommandBuffer instance
        auto& profilerCommandBuffer = emplaced.first->second;

        // Prepare the command buffer for the next profiling run
        profilerCommandBuffer.Begin( pBeginInfo );
    }

    /***********************************************************************************\

    Function:
        EndCommandBuffer

    Description:

    \***********************************************************************************/
    void DeviceProfiler::EndCommandBuffer( VkCommandBuffer commandBuffer )
    {
        // ProfilerCommandBuffer object should already be in the map
        auto& profilerCommandBuffer = m_ProfiledCommandBuffers.interlocked_at( commandBuffer );

        // Prepare the command buffer for the next profiling run
        profilerCommandBuffer.End();
    }

    /***********************************************************************************\

    Function:
        FreeCommandBuffers

    Description:

    \***********************************************************************************/
    void DeviceProfiler::FreeCommandBuffers( uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers )
    {
        // Block access from other threads
        std::scoped_lock lk( m_ProfiledCommandBuffers );

        for( uint32_t i = 0; i < commandBufferCount; ++i )
        {
            m_ProfiledCommandBuffers.erase( pCommandBuffers[i] );
        }
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
        // Wait for the submitted command buffers to execute
        //m_pDevice->Callbacks.QueueSubmit( queue, 0, nullptr, m_SubmitFence );
        //m_pDevice->Callbacks.WaitForFences( m_pDevice->Handle, 1, &m_SubmitFence, true, std::numeric_limits<uint64_t>::max() );
        //m_pDevice->Callbacks.ResetFences( m_pDevice->Handle, 1, &m_SubmitFence );

        // Store submitted command buffers and get results
        for( uint32_t submitIdx = 0; submitIdx < count; ++submitIdx )
        {
            const VkSubmitInfo& submitInfo = pSubmitInfo[submitIdx];

            // Wrap submit info into our structure
            ProfilerSubmit submit;

            for( uint32_t commandBufferIdx = 0; commandBufferIdx < submitInfo.commandBufferCount; ++commandBufferIdx )
            {
                // Get command buffer handle
                VkCommandBuffer commandBuffer = submitInfo.pCommandBuffers[commandBufferIdx];

                // Block access from other threads
                std::scoped_lock lk( m_ProfiledCommandBuffers );

                auto& profilerCommandBuffer = m_ProfiledCommandBuffers.at( commandBuffer );

                // Dirty command buffer profiling data
                profilerCommandBuffer.Submit();

                submit.m_pCommandBuffers.push_back( &profilerCommandBuffer );
            }

            // Store the submit wrapper
            m_DataAggregator.AppendSubmit( submit );
        }

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
        m_CurrentFrame++;

        m_CpuTimestampCounter.End();

        // TMP (doesn't introduce in-frame CPU overhead but may cause some image-count-related issues disappear)
        m_pDevice->Callbacks.DeviceWaitIdle( m_pDevice->Handle );

        // TMP
        std::scoped_lock lk( m_DataMutex );
        m_Data = m_DataAggregator.GetAggregatedData();

        // TODO: Move to aggregator
        if( m_MetricsApiINTEL.IsAvailable() )
        {
            m_Data.m_VendorMetrics = m_MetricsApiINTEL.ParseReport(
                m_Data.m_Submits.front().m_CommandBuffers.front().tmp.data(),
                m_Data.m_Submits.front().m_CommandBuffers.front().tmp.size() );
        }

        // TODO: Move to CPU tracker
        m_Data.m_CPU.m_TimeNs = m_CpuTimestampCounter.GetValue<std::chrono::nanoseconds>().count();

        // TODO: Move to memory tracker
        m_Data.m_Memory.m_TotalAllocationCount = m_TotalAllocationCount;
        m_Data.m_Memory.m_TotalAllocationSize = m_TotalAllocatedMemorySize;
        m_Data.m_Memory.m_DeviceLocalAllocationSize = m_DeviceLocalAllocatedMemorySize;
        m_Data.m_Memory.m_HostVisibleAllocationSize = m_HostVisibleAllocatedMemorySize;

        // TODO: Move to memory tracker
        ClearStructure( &m_MemoryProperties2, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2 );
        m_pDevice->pInstance->Callbacks.GetPhysicalDeviceMemoryProperties2KHR(
            m_pDevice->PhysicalDevice,
            &m_MemoryProperties2 );

        if( m_CpuTimestampCounter.GetValue<std::chrono::milliseconds>() > m_Config.m_OutputUpdateInterval )
        {
            // Process data (TODO: Move to profiler_ext)
            #if 0
            m_DataStaging.memory.deviceLocalMemoryAllocated = data.m_Memory.m_DeviceLocalAllocationSize;

            FreeProfilerData( &m_DataStaging.frame );

            m_DataStaging.frame.regionType = VK_PROFILER_REGION_TYPE_FRAME_EXT;
            m_DataStaging.frame.pRegionName = CreateRegionName( "Frame #", m_CurrentFrame );
            
            // Frame stats
            FillProfilerData( &m_DataStaging.frame, data.m_Stats );

            m_DataStaging.frame.subregionCount = data.m_Submits.size();
            m_DataStaging.frame.pSubregions = new VkProfilerRegionDataEXT[ data.m_Submits.size() ];

            uint32_t submitIdx = 0;
            VkProfilerRegionDataEXT* pSubmitRegion = m_DataStaging.frame.pSubregions;

            for( const auto& submit : data.m_Submits )
            {
                pSubmitRegion->regionType = VK_PROFILER_REGION_TYPE_SUBMIT_EXT;
                pSubmitRegion->regionObject = VK_NULL_HANDLE;
                pSubmitRegion->pRegionName = CreateRegionName( "Submit #", submitIdx );

                // Submit stats (TODO)

                pSubmitRegion->subregionCount = submit.m_CommandBuffers.size();
                pSubmitRegion->pSubregions = new VkProfilerRegionDataEXT[ submit.m_CommandBuffers.size() ];

                VkProfilerRegionDataEXT* pCmdBufferRegion = pSubmitRegion->pSubregions;

                for( const auto& cmdBuffer : submit.m_CommandBuffers )
                {
                    pCmdBufferRegion
                }

                // Move to next submit region
                submitIdx++;
                pSubmitRegion++;
            }
            #endif

        }

        m_CpuTimestampCounter.Begin();

        m_LastFrameBeginTimestamp = m_Data.m_Stats.m_BeginTimestamp;

        // TMP
        m_DataAggregator.Reset();
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

    #if 0
    /***********************************************************************************\

    Function:
        PresentResults

    Description:

    \***********************************************************************************/
    void DeviceProfiler::PresentResults( const ProfilerAggregatedData& data )
    {
        // Summary stats
        m_Output.Summary.FPS = 1000000000. / (m_TimestampPeriod * (data.m_Stats.m_BeginTimestamp - m_LastFrameBeginTimestamp));

        if( m_Output.NextLinesVisible( 12 ) )
        {
            // Tuple stats
            int numTopTuples = 10;
            m_Output.WriteLine( " Top 10 pipelines:" );
            m_Output.WriteLine( "     Vert     Frag" );
            for( const auto& pipeline : data.m_TopPipelines )
            {
                if( numTopTuples == 0 )
                    break;

                m_Output.WriteLine( " %2d. %08x %08x  (%0.1f%%)",
                    11 - numTopTuples,
                    pipeline.m_ShaderTuple.m_Vert,
                    pipeline.m_ShaderTuple.m_Frag,
                    100.f * static_cast<float>(pipeline.m_Stats.m_TotalTicks) / static_cast<float>(data.m_Stats.m_TotalTicks) );

                numTopTuples--;
            }
            m_Output.SkipLines( numTopTuples );

            // Drawcall stats
            m_Output.WriteAt( 40, 3, "Frame statistics:" );
            m_Output.WriteAt( 40, 4, " Draw:                 %5u", data.m_Stats.m_TotalDrawCount );
            m_Output.WriteAt( 40, 5, " Draw (indirect):      %5u", data.m_Stats.m_TotalDrawIndirectCount );
            m_Output.WriteAt( 40, 6, " Dispatch:             %5u", data.m_Stats.m_TotalDispatchCount );
            m_Output.WriteAt( 40, 7, " Dispatch (indirect):  %5u", data.m_Stats.m_TotalDispatchIndirectCount );
            m_Output.WriteAt( 40, 8, " Copy:                 %5u", data.m_Stats.m_TotalCopyCount );
            m_Output.WriteAt( 40, 9, " Clear:                %5u", data.m_Stats.m_TotalClearCount );
            m_Output.WriteAt( 40, 10, " Clear (implicit):     %5u", data.m_Stats.m_TotalClearImplicitCount );
            m_Output.WriteAt( 40, 11, " Barrier:              %5u", data.m_Stats.m_TotalBarrierCount );
            m_Output.WriteAt( 40, 12, "                            " ); // clear
            m_Output.WriteAt( 40, 13, " TOTAL:                %5u",
                data.m_Stats.m_TotalDrawCount +
                data.m_Stats.m_TotalDrawIndirectCount +
                data.m_Stats.m_TotalDispatchCount +
                data.m_Stats.m_TotalDispatchIndirectCount +
                data.m_Stats.m_TotalCopyCount +
                data.m_Stats.m_TotalClearCount +
                data.m_Stats.m_TotalClearImplicitCount +
                data.m_Stats.m_TotalBarrierCount );
        }
        else
        {
            m_Output.SkipLines( 12 );
        }

        m_Output.WriteLine( "" );

        for( uint32_t submitIdx = 0; submitIdx < data.m_Submits.size(); ++submitIdx )
        {
            PresentSubmit( submitIdx, data.m_Submits.at( submitIdx ) );
        }

        m_Output.WriteLine( "" );
        m_Output.Flush();
    }

    namespace
    {
        static constexpr char fillLine[] =
            "...................................................................................................."
            "...................................................................................................."
            "...................................................................................................."
            "....................................................................................................";

    }

    /***********************************************************************************\

    Function:
        PresentResults

    Description:

    \***********************************************************************************/
    void DeviceProfiler::PresentSubmit( uint32_t submitIdx, const ProfilerSubmitData& submit )
    {
        // Calculate how many lines this submit will generate
        uint32_t requiredLineCount = 1 + submit.m_CommandBuffers.size();

        if( m_Config.m_DisplayMode <= VK_PROFILER_MODE_PER_RENDER_PASS_EXT )
        for( const auto& commandBuffer : submit.m_CommandBuffers )
        {
            // Add number of valid render passes
            for( const auto& renderPass : commandBuffer.m_Subregions )
            {
                if( renderPass.m_Handle != VK_NULL_HANDLE )
                    requiredLineCount++;

                // Add number of valid pipelines in the render pass
                for( const auto& pipeline : renderPass.m_Subregions )
                {
                    if( pipeline.m_Handle != VK_NULL_HANDLE )
                        requiredLineCount++;

                    if( m_Config.m_DisplayMode <= VK_PROFILER_MODE_PER_DRAWCALL_EXT )
                    {
                        // Add number of drawcalls in each pipeline
                        requiredLineCount += pipeline.m_Subregions.size();
                    }
                }
            }
        }

        if( !m_Output.NextLinesVisible( requiredLineCount ) )
        {
            m_Output.SkipLines( requiredLineCount );
            return;
        }

        m_Output.WriteLine( "VkSubmitInfo #%2u", submitIdx );

        for( uint32_t i = 0; i < submit.m_CommandBuffers.size(); ++i )
        {
            const auto& commandBuffer = submit.m_CommandBuffers[ i ];

            m_Output.WriteLine( " VkCommandBuffer (%s)",
                m_Debug.GetDebugObjectName( (uint64_t)commandBuffer.m_Handle ).c_str() );

            if( commandBuffer.m_Subregions.empty() )
            {
                continue;
            }

            if( m_Config.m_DisplayMode > VK_PROFILER_MODE_PER_RENDER_PASS_EXT )
            {
                continue;
            }

            uint32_t currentRenderPass = 0;

            while( currentRenderPass < commandBuffer.m_Subregions.size() )
            {
                const auto& renderPass = commandBuffer.m_Subregions[currentRenderPass];
                const uint32_t pipelineCount = renderPass.m_Subregions.size();
                const uint32_t drawcallCount = renderPass.m_Stats.m_TotalDrawcallCount;

                const uint32_t sublines =
                    (m_Config.m_DisplayMode <= VK_PROFILER_MODE_PER_PIPELINE_EXT) * pipelineCount +
                    (m_Config.m_DisplayMode <= VK_PROFILER_MODE_PER_DRAWCALL_EXT) * drawcallCount;

                if( !m_Output.NextLinesVisible( 1 + sublines ) )
                {
                    m_Output.SkipLines( 1 + pipelineCount );
                    currentRenderPass++;
                    continue;
                }

                if( renderPass.m_Handle != VK_NULL_HANDLE )
                {
                    // Compute time difference between timestamps
                    float us = (renderPass.m_Stats.m_TotalTicks * m_TimestampPeriod) / 1000.0f;

                    uint32_t lineWidth = 48 +
                        DigitCount( currentRenderPass ) +
                        DigitCount( pipelineCount );

                    std::string renderPassName =
                        m_Debug.GetDebugObjectName( (uint64_t)renderPass.m_Handle );

                    lineWidth += renderPassName.length();

                    m_Output.WriteLine( "  VkRenderPass #%u (%s) - %u pipelines %.*s %10.4f us",
                        currentRenderPass,
                        renderPassName.c_str(),
                        pipelineCount,
                        m_Output.Width() - lineWidth, fillLine,
                        us );
                }

                if( m_Config.m_DisplayMode > VK_PROFILER_MODE_PER_PIPELINE_EXT )
                {
                    currentRenderPass++;
                    continue;
                }

                uint32_t currentRenderPassPipeline = 0;

                while( currentRenderPassPipeline < pipelineCount )
                {
                    const auto& pipeline = renderPass.m_Subregions[currentRenderPassPipeline];
                    const size_t drawcallCount = pipeline.m_Subregions.size();

                    const uint32_t sublines =
                        (m_Config.m_DisplayMode <= VK_PROFILER_MODE_PER_DRAWCALL_EXT) * drawcallCount;

                    if( !m_Output.NextLinesVisible( 1 + sublines ) )
                    {
                        m_Output.SkipLines( 1 + drawcallCount );
                        currentRenderPassPipeline++;
                        continue;
                    }

                    if( pipeline.m_Handle != VK_NULL_HANDLE )
                    {
                        // Convert to microseconds
                        float us = (pipeline.m_Stats.m_TotalTicks * m_TimestampPeriod) / 1000.0f;

                        uint32_t lineWidth = 47 +
                            DigitCount( currentRenderPassPipeline ) +
                            DigitCount( drawcallCount );

                        std::string pipelineName = m_Debug.GetDebugObjectName( (uint64_t)pipeline.m_Handle );

                        lineWidth += pipelineName.length();

                        m_Output.WriteLine( "   VkPipeline #%u (%s) - %u drawcalls %.*s %10.4f us",
                            currentRenderPassPipeline,
                            pipelineName.c_str(),
                            drawcallCount,
                            m_Output.Width() - lineWidth, fillLine,
                            us );
                    }

                    if( m_Config.m_DisplayMode > VK_PROFILER_MODE_PER_DRAWCALL_EXT )
                    {
                        currentRenderPassPipeline++;
                        continue;
                    }

                    uint32_t currentPipelineDrawcall = 0;

                    while( currentPipelineDrawcall < drawcallCount )
                    {
                        const auto& drawcall = pipeline.m_Subregions[ currentPipelineDrawcall ];

                        if( !m_Output.NextLinesVisible( 1 ) )
                        {
                            m_Output.SkipLines( 1 );
                            currentPipelineDrawcall++;
                            continue;
                        }

                        // Convert to microseconds
                        float us = (drawcall.m_Ticks * m_TimestampPeriod) / 1000.0f;

                        uint32_t lineWidth = 50 +
                            DigitCount( currentPipelineDrawcall );

                        switch( drawcall.m_Type )
                        {
                        case ProfilerDrawcallType::eDraw:
                            m_Output.WriteLine( "    vkCmdDraw %.*s %8.4f us",
                                //drawcall.m_DrawArgs.vertexCount,
                                //drawcall.m_DrawArgs.instanceCount,
                                //drawcall.m_DrawArgs.firstVertex,
                                //drawcall.m_DrawArgs.firstInstance,
                                m_Output.Width() - lineWidth, fillLine, us );
                            break;

                        case ProfilerDrawcallType::eCopy:
                            m_Output.WriteLine( "    vkCmdCopy %.*s %8.4f us",
                                m_Output.Width() - lineWidth, fillLine, us );
                            break;
                        }


                        currentPipelineDrawcall++;
                    }

                    currentRenderPassPipeline++;
                }

                currentRenderPass++;
            }
        }
    }
    #endif

    /***********************************************************************************\

    Function:
        FreeProfilerData

    Description:

    \***********************************************************************************/
    void DeviceProfiler::FreeProfilerData( VkProfilerRegionDataEXT* pData ) const
    {
        for( uint32_t i = 0; i < pData->subregionCount; ++i )
        {
            FreeProfilerData( &pData->pSubregions[ i ] );
            delete[] pData->pSubregions;
        }

        std::free( const_cast<char*>(pData->pRegionName) );

        std::memset( pData, 0, sizeof( VkProfilerRegionDataEXT ) );
    }

    /***********************************************************************************\

    Function:
        FillProfilerData

    Description:

    \***********************************************************************************/
    void DeviceProfiler::FillProfilerData( VkProfilerRegionDataEXT* pData, const ProfilerRangeStats& stats ) const
    {
        pData->duration = 1000000.f * stats.m_TotalTicks / m_TimestampPeriod;
        pData->drawCount = stats.m_TotalDrawCount;
        pData->drawIndirectCount = stats.m_TotalDrawIndirectCount;
        pData->dispatchCount = stats.m_TotalDispatchCount;
        pData->dispatchIndirectCount = stats.m_TotalDispatchIndirectCount;
        pData->clearCount = stats.m_TotalClearCount + stats.m_TotalClearImplicitCount;
        pData->barrierCount = stats.m_TotalBarrierCount;
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
            uint32_t hash = m_ProfiledShaderModules.interlocked_at( createInfo.pStages[i].module );

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
        uint32_t hash = m_ProfiledShaderModules.interlocked_at( createInfo.stage.module );

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
}

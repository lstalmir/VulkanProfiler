#include "profiler.h"
#include "profiler_helpers.h"
#include "farmhash/farmhash.h"
#include <sstream>

namespace Profiler
{
    /***********************************************************************************\

    Function:
        PerformanceProfiler

    Description:
        Constructor

    \***********************************************************************************/
    Profiler::Profiler()
        : m_Device( nullptr )
        , m_Callbacks()
        , m_Config()
        , m_Output()
        , m_Debug()
        , m_DataAggregator()
        , m_pCurrentFrameStats( nullptr )
        , m_pPreviousFrameStats( nullptr )
        , m_CurrentFrame( 0 )
        , m_CpuTimestampCounter()
        , m_Allocations()
        , m_AllocatedMemorySize( 0 )
        , m_ProfiledCommandBuffers()
        , m_TimestampPeriod( 0.0f )
    {
        ClearMemory( &m_Callbacks );
    }

    /***********************************************************************************\

    Function:
        Initialize

    Description:
        Initializes profiler resources.

    \***********************************************************************************/
    VkResult Profiler::Initialize( const VkApplicationInfo* pApplicationInfo,
        VkPhysicalDevice physicalDevice, const VkLayerInstanceDispatchTable* pInstanceDispatchTable,
        VkDevice device, const VkLayerDispatchTable* pDispatchTable )
    {
        m_Callbacks = *pDispatchTable;
        m_Device = device;

        m_CurrentFrame = 0;

        // Load config
        std::filesystem::path customConfigPath = ProfilerPlatformFunctions::GetCustomConfigPath();

        // Check if custom configuration file exists
        if( !customConfigPath.empty() &&
            !std::filesystem::exists( customConfigPath ) )
        {
            m_Output.WriteLine( "ERROR: Custom config file %s not found",
                customConfigPath.c_str() );

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
                    m_Config.m_DisplayMode = static_cast<ProfilerMode>(value);

                if( key == "NUM_QUERIES_PER_CMD_BUFFER" )
                    m_Config.m_NumQueriesPerCommandBuffer = value;

                if( key == "OUTPUT_UPDATE_INTERVAL" )
                    m_Config.m_OutputUpdateInterval = static_cast<std::chrono::milliseconds>(value);
            }
        }

        m_Config.m_SamplingMode = m_Config.m_DisplayMode;

        // Check if preemption is enabled
        // It may break the results
        if( ProfilerPlatformFunctions::IsPreemptionEnabled() )
        {
            // Sample per drawcall to avoid DMA packet splits between timestamps
            m_Config.m_SamplingMode = ProfilerMode::ePerDrawcall;
            m_Output.Summary.Message = "Preemption enabled. Profiler will collect results per drawcall.";
        }

        // Create frame stats counters
        m_pCurrentFrameStats = new FrameStats;
        m_pPreviousFrameStats = new FrameStats; // will be swapped every frame

        // Create submit fence
        VkFenceCreateInfo fenceCreateInfo;
        ClearStructure( &fenceCreateInfo, VK_STRUCTURE_TYPE_FENCE_CREATE_INFO );

        VkResult result = m_Callbacks.CreateFence(
            m_Device, &fenceCreateInfo, nullptr, &m_SubmitFence );

        if( result != VK_SUCCESS )
        {
            // Fence creation failed
            Destroy();
            return result;
        }

        // Get GPU timestamp period
        VkPhysicalDeviceProperties physicalDeviceProperties;
        pInstanceDispatchTable->GetPhysicalDeviceProperties(
            physicalDevice, &physicalDeviceProperties );

        m_TimestampPeriod = physicalDeviceProperties.limits.timestampPeriod;

        // Update application summary view
        m_Output.Summary.Version = pApplicationInfo->apiVersion;
        m_Output.Summary.Mode = m_Config.m_DisplayMode;

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:
        Frees resources allocated by the profiler.

    \***********************************************************************************/
    void Profiler::Destroy()
    {
        m_ProfiledCommandBuffers.clear();

        delete m_pCurrentFrameStats;
        m_pCurrentFrameStats = nullptr;

        delete m_pPreviousFrameStats;
        m_pPreviousFrameStats = nullptr;

        m_Allocations.clear();
        m_AllocatedMemorySize = 0;

        if( m_SubmitFence != VK_NULL_HANDLE )
        {
            m_Callbacks.DestroyFence( m_Device, m_SubmitFence, nullptr );
            m_SubmitFence = VK_NULL_HANDLE;
        }

        m_CurrentFrame = 0;

        ClearMemory( &m_Callbacks );
        m_Device = nullptr;
    }

    /***********************************************************************************\

    Function:
        SetDebugObjectName

    Description:
        Assign user-defined name to the object

    \***********************************************************************************/
    void Profiler::SetDebugObjectName( uint64_t objectHandle, const char* pObjectName )
    {
        m_Debug.SetDebugObjectName( objectHandle, pObjectName );
    }

    /***********************************************************************************\

    Function:
        PreDraw

    Description:

    \***********************************************************************************/
    void Profiler::PreDraw( VkCommandBuffer commandBuffer )
    {
        // ProfilerCommandBuffer object should already be in the map
        auto& profilerCommandBuffer = m_ProfiledCommandBuffers.interlocked_at( commandBuffer );

        profilerCommandBuffer.PreDraw();
    }

    /***********************************************************************************\

    Function:
        PostDraw

    Description:

    \***********************************************************************************/
    void Profiler::PostDraw( VkCommandBuffer commandBuffer )
    {
        // ProfilerCommandBuffer object should already be in the map
        auto& profilerCommandBuffer = m_ProfiledCommandBuffers.interlocked_at( commandBuffer );

        profilerCommandBuffer.PostDraw();
    }

    /***********************************************************************************\

    Function:
        PreDrawIndirect

    Description:

    \***********************************************************************************/
    void Profiler::PreDrawIndirect( VkCommandBuffer commandBuffer )
    {
        // ProfilerCommandBuffer object should already be in the map
        auto& profilerCommandBuffer = m_ProfiledCommandBuffers.interlocked_at( commandBuffer );

        profilerCommandBuffer.PreDrawIndirect();
    }

    /***********************************************************************************\

    Function:
        PostDrawIndirect

    Description:

    \***********************************************************************************/
    void Profiler::PostDrawIndirect( VkCommandBuffer commandBuffer )
    {
        // ProfilerCommandBuffer object should already be in the map
        auto& profilerCommandBuffer = m_ProfiledCommandBuffers.interlocked_at( commandBuffer );

        profilerCommandBuffer.PostDrawIndirect();
    }

    /***********************************************************************************\

    Function:
        PreDispatch

    Description:

    \***********************************************************************************/
    void Profiler::PreDispatch( VkCommandBuffer commandBuffer )
    {
        // ProfilerCommandBuffer object should already be in the map
        auto& profilerCommandBuffer = m_ProfiledCommandBuffers.interlocked_at( commandBuffer );

        profilerCommandBuffer.PreDispatch();
    }

    /***********************************************************************************\

    Function:
        PostDispatch

    Description:

    \***********************************************************************************/
    void Profiler::PostDispatch( VkCommandBuffer commandBuffer )
    {
        // ProfilerCommandBuffer object should already be in the map
        auto& profilerCommandBuffer = m_ProfiledCommandBuffers.interlocked_at( commandBuffer );

        profilerCommandBuffer.PostDispatch();
    }

    /***********************************************************************************\

    Function:
        PreDispatchIndirect

    Description:

    \***********************************************************************************/
    void Profiler::PreDispatchIndirect( VkCommandBuffer commandBuffer )
    {
        // ProfilerCommandBuffer object should already be in the map
        auto& profilerCommandBuffer = m_ProfiledCommandBuffers.interlocked_at( commandBuffer );

        profilerCommandBuffer.PreDispatchIndirect();
    }

    /***********************************************************************************\

    Function:
        PostDispatchIndirect

    Description:

    \***********************************************************************************/
    void Profiler::PostDispatchIndirect( VkCommandBuffer commandBuffer )
    {
        // ProfilerCommandBuffer object should already be in the map
        auto& profilerCommandBuffer = m_ProfiledCommandBuffers.interlocked_at( commandBuffer );

        profilerCommandBuffer.PostDispatchIndirect();
    }

    /***********************************************************************************\

    Function:
        PreCopy

    Description:

    \***********************************************************************************/
    void Profiler::PreCopy( VkCommandBuffer commandBuffer )
    {
        // ProfilerCommandBuffer object should already be in the map
        auto& profilerCommandBuffer = m_ProfiledCommandBuffers.interlocked_at( commandBuffer );

        profilerCommandBuffer.PreCopy();
    }

    /***********************************************************************************\

    Function:
        PostCopy

    Description:

    \***********************************************************************************/
    void Profiler::PostCopy( VkCommandBuffer commandBuffer )
    {
        // ProfilerCommandBuffer object should already be in the map
        auto& profilerCommandBuffer = m_ProfiledCommandBuffers.interlocked_at( commandBuffer );

        profilerCommandBuffer.PostCopy();
    }

    /***********************************************************************************\

    Function:
        PreClear

    Description:

    \***********************************************************************************/
    void Profiler::PreClear( VkCommandBuffer commandBuffer )
    {
        // ProfilerCommandBuffer object should already be in the map
        auto& profiledCommandBuffer = m_ProfiledCommandBuffers.interlocked_at( commandBuffer );

        profiledCommandBuffer.PreClear();
    }

    /***********************************************************************************\

    Function:
        PostClear

    Description:

    \***********************************************************************************/
    void Profiler::PostClear( VkCommandBuffer commandBuffer, uint32_t attachmentCount )
    {
        // ProfilerCommandBuffer object should already be in the map
        auto& profiledCommandBuffer = m_ProfiledCommandBuffers.interlocked_at( commandBuffer );

        profiledCommandBuffer.PostClear( attachmentCount );
    }

    /***********************************************************************************\

    Function:
        PipelineBarrier

    Description:
        Collects barrier statistics.

    \***********************************************************************************/
    void Profiler::OnPipelineBarrier( VkCommandBuffer commandBuffer,
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

    \***********************************************************************************/
    void Profiler::CreatePipelines( uint32_t pipelineCount, const VkGraphicsPipelineCreateInfo* pCreateInfos, VkPipeline* pPipelines )
    {
        for( uint32_t i = 0; i < pipelineCount; ++i )
        {
            ProfilerPipeline profilerPipeline;
            profilerPipeline.m_Handle = pPipelines[i];
            profilerPipeline.m_ShaderTuple = CreateShaderTuple( pCreateInfos[i] );

            std::stringstream stringBuilder;
            stringBuilder
                << "VS=" << std::hex << std::setfill( '0' ) << std::setw( 8 )
                << profilerPipeline.m_ShaderTuple.m_Vert
                << ",PS=" << std::hex << std::setfill( '0' ) << std::setw( 8 )
                << profilerPipeline.m_ShaderTuple.m_Frag;

            m_Debug.SetDebugObjectName((uint64_t)profilerPipeline.m_Handle,
                stringBuilder.str().c_str() );

            m_ProfiledPipelines.interlocked_emplace( pPipelines[i], profilerPipeline );
        }
    }

    /***********************************************************************************\

    Function:
        DestroyPipeline

    Description:

    \***********************************************************************************/
    void Profiler::DestroyPipeline( VkPipeline pipeline )
    {
        m_ProfiledPipelines.interlocked_erase( pipeline );
    }

    /***********************************************************************************\

    Function:
        BindPipeline

    Description:

    \***********************************************************************************/
    void Profiler::BindPipeline( VkCommandBuffer commandBuffer, VkPipeline pipeline )
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
    void Profiler::CreateShaderModule( VkShaderModule module, const VkShaderModuleCreateInfo* pCreateInfo )
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
    void Profiler::DestroyShaderModule( VkShaderModule module )
    {
        m_ProfiledShaderModules.interlocked_erase( module );
    }

    /***********************************************************************************\

    Function:
        BeginRenderPass

    Description:

    \***********************************************************************************/
    void Profiler::BeginRenderPass( VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo* pBeginInfo )
    {
        // ProfilerCommandBuffer object should already be in the map
        auto& profilerCommandBuffer = m_ProfiledCommandBuffers.interlocked_at( commandBuffer );

        profilerCommandBuffer.BeginRenderPass( pBeginInfo );
    }

    /***********************************************************************************\

    Function:
        EndRenderPass

    Description:

    \***********************************************************************************/
    void Profiler::EndRenderPass( VkCommandBuffer commandBuffer )
    {
        // ProfilerCommandBuffer object should already be in the map
        auto& profilerCommandBuffer = m_ProfiledCommandBuffers.interlocked_at( commandBuffer );

        profilerCommandBuffer.EndRenderPass();
    }

    /***********************************************************************************\

    Function:
        BeginCommandBuffer

    Description:

    \***********************************************************************************/
    void Profiler::BeginCommandBuffer( VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo* pBeginInfo )
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
    void Profiler::EndCommandBuffer( VkCommandBuffer commandBuffer )
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
    void Profiler::FreeCommandBuffers( uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers )
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

    /***********************************************************************************\

    Function:
        PostSubmitCommandBuffers

    Description:

    \***********************************************************************************/
    void Profiler::PostSubmitCommandBuffers( VkQueue queue, uint32_t count, const VkSubmitInfo* pSubmitInfo, VkFence fence )
    {
        // Wait for the submitted command buffers to execute
        if( m_Config.m_SamplingMode < ProfilerMode::ePerFrame )
        {
            m_Callbacks.QueueSubmit( queue, 0, nullptr, m_SubmitFence );
            m_Callbacks.WaitForFences( m_Device, 1, &m_SubmitFence, true, std::numeric_limits<uint64_t>::max() );
        }

        // Store submitted command buffers and get results
        for( uint32_t submitIdx = 0; submitIdx < count; ++submitIdx )
        {
            const VkSubmitInfo& submitInfo = pSubmitInfo[submitIdx];

            // Wrap submit info into our structure
            ProfilerSubmitData submit;

            for( uint32_t commandBufferIdx = 0; commandBufferIdx < submitInfo.commandBufferCount; ++commandBufferIdx )
            {
                // Get command buffer handle
                VkCommandBuffer commandBuffer = submitInfo.pCommandBuffers[commandBufferIdx];

                // Block access from other threads
                std::scoped_lock lk( m_ProfiledCommandBuffers );

                auto& profilerCommandBuffer = m_ProfiledCommandBuffers.at( commandBuffer );

                // Dirty command buffer profiling data
                profilerCommandBuffer.Submit();

                submit.m_CommandBuffers.push_back( profilerCommandBuffer.GetData() );
            }

            // Store the submit wrapper
            m_DataAggregator.AppendData( submit );
        }
    }

    /***********************************************************************************\

    Function:
        PrePresent

    Description:
    
    \***********************************************************************************/
    void Profiler::PrePresent( VkQueue queue )
    {
    }

    /***********************************************************************************\

    Function:
        PostPresent

    Description:

    \***********************************************************************************/
    void Profiler::PostPresent( VkQueue queue )
    {
        m_CurrentFrame++;

        m_CpuTimestampCounter.End();

        // TMP
        ProfilerAggregatedData data = m_DataAggregator.GetAggregatedData();

        if( m_CpuTimestampCounter.GetValue<std::chrono::milliseconds>() > m_Config.m_OutputUpdateInterval )
        {
            // Present profiling results
            PresentResults( data );

            m_CpuTimestampCounter.Begin();
        }

        m_LastFrameBeginTimestamp = data.m_Stats.m_BeginTimestamp;

        // TMP
        m_DataAggregator.Reset();
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:

    \***********************************************************************************/
    void Profiler::OnAllocateMemory( VkDeviceMemory allocatedMemory, const VkMemoryAllocateInfo* pAllocateInfo )
    {
        // Insert allocation info to the map, it will be needed during deallocation.
        m_Allocations.emplace( allocatedMemory, *pAllocateInfo );

        m_AllocatedMemorySize += pAllocateInfo->allocationSize;
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:

    \***********************************************************************************/
    void Profiler::OnFreeMemory( VkDeviceMemory allocatedMemory )
    {
        auto it = m_Allocations.find( allocatedMemory );
        if( it != m_Allocations.end() )
        {
            m_AllocatedMemorySize -= it->second.allocationSize;

            // Remove allocation entry from the map
            m_Allocations.erase( it );
        }
    }

    /***********************************************************************************\

    Function:
        GetCurrentFrameStats

    Description:

    \***********************************************************************************/
    FrameStats& Profiler::GetCurrentFrameStats()
    {
        return *m_pCurrentFrameStats;
    }

    /***********************************************************************************\

    Function:
        GetPreviousFrameStats

    Description:

    \***********************************************************************************/
    const FrameStats& Profiler::GetPreviousFrameStats() const
    {
        return *m_pPreviousFrameStats;
    }

    /***********************************************************************************\

    Function:
        PresentResults

    Description:

    \***********************************************************************************/
    void Profiler::PresentResults( const ProfilerAggregatedData& data )
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
    void Profiler::PresentSubmit( uint32_t submitIdx, const ProfilerSubmitData& submit )
    {
        // Calculate how many lines this submit will generate
        uint32_t requiredLineCount = 1 + submit.m_CommandBuffers.size();

        if( m_Config.m_DisplayMode <= ProfilerMode::ePerRenderPass )
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

                    if( m_Config.m_DisplayMode <= ProfilerMode::ePerDrawcall )
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

            if( m_Config.m_DisplayMode > ProfilerMode::ePerRenderPass )
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
                    (m_Config.m_DisplayMode <= ProfilerMode::ePerPipeline) * pipelineCount +
                    (m_Config.m_DisplayMode <= ProfilerMode::ePerDrawcall) * drawcallCount;

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

                if( m_Config.m_DisplayMode > ProfilerMode::ePerPipeline )
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
                        (m_Config.m_DisplayMode <= ProfilerMode::ePerDrawcall) * drawcallCount;

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

                    if( m_Config.m_DisplayMode > ProfilerMode::ePerDrawcall )
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

    /***********************************************************************************\

    Function:
        CreateShaderTuple

    Description:

    \***********************************************************************************/
    ProfilerShaderTuple Profiler::CreateShaderTuple( const VkGraphicsPipelineCreateInfo& createInfo )
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

                m_Output.WriteLine( "WARNING: Unsupported graphics shader stage (%u)",
                    createInfo.pStages[i].stage );
            }
            }
        }

        // Compute aggregated tuple hash for fast comparison
        tuple.m_Hash = Hash::Fingerprint32( reinterpret_cast<const char*>(&tuple), sizeof( tuple ) );

        return tuple;
    }
}

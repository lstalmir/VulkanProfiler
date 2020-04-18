#include "profiler.h"
#include "profiler_helpers.h"
#include "farmhash/src/farmhash.h"
#include <sstream>

#undef max

namespace Profiler
{
    /***********************************************************************************\

    Function:
        PerformanceProfiler

    Description:
        Constructor

    \***********************************************************************************/
    Profiler::Profiler()
        : m_pDevice( nullptr )
        , m_Config()
        , m_Output()
        , m_Overlay( *this )
        , m_Debug()
        , m_DataAggregator()
        , m_pCurrentFrameStats( nullptr )
        , m_pPreviousFrameStats( nullptr )
        , m_CurrentFrame( 0 )
        , m_CpuTimestampCounter()
        , m_Allocations()
        , m_AllocatedMemorySize( 0 )
        , m_ProfiledCommandBuffers()
        , m_IsFirstSubmitInFrame( false )
        , m_TimestampPeriod( 0.0f )
    {
    }

    /***********************************************************************************\

    Function:
        Initialize

    Description:
        Initializes profiler resources.

    \***********************************************************************************/
    VkResult Profiler::Initialize( VkDevice_Object* pDevice )
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
                    m_Config.m_Mode = static_cast<ProfilerMode>(value);

                if( key == "NUM_QUERIES_PER_CMD_BUFFER" )
                    m_Config.m_NumQueriesPerCommandBuffer = value;

                if( key == "OUTPUT_UPDATE_INTERVAL" )
                    m_Config.m_OutputUpdateInterval = static_cast<std::chrono::milliseconds>(value);
            }
        }
        
        // Create frame stats counters
        m_pCurrentFrameStats = new FrameStats;
        m_pPreviousFrameStats = new FrameStats; // will be swapped every frame

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

        m_IsFirstSubmitInFrame = true;

        // Get GPU timestamp period
        VkPhysicalDeviceProperties physicalDeviceProperties;
        m_pDevice->pInstance->Callbacks.GetPhysicalDeviceProperties(
            m_pDevice->PhysicalDevice, &physicalDeviceProperties );

        m_TimestampPeriod = physicalDeviceProperties.limits.timestampPeriod;

        // Update application summary view
        m_Output.Summary.Version = m_pDevice->pInstance->ApplicationInfo.apiVersion;
        m_Output.Summary.Mode = m_Config.m_Mode;

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
            m_pDevice->Callbacks.DestroyFence( m_pDevice->Handle, m_SubmitFence, nullptr );
            m_SubmitFence = VK_NULL_HANDLE;
        }

        m_CurrentFrame = 0;
        m_pDevice = nullptr;
    }

    /***********************************************************************************\

    \***********************************************************************************/
    VkResult Profiler::SetMode( ProfilerMode mode )
    {
        // TODO: Invalidate all command buffers
        m_Config.m_Mode = mode;

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    \***********************************************************************************/
    void Profiler::CreateSwapchain( const VkSwapchainCreateInfoKHR* pCreateInfo, VkSwapchainKHR swapchain )
    {
        // Initialize overlay output
        m_Overlay.Initialize( pCreateInfo, swapchain );
    }

    /***********************************************************************************\

    \***********************************************************************************/
    void Profiler::DestroySwapchain( VkSwapchainKHR swapchain )
    {
        m_Overlay.Destroy();
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
        Executed before drawcall

    \***********************************************************************************/
    void Profiler::PreDraw( VkCommandBuffer commandBuffer )
    {
        // ProfilerCommandBuffer object should already be in the map
        auto& profilerCommandBuffer = m_ProfiledCommandBuffers.interlocked_at( commandBuffer );

        profilerCommandBuffer.Draw();
    }

    /***********************************************************************************\

    Function:
        PostDraw

    Description:
        Executed after drawcall

    \***********************************************************************************/
    void Profiler::PostDraw( VkCommandBuffer commandBuffer )
    {
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
            profilerPipeline.m_Pipeline = pPipelines[i];
            profilerPipeline.m_ShaderTuple = CreateShaderTuple( pCreateInfos[i] );

            std::stringstream stringBuilder;
            stringBuilder
                << "(VS=" << std::hex << std::setfill( '0' ) << std::setw( 8 )
                << profilerPipeline.m_ShaderTuple.m_Vert
                << ",PS=" << std::hex << std::setfill( '0' ) << std::setw( 8 )
                << profilerPipeline.m_ShaderTuple.m_Frag
                << ")";

            m_Debug.SetDebugObjectName((uint64_t)profilerPipeline.m_Pipeline,
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
        auto& profilerPipeline = m_ProfiledPipelines.interlocked_at( pipeline );

        profilerCommandBuffer.BindPipeline( profilerPipeline );
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
    void Profiler::BeginRenderPass( VkCommandBuffer commandBuffer, VkRenderPass renderPass )
    {
        // ProfilerCommandBuffer object should already be in the map
        auto& profilerCommandBuffer = m_ProfiledCommandBuffers.interlocked_at( commandBuffer );

        profilerCommandBuffer.BeginRenderPass( renderPass );
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
        if( m_Config.m_Mode < ProfilerMode::ePerFrame )
        {
            m_pDevice->Callbacks.QueueSubmit( queue, 0, nullptr, m_SubmitFence );
            m_pDevice->Callbacks.WaitForFences( m_pDevice->Handle, 1, &m_SubmitFence, true, std::numeric_limits<uint64_t>::max() );
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
    void Profiler::AcquireNextImage( uint32_t acquiredImageIndex )
    {
        m_Overlay.AcquireNextImage( acquiredImageIndex );
    }

    /***********************************************************************************\

    Function:
        PostPresent

    Description:

    \***********************************************************************************/
    void Profiler::Present( VkPresentInfoKHR* pPresentInfo )
    {
        m_CurrentFrame++;

        m_CpuTimestampCounter.End();

        //if( m_CpuTimestampCounter.GetValue<std::chrono::milliseconds>() > m_Config.m_OutputUpdateInterval )
        {
            // Present profiling results
            PresentResults();

            m_CpuTimestampCounter.Begin();
        }

        m_Overlay.Flush();
        m_Overlay.Present( pPresentInfo );
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
    void Profiler::PresentResults()
    {
        static constexpr char fillLine[] =
            "...................................................................................................."
            "...................................................................................................."
            "...................................................................................................."
            "....................................................................................................";

        m_Overlay.BeginWindow();

        const uint32_t consoleWidth = m_Output.Width();
        uint32_t lineWidth = 0;

        std::vector<ProfilerSubmitData> submits = m_DataAggregator.GetAggregatedData();

        for( uint32_t submitIdx = 0; submitIdx < submits.size(); ++submitIdx )
        {
            // Get the submit info wrapper
            const auto& submit = submits.at( submitIdx );

            m_Overlay.WriteSubmit( submit );

            m_Output.WriteLine( "VkSubmitInfo #%-2u",
                submitIdx );

            for( uint32_t i = 0; i < submit.m_CommandBuffers.size(); ++i )
            {
                const auto& data = submit.m_CommandBuffers[i];

                m_Output.WriteLine( " VkCommandBuffer (%s)",
                    m_Debug.GetDebugObjectName( (uint64_t)data.m_CommandBuffer ).c_str() );

                if( data.m_CollectedTimestamps.empty() )
                {
                    m_Output.WriteLine( "  No profiling data was collected for this command buffer" );
                    continue;
                }

                switch( m_Config.m_Mode )
                {
                case ProfilerMode::ePerRenderPass:
                {
                    // Each render pass has 2 timestamps - begin and end
                    for( uint32_t i = 0; i < data.m_CollectedTimestamps.size(); i += 2 )
                    {
                        uint64_t beginTimestamp = data.m_CollectedTimestamps[i];
                        uint64_t endTimestamp = data.m_CollectedTimestamps[i + 1];

                        const uint32_t currentRenderPass = i >> 1;

                        // Compute time difference between timestamps
                        uint64_t dt = endTimestamp - beginTimestamp;

                        // Convert to microseconds
                        float us = (dt * m_TimestampPeriod) / 1000.0f;

                        lineWidth = 46 +
                            DigitCount( currentRenderPass );

                        std::string renderPassName =
                            m_Debug.GetDebugObjectName( (uint64_t)data.m_RenderPassPipelineCount[currentRenderPass].first );

                        lineWidth += renderPassName.length();

                        m_Output.WriteLine( "  VkRenderPass #%u (%s) - %.*s %8.4f us",
                            currentRenderPass,
                            renderPassName.c_str(),
                            consoleWidth - lineWidth, fillLine,
                            us );
                    }
                    break;
                }

                case ProfilerMode::ePerPipeline:
                {
                    // Pipelines are separated with timestamps
                    uint64_t beginTimestamp = data.m_CollectedTimestamps[0];

                    uint32_t currentPipeline = 1;
                    uint32_t currentRenderPass = 0;

                    while( currentRenderPass < data.m_RenderPassPipelineCount.size() )
                    {
                        const auto& renderPassPipelineCount = data.m_RenderPassPipelineCount[currentRenderPass];

                        // Get last timestamp in this render pass
                        uint64_t renderPassEndTimestamp = data.m_CollectedTimestamps[currentPipeline + renderPassPipelineCount.second - 1];

                        // Compute time difference between timestamps
                        uint64_t dt = renderPassEndTimestamp - beginTimestamp;

                        // Convert to microseconds
                        float us = (dt * m_TimestampPeriod) / 1000.0f;

                        lineWidth = 46 +
                            DigitCount( currentRenderPass ) +
                            DigitCount( renderPassPipelineCount.second );

                        std::string renderPassName =
                            m_Debug.GetDebugObjectName( (uint64_t)renderPassPipelineCount.first );

                        lineWidth += renderPassName.length();

                        m_Output.WriteLine( "  VkRenderPass #%u (%s) - %u pipelines %.*s %8.4f us",
                            currentRenderPass,
                            renderPassName.c_str(),
                            renderPassPipelineCount.second,
                            consoleWidth - lineWidth, fillLine,
                            us );

                        uint32_t currentRenderPassPipeline = 0;

                        while( currentRenderPassPipeline < renderPassPipelineCount.second )
                        {
                            const auto& pipelineDrawcallCount = data.m_PipelineDrawCount[currentRenderPassPipeline];

                            uint64_t endTimestamp = data.m_CollectedTimestamps[currentPipeline];

                            // Compute time difference between timestamps
                            dt = endTimestamp - beginTimestamp;

                            // Convert to microseconds
                            us = (dt * m_TimestampPeriod) / 1000.0f;

                            lineWidth = 45 +
                                DigitCount( currentRenderPassPipeline ) +
                                DigitCount( pipelineDrawcallCount.second );

                            std::string pipelineName = m_Debug.GetDebugObjectName( (uint64_t)pipelineDrawcallCount.first.m_Pipeline );

                            lineWidth += pipelineName.length();

                            m_Output.WriteLine( "   VkPipeline #%u (%s) - %u drawcalls %.*s %8.4f us",
                                currentRenderPassPipeline,
                                pipelineName.c_str(),
                                pipelineDrawcallCount.second,
                                consoleWidth - lineWidth, fillLine,
                                us );

                            beginTimestamp = endTimestamp;

                            currentPipeline++;
                            currentRenderPassPipeline++;
                        }

                        currentRenderPass++;
                    }

                    break;
                }

                case ProfilerMode::ePerDrawcall:
                {

                }
                }
            }
        }

        m_Output.WriteLine( "" );
        m_Output.Flush();

        m_Overlay.EndWindow();
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

        return tuple;
    }
}

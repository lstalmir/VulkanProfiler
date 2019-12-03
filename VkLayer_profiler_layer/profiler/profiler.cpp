#include "profiler.h"
#include "profiler_helpers.h"

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
        , m_Mode( ProfilerMode::ePerPipeline )
        , m_Output()
        , m_pCurrentFrameStats( nullptr )
        , m_pPreviousFrameStats( nullptr )
        , m_CurrentFrame( 0 )
        , m_pCpuTimestampQueryPool( nullptr )
        , m_TimestampQueryPoolSize( 0 )
        , m_CurrentCpuTimestampQuery( 0 )
        , m_Allocations()
        , m_AllocatedMemorySize( 0 )
        , m_ProfiledCommandBuffers()
        , m_HelperCommandPool( nullptr )
        , m_HelperCommandBuffer( nullptr )
        , m_IsFirstSubmitInFrame( false )
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

        m_TimestampQueryPoolSize = 128;

        // Create the CPU timestamp query pool
        m_pCpuTimestampQueryPool = new CpuTimestampCounter[m_TimestampQueryPoolSize];
        m_CurrentCpuTimestampQuery = 0;

        // Create frame stats counters
        m_pCurrentFrameStats = new FrameStats;
        m_pPreviousFrameStats = new FrameStats; // will be swapped every frame

        // Create submit fence
        VkStructure<VkFenceCreateInfo> fenceCreateInfo;
        
        VkResult result = m_Callbacks.CreateFence(
            m_Device, &fenceCreateInfo, nullptr, &m_SubmitFence );

        if( result != VK_SUCCESS )
        {
            // Fence creation failed
            Destroy();
            return result;
        }

        // Prepare helper command buffer
        VkStructure<VkCommandPoolCreateInfo> commandPoolCreateInfo;
        commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        commandPoolCreateInfo.queueFamilyIndex = 0; // TODO

        result = m_Callbacks.CreateCommandPool(
            m_Device, &commandPoolCreateInfo, nullptr, &m_HelperCommandPool );

        if( result != VK_SUCCESS )
        {
            // Command pool allocation failed
            Destroy();
            return result;
        }

        VkStructure<VkCommandBufferAllocateInfo> commandBufferAllocateInfo;
        commandBufferAllocateInfo.commandPool = m_HelperCommandPool;
        commandBufferAllocateInfo.commandBufferCount = 1;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        result = m_Callbacks.AllocateCommandBuffers(
            m_Device, &commandBufferAllocateInfo, &m_HelperCommandBuffer );

        if( result != VK_SUCCESS )
        {
            // Command buffer allocation failed
            Destroy();
            return result;
        }

        VkStructure<VkSemaphoreCreateInfo> commandBufferSemaphoreCreateInfo;

        result = m_Callbacks.CreateSemaphore(
            m_Device, &commandBufferSemaphoreCreateInfo, nullptr, &m_HelperCommandBufferExecutionSemaphore );

        if( result != VK_SUCCESS )
        {
            Destroy();
            return result;
        }

        m_IsFirstSubmitInFrame = true;

        // Get GPU timestamp period
        VkPhysicalDeviceProperties physicalDeviceProperties;
        pInstanceDispatchTable->GetPhysicalDeviceProperties(
            physicalDevice, &physicalDeviceProperties );

        m_TimestampPeriod = physicalDeviceProperties.limits.timestampPeriod;

        // Update application summary view
        m_Output.Summary.Version = pApplicationInfo->apiVersion;
        m_Output.Summary.Mode = m_Mode;

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

        delete m_pCpuTimestampQueryPool;
        m_pCpuTimestampQueryPool = nullptr;

        m_CurrentCpuTimestampQuery = 0;
        m_TimestampQueryPoolSize = 0;

        m_Allocations.clear();
        m_AllocatedMemorySize = 0;

        if( m_HelperCommandBuffer != VK_NULL_HANDLE )
        {
            m_Callbacks.FreeCommandBuffers( m_Device, m_HelperCommandPool, 1, &m_HelperCommandBuffer );
            m_HelperCommandBuffer = VK_NULL_HANDLE;
        }

        if( m_HelperCommandPool != VK_NULL_HANDLE )
        {
            m_Callbacks.DestroyCommandPool( m_Device, m_HelperCommandPool, nullptr );
            m_HelperCommandPool = VK_NULL_HANDLE;
        }

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
        Executed before drawcall

    \***********************************************************************************/
    void Profiler::PreDraw( VkCommandBuffer commandBuffer )
    {
        // ProfilerCommandBuffer object should already be in the map
        auto& profilerCommandBuffer = m_ProfiledCommandBuffers.at( commandBuffer );

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
        BindPipeline

    Description:

    \***********************************************************************************/
    void Profiler::BindPipeline( VkCommandBuffer commandBuffer, VkPipeline pipeline )
    {
        // ProfilerCommandBuffer object should already be in the map
        auto& profilerCommandBuffer = m_ProfiledCommandBuffers.at( commandBuffer );

        profilerCommandBuffer.BindPipeline( pipeline );
    }

    /***********************************************************************************\

    Function:
        BeginRenderPass

    Description:

    \***********************************************************************************/
    void Profiler::BeginRenderPass( VkCommandBuffer commandBuffer, VkRenderPass renderPass )
    {
        // ProfilerCommandBuffer object should already be in the map
        auto& profilerCommandBuffer = m_ProfiledCommandBuffers.at( commandBuffer );

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
        auto& profilerCommandBuffer = m_ProfiledCommandBuffers.at( commandBuffer );

        profilerCommandBuffer.EndRenderPass();
    }

    /***********************************************************************************\

    Function:
        BeginCommandBuffer

    Description:

    \***********************************************************************************/
    void Profiler::BeginCommandBuffer( VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo* pBeginInfo )
    {
        auto emplaced = m_ProfiledCommandBuffers.try_emplace( commandBuffer, *this, commandBuffer );

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
        auto& profilerCommandBuffer = m_ProfiledCommandBuffers.at( commandBuffer );

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
        if( m_Mode < ProfilerMode::ePerFrame )
        {
            m_Callbacks.QueueSubmit( queue, 0, nullptr, m_SubmitFence );
            m_Callbacks.WaitForFences( m_Device, 1, &m_SubmitFence, true, std::numeric_limits<uint64_t>::max() );
        }

        // Store submitted command buffers and get results
        for( uint32_t submitIdx = 0; submitIdx < count; ++submitIdx )
        {
            const VkSubmitInfo& submitInfo = pSubmitInfo[submitIdx];

            // Wrap submit info into our structure
            Submit submit;

            for( uint32_t commandBufferIdx = 0; commandBufferIdx < submitInfo.commandBufferCount; ++commandBufferIdx )
            {
                // Get command buffer handle
                VkCommandBuffer commandBuffer = submitInfo.pCommandBuffers[commandBufferIdx];

                submit.m_CommandBuffers.push_back( commandBuffer );

                auto& profilerCommandBuffer = m_ProfiledCommandBuffers.at( commandBuffer );

                // Dirty command buffer profiling data
                profilerCommandBuffer.Submit();

                submit.m_ProfilingData.push_back( profilerCommandBuffer.GetData() );
            }

            // Store the submit wrapper
            m_Submits.push_back( submit );
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

        // Present profiling results
        PresentResults();

        m_Submits.clear();
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

        const uint32_t consoleWidth = m_Output.Width();
        uint32_t lineWidth = 0;

        for( uint32_t submitIdx = 0; submitIdx < m_Submits.size(); ++submitIdx )
        {
            // Get the submit info wrapper
            const auto& submit = m_Submits.at( submitIdx );

            m_Output.WriteLine( "VkSubmitInfo #%-2u",
                submitIdx );

            for( uint32_t i = 0; i < submit.m_CommandBuffers.size(); ++i )
            {
                m_Output.WriteLine( " VkCommandBuffer (%s)",
                    m_Debug.GetDebugObjectName( (uint64_t)submit.m_CommandBuffers[i] ).c_str() );

                const auto& data = submit.m_ProfilingData[i];

                if( data.m_CollectedTimestamps.empty() )
                {
                    m_Output.WriteLine( "  No profiling data was collected for this command buffer" );
                    return;
                }

                switch( m_Mode )
                {
                case ProfilerMode::ePerRenderPass:
                {
                    // Each render pass has 2 timestamps - begin and end
                    for( uint32_t i = 0; i < data.m_CollectedTimestamps.size(); i += 2 )
                    {
                        uint64_t beginTimestamp = data.m_CollectedTimestamps[i];
                        uint64_t endTimestamp = data.m_CollectedTimestamps[i + 1];

                        // Compute time difference between timestamps
                        uint64_t dt = endTimestamp - beginTimestamp;

                        // Convert to microseconds
                        float us = (dt * m_TimestampPeriod) / 1000.0f;

                        m_Output.WriteLine( "  VkRenderPass #%-2u - %f us", i >> 1, us );
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

                            std::string pipelineName =
                                m_Debug.GetDebugObjectName( (uint64_t)pipelineDrawcallCount.first );

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
                }
            }
        }

        m_Output.WriteLine( "" );
        m_Output.Flush();
    }
}

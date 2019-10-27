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
        , m_Mode( ProfilerMode::ePerRenderPass )
        , m_pCurrentFrameStats( nullptr )
        , m_pPreviousFrameStats( nullptr )
        , m_CurrentFrame( 0 )
        , m_pCpuTimestampQueryPool( nullptr )
        , m_TimestampQueryPoolSize( 0 )
        , m_CurrentCpuTimestampQuery( 0 )
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
    VkResult Profiler::Initialize(
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

        // Get GPU timestamp period
        VkPhysicalDeviceProperties physicalDeviceProperties;
        pInstanceDispatchTable->GetPhysicalDeviceProperties(
            physicalDevice, &physicalDeviceProperties );

        m_TimestampPeriod = physicalDeviceProperties.limits.timestampPeriod;

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

        m_CurrentFrame = 0;

        ClearMemory( &m_Callbacks );
        m_Device = nullptr;
    }

    /***********************************************************************************\

    Function:
        PreDraw

    Description:
        Executed before drawcall

    \***********************************************************************************/
    void Profiler::PreDraw( VkCommandBuffer commandBuffer )
    {
        if( m_Mode == ProfilerMode::ePerDrawcall )
        {
            // ProfilerCommandBuffer object should already be in the map
            auto& profilerCommandBuffer = m_ProfiledCommandBuffers.at( commandBuffer );

            SendTimestampQuery( profilerCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );
        }
    }

    /***********************************************************************************\

    Function:
        PostDraw

    Description:
        Executed after drawcall

    \***********************************************************************************/
    void Profiler::PostDraw( VkCommandBuffer commandBuffer )
    {
        if( m_Mode == ProfilerMode::ePerDrawcall )
        {
            // ProfilerCommandBuffer object should already be in the map
            auto& profilerCommandBuffer = m_ProfiledCommandBuffers.at( commandBuffer );

            SendTimestampQuery( profilerCommandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT );
        }
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

        if( m_Mode == ProfilerMode::ePerPipeline )
        {
            SendTimestampQuery( profilerCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );
        }

        // Update command buffer state
        profilerCommandBuffer.m_CurrentPipeline = pipeline;
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

        if( m_Mode == ProfilerMode::ePerRenderPass )
        {
            SendTimestampQuery( profilerCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );
        }

        // Update command buffer state
        profilerCommandBuffer.m_CurrentRenderPass = renderPass;
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

        if( m_Mode == ProfilerMode::ePerRenderPass ||
            m_Mode == ProfilerMode::ePerPipeline )
        {
            SendTimestampQuery( profilerCommandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT );
        }

        // Update command buffer state
        profilerCommandBuffer.m_CurrentRenderPass = VK_NULL_HANDLE;
        profilerCommandBuffer.m_CurrentPipeline = VK_NULL_HANDLE;
    }

    /***********************************************************************************\

    Function:
        BeginCommandBuffer

    Description:

    \***********************************************************************************/
    void Profiler::BeginCommandBuffer( VkCommandBuffer commandBuffer )
    {
        auto emplaced = m_ProfiledCommandBuffers.try_emplace( commandBuffer );

        // Grab reference to the ProfilerCommandBuffer instance
        auto& profilerCommandBuffer = emplaced.first->second;

        if( emplaced.second )
        {
            // New ProfilerCommandBuffer created
            profilerCommandBuffer.m_CommandBuffer = commandBuffer;
            profilerCommandBuffer.m_ExecutionFence = VK_NULL_HANDLE;
            profilerCommandBuffer.m_IsExecuting = false;
            profilerCommandBuffer.m_ReallocPool = false;
            profilerCommandBuffer.m_TimestampQueryPool = VK_NULL_HANDLE;
            profilerCommandBuffer.m_TimestampQueryPoolSize = 0;
            profilerCommandBuffer.m_TimestampQueryResults = nullptr;
            profilerCommandBuffer.m_TimestampQueryResultsSize = 0;
        }
        else
        {
            // Already profiling this command buffer
            if( profilerCommandBuffer.m_ReallocPool )
            {
                // Query pool was too small previous time, reallocate it
                profilerCommandBuffer.m_ReallocPool = false;

                if( profilerCommandBuffer.m_TimestampQueryPool != VK_NULL_HANDLE )
                {
                    // Free previous query pool
                    m_Callbacks.DestroyQueryPool( m_Device,
                        profilerCommandBuffer.m_TimestampQueryPool, nullptr );
                }

                VkStructure<VkQueryPoolCreateInfo> queryPoolCreateInfo;
                queryPoolCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
                queryPoolCreateInfo.queryCount = profilerCommandBuffer.m_TimestampQueryPoolRequiredSize * 2;

                // Allocate new query pool
                VkResult result = m_Callbacks.CreateQueryPool(
                    m_Device, &queryPoolCreateInfo, nullptr, &profilerCommandBuffer.m_TimestampQueryPool );

                if( result == VK_SUCCESS )
                {
                    // Successfully reallocated pool
                    profilerCommandBuffer.m_TimestampQueryPoolSize = queryPoolCreateInfo.queryCount;
                }
                else
                {
                    // Failed to allocate new query pool
                    profilerCommandBuffer.m_TimestampQueryPool = VK_NULL_HANDLE;
                    profilerCommandBuffer.m_TimestampQueryPoolSize = 0;
                }
            }
            else
            {
                // UNLESS the command buffer had Simultaneous bit set, the results are ready
                if( profilerCommandBuffer.m_TimestampQueryPool != VK_NULL_HANDLE )
                {
                    // Check if results buffer is big enough
                    if( profilerCommandBuffer.m_TimestampQueryResultsSize < profilerCommandBuffer.m_TimestampQueryCount )
                    {
                        delete[] profilerCommandBuffer.m_TimestampQueryResults;

                        // Allocate host memory for results
                        profilerCommandBuffer.m_TimestampQueryResults =
                            new uint64_t[profilerCommandBuffer.m_TimestampQueryPoolSize];

                        profilerCommandBuffer.m_TimestampQueryResultsSize = profilerCommandBuffer.m_TimestampQueryPoolSize;
                    }

                    // Copy the results
                    m_Callbacks.GetQueryPoolResults(
                        m_Device,
                        profilerCommandBuffer.m_TimestampQueryPool,
                        0,
                        profilerCommandBuffer.m_TimestampQueryCount,
                        sizeof( uint64_t ) * profilerCommandBuffer.m_TimestampQueryResultsSize,
                        profilerCommandBuffer.m_TimestampQueryResults,
                        sizeof( uint64_t ),
                        VK_QUERY_RESULT_64_BIT );

                    // Number of valid queries in the results buffer
                    profilerCommandBuffer.m_TimestampQueryResultsCount = profilerCommandBuffer.m_TimestampQueryCount;

                    PresentResults( profilerCommandBuffer );
                }
            }
        }

        // Clear dynamic buffer state
        profilerCommandBuffer.m_CurrentPipeline = VK_NULL_HANDLE;
        profilerCommandBuffer.m_CurrentRenderPass = VK_NULL_HANDLE;

        // Clear timestamp query statistics
        profilerCommandBuffer.m_TimestampQueryCount = 0;
        profilerCommandBuffer.m_TimestampQueryPoolRequiredSize = 0;
    }

    /***********************************************************************************\

    Function:
        EndCommandBuffer

    Description:

    \***********************************************************************************/
    void Profiler::EndCommandBuffer( VkCommandBuffer commandBuffer )
    {
    }

    /***********************************************************************************\

    Function:
        EndCommandBuffer

    Description:

    \***********************************************************************************/
    void Profiler::SubmitCommandBuffers( VkQueue queue, uint32_t count, const VkSubmitInfo* pSubmitInfo )
    {
        m_pCurrentFrameStats->submitCount += count;
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
        uint32_t cpuQueryIndex = m_CurrentCpuTimestampQuery++;
        bool cpuQueryIndexOverflow = false;

        if( cpuQueryIndex == m_TimestampQueryPoolSize )
        {
            // Loop back to 0
            cpuQueryIndex = (m_CurrentCpuTimestampQuery = 0);

            cpuQueryIndexOverflow = true;
        }

        if( cpuQueryIndex > 0 || cpuQueryIndexOverflow )
        {
            uint32_t prevCpuQueryIndex = cpuQueryIndex - 1;

            if( cpuQueryIndexOverflow )
            {
                // Previous query was last in the pool
                prevCpuQueryIndex = m_TimestampQueryPoolSize - 1;
            }

            // Send query to end previous frame
            m_pCpuTimestampQueryPool[prevCpuQueryIndex].End();
        }

        // Send query to begin next frame
        m_pCpuTimestampQueryPool[cpuQueryIndex].Begin();

        // Store current frame stats
        std::swap( m_pCurrentFrameStats, m_pPreviousFrameStats );

        // Clear structure for the next frame
        m_pCurrentFrameStats->Reset();

        m_CurrentFrame++;
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
        SendTimestampQuery

    Description:

    \***********************************************************************************/
    void Profiler::SendTimestampQuery( ProfilerCommandBuffer& profilerCommandBuffer, VkPipelineStageFlagBits stage )
    {
        // Check if there is place for new query
        if( profilerCommandBuffer.m_TimestampQueryCount < profilerCommandBuffer.m_TimestampQueryPoolSize )
        {
            // Write begin timestamp
            m_Callbacks.CmdWriteTimestamp(
                profilerCommandBuffer.m_CommandBuffer,
                stage,
                profilerCommandBuffer.m_TimestampQueryPool,
                profilerCommandBuffer.m_TimestampQueryCount );

            // Advance the query counter
            profilerCommandBuffer.m_TimestampQueryCount++;
        }
        else
        {
            // Cannot profile this command buffer, query pool is too small
            profilerCommandBuffer.m_ReallocPool = true;
        }

        profilerCommandBuffer.m_TimestampQueryPoolRequiredSize++;
    }

    /***********************************************************************************\

    Function:
        PresentResults

    Description:

    \***********************************************************************************/
    void Profiler::PresentResults( const ProfilerCommandBuffer& profilerCommandBuffer )
    {
        printf( "VkCommandBuffer (0x%016p) profiling results:\n",
            profilerCommandBuffer.m_CommandBuffer );

        switch( m_Mode )
        {
        case ProfilerMode::ePerRenderPass:
        {
            // Each render pass has 2 timestamps - begin and end
            for( uint32_t i = 0; i < profilerCommandBuffer.m_TimestampQueryResultsCount; i += 2 )
            {
                uint64_t beginTimestamp = profilerCommandBuffer.m_TimestampQueryResults[i];
                uint64_t endTimestamp = profilerCommandBuffer.m_TimestampQueryResults[i + 1];

                // Compute time difference between timestamps
                uint64_t dt = endTimestamp - beginTimestamp;

                // Convert to microseconds
                float us = (dt * m_TimestampPeriod) / 1000.0f;

                printf( " VkRenderPass #%-2u - %f us\n", i >> 1, us );
            }
        }
        }

        printf( "\n" );
    }
}

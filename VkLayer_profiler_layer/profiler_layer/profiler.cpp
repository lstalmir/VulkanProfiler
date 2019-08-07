#include "profiler.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        Profiler

    Description:
        Constructor

    \***********************************************************************************/
    Profiler::Profiler()
    {
    }

    /***********************************************************************************\

    Function:
        Initialize

    Description:
        Initializes profiler resources.

    \***********************************************************************************/
    VkResult Profiler::Initialize( VkDevice device, ProfilerCallbacks profilerCallbacks )
    {
        m_Callbacks = profilerCallbacks;

        m_CurrentFrame = 0;

        m_TimestampQueryPoolSize = 128;
        m_CurrentTimestampQuery = 0;

        VkQueryPoolCreateInfo queryPoolCreateInfo;
        memset( &queryPoolCreateInfo, 0, sizeof( queryPoolCreateInfo ) );

        queryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        queryPoolCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
        queryPoolCreateInfo.queryCount = m_TimestampQueryPoolSize;

        // Create the GPU timestamp query pool
        VkResult result = m_Callbacks.pfnCreateQueryPool(
            device, &queryPoolCreateInfo, nullptr, &m_TimestampQueryPool );

        if( result != VK_SUCCESS )
        {
            // Failed to create timestamp query pool
            return result;
        }

        // Create the CPU timestamp query pool
        m_pCpuTimestampQueryPool = new CpuTimestampCounter[m_TimestampQueryPoolSize];
        m_CurrentCpuTimestampQuery = 0;

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:
        Destructor

    \***********************************************************************************/
    void Profiler::Destroy( VkDevice device )
    {
        delete m_pCpuTimestampQueryPool;

        // Destroy the GPU timestamp query pool
        if( m_TimestampQueryPool && m_Callbacks.pfnDestroyQueryPool )
        {
            m_Callbacks.pfnDestroyQueryPool( device, m_TimestampQueryPool, nullptr );
        }
    }

    /***********************************************************************************\

    Function:
        PreDraw

    Description:
        Executed before drawcall

    \***********************************************************************************/
    void Profiler::PreDraw( VkCommandBuffer commandBuffer )
    {
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

            uint64_t microseconds = m_pCpuTimestampQueryPool[prevCpuQueryIndex].GetValue();

            // TMP
            printf( __FUNCTION__ ": %f us\n", static_cast<float>(microseconds) / 1000.f );

            if( cpuQueryIndexOverflow )
            {
                printf( __FUNCTION__ ": FRAME #%u :: Previous frame stats :: drawCount=%llu, submitCount=%llu\n",
                    m_CurrentFrame,
                    m_PreviousFrameStats.drawCount,
                    m_PreviousFrameStats.submitCount );
            }
        }

        // Send query to begin next frame
        m_pCpuTimestampQueryPool[cpuQueryIndex].Begin();

        // Store and clear the stats
        std::swap( m_CurrentFrameStats, m_PreviousFrameStats );
        memset( &m_CurrentFrameStats, 0, sizeof( m_CurrentFrameStats ) );

        m_CurrentFrame++;
    }

    /***********************************************************************************\

    Function:
        GetCurrentFrameStats

    Description:

    \***********************************************************************************/
    FrameStats& Profiler::GetCurrentFrameStats()
    {
        return m_CurrentFrameStats;
    }

    /***********************************************************************************\

    Function:
        GetPreviousFrameStats

    Description:

    \***********************************************************************************/
    FrameStats Profiler::GetPreviousFrameStats() const
    {
        return m_PreviousFrameStats;
    }
}

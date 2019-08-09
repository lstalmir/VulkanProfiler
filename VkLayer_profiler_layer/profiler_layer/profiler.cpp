#include "profiler.h"
#include "profiler_overlay.h"

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
    VkResult Profiler::Initialize( VkDevice_Object* pDevice, ProfilerCallbacks callbacks )
    {
        m_Callbacks = callbacks;

        m_CurrentFrame = 0;

        m_TimestampQueryPoolSize = 128;
        m_CurrentTimestampQuery = 0;

        // Create the GPU timestamp query pool
        VkQueryPoolCreateInfo queryPoolCreateInfo;
        memset( &queryPoolCreateInfo, 0, sizeof( queryPoolCreateInfo ) );

        queryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        queryPoolCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
        queryPoolCreateInfo.queryCount = m_TimestampQueryPoolSize;

        VkResult result = m_Callbacks.pfnCreateQueryPool(
            pDevice->Device, &queryPoolCreateInfo, nullptr, &m_TimestampQueryPool );

        if( result != VK_SUCCESS )
        {
            // Failed to create timestamp query pool

            // Cleanup the profiler
            Destroy( pDevice->Device );

            return result;
        }

        // Create the CPU timestamp query pool
        m_pCpuTimestampQueryPool = new CpuTimestampCounter[m_TimestampQueryPoolSize];
        m_CurrentCpuTimestampQuery = 0;

        // Create frame stats counters
        m_pCurrentFrameStats = new FrameStats;
        m_pPreviousFrameStats = new FrameStats; // will be swapped every frame

        // Create profiler overlay
        m_pOverlay = new ProfilerOverlay;

        result = m_pOverlay->Initialize( pDevice, this, m_Callbacks );

        if( result != VK_SUCCESS )
        {
            // Creation of the overlay failed

            // Cleanup the profiler
            Destroy( pDevice->Device );

            return result;
        }

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:
        Frees resources allocated by the profiler.

    \***********************************************************************************/
    void Profiler::Destroy( VkDevice device )
    {
        if( m_pOverlay )
        {
            m_pOverlay->Destroy( device );
        }

        delete m_pOverlay;

        delete m_pCurrentFrameStats;
        delete m_pPreviousFrameStats;

        delete m_pCpuTimestampQueryPool;

        // Destroy the GPU timestamp query pool
        if( m_TimestampQueryPool )
        {
            m_Callbacks.pfnDestroyQueryPool( device, m_TimestampQueryPool, nullptr );

            m_TimestampQueryPool = VK_NULL_HANDLE;
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
        m_pOverlay->DrawFrameStats( queue );
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
            printf( __FUNCTION__ ": %f ms\n", static_cast<float>(microseconds) / 1000.f );

            if( cpuQueryIndexOverflow )
            {
                printf( __FUNCTION__ ": FRAME #%u :: Previous frame stats :: drawCount=%llu, submitCount=%llu\n",
                    m_CurrentFrame,
                    static_cast<uint64_t>(m_pPreviousFrameStats->drawCount),
                    static_cast<uint64_t>(m_pPreviousFrameStats->submitCount) );
            }
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
}

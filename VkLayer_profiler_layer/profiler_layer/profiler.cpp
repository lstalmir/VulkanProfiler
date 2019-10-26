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
    {
        ClearMemory( this );
    }

    /***********************************************************************************\

    Function:
        Initialize

    Description:
        Initializes profiler resources.

    \***********************************************************************************/
    VkResult Profiler::Initialize( VkDevice device, const VkLayerDispatchTable* pDispatchTable )
    {
        m_Callbacks = *pDispatchTable;
        m_Device = device;

        m_CurrentFrame = 0;

        m_TimestampQueryPoolSize = 128;
        m_CurrentTimestampQuery = 0;

        // Create the GPU timestamp query pool
        VkStructure<VkQueryPoolCreateInfo> queryPoolCreateInfo;

        queryPoolCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
        queryPoolCreateInfo.queryCount = m_TimestampQueryPoolSize;

        VkResult result = m_Callbacks.CreateQueryPool(
            m_Device, &queryPoolCreateInfo, nullptr, &m_TimestampQueryPool );

        if( result != VK_SUCCESS )
        {
            // Failed to create timestamp query pool

            // Cleanup the profiler
            Destroy();

            return result;
        }

        // Create the CPU timestamp query pool
        m_pCpuTimestampQueryPool = new CpuTimestampCounter[m_TimestampQueryPoolSize];
        m_CurrentCpuTimestampQuery = 0;

        // Create frame stats counters
        m_pCurrentFrameStats = new FrameStats;
        m_pPreviousFrameStats = new FrameStats; // will be swapped every frame

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
        delete m_pPreviousFrameStats;

        delete m_pCpuTimestampQueryPool;

        if( m_TimestampQueryPool )
        {
            // Destroy the GPU timestamp query pool
            m_Callbacks.DestroyQueryPool( m_Device, m_TimestampQueryPool, nullptr );
        }

        ClearMemory( this );
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
        Destroy

    Description:

    \***********************************************************************************/
    void Profiler::OnAllocateMemory( VkDeviceMemory allocatedMemory, const VkMemoryAllocateInfo* pAllocateInfo )
    {
        // Insert allocation info to the map, it will be needed during deallocation.
        //m_AllocatedMemory.emplace( allocatedMemory, *pAllocateInfo );

        m_AllocatedMemorySize += pAllocateInfo->allocationSize;
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:

    \***********************************************************************************/
    void Profiler::OnFreeMemory( VkDeviceMemory allocatedMemory )
    {
        //auto it = m_AllocatedMemory.find( allocatedMemory );
        //if( it != m_AllocatedMemory.end() )
        {
            //m_AllocatedMemorySize -= it->second.allocationSize;

            // Remove allocation entry from the map
            //m_AllocatedMemory.erase( it );
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
}

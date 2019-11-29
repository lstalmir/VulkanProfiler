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

        // Prepare helper command buffer
        VkStructure<VkCommandPoolCreateInfo> commandPoolCreateInfo;
        commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        commandPoolCreateInfo.queueFamilyIndex = 0; // TODO

        VkResult result = m_Callbacks.CreateCommandPool(
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

            profilerCommandBuffer.Draw();
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

        if( !emplaced.second )
        {
            // Already profiling this command buffer
            PresentResults( profilerCommandBuffer );
        }

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
        EndCommandBuffer

    Description:

    \***********************************************************************************/
    void Profiler::SubmitCommandBuffers( VkQueue queue, uint32_t& count, const VkSubmitInfo*& pSubmitInfo )
    {
        m_pCurrentFrameStats->submitCount += count;

        if( m_IsFirstSubmitInFrame )
        {
            VkSubmitInfo* pNewSubmitInfos = new VkSubmitInfo[count + 1];

            memcpy( pNewSubmitInfos + 1, pSubmitInfo, sizeof( VkSubmitInfo ) * count );

            VkSubmitInfo* pFirstSubmitInfo = pNewSubmitInfos + 1;

            VkStructure<VkSubmitInfo> helperSubmitInfo;
            helperSubmitInfo.commandBufferCount = 1;
            helperSubmitInfo.pCommandBuffers = &m_HelperCommandBuffer;

            // Wait for original semaphores
            helperSubmitInfo.waitSemaphoreCount = pFirstSubmitInfo->waitSemaphoreCount;
            helperSubmitInfo.pWaitSemaphores = pFirstSubmitInfo->pWaitSemaphores;
            helperSubmitInfo.pWaitDstStageMask = pFirstSubmitInfo->pWaitDstStageMask;

            // Signal helper command buffer execution semaphore on end
            helperSubmitInfo.signalSemaphoreCount = 1;
            helperSubmitInfo.pSignalSemaphores = &m_HelperCommandBufferExecutionSemaphore;

            // Modify original submit info to wait for helper command buffer
            pFirstSubmitInfo->waitSemaphoreCount = 1;
            pFirstSubmitInfo->pWaitSemaphores = &m_HelperCommandBufferExecutionSemaphore;

            pNewSubmitInfos[0] = helperSubmitInfo;

            pSubmitInfo = pNewSubmitInfos;
            count = count + 1;

            m_IsFirstSubmitInFrame = false;
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

        m_IsFirstSubmitInFrame = true;
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
    void Profiler::PresentResults( ProfilerCommandBuffer& profilerCommandBuffer )
    {
        m_Output.Printf( "VkCommandBuffer (0x%016p) profiling results:\n",
            profilerCommandBuffer.GetCommandBuffer() );

        auto data = profilerCommandBuffer.GetData();

        if( data.m_CollectedTimestamps.empty() )
        {
            m_Output.Printf( " No profiling data was collected for this command buffer\n" );
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

                m_Output.Printf( " VkRenderPass #%-2u - %f us\n", i >> 1, us );
            }
            break;
        }

        case ProfilerMode::ePerPipeline:
        {
            // Pipelines are separated with timestamps
            uint64_t beginTimestamp = data.m_CollectedTimestamps[0];

            for( uint32_t i = 1; i < data.m_CollectedTimestamps.size(); ++i )
            {
                uint64_t endTimestamp = data.m_CollectedTimestamps[i];

                // Compute time difference between timestamps
                uint64_t dt = endTimestamp - beginTimestamp;

                // Convert to microseconds
                float us = (dt * m_TimestampPeriod) / 1000.0f;

                m_Output.Printf( " VkPipeline #%-2u - %f us\n", i >> 1, us );

                beginTimestamp = endTimestamp;
            }

            break;
        }
        }

        m_Output.Printf( "\n" );
    }
}

// Copyright (c) 2020 Lukasz Stalmirski
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

#include "profiler_command_buffer.h"
#include "profiler_command_tree.h"
#include "profiler.h"
#include "profiler_helpers.h"
#include <algorithm>
#include <assert.h>

namespace
{
    template<typename T, typename U>
    inline static void UpdateTimestamps( T& parent, const U& child )
    {
        // If parent begin timestamp is undefined, use first defined child begin timestamp
        if( parent.m_BeginTimestamp == 0 )
        {
            parent.m_BeginTimestamp = child.m_BeginTimestamp;
        }
        // If child end timestamp is defined, it must be larger than current parent end timestamp
        if( child.m_EndTimestamp > 0 )
        {
            parent.m_EndTimestamp = child.m_EndTimestamp;
        }
    }
}

namespace Profiler
{
    /***********************************************************************************\

    Function:
        TimestampQueryPool

    Description:
        Constructor.

    \***********************************************************************************/
    TimestampQueryPool::TimestampQueryPool( DeviceProfiler& profiler, VkCommandBuffer commandBuffer )
        : m_Profiler( profiler )
        , m_CommandBuffer( commandBuffer )
        , m_QueryPools()
        , m_QueryPoolSize( 4096 )
        , m_CurrentQueryPoolIndex( -1 )
        , m_CurrentQueryIndex( -1 )
    {
    }

    /***********************************************************************************\

    Function:
        ~TimestampQueryPool

    Description:
        Destructor.

    \***********************************************************************************/
    TimestampQueryPool::~TimestampQueryPool()
    {
        // Destroy allocated query pools
        for( auto& pool : m_QueryPools )
        {
            m_Profiler.m_pDevice->Callbacks.DestroyQueryPool(
                m_Profiler.m_pDevice->Handle, pool, nullptr );
        }

        m_QueryPools.clear();
    }

    /***********************************************************************************\

    Function:
        IsEmpty

    Description:
        Checks if there are no pending queries in the command buffer.

    \***********************************************************************************/
    bool TimestampQueryPool::IsEmpty() const
    {
        return m_QueryPools.empty();
    }

    /***********************************************************************************\

    Function:
        Reset

    Description:
        Discard all queries.

    \***********************************************************************************/
    void TimestampQueryPool::Reset()
    {
        m_CurrentQueryIndex = -1;
        m_CurrentQueryPoolIndex = -1;
    }

    /***********************************************************************************\

    Function:
        Begin

    Description:
        Prepare query pools for writing timestamps.

    \***********************************************************************************/
    void TimestampQueryPool::Begin()
    {
        Reset();

        if( m_QueryPools.empty() )
        {
            // Allocate initial query pool
            Allocate();
        }
        else
        {
            // Reset existing query pool to reuse the queries
            m_Profiler.m_pDevice->Callbacks.CmdResetQueryPool(
                m_CommandBuffer,
                m_QueryPools.front(),
                0, m_QueryPoolSize );
        }

        // Move to the first query pool
        m_CurrentQueryPoolIndex++;
    }

    /***********************************************************************************\

    Function:
        Allocate

    Description:
        Create new VkQueryPool object and insert vkCmdResetQueryPool into the profiled
        command buffer.

    \***********************************************************************************/
    void TimestampQueryPool::Allocate()
    {
        VkQueryPool queryPool = VK_NULL_HANDLE;

        VkQueryPoolCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        info.queryType = VK_QUERY_TYPE_TIMESTAMP;
        info.queryCount = m_QueryPoolSize;

        // Allocate new query pool
        VkResult result = m_Profiler.m_pDevice->Callbacks.CreateQueryPool(
            m_Profiler.m_pDevice->Handle, &info, nullptr, &queryPool );

        if( result != VK_SUCCESS )
        {
            // Allocation failed
            assert( false );
            return;
        }

        // Pools must be reset before first use
        m_Profiler.m_pDevice->Callbacks.CmdResetQueryPool(
            m_CommandBuffer, queryPool, 0, m_QueryPoolSize );

        m_QueryPools.push_back( queryPool );
    }

    /***********************************************************************************\

    Function:
        AllocateIfAlmostFull

    Description:
        Check if the pool is running out of query handles. If so, allocate new object.

    \***********************************************************************************/
    bool TimestampQueryPool::AllocateIfAlmostFull( float threshold )
    {
        // Check if we're running out of current query pool
        if( m_CurrentQueryPoolIndex + 1 == m_QueryPools.size() &&
            m_CurrentQueryIndex + 1 > m_QueryPoolSize * threshold )
        {
            Allocate();
            return true;
        }

        return false;
    }

    /***********************************************************************************\

    Function:
        WriteTimestampQuery

    Description:
        Send new timestamp query to the command buffer associated with this instance.

    \***********************************************************************************/
    void TimestampQueryPool::WriteTimestampQuery( VkPipelineStageFlagBits stage )
    {
        // Allocate query from the pool
        m_CurrentQueryIndex++;

        if( m_CurrentQueryIndex == m_QueryPoolSize )
        {
            // Try to reuse next query pool
            m_CurrentQueryIndex = 0;
            m_CurrentQueryPoolIndex++;

            if( m_CurrentQueryPoolIndex == m_QueryPools.size() )
            {
                // If command buffer is not in render pass we must allocate next query pool now
                // Otherwise something went wrong in PreBeginRenderPass
                Allocate();
            }
        }

        // Send the query
        m_Profiler.m_pDevice->Callbacks.CmdWriteTimestamp(
            m_CommandBuffer,
            stage,
            m_QueryPools[ m_CurrentQueryPoolIndex ],
            m_CurrentQueryIndex );
    }

    /***********************************************************************************\

    Function:
        GetData

    Description:
        Collect data from all pending queries.

    \***********************************************************************************/
    std::vector<uint64_t> TimestampQueryPool::GetData()
    {
        // Calculate number of queried timestamps
        const uint32_t numQueries =
            (m_QueryPoolSize * m_CurrentQueryPoolIndex) +
            (m_CurrentQueryIndex + 1);

        std::vector<uint64_t> collectedQueries( numQueries );

        // Count how many queries we need to get
        uint32_t numQueriesLeft = numQueries;
        uint32_t dataOffset = 0;

        // Collect queried timestamps
        for( uint32_t i = 0; i < m_CurrentQueryPoolIndex + 1; ++i )
        {
            const uint32_t numQueriesInPool = std::min( m_QueryPoolSize, numQueriesLeft );
            const uint32_t dataSize = numQueriesInPool * sizeof( uint64_t );

            // Get results from next query pool
            m_Profiler.m_pDevice->Callbacks.GetQueryPoolResults(
                m_Profiler.m_pDevice->Handle,
                m_QueryPools[ i ],
                0, numQueriesInPool,
                dataSize,
                collectedQueries.data() + dataOffset,
                sizeof( uint64_t ),
                VK_QUERY_RESULT_64_BIT );

            numQueriesLeft -= numQueriesInPool;
            dataOffset += numQueriesInPool;
        }

        return collectedQueries;
    }

    /***********************************************************************************\

    Class:
        CommandQueryVisitor

    Description:
        Base class for timestamp query measurements.

    \***********************************************************************************/
    class CommandQueryVisitor : public CommandVisitor
    {
    public:
        CommandQueryVisitor( TimestampQueryPool& pool ) : m_QueryPool( pool ) {}

    protected:
        TimestampQueryPool& m_QueryPool;
    };

    /***********************************************************************************\

    Class:
        PreCommandQueryVisitor

    Description:
        Inserts begin timestamp query for profiled commands.

    \***********************************************************************************/
    class PreCommandQueryVisitor : public CommandQueryVisitor
    {
    public:
        using CommandQueryVisitor::CommandQueryVisitor;

        void Visit( std::shared_ptr<BeginRenderPassCommand> ) override
        {
            // This is the last time before vkCmdEndRenderPass when new query pool can be created.
            // Check if there is enough space for the render pass.
            m_QueryPool.AllocateIfAlmostFull( 0.85 );

            // Record initial transitions and clears
            m_QueryPool.WriteTimestampQuery( VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );
        }

        void Visit( std::shared_ptr<InternalPipelineCommand> ) override
        {
            // Record final transitions and clears
            m_QueryPool.WriteTimestampQuery( VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );
        }

        void Visit( std::shared_ptr<PipelineCommand> ) override
        {
            // Profile all pipeline commands
            m_QueryPool.WriteTimestampQuery( VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );
        }
    };

    /***********************************************************************************\

    Class:
        PostCommandQueryVisitor

    Description:
        Inserts end timestamp query for profiled commands.

    \***********************************************************************************/
    class PostCommandQueryVisitor : public CommandQueryVisitor
    {
    public:
        using CommandQueryVisitor::CommandQueryVisitor;

        void Visit( std::shared_ptr<BeginRenderPassCommand> ) override
        {
            // Record final transitions and clears
            m_QueryPool.WriteTimestampQuery( VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT );
        }

        void Visit( std::shared_ptr<InternalPipelineCommand> ) override
        {
            // Record final transitions and clears
            m_QueryPool.WriteTimestampQuery( VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT );
        }

        void Visit( std::shared_ptr<PipelineCommand> ) override
        {
            // Profile all pipeline commands
            m_QueryPool.WriteTimestampQuery( VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT );
        }
    };

    /***********************************************************************************\

    Class:
        CommandQueryResolveVisitor

    Description:
        Assigns timestamp queries inserted by PreCommandQueryVisitor and PostCommandQueryVisitor.

    \***********************************************************************************/
    class CommandQueryResolveVisitor : public CommandVisitor
    {
        DeviceProfiler& m_Profiler;
        const std::vector<uint64_t>& m_Timestamps;
        std::shared_ptr<CommandGroup> m_pResolvedCommands;
        size_t m_CurrentIndex;

        void CollectCommandTimestamps( std::shared_ptr<Command> pCommand )
        {
            std::shared_ptr<Command> pResolvedCommand( Command::Copy( pCommand ) );

            CommandTimestampData* pCommandData = new CommandTimestampData;
            pCommandData->m_BeginTimestamp = m_Timestamps[ m_CurrentIndex++ ];
            pCommandData->m_EndTimestamp = m_Timestamps[ m_CurrentIndex++ ];
            pResolvedCommand->SetCommandData( pCommandData );

            m_pResolvedCommands->AddCommand( pResolvedCommand );
        }

    public:
        CommandQueryResolveVisitor( DeviceProfiler& profiler, const std::vector<uint64_t>& data, size_t commandCount )
            : m_Profiler( profiler )
            , m_Timestamps( data )
            , m_pResolvedCommands()
            , m_CurrentIndex( 0 )
        {
            // TODO: Optimize allocations in pResolvedCommnads
            //m_pResolvedCommands.reserve( commandCount );
        }

        std::shared_ptr<CommandGroup> GetCommands() const
        {
            return m_pResolvedCommands;
        }

        void Visit( std::shared_ptr<InternalPipelineCommand> pCommand ) override
        {
            CollectCommandTimestamps( pCommand );
        }

        void Visit( std::shared_ptr<PipelineCommand> pCommand ) override
        {
            CollectCommandTimestamps( pCommand );
        }

        void Visit( std::shared_ptr<ExecuteCommandsCommand> pCommand ) override
        {
            // Get number of secondary command buffers
            const uint32_t commandBufferCount = pCommand->GetCommandBufferCount();

            std::shared_ptr<Command> pResolvedCommand = std::make_shared<ExecuteCommandsCommand>( pCommand );

            ExecuteCommandsCommandData* pCommandData = new ExecuteCommandsCommandData;
            pCommandData->m_pCommandBuffers.reserve( commandBufferCount );

            // Resolve data for secondary command buffers
            for( uint32_t i = 0; i < commandBufferCount; ++i )
            {
                VkCommandBuffer commandBuffer = pCommand->GetPCommandBuffers()[ i ];
                ProfilerCommandBuffer& profilerCommandBuffer = m_Profiler.GetCommandBuffer( commandBuffer );

                pCommandData->m_pCommandBuffers.push_back( profilerCommandBuffer.GetData() );
            }

            pResolvedCommand->SetCommandData( pCommandData );

            // Insert execute commands command with resolved data
            m_pResolvedCommands->AddCommand( pResolvedCommand );
        }

        void Visit( std::shared_ptr<Command> pCommand ) override
        {
            m_pResolvedCommands->AddCommand( pCommand );
        }
    };

    /***********************************************************************************\

    Function:
        ProfilerCommandBuffer

    Description:
        Constructor.

    \***********************************************************************************/
    ProfilerCommandBuffer::ProfilerCommandBuffer( DeviceProfiler& profiler, VkCommandPool commandPool, VkCommandBuffer commandBuffer, VkCommandBufferLevel level )
        : m_Profiler( profiler )
        , m_CommandPool( commandPool )
        , m_CommandBuffer( commandBuffer )
        , m_Level( level )
        , m_pData()
        , m_Dirty( false )
        , m_SecondaryCommandBuffers()
        , m_PerformanceQueryPoolINTEL( VK_NULL_HANDLE )
        , m_pCommands()
    {
        // Initialize timestamp query pool
        m_pTimestampQueryPool = std::make_unique<TimestampQueryPool>( m_Profiler, m_CommandBuffer );

        // Initialize command recorders
        m_pPreCommandQueryVisitor = std::make_unique<PreCommandQueryVisitor>( *m_pTimestampQueryPool );
        m_pPostCommandQueryVisitor = std::make_unique<PostCommandQueryVisitor>( *m_pTimestampQueryPool );
        
        // Initialize performance query once
        if( (m_Level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) && (m_Profiler.m_MetricsApiINTEL.IsAvailable()) )
        {
            VkQueryPoolCreateInfoINTEL intelCreateInfo = {};
            intelCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO_INTEL;
            intelCreateInfo.performanceCountersSampling = VK_QUERY_POOL_SAMPLING_MODE_MANUAL_INTEL;

            VkQueryPoolCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
            createInfo.pNext = &intelCreateInfo;
            createInfo.queryType = VK_QUERY_TYPE_PERFORMANCE_QUERY_INTEL;
            createInfo.queryCount = 1;

            VkResult result = m_Profiler.m_pDevice->Callbacks.CreateQueryPool(
                m_Profiler.m_pDevice->Handle,
                &createInfo,
                nullptr,
                &m_PerformanceQueryPoolINTEL );

            assert( result == VK_SUCCESS );
        }
    }

    /***********************************************************************************\

    Function:
        ~ProfilerCommandBuffer

    Description:
        Destructor.

    \***********************************************************************************/
    ProfilerCommandBuffer::~ProfilerCommandBuffer()
    {
    }

    /***********************************************************************************\

    Function:
        GetCommandPool

    Description:
        Returns VkCommandPool object associated with this instance.

    \***********************************************************************************/
    VkCommandPool ProfilerCommandBuffer::GetCommandPool() const
    {
        return m_CommandPool;
    }

    /***********************************************************************************\

    Function:
        GetCommandBuffer

    Description:
        Returns VkCommandBuffer object associated with this instance.

    \***********************************************************************************/
    VkCommandBuffer ProfilerCommandBuffer::GetCommandBuffer() const
    {
        return m_CommandBuffer;
    }

    /***********************************************************************************\

    Function:
        Submit

    Description:
        Dirty cached profiling data.

    \***********************************************************************************/
    void ProfilerCommandBuffer::Submit()
    {
        // Contents of the command buffer did not change, but all queries will be executed again
        m_Dirty = true;

        // Secondary command buffers will be executed as well
        for( VkCommandBuffer commandBuffer : m_SecondaryCommandBuffers )
        {
            // Use direct access - m_CommandBuffers map is already locked
            m_Profiler.m_pCommandBuffers.at( commandBuffer )->Submit();
        }
    }

    /***********************************************************************************\

    Function:
        Begin

    Description:
        Marks beginning of command buffer.

    \***********************************************************************************/
    void ProfilerCommandBuffer::Begin( const VkCommandBufferBeginInfo* pBeginInfo )
    {
        // Restore initial state
        Reset( 0 /*flags*/ );

        // Create tree builder
        m_pCommandTreeBuilder = std::make_unique<CommandTreeBuilder>( pBeginInfo->flags, m_Level );

        m_pTimestampQueryPool->Begin();

        // Begin collection of vendor metrics
        if( m_PerformanceQueryPoolINTEL )
        {
            m_Profiler.m_pDevice->Callbacks.CmdResetQueryPool(
                m_CommandBuffer,
                m_PerformanceQueryPoolINTEL, 0, 1 );

            m_Profiler.m_pDevice->Callbacks.CmdBeginQuery(
                m_CommandBuffer,
                m_PerformanceQueryPoolINTEL, 0, 0 );
        }
    }

    /***********************************************************************************\

    Function:
        End

    Description:
        Marks end of command buffer.

    \***********************************************************************************/
    void ProfilerCommandBuffer::End()
    {
        if( m_PerformanceQueryPoolINTEL )
        {
            m_Profiler.m_pDevice->Callbacks.CmdEndQuery(
                m_CommandBuffer,
                m_PerformanceQueryPoolINTEL, 0 );
        }
    }

    /***********************************************************************************\

    Function:
        Reset

    Description:
        Stop profiling of the command buffer.

    \***********************************************************************************/
    void ProfilerCommandBuffer::Reset( VkCommandBufferResetFlags flags )
    {
        m_Dirty = false;
    }

    /***********************************************************************************\

    Function:
        PreCommand

    Description:
        Send begin timestamp query.

    \***********************************************************************************/
    void ProfilerCommandBuffer::PreCommand( std::shared_ptr<Command> pCommand )
    {
        pCommand->Accept( *m_pPreCommandQueryVisitor );
    }

    /***********************************************************************************\

    Function:
        PostCommand

    Description:
        Send begin timestamp query.

    \***********************************************************************************/
    void ProfilerCommandBuffer::PostCommand( std::shared_ptr<Command> pCommand )
    {
        pCommand->Accept( *m_pPostCommandQueryVisitor );

        // Save command
        m_pCommands.push_back( pCommand );
    }

    /***********************************************************************************\

    Function:
        GetData

    Description:
        Reads all queried timestamps. Waits if any timestamp is not available yet.
        Returns structure containing ordered list of timestamps and statistics.

    \***********************************************************************************/
    std::shared_ptr<const CommandBufferData> ProfilerCommandBuffer::GetData()
    {
        if( m_Dirty && !m_pTimestampQueryPool->IsEmpty() )
        {
            // Reset accumulated stats if buffer is being reused
            m_pData->m_Stats = {};
            m_pData->m_PerformanceQueryReportINTEL = {};
            m_pData->m_pCommands = {};

            // Collect data from all queries
            std::vector<uint64_t> collectedQueries = m_pTimestampQueryPool->GetData();

            // Update timestamp stats for each profiled entity
            if( collectedQueries.size() > 1 )
            {
                // Create data collection visitor object
                CommandQueryResolveVisitor queryResolveVisitor( m_Profiler, collectedQueries, m_pCommands.size() );

                for( std::shared_ptr<Command> pCommand : m_pCommands )
                {
                    pCommand->Accept( queryResolveVisitor );
                }

                m_pData->m_pCommands = queryResolveVisitor.GetCommands();
            }

            // Read vendor-specific data
            if( m_PerformanceQueryPoolINTEL )
            {
                const size_t reportSize = m_Profiler.m_MetricsApiINTEL.GetReportSize();

                m_pData->m_PerformanceQueryReportINTEL.resize( reportSize );

                VkResult result = m_Profiler.m_pDevice->Callbacks.GetQueryPoolResults(
                    m_Profiler.m_pDevice->Handle,
                    m_PerformanceQueryPoolINTEL,
                    0, 1, reportSize,
                    m_pData->m_PerformanceQueryReportINTEL.data(),
                    reportSize, 0 );

                if( result != VK_SUCCESS )
                {
                    // No data available
                    m_pData->m_PerformanceQueryReportINTEL.clear();
                }
            }

            // Subsequent calls to GetData will return the same results
            m_Dirty = false;
        }

        return m_pData;
    }
}

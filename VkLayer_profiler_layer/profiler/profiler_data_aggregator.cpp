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

#include "profiler_data_aggregator.h"
#include "profiler_command_tree.h"
#include "profiler.h"
#include "profiler_helpers.h"
#include "profiler_layer_objects/VkDevice_object.h"
#include "intel/profiler_metrics_api.h"
#include <assert.h>
#include <unordered_set>

namespace Profiler
{
    struct SumAggregator
    {
        template<typename T>
        inline void operator()( uint64_t&, T& acc, uint64_t, const T& value ) const
        {
            acc += value;
        }
    };

    struct AvgAggregator
    {
        template<typename T>
        inline void operator()( uint64_t& accWeight, T& acc, uint64_t valueWeight, const T& value ) const
        {
            accWeight += valueWeight;
            acc += valueWeight * value;
        }
    };

    struct NormAggregator
    {
        template<typename T>
        inline void operator()( uint64_t&, T& acc, uint64_t valueWeight, const T& value ) const
        {
            if( valueWeight > 0 )
                acc = static_cast<T>(value / static_cast<double>(valueWeight));
            else
                acc = value;
        }
    };

    template<typename AggregatorType>
    static inline void Aggregate(
        uint64_t& accWeight,
        VkProfilerPerformanceCounterResultEXT& acc,
        uint64_t valueWeight,
        const VkProfilerPerformanceCounterResultEXT& value,
        VkProfilerPerformanceCounterStorageEXT storage )
    {
        AggregatorType aggregator;
        switch( storage )
        {
        case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_FLOAT32_EXT:
            aggregator( accWeight, acc.float32, valueWeight, value.float32 );
            break;

        case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_FLOAT64_EXT:
            aggregator( accWeight, acc.float64, valueWeight, value.float64 );
            break;

        case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_INT32_EXT:
            aggregator( accWeight, acc.int32, valueWeight, value.int32 );
            break;

        case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_INT64_EXT:
            aggregator( accWeight, acc.int64, valueWeight, value.int64 );
            break;

        case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_UINT32_EXT:
            aggregator( accWeight, acc.uint32, valueWeight, value.uint32 );
            break;

        case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_UINT64_EXT:
            aggregator( accWeight, acc.uint64, valueWeight, value.uint64 );
            break;
        }
    }

    template<typename AggregatorType>
    static inline void Aggregate(
        VkProfilerPerformanceCounterResultEXT& acc,
        uint64_t valueWeight,
        const VkProfilerPerformanceCounterResultEXT& value,
        VkProfilerPerformanceCounterStorageEXT storage )
    {
        uint64_t dummy = 0;
        Aggregate<AggregatorType>( dummy, acc, valueWeight, value, storage );
    }


    class PipelineAggregator : public CommandVisitor
    {
        uint32_t m_CurrentGraphicsPipelineHash;
        uint32_t m_CurrentComputePipelineHash;

    public:
        std::map<uint32_t, AggregatedPipelineData> m_AggregatedPipelines;

        void Visit( std::shared_ptr<BindPipelineCommand> pCommand ) override
        {
            const BindPipelineCommandData* pCommandData = pCommand->GetCommandData();

            switch( pCommand->GetPipelineBindPoint() )
            {
            case VK_PIPELINE_BIND_POINT_GRAPHICS:
                m_CurrentGraphicsPipelineHash = pCommandData->m_ShaderTuple.m_Hash;
                break;

            case VK_PIPELINE_BIND_POINT_COMPUTE:
                m_CurrentComputePipelineHash = pCommandData->m_ShaderTuple.m_Hash;
                break;

            case VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR:
                assert( !"Ray-tracing not supported" );
                break;
            }

            if( !m_AggregatedPipelines.count( pCommandData->m_ShaderTuple.m_Hash ) )
            {
                AggregatedPipelineData pipelineData = {};
                pipelineData.m_Handle = pCommand->GetPipelineHandle();
                pipelineData.m_BindPoint = pCommand->GetPipelineBindPoint();
                pipelineData.m_Ticks = 0;

                m_AggregatedPipelines.insert( { pCommandData->m_ShaderTuple.m_Hash, pipelineData } );
            }
        }

        void Visit( std::shared_ptr<ExecuteCommandsCommand> pCommand ) override
        {
            const ExecuteCommandsCommandData* pCommandData = pCommand->GetCommandData();
            const uint32_t commandBufferCount = pCommand->GetCommandBufferCount();

            // Aggregate pipelines in each command buffer
            for( uint32_t i = 0; i < commandBufferCount; ++i )
            {
                const auto& pCommandBufferData = pCommandData->m_pCommandBuffers[ i ];
                for( const auto& pCommand : *pCommandBufferData->m_pCommands )
                {
                    // Invoke recursively
                    pCommand->Accept( *this );
                }
            }
        }

        void Visit( std::shared_ptr<GraphicsCommand> pCommand ) override
        {
            m_AggregatedPipelines[ m_CurrentGraphicsPipelineHash ].m_Ticks +=
                pCommand->GetCommandData<CommandTimestampData>()->GetTicks();
        }

        void Visit( std::shared_ptr<ComputeCommand> pCommand ) override
        {
            m_AggregatedPipelines[ m_CurrentComputePipelineHash ].m_Ticks +=
                pCommand->GetCommandData<CommandTimestampData>()->GetTicks();
        }

        void Visit( std::shared_ptr<InternalPipelineCommand> pCommand ) override
        {
            m_AggregatedPipelines[ pCommand->GetInternalPipelineHash() ].m_Ticks +=
                pCommand->GetCommandData<CommandTimestampData>()->GetTicks();
        }
    };

    /***********************************************************************************\

    Function:
        Initialize

    Description:
        Initializer.

    \***********************************************************************************/
    VkResult ProfilerDataAggregator::Initialize( DeviceProfiler* pProfiler )
    {
        m_pProfiler = pProfiler;
        m_VendorMetricProperties = m_pProfiler->m_MetricsApiINTEL.GetMetricsProperties();

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        AppendSubmit

    Description:
        Add submit data to the aggregator.

    \***********************************************************************************/
    void ProfilerDataAggregator::AppendSubmit( const DeviceProfilerSubmitBatch& submit )
    {
        std::scoped_lock lk( m_Mutex );
        m_Submits.push_back( submit );
    }

    /***********************************************************************************\

    Function:
        AppendData

    Description:
        Add command buffer data to the aggregator.

    \***********************************************************************************/
    void ProfilerDataAggregator::AppendData( ProfilerCommandBuffer* pCommandBuffer, std::shared_ptr<const CommandBufferData> pData )
    {
        std::scoped_lock lk( m_Mutex );
        m_Data.emplace( pCommandBuffer, pData );
    }

    /***********************************************************************************\

    Function:
        Aggregate

    Description:
        Collect data from the submitted command buffers.

    \***********************************************************************************/
    void ProfilerDataAggregator::Aggregate()
    {
        decltype(m_Submits) submits;
        decltype(m_Data) data;

        // Copy submits and data to local memory
        {
            std::scoped_lock lk( m_Mutex );
            std::swap( m_Submits, submits );
            std::swap( m_Data, data );
        }

        for( const auto& submitBatch : submits )
        {
            DeviceProfilerSubmitBatchData submitBatchData;
            submitBatchData.m_Handle = submitBatch.m_Handle;
            submitBatchData.m_Timestamp = submitBatch.m_Timestamp;

            for( const auto& submit : submitBatch.m_Submits )
            {
                DeviceProfilerSubmitData submitData;

                for( const auto& pCommandBuffer : submit.m_pCommandBuffers )
                {
                    // Check if buffer was freed before present
                    // In such case pCommandBuffer is pointer to freed memory and cannot be dereferenced
                    auto it = data.find( pCommandBuffer );
                    if( it != data.end() )
                    {
                        submitData.m_pCommandBuffers.push_back( it->second );
                    }
                    else
                    {
                        // Collect command buffer data now
                        submitData.m_pCommandBuffers.push_back( pCommandBuffer->GetData() );
                    }
                }

                submitBatchData.m_Submits.push_back( submitData );
            }

            m_AggregatedData.push_back( submitBatchData );
        }
    }

    /***********************************************************************************\

    Function:
        Reset

    Description:
        Clear results map.

    \***********************************************************************************/
    void ProfilerDataAggregator::Reset()
    {
        std::scoped_lock lk( m_Mutex );
        m_Submits.clear();
        m_AggregatedData.clear();
        m_Data.clear();
    }

    /***********************************************************************************\

    Function:
        GetAggregatedData

    Description:
        Merge similar command buffers and return aggregated results.
        Prepare aggregator for the next profiling run.

    \***********************************************************************************/
    DeviceProfilerFrameData ProfilerDataAggregator::GetAggregatedData() const
    {
        auto aggregatedSubmits = ConstructFrameTree();
        auto aggregatedPipelines = CollectTopPipelines();
        auto aggregatedVendorMetrics = AggregateVendorMetrics();

        DeviceProfilerFrameData frameData;
        frameData.m_Submits = { aggregatedSubmits.begin(), aggregatedSubmits.end() };
        frameData.m_TopPipelines = { aggregatedPipelines.begin(), aggregatedPipelines.end() };
        frameData.m_VendorMetrics = aggregatedVendorMetrics;

        // Collect per-frame stats
        for( const auto& submitBatch : m_AggregatedData )
        {
            for( const auto& submit : submitBatch.m_Submits )
            {
                for( const auto& commandBuffer : submit.m_pCommandBuffers )
                {
                    frameData.m_Stats += commandBuffer->m_Stats;
                    frameData.m_Ticks += (commandBuffer->m_EndTimestamp - commandBuffer->m_BeginTimestamp);
                }
            }
        }

        return frameData;
    }

    /***********************************************************************************\

    Function:
        AggregateVendorMetrics

    Description:
        Merge vendor metrics collected from different command buffers.

    \***********************************************************************************/
    std::vector<VkProfilerPerformanceCounterResultEXT> ProfilerDataAggregator::AggregateVendorMetrics() const
    {
        const uint32_t metricCount = m_VendorMetricProperties.size();

        // No vendor metrics available
        if( metricCount == 0 )
            return {};

        // Helper structure containing aggregated metric value and its weight.
        struct __WeightedMetric
        {
            VkProfilerPerformanceCounterResultEXT value;
            uint64_t weight;
        };

        std::vector<__WeightedMetric> aggregatedVendorMetrics( metricCount );

        for( const auto& submitBatchData : m_AggregatedData )
        {
            for( const auto& submitData : submitBatchData.m_Submits )
            {
                for( const auto& commandBufferData : submitData.m_pCommandBuffers )
                {
                    // Preprocess metrics for the command buffer
                    const std::vector<VkProfilerPerformanceCounterResultEXT> commandBufferVendorMetrics =
                        m_pProfiler->m_MetricsApiINTEL.ParseReport(
                            commandBufferData->m_PerformanceQueryReportINTEL.data(),
                            commandBufferData->m_PerformanceQueryReportINTEL.size() );

                    assert( commandBufferVendorMetrics.size() == metricCount );

                    for( uint32_t i = 0; i < metricCount; ++i )
                    {
                        // Get metric accumulator
                        __WeightedMetric& weightedMetric = aggregatedVendorMetrics[ i ];

                        switch( m_VendorMetricProperties[ i ].unit )
                        {
                        case VK_PROFILER_PERFORMANCE_COUNTER_UNIT_BYTES_EXT:
                        case VK_PROFILER_PERFORMANCE_COUNTER_UNIT_CYCLES_EXT:
                        case VK_PROFILER_PERFORMANCE_COUNTER_UNIT_GENERIC_EXT:
                        case VK_PROFILER_PERFORMANCE_COUNTER_UNIT_NANOSECONDS_EXT:
                        {
                            // Metrics aggregated by sum
                            Profiler::Aggregate<SumAggregator>(
                                weightedMetric.weight,
                                weightedMetric.value,
                                (commandBufferData->m_EndTimestamp - commandBufferData->m_BeginTimestamp),
                                commandBufferVendorMetrics[ i ],
                                m_VendorMetricProperties[ i ].storage );

                            break;
                        }

                        case VK_PROFILER_PERFORMANCE_COUNTER_UNIT_AMPS_EXT:
                        case VK_PROFILER_PERFORMANCE_COUNTER_UNIT_BYTES_PER_SECOND_EXT:
                        case VK_PROFILER_PERFORMANCE_COUNTER_UNIT_HERTZ_EXT:
                        case VK_PROFILER_PERFORMANCE_COUNTER_UNIT_KELVIN_EXT:
                        case VK_PROFILER_PERFORMANCE_COUNTER_UNIT_PERCENTAGE_EXT:
                        case VK_PROFILER_PERFORMANCE_COUNTER_UNIT_VOLTS_EXT:
                        case VK_PROFILER_PERFORMANCE_COUNTER_UNIT_WATTS_EXT:
                        {
                            // Metrics aggregated by average
                            Profiler::Aggregate<AvgAggregator>(
                                weightedMetric.weight,
                                weightedMetric.value,
                                (commandBufferData->m_EndTimestamp - commandBufferData->m_BeginTimestamp),
                                commandBufferVendorMetrics[ i ],
                                m_VendorMetricProperties[ i ].storage );

                            break;
                        }
                        }
                    }
                }
            }
        }

        // Normalize aggregated metrics by weight
        std::vector<VkProfilerPerformanceCounterResultEXT> normalizedAggregatedVendorMetrics( metricCount );

        for( uint32_t i = 0; i < metricCount; ++i )
        {
            const __WeightedMetric& weightedMetric = aggregatedVendorMetrics[ i ];
            Profiler::Aggregate<NormAggregator>(
                normalizedAggregatedVendorMetrics[ i ],
                weightedMetric.weight,
                weightedMetric.value,
                m_VendorMetricProperties[ i ].storage );
        }

        return normalizedAggregatedVendorMetrics;
    }

    /***********************************************************************************\

    Function:
        CollectTopPipelines

    Description:
        Enumerate and sort all pipelines by duration descending.

    \***********************************************************************************/
    std::vector<AggregatedPipelineData> ProfilerDataAggregator::CollectTopPipelines() const
    {
        PipelineAggregator aggregator;

        for( const auto& submitBatch : m_AggregatedData )
        {
            for( const auto& submit : submitBatch.m_Submits )
            {
                for( const auto& pCommandBuffer : submit.m_pCommandBuffers )
                {
                    for( const auto& pCommand : *pCommandBuffer->m_pCommands )
                    {
                        pCommand->Accept( aggregator );
                    }
                }
            }
        }

        std::vector<AggregatedPipelineData> pipelines;
        pipelines.reserve( aggregator.m_AggregatedPipelines.size());

        for( const auto& [hash, pipeline] : aggregator.m_AggregatedPipelines )
        {
            pipelines.push_back( pipeline );
        }

        return pipelines;
    }

    /***********************************************************************************\

    Function:
        ConstructFrameTree

    Description:

    \***********************************************************************************/
    std::vector<DeviceProfilerSubmitBatchData> ProfilerDataAggregator::ConstructFrameTree() const
    {
        std::vector<DeviceProfilerSubmitBatchData> frame;

        for( const auto& submitBatch : m_AggregatedData )
        {
            DeviceProfilerSubmitBatchData submitBatchData = {};
            submitBatchData.m_Handle = submitBatch.m_Handle;
            submitBatchData.m_Timestamp = submitBatch.m_Timestamp;

            for( const auto& submit : submitBatch.m_Submits )
            {
                DeviceProfilerSubmitData submitData = {};

                for( const auto& pCommandBuffer : submit.m_pCommandBuffers )
                {
                    std::shared_ptr<CommandBufferData> pCommandBufferData = std::make_shared<CommandBufferData>();
                    pCommandBufferData->m_Handle = pCommandBuffer->m_Handle;
                    pCommandBufferData->m_Level = pCommandBuffer->m_Level;
                    pCommandBufferData->m_Usage = pCommandBuffer->m_Usage;
                    pCommandBufferData->m_Stats = pCommandBuffer->m_Stats;
                    pCommandBufferData->m_BeginTimestamp = pCommandBuffer->m_BeginTimestamp;
                    pCommandBufferData->m_EndTimestamp = pCommandBuffer->m_EndTimestamp;

                    // Create command tree builder for current command buffer
                    CommandTreeBuilder treeBuilder( pCommandBuffer->m_Usage, pCommandBuffer->m_Level );

                    for( const auto& pCommand : *pCommandBuffer->m_pCommands )
                    {
                        pCommand->Accept( treeBuilder );
                    }

                    pCommandBufferData->m_pCommands = treeBuilder.GetCommands();
                }
            }
        }
    }
}

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
    void ProfilerDataAggregator::AppendData( ProfilerCommandBuffer* pCommandBuffer, const DeviceProfilerCommandBufferData& data )
    {
        std::scoped_lock lk( m_Mutex );
        m_Data.emplace( pCommandBuffer, data );
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
                        submitData.m_CommandBuffers.push_back( it->second );
                    }
                    else
                    {
                        // Collect command buffer data now
                        submitData.m_CommandBuffers.push_back( pCommandBuffer->GetData() );
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
        auto aggregatedSubmits = m_AggregatedData;
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
                for( const auto& commandBuffer : submit.m_CommandBuffers )
                {
                    frameData.m_Stats += commandBuffer.m_Stats;
                    frameData.m_Ticks += commandBuffer.m_Ticks;
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
                for( const auto& commandBufferData : submitData.m_CommandBuffers )
                {
                    // Preprocess metrics for the command buffer
                    const std::vector<VkProfilerPerformanceCounterResultEXT> commandBufferVendorMetrics =
                        m_pProfiler->m_MetricsApiINTEL.ParseReport(
                            commandBufferData.m_PerformanceQueryReportINTEL.data(),
                            commandBufferData.m_PerformanceQueryReportINTEL.size() );

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
                                commandBufferData.m_Ticks,
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
                                commandBufferData.m_Ticks,
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
    std::list<DeviceProfilerPipelineData> ProfilerDataAggregator::CollectTopPipelines() const
    {
        std::unordered_set<DeviceProfilerPipelineData> aggregatedPipelines;

        for( const auto& submitBatch : m_AggregatedData )
        {
            for( const auto& submit : submitBatch.m_Submits )
            {
                for( const auto& commandBuffer : submit.m_CommandBuffers )
                {
                    CollectPipelinesFromCommandBuffer( commandBuffer, aggregatedPipelines );
                }
            }
        }

        // Sort by time
        std::list<DeviceProfilerPipelineData> pipelines = { aggregatedPipelines.begin(), aggregatedPipelines.end() };

        pipelines.sort( []( const DeviceProfilerPipelineData& a, const DeviceProfilerPipelineData& b )
            {
                return a.m_Ticks > b.m_Ticks;
            } );

        return pipelines;
    }

    /***********************************************************************************\

    Function:
        CollectPipelinesFromCommandBuffer

    Description:
        Enumerate and sort all pipelines in command buffer by duration descending.

    \***********************************************************************************/
    void ProfilerDataAggregator::CollectPipelinesFromCommandBuffer(
        const DeviceProfilerCommandBufferData& commandBuffer,
        std::unordered_set<DeviceProfilerPipelineData>& aggregatedPipelines ) const
    {
        // Include begin/end
        DeviceProfilerPipelineData beginRenderPassPipeline = m_pProfiler->GetPipeline(
            (VkPipeline)DeviceProfilerPipelineType::eBeginRenderPass );

        DeviceProfilerPipelineData endRenderPassPipeline = m_pProfiler->GetPipeline(
            (VkPipeline)DeviceProfilerPipelineType::eEndRenderPass );

        for( const auto& renderPass : commandBuffer.m_RenderPasses )
        {
            // Aggregate begin/end render pass time
            beginRenderPassPipeline.m_Ticks += renderPass.m_BeginTicks;
            endRenderPassPipeline.m_Ticks += renderPass.m_EndTicks;

            for( const auto& subpass : renderPass.m_Subpasses )
            {
                if( subpass.m_Contents == VK_SUBPASS_CONTENTS_INLINE )
                {
                    for( const auto& pipeline : subpass.m_Pipelines )
                    {
                        CollectPipeline( pipeline, aggregatedPipelines );
                    }
                }

                else if( subpass.m_Contents == VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS )
                {
                    for( const auto& secondaryCommandBuffer : subpass.m_SecondaryCommandBuffers )
                    {
                        CollectPipelinesFromCommandBuffer( secondaryCommandBuffer, aggregatedPipelines );
                    }
                }
            }
        }

        // Insert aggregated begin/end render pass pipelines
        CollectPipeline( beginRenderPassPipeline, aggregatedPipelines );
        CollectPipeline( endRenderPassPipeline, aggregatedPipelines );
    }

    /***********************************************************************************\

    Function:
        CollectPipeline

    Description:
        Aggregate pipeline data for top pipelines enumeration.

    \***********************************************************************************/
    void ProfilerDataAggregator::CollectPipeline(
        const DeviceProfilerPipelineData& pipeline,
        std::unordered_set<DeviceProfilerPipelineData>& aggregatedPipelines ) const
    {
        DeviceProfilerPipelineData aggregatedPipeline = pipeline;

        auto it = aggregatedPipelines.find( pipeline );
        if( it != aggregatedPipelines.end() )
        {
            aggregatedPipeline = *it;
            aggregatedPipeline.m_Ticks += pipeline.m_Ticks;

            aggregatedPipelines.erase( it );
        }

        // Clear values which don't make sense after aggregation
        aggregatedPipeline.m_Handle = pipeline.m_Handle;
        aggregatedPipeline.m_Hash = pipeline.m_Hash;
        aggregatedPipeline.m_Drawcalls.clear();

        aggregatedPipelines.insert( aggregatedPipeline );
    }
}

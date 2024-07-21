// Copyright (c) 2019-2021 Lukasz Stalmirski
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
#include <algorithm>
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
            acc += static_cast<T>( valueWeight * value );
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
        m_VendorMetricsSetIndex = UINT32_MAX;
        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:

    \***********************************************************************************/
    void ProfilerDataAggregator::Destroy()
    {
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

        // Append the submit to the last frame in the queue.
        if( m_NextFrames.empty() || m_NextFrames.back().m_FrameIndex != m_FrameIndex )
        {
            m_NextFrames.push_back( { m_FrameIndex } );
        }

        Frame& frame = m_NextFrames.back();

        DeviceProfilerSubmitBatch& submitBatch = frame.m_PendingSubmits.emplace_back( submit );
        submitBatch.m_SubmitBatchDataIndex = frame.m_CompleteSubmits.size();

        DeviceProfilerSubmitBatchData& submitBatchData = frame.m_CompleteSubmits.emplace_back();
        submitBatchData.m_Handle = submit.m_Handle;
        submitBatchData.m_ThreadId = submit.m_ThreadId;
        submitBatchData.m_Timestamp = submit.m_Timestamp;
    }

    /***********************************************************************************\

    Function:
        Aggregate

    Description:
        Collect data from the submitted command buffers.

    \***********************************************************************************/
    void ProfilerDataAggregator::Aggregate()
    {
        PROFILER_SELF_TIME( m_pProfiler->m_pDevice );

        std::scoped_lock lk( m_Mutex );

        LoadVendorMetricsProperties();

        // Check if any submit has completed
        for( Frame& frame : m_NextFrames )
        {
            auto submitBatchIt = frame.m_PendingSubmits.begin();
            while( submitBatchIt != frame.m_PendingSubmits.end() )
            {
                VkResult result = m_pProfiler->m_pDevice->Callbacks.GetFenceStatus(
                    m_pProfiler->m_pDevice->Handle,
                    submitBatchIt->m_TimestampCopyFence );

                if( result == VK_SUCCESS )
                {
                    ResolveSubmitBatchData(
                        *submitBatchIt,
                        frame.m_CompleteSubmits[ submitBatchIt->m_SubmitBatchDataIndex ] );

                    m_pProfiler->m_pDevice->Callbacks.DestroyFence(
                        m_pProfiler->m_pDevice->Handle,
                        submitBatchIt->m_TimestampCopyFence,
                        nullptr );

                    m_pProfiler->m_pDevice->Callbacks.FreeCommandBuffers(
                        m_pProfiler->m_pDevice->Handle,
                        submitBatchIt->m_TimestampCopyCommandPool,
                        1, &submitBatchIt->m_TimestampCopyCommandBuffer );

                    delete submitBatchIt->m_pTimestampData;

                    submitBatchIt = frame.m_PendingSubmits.erase( submitBatchIt );
                }
                else
                {
                    submitBatchIt = std::next( submitBatchIt );
                }
            }
        }

        // Check if any frame has completed
        if( !m_NextFrames.empty() )
        {
            auto frameIt = m_NextFrames.begin();
            while( (frameIt != m_NextFrames.end()) && (frameIt->m_FrameIndex < m_FrameIndex) && (frameIt->m_PendingSubmits.empty()) )
            {
                frameIt++;
            }

            if( frameIt != m_NextFrames.begin() )
            {
                auto lastFrameIt = frameIt--;

                DeviceProfilerFrameData frameData;
                ResolveFrameData( *frameIt, frameData );
                std::swap( m_CurrentFrameData, frameData );

                m_NextFrames.erase( m_NextFrames.begin(), lastFrameIt );
            }
        }
    }

    void ProfilerDataAggregator::ResolveSubmitBatchData(
        const DeviceProfilerSubmitBatch& submitBatch,
        DeviceProfilerSubmitBatchData& submitBatchData ) const
    {
        uint32_t commandBufferIndex = 0;

        for( const DeviceProfilerSubmit& submit : submitBatch.m_Submits )
        {
            DeviceProfilerSubmitData& submitData = submitBatchData.m_Submits.emplace_back();
            submitData.m_SignalSemaphores = submit.m_SignalSemaphores;
            submitData.m_WaitSemaphores = submit.m_WaitSemaphores;

            submitData.m_BeginTimestamp.m_Value = std::numeric_limits<uint64_t>::max();
            submitData.m_EndTimestamp.m_Value = 0;

            for( const auto& pCommandBuffer : submit.m_pCommandBuffers )
            {
                // Collect command buffer data
                const DeviceProfilerCommandBufferData& commandBufferData = pCommandBuffer->GetData(
                    *submitBatch.m_pTimestampData,
                    commandBufferIndex );
                submitData.m_CommandBuffers.push_back( commandBufferData );

                submitData.m_BeginTimestamp.m_Value = std::min(
                    submitData.m_BeginTimestamp.m_Value, submitData.m_CommandBuffers.back().m_BeginTimestamp.m_Value );
                submitData.m_EndTimestamp.m_Value = std::max(
                    submitData.m_EndTimestamp.m_Value, submitData.m_CommandBuffers.back().m_EndTimestamp.m_Value );
            }
        }
    }

    /***********************************************************************************\

    Function:
        GetAggregatedData

    Description:
        Merge similar command buffers and return aggregated results.
        Prepare aggregator for the next profiling run.

    \***********************************************************************************/
    void ProfilerDataAggregator::ResolveFrameData( const Frame& frame, DeviceProfilerFrameData& frameData ) const
    {
        PROFILER_SELF_TIME( m_pProfiler->m_pDevice );

        frameData.m_TopPipelines = CollectTopPipelines( frame );
        frameData.m_VendorMetrics = AggregateVendorMetrics( frame );

        // Collect per-frame stats
        for( const auto& submitBatch : frame.m_CompleteSubmits )
        {
            for( const auto& submit : submitBatch.m_Submits )
            {
                for( const auto& commandBuffer : submit.m_CommandBuffers )
                {
                    frameData.m_Stats += commandBuffer.m_Stats;
                    frameData.m_Ticks += (commandBuffer.m_EndTimestamp.m_Value - commandBuffer.m_BeginTimestamp.m_Value);
                }
            }
        }

        frameData.m_Submits = std::move( frame.m_CompleteSubmits );
    }

    /***********************************************************************************\

    Function:
        LoadVendorMetricsProperties

    Description:
        Get metrics properties from the metrics API.

    \***********************************************************************************/
    void ProfilerDataAggregator::LoadVendorMetricsProperties()
    {
        PROFILER_SELF_TIME( m_pProfiler->m_pDevice );

        if( m_pProfiler->m_MetricsApiINTEL.IsAvailable() )
        {
            // Check if vendor metrics set has changed.
            const uint32_t activeMetricsSetIndex = m_pProfiler->m_MetricsApiINTEL.GetActiveMetricsSetIndex();

            if( m_VendorMetricsSetIndex != activeMetricsSetIndex )
            {
                m_VendorMetricsSetIndex = activeMetricsSetIndex;

                if( m_VendorMetricsSetIndex != UINT32_MAX )
                {
                    // Preallocate space for the metrics properties.
                    uint32_t vendorMetricsCount = m_pProfiler->m_MetricsApiINTEL.GetMetricsCount( m_VendorMetricsSetIndex );
                    m_VendorMetricProperties.resize( vendorMetricsCount );

                    // Copy metrics properties to the local vector.
                    VkResult result = m_pProfiler->m_MetricsApiINTEL.GetMetricsProperties(
                        m_VendorMetricsSetIndex,
                        &vendorMetricsCount,
                        m_VendorMetricProperties.data() );

                    if( result != VK_SUCCESS )
                    {
                        m_VendorMetricProperties.clear();
                    }
                }
                else
                {
                    m_VendorMetricProperties.clear();
                }
            }
        }
    }

    /***********************************************************************************\

    Function:
        AggregateVendorMetrics

    Description:
        Merge vendor metrics collected from different command buffers.

    \***********************************************************************************/
    std::vector<VkProfilerPerformanceCounterResultEXT> ProfilerDataAggregator::AggregateVendorMetrics(
        const Frame& frame ) const
    {
        PROFILER_SELF_TIME( m_pProfiler->m_pDevice );

        const uint32_t metricCount = static_cast<uint32_t>( m_VendorMetricProperties.size() );

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

        for( const auto& submitBatchData : frame.m_CompleteSubmits )
        {
            for( const auto& submitData : submitBatchData.m_Submits )
            {
                for( const auto& commandBufferData : submitData.m_CommandBuffers )
                {
                    if( commandBufferData.m_PerformanceQueryMetricsSetIndex != m_VendorMetricsSetIndex )
                    {
                        // The command buffer has been recorded with at different set of metrics.
                        continue;
                    }

                    if( commandBufferData.m_PerformanceQueryResults.empty() )
                    {
                        // No performance query data collected for this command buffer.
                        continue;
                    }

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
                                (commandBufferData.m_EndTimestamp.m_Value - commandBufferData.m_BeginTimestamp.m_Value),
                                commandBufferData.m_PerformanceQueryResults[ i ],
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
                                (commandBufferData.m_EndTimestamp.m_Value - commandBufferData.m_BeginTimestamp.m_Value),
                                commandBufferData.m_PerformanceQueryResults[ i ],
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
    ContainerType<DeviceProfilerPipelineData> ProfilerDataAggregator::CollectTopPipelines(
        const Frame& frame ) const
    {
        PROFILER_SELF_TIME( m_pProfiler->m_pDevice );

        // Identify pipelines by combined hash value
        std::unordered_map<uint32_t, DeviceProfilerPipelineData> aggregatedPipelines;

        for( const auto& submitBatch : frame.m_CompleteSubmits )
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
        ContainerType<DeviceProfilerPipelineData> pipelines;

        for( const auto& [_, aggregatedPipeline] : aggregatedPipelines )
        {
            pipelines.push_back( aggregatedPipeline );
        }

        std::sort( pipelines.begin(), pipelines.end(),
            []( const DeviceProfilerPipelineData& a, const DeviceProfilerPipelineData& b )
            {
                return (a.m_EndTimestamp.m_Value - a.m_BeginTimestamp.m_Value) > (b.m_EndTimestamp.m_Value - b.m_BeginTimestamp.m_Value);
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
        std::unordered_map<uint32_t, DeviceProfilerPipelineData>& aggregatedPipelines ) const
    {
        PROFILER_SELF_TIME( m_pProfiler->m_pDevice );

        // Include begin/end
        DeviceProfilerPipelineData beginRenderPassPipeline = m_pProfiler->GetPipeline(
            (VkPipeline)DeviceProfilerPipelineType::eBeginRenderPass );

        DeviceProfilerPipelineData endRenderPassPipeline = m_pProfiler->GetPipeline(
            (VkPipeline)DeviceProfilerPipelineType::eEndRenderPass );

        for( const auto& renderPass : commandBuffer.m_RenderPasses )
        {
            // Aggregate begin/end render pass time
            beginRenderPassPipeline.m_EndTimestamp.m_Value += (renderPass.m_Begin.m_EndTimestamp.m_Value - renderPass.m_Begin.m_BeginTimestamp.m_Value);
            endRenderPassPipeline.m_EndTimestamp.m_Value += (renderPass.m_End.m_EndTimestamp.m_Value - renderPass.m_End.m_BeginTimestamp.m_Value);

            for( const auto& subpass : renderPass.m_Subpasses )
            {
                // Treat data as pipelines if subpass contents are inline-only.
                if( subpass.m_Contents == VK_SUBPASS_CONTENTS_INLINE )
                {
                    for( const auto& data : subpass.m_Data )
                    {
                        CollectPipeline( std::get<DeviceProfilerPipelineData>( data ), aggregatedPipelines );
                    }
                }

                // Treat data as secondary command buffers if subpass contents are secondary command buffers only.
                else if( subpass.m_Contents == VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS )
                {
                    for( const auto& data : subpass.m_Data )
                    {
                        CollectPipelinesFromCommandBuffer( std::get<DeviceProfilerCommandBufferData>( data ), aggregatedPipelines );
                    }
                }

                // With VK_EXT_nested_command_buffer, it is possible to insert both command buffers and inline commands in the same subpass.
                else if( subpass.m_Contents == VK_SUBPASS_CONTENTS_INLINE_AND_SECONDARY_COMMAND_BUFFERS_EXT )
                {
                    for( const auto& data : subpass.m_Data )
                    {
                        switch( data.GetType() )
                        {
                        case DeviceProfilerSubpassDataType::ePipeline:
                            CollectPipeline( std::get<DeviceProfilerPipelineData>( data ), aggregatedPipelines );
                            break;
                        case DeviceProfilerSubpassDataType::eCommandBuffer:
                            CollectPipelinesFromCommandBuffer( std::get<DeviceProfilerCommandBufferData>( data ), aggregatedPipelines );
                            break;
                        }
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
        std::unordered_map<uint32_t, DeviceProfilerPipelineData>& aggregatedPipelines ) const
    {
        PROFILER_SELF_TIME( m_pProfiler->m_pDevice );

        auto it = aggregatedPipelines.find( pipeline.m_ShaderTuple.m_Hash );
        if( it == aggregatedPipelines.end() )
        {
            // Create aggregated data struct for this pipeline
            DeviceProfilerPipelineData aggregatedPipelineData = {};
            aggregatedPipelineData.m_Handle = pipeline.m_Handle;
            aggregatedPipelineData.m_BindPoint = pipeline.m_BindPoint;
            aggregatedPipelineData.m_ShaderTuple = pipeline.m_ShaderTuple;

            it = aggregatedPipelines.emplace( pipeline.m_ShaderTuple.m_Hash, aggregatedPipelineData ).first;
        }

        // Increase total pipeline time
        (*it).second.m_EndTimestamp.m_Value += (pipeline.m_EndTimestamp.m_Value - pipeline.m_BeginTimestamp.m_Value);
    }
}

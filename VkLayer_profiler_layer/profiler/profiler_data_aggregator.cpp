// Copyright (c) 2019-2025 Lukasz Stalmirski
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
#include "profiler_query_pool.h"
#include "profiler_helpers.h"
#include "profiler_layer_objects/VkDevice_object.h"
#include "profiler_performance_counters.h"
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
        ProfilerDataAggregator

    Description:
        Constructor.

    \***********************************************************************************/
    ProfilerDataAggregator::ProfilerDataAggregator()
        : m_pProfiler( nullptr )
        , m_DataCollectionThread()
        , m_DataCollectionThreadRunning( false )
        , m_pResolvedFrames()
        , m_NextFrames()
        , m_Mutex()
        , m_FrameIndex( 0 )
        , m_MaxResolvedFrameCount( 1 )
        , m_CopyCommandPools()
        , m_PerformanceMetricProperties()
        , m_PerformanceMetricsSetIndex( UINT32_MAX )
    {
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
        m_PerformanceMetricsSetIndex = UINT32_MAX;

        VkResult result = VK_SUCCESS;

        // Create command pools to copy query data using GPU.
        for( const auto& [queue, queueObj] : m_pProfiler->m_pDevice->Queues )
        {
            VkCommandPoolCreateInfo commandPoolCreateInfo = {};
            commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            commandPoolCreateInfo.queueFamilyIndex = queueObj.Family;

            VkCommandPool commandPool = VK_NULL_HANDLE;
            result = m_pProfiler->m_pDevice->Callbacks.CreateCommandPool(
                m_pProfiler->m_pDevice->Handle,
                &commandPoolCreateInfo,
                nullptr,
                &commandPool );

            if( result != VK_SUCCESS )
            {
                break;
            }

            m_CopyCommandPools.try_emplace( queue, *m_pProfiler, commandPool, commandPoolCreateInfo );
        }

        // Prepare the initial frame with no data.
        DeviceProfilerSynchronizationTimestamps createTimestamps = m_pProfiler->m_Synchronization.GetCreateTimestamps();

        auto pInitFrame = std::make_shared<DeviceProfilerFrameData>();
        pInitFrame->m_SyncTimestamps = m_pProfiler->m_Synchronization.GetSynchronizationTimestamps();
        pInitFrame->m_BeginTimestamp = pInitFrame->m_EndTimestamp = createTimestamps.m_DeviceCalibratedTimestamp;
        pInitFrame->m_CPU.m_BeginTimestamp = pInitFrame->m_CPU.m_EndTimestamp = createTimestamps.m_HostCalibratedTimestamp;
        pInitFrame->m_Memory = m_pProfiler->m_MemoryTracker.GetMemoryData();
        m_pResolvedFrames.push_back( pInitFrame );

        // Try to start data collection thread.
        if( m_pProfiler->m_Config.m_EnableThreading )
        {
            try
            {
                m_DataCollectionThreadRunning = true;
                m_DataCollectionThread = std::thread( &ProfilerDataAggregator::DataCollectionThreadProc, this );
            }
            catch( ... )
            {
                m_DataCollectionThreadRunning = false;
            }
        }

        // Cleanup if initialization failed.
        if( result != VK_SUCCESS )
        {
            Destroy();
        }

        return result;
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:

    \***********************************************************************************/
    void ProfilerDataAggregator::Destroy()
    {
        StopDataCollectionThread();

        m_CopyCommandPools.clear();
        m_pProfiler = nullptr;
    }

    /***********************************************************************************\

    Function:
        StopDataCollectionThread

    Description:

    \***********************************************************************************/
    void ProfilerDataAggregator::StopDataCollectionThread()
    {
        m_DataCollectionThreadRunning = false;

        if( m_DataCollectionThread.joinable() )
        {
            m_DataCollectionThread.join();
            m_DataCollectionThread = std::thread();
        }
    }

    /***********************************************************************************\

    Function:
        SetDataBufferSize

    Description:

    \***********************************************************************************/
    void ProfilerDataAggregator::SetDataBufferSize( uint32_t maxFrames )
    {
        std::scoped_lock lk( m_Mutex );
        m_MaxResolvedFrameCount = maxFrames;
    }

    /***********************************************************************************\

    Function:
        BeginNextFrame

    Description:
        Add new frame to the aggregator.

    \***********************************************************************************/
    void ProfilerDataAggregator::AppendFrame( const DeviceProfilerFrame& frame )
    {
        TipGuard tip( m_pProfiler->m_pDevice->TIP, __func__ );

        std::scoped_lock lk( m_Mutex );

        if( !m_NextFrames.empty() )
        {
            // Finalize the previous frame.
            m_NextFrames.back().m_EndTimestamp = frame.m_Timestamp;
        }

        m_FrameIndex = frame.m_FrameIndex;
        m_NextFrames.emplace_back( frame );
    }

    /***********************************************************************************\

    Function:
        AppendSubmit

    Description:
        Add submit data to the aggregator.

    \***********************************************************************************/
    void ProfilerDataAggregator::AppendSubmit( const DeviceProfilerSubmitBatch& submit )
    {
        TipGuard tip( m_pProfiler->m_pDevice->TIP, __func__ );

        // Prepare submit batch info.
        SubmitBatch submitBatch( (submit) );

        for( const DeviceProfilerSubmit& _submit : submitBatch.m_Submits )
        {
            submitBatch.m_pSubmittedCommandBuffers.insert(
                _submit.m_pCommandBuffers.begin(),
                _submit.m_pCommandBuffers.end() );
        }

        // Allocate a buffer for the query data.
        uint64_t bufferSize = 0;

        for( const ProfilerCommandBuffer* pCommandBuffer : submitBatch.m_pSubmittedCommandBuffers )
        {
            bufferSize += pCommandBuffer->GetRequiredQueryDataBufferSize();
        }

        submitBatch.m_pDataBuffer = new DeviceProfilerQueryDataBuffer( *m_pProfiler, bufferSize );

        // Try to copy the data using GPU.
        // It may fallback to CPU allocation if the function fails to allocate the command buffer.
        bool succeeded = WriteQueryDataToGpuBuffer( submitBatch );
        if( !succeeded )
        {
            FreeDynamicAllocations( submitBatch );
            return;
        }

        // Synchronize with data collection thread.
        std::scoped_lock lk( m_Mutex );

        Frame& frame = m_NextFrames.back();
        submitBatch.m_SubmitBatchDataIndex = static_cast<uint32_t>( frame.m_CompleteSubmits.size() );

        DeviceProfilerSubmitBatchData& submitBatchData = frame.m_CompleteSubmits.emplace_back();
        submitBatchData.m_Handle = submit.m_Handle;
        submitBatchData.m_ThreadId = submit.m_ThreadId;
        submitBatchData.m_Timestamp = submit.m_Timestamp;

        // Append the submit to the last frame in the queue.
        frame.m_PendingSubmits.push_back( std::move( submitBatch ) );
    }

    /***********************************************************************************\

    Function:
        Aggregate

    Description:
        Collect data from the submitted command buffers.

    \***********************************************************************************/
    void ProfilerDataAggregator::Aggregate( ProfilerCommandBuffer* pWaitForCommandBuffer )
    {
        TipGuard tip( m_pProfiler->m_pDevice->TIP, __func__ );

        std::unique_lock uniqueLock( m_Mutex, std::defer_lock );

        // The synchronization may be required if a command buffer is being freed.
        // In such case, the profiler has to wait for the timestamp data.
        if( pWaitForCommandBuffer )
        {
            // Acquire a shared lock so other thread doesn't free the fences while this thread waits.
            std::shared_lock sharedLock( m_Mutex );
            std::vector<VkFence> waitFences;

            // Wait for all pending submits that reference the command buffer.
            for( const Frame& frame : m_NextFrames )
            {
                for( const SubmitBatch& submitBatch : frame.m_PendingSubmits )
                {
                    if( submitBatch.m_pSubmittedCommandBuffers.count( pWaitForCommandBuffer ) )
                    {
                        // Wait for this submit batch.
                        waitFences.push_back( submitBatch.m_DataCopyFence );
                    }
                }
            }

            if( !waitFences.empty() )
            {
                // Wait for the fences.
                m_pProfiler->m_pDevice->Callbacks.WaitForFences(
                    m_pProfiler->m_pDevice->Handle,
                    static_cast<uint32_t>(waitFences.size()),
                    waitFences.data(),
                    VK_TRUE,
                    UINT64_MAX );
            }

            // Force synchronization because the command buffer is about to be destroyed.
            sharedLock.unlock();
            uniqueLock.lock();
        }
        else
        {
            // Don't aggregate if another thread already processes the data.
            if( !uniqueLock.try_lock() )
            {
                return;
            }
        }

        // Check if any submit has completed
        for( Frame& frame : m_NextFrames )
        {
            auto submitBatchIt = frame.m_PendingSubmits.begin();
            while( submitBatchIt != frame.m_PendingSubmits.end() )
            {
                VkResult result = VK_NOT_READY;

                // Aggregate only submits that contain the specified command buffer
                if( !pWaitForCommandBuffer || submitBatchIt->m_pSubmittedCommandBuffers.count( pWaitForCommandBuffer ) )
                {
                    result = m_pProfiler->m_pDevice->Callbacks.GetFenceStatus(
                        m_pProfiler->m_pDevice->Handle,
                        submitBatchIt->m_DataCopyFence );
                }

                if( result == VK_SUCCESS )
                {
                    bool succeeded = true;
                    if( !submitBatchIt->m_pDataBuffer->UsesGpuAllocation() )
                    {
                        succeeded = WriteQueryDataToCpuBuffer( *submitBatchIt );
                    }

                    if( succeeded )
                    {
                        ResolveSubmitBatchData(
                            *submitBatchIt,
                            frame.m_CompleteSubmits[submitBatchIt->m_SubmitBatchDataIndex] );
                    }

                    FreeDynamicAllocations( *submitBatchIt );

                    submitBatchIt = frame.m_PendingSubmits.erase( submitBatchIt );
                }
                else
                {
                    submitBatchIt = std::next( submitBatchIt );
                }
            }
        }

        // Check if any frame has completed
        if( !pWaitForCommandBuffer && !m_NextFrames.empty() )
        {
            auto frameIt = m_NextFrames.begin();
            while( (frameIt != m_NextFrames.end()) && (frameIt->m_FrameIndex < m_FrameIndex) && (frameIt->m_PendingSubmits.empty()) )
            {
                LoadPerformanceMetricsProperties();

                std::shared_ptr<DeviceProfilerFrameData> pFrameData = std::make_shared<DeviceProfilerFrameData>();
                ResolveFrameData( *frameIt, *pFrameData );

                // Remove unconsumed frames.
                if( m_MaxResolvedFrameCount != 0 )
                {
                    while( m_pResolvedFrames.size() >= m_MaxResolvedFrameCount )
                    {
                        m_pResolvedFrames.pop_front();
                    }
                }

                m_pResolvedFrames.push_back( std::move( pFrameData ) );

                frameIt++;
            }

            if( frameIt != m_NextFrames.begin() )
            {
                m_NextFrames.erase( m_NextFrames.begin(), frameIt );
            }
        }
    }

    /***********************************************************************************\

    Function:
        GetAggregatedData

    Description:

    \***********************************************************************************/
    std::list<std::shared_ptr<DeviceProfilerFrameData>> ProfilerDataAggregator::GetAggregatedData()
    {
        std::scoped_lock lk( m_Mutex );
        std::list<std::shared_ptr<DeviceProfilerFrameData>> pResolvedFrames;
        std::swap( pResolvedFrames, m_pResolvedFrames );
        return pResolvedFrames;
    }

    /***********************************************************************************\

    Function:
        ResolveSubmitBatchData

    Description:
        Collect timestamps sent by the command buffers in the submit batch and store
        the results in the given data structure.

    \***********************************************************************************/
    void ProfilerDataAggregator::ResolveSubmitBatchData(
        SubmitBatch& submitBatch,
        DeviceProfilerSubmitBatchData& submitBatchData ) const
    {
        TipGuard tip( m_pProfiler->m_pDevice->TIP, __func__ );

        DeviceProfilerQueryDataBufferReader reader(
            *m_pProfiler,
            *submitBatch.m_pDataBuffer );

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
                const DeviceProfilerCommandBufferData& commandBufferData = pCommandBuffer->GetData( reader );

                // Skip if instrumentation was disabled for this command buffer
                if( commandBufferData.m_DataValid )
                {
                    submitData.m_CommandBuffers.push_back( commandBufferData );

                    submitData.m_BeginTimestamp.m_Value = std::min(
                        submitData.m_BeginTimestamp.m_Value, commandBufferData.m_BeginTimestamp.m_Value );
                    submitData.m_EndTimestamp.m_Value = std::max(
                        submitData.m_EndTimestamp.m_Value, commandBufferData.m_EndTimestamp.m_Value );
                }
            }
        }
    }

    /***********************************************************************************\

    Function:
        ResolveFrameData

    Description:
        Create summary of the frame and store it in the given data structure.

    \***********************************************************************************/
    void ProfilerDataAggregator::ResolveFrameData( Frame& frame, DeviceProfilerFrameData& frameData ) const
    {
        TipGuard tip( m_pProfiler->m_pDevice->TIP, __func__ );

        frameData.m_TopPipelines = CollectTopPipelines( frame );
        frameData.m_PerformanceCounters = AggregatePerformanceMetrics( frame );

        frameData.m_Ticks = 0;
        frameData.m_BeginTimestamp = std::numeric_limits<uint64_t>::max();
        frameData.m_EndTimestamp = 0;

        frameData.m_FrameDelimiter = frame.m_FrameDelimiter;

        // Collect per-frame stats
        for( const auto& submitBatch : frame.m_CompleteSubmits )
        {
            for( const auto& submit : submitBatch.m_Submits )
            {
                for( const auto& commandBuffer : submit.m_CommandBuffers )
                {
                    frameData.m_Stats += commandBuffer.m_Stats;
                    frameData.m_Ticks += (commandBuffer.m_EndTimestamp.m_Value - commandBuffer.m_BeginTimestamp.m_Value);
                    frameData.m_BeginTimestamp = std::min( frameData.m_BeginTimestamp, commandBuffer.m_BeginTimestamp.m_Value );
                    frameData.m_EndTimestamp = std::max( frameData.m_EndTimestamp, commandBuffer.m_EndTimestamp.m_Value );
                }
            }
        }

        frameData.m_Submits = std::move( frame.m_CompleteSubmits );

        // Collect memory data.
        frameData.m_Memory = m_pProfiler->m_MemoryTracker.GetMemoryData();

        // Return CPU data.
        frameData.m_CPU.m_BeginTimestamp = frame.m_Timestamp;
        frameData.m_CPU.m_EndTimestamp = frame.m_EndTimestamp;
        frameData.m_CPU.m_FramesPerSec = frame.m_FramesPerSec;
        frameData.m_CPU.m_FrameIndex = frame.m_FrameIndex;
        frameData.m_CPU.m_ThreadId = frame.m_ThreadId;

        // Return synchronization timestamps.
        frameData.m_SyncTimestamps = frame.m_SyncTimestamps;
    }

    /***********************************************************************************\

    Function:
        DataCollectionThreadProc

    Description:
        Collects data from the submitted command buffers.

    \***********************************************************************************/
    void ProfilerDataAggregator::DataCollectionThreadProc()
    {
        while( m_DataCollectionThreadRunning )
        {
            try
            {
                Aggregate();

                std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
            }
            catch( ... )
            {
                assert( false );
            }
        }
    }

    /***********************************************************************************\

    Function:
        LoadPerformanceMetricsProperties

    Description:
        Get metrics properties from the metrics API.

    \***********************************************************************************/
    void ProfilerDataAggregator::LoadPerformanceMetricsProperties()
    {
        TipGuard tip( m_pProfiler->m_pDevice->TIP, __func__ );

        auto* pPerformanceCounters = m_pProfiler->m_pPerformanceCounters.get();
        if( pPerformanceCounters )
        {
            // Check if metrics set has changed.
            const uint32_t activeMetricsSetIndex = pPerformanceCounters->GetActiveMetricsSetIndex();
            if( m_PerformanceMetricsSetIndex != activeMetricsSetIndex )
            {
                m_PerformanceMetricsSetIndex = activeMetricsSetIndex;
                m_PerformanceMetricProperties.clear();

                if( m_PerformanceMetricsSetIndex != UINT32_MAX )
                {
                    const uint32_t metricCount = pPerformanceCounters->GetMetricsCount( m_PerformanceMetricsSetIndex );
                    m_PerformanceMetricProperties.resize( metricCount );

                    pPerformanceCounters->GetMetricsSetMetricsProperties(
                        m_PerformanceMetricsSetIndex,
                        metricCount,
                        m_PerformanceMetricProperties.data() );
                }
            }
        }
    }

    /***********************************************************************************\

    Function:
        AggregatePerformanceMetrics

    Description:
        Merge performance metrics collected from different command buffers.

    \***********************************************************************************/
    DeviceProfilerPerformanceCountersData ProfilerDataAggregator::AggregatePerformanceMetrics(
        const Frame& frame ) const
    {
        TipGuard tip( m_pProfiler->m_pDevice->TIP, __func__ );

        const uint32_t metricCount = static_cast<uint32_t>( m_PerformanceMetricProperties.size() );

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
                    if( commandBufferData.m_PerformanceCounters.m_MetricsSetIndex != m_PerformanceMetricsSetIndex )
                    {
                        // The command buffer has been recorded with at different set of metrics.
                        continue;
                    }

                    if( commandBufferData.m_PerformanceCounters.m_Results.empty() )
                    {
                        // No performance query data collected for this command buffer.
                        continue;
                    }

                    for( uint32_t i = 0; i < metricCount; ++i )
                    {
                        // Get metric accumulator
                        __WeightedMetric& weightedMetric = aggregatedVendorMetrics[ i ];

                        switch( m_PerformanceMetricProperties[ i ].unit )
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
                                commandBufferData.m_PerformanceCounters.m_Results[ i ],
                                m_PerformanceMetricProperties[ i ].storage );

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
                                commandBufferData.m_PerformanceCounters.m_Results[ i ],
                                m_PerformanceMetricProperties[ i ].storage );

                            break;
                        }
                        }
                    }
                }
            }
        }

        // Normalize aggregated metrics by weight
        DeviceProfilerPerformanceCountersData normalizedAggregatedVendorMetrics;
        normalizedAggregatedVendorMetrics.m_MetricsSetIndex = m_PerformanceMetricsSetIndex;
        normalizedAggregatedVendorMetrics.m_Results.resize( metricCount );

        for( uint32_t i = 0; i < metricCount; ++i )
        {
            const __WeightedMetric& weightedMetric = aggregatedVendorMetrics[ i ];
            Profiler::Aggregate<NormAggregator>(
                normalizedAggregatedVendorMetrics.m_Results[ i ],
                weightedMetric.weight,
                weightedMetric.value,
                m_PerformanceMetricProperties[ i ].storage );
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
        TipGuard tip( m_pProfiler->m_pDevice->TIP, __func__ );

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
        TipGuard tip( m_pProfiler->m_pDevice->TIP, __func__ );

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
        TipGuard tip( m_pProfiler->m_pDevice->TIP, __func__ );

        auto emplaced = aggregatedPipelines.try_emplace(
            pipeline.m_ShaderTuple.m_Hash,
            static_cast<const DeviceProfilerPipeline&>( pipeline ) );

        DeviceProfilerPipelineData& aggregatedPipelineData = emplaced.first->second;

        if( emplaced.second )
        {
            // Initialize aggregated pipeline data if new element was inserted into the map
            aggregatedPipelineData.m_BeginTimestamp.m_Value = 0;
            aggregatedPipelineData.m_EndTimestamp.m_Value = 0;
        }

        // Increase total pipeline time
        aggregatedPipelineData.m_EndTimestamp.m_Value += ( pipeline.m_EndTimestamp.m_Value - pipeline.m_BeginTimestamp.m_Value );
    }

    /***********************************************************************************\

    Function:
        FreeDynamicAllocations

    Description:
        Frees all dynamic allocations of the submit batch.

    \***********************************************************************************/
    void ProfilerDataAggregator::FreeDynamicAllocations( SubmitBatch& submitBatch )
    {
        if( submitBatch.m_DataCopyFence )
        {
            m_pProfiler->m_pDevice->Callbacks.DestroyFence(
                m_pProfiler->m_pDevice->Handle,
                submitBatch.m_DataCopyFence,
                nullptr );
        }
        if( submitBatch.m_DataCopyCommandBuffer )
        {
            assert( submitBatch.m_pDataCopyCommandPool != nullptr );
            std::unique_lock commandPoolLock( submitBatch.m_pDataCopyCommandPool->GetMutex() );

            m_pProfiler->m_pDevice->Callbacks.FreeCommandBuffers(
                m_pProfiler->m_pDevice->Handle,
                submitBatch.m_pDataCopyCommandPool->GetHandle(),
                1, &submitBatch.m_DataCopyCommandBuffer );
        }

        delete submitBatch.m_pDataBuffer;
    }

    /***********************************************************************************\

    Function:
        WriteQueryDataToGpuBuffer

    Description:
        Allocate a command buffer, record copy commands and submit it for execution.
        Fallback to a CPU allocation if the allocation or recording fails.

    \***********************************************************************************/
    bool ProfilerDataAggregator::WriteQueryDataToGpuBuffer( SubmitBatch& submitBatch )
    {
        // Always submit the fence to GPU to check for data availability later.
        uint32_t submitCount = 0;
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        // Try to copy using GPU if allocation is available.
        if( submitBatch.m_pDataBuffer->UsesGpuAllocation() )
        {
            // Get the command pool associated with the queue.
            DeviceProfilerInternalCommandPool& commandPool = m_CopyCommandPools.at( submitBatch.m_Handle );
            submitBatch.m_pDataCopyCommandPool = &commandPool;

            // Synchronize access to the command pool.
            std::unique_lock commandPoolLock( commandPool.GetMutex() );

            // Allocate the command buffer.
            VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
            commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            commandBufferAllocateInfo.commandBufferCount = 1;
            commandBufferAllocateInfo.commandPool = commandPool.GetHandle();

            VkResult result = m_pProfiler->m_pDevice->Callbacks.AllocateCommandBuffers(
                m_pProfiler->m_pDevice->Handle,
                &commandBufferAllocateInfo,
                &submitBatch.m_DataCopyCommandBuffer );

            if( result == VK_SUCCESS )
            {
                // Command buffers are dispatchable handles, update pointers to parent's dispatch table.
                result = m_pProfiler->m_pDevice->SetDeviceLoaderData(
                    m_pProfiler->m_pDevice->Handle,
                    submitBatch.m_DataCopyCommandBuffer );
            }

            if( result == VK_SUCCESS )
            {
                // Begin recording commands to the copy command buffer.
                VkCommandBufferBeginInfo commandBufferBeginInfo = {};
                commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

                result = m_pProfiler->m_pDevice->Callbacks.BeginCommandBuffer(
                    submitBatch.m_DataCopyCommandBuffer,
                    &commandBufferBeginInfo );
            }

            if( result == VK_SUCCESS )
            {
                // Write timestamp queries to the buffer using the GPU.
                DeviceProfilerQueryDataBufferWriter writer(
                    *m_pProfiler,
                    *submitBatch.m_pDataBuffer,
                    submitBatch.m_DataCopyCommandBuffer );

                for( ProfilerCommandBuffer* pCommandBuffer : submitBatch.m_pSubmittedCommandBuffers )
                {
                    pCommandBuffer->WriteQueryData( writer );
                }

                result = m_pProfiler->m_pDevice->Callbacks.EndCommandBuffer(
                    submitBatch.m_DataCopyCommandBuffer );
            }

            if( result == VK_SUCCESS )
            {
                // Submit the command buffer for execution.
                submitCount = 1;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &submitBatch.m_DataCopyCommandBuffer;
            }

            if( result != VK_SUCCESS )
            {
                // If any of the steps above has failed, fallback to CPU allocation and collect the data later.
                submitBatch.m_pDataBuffer->FallbackToCpuAllocation();
            }
        }

        // Always submit the fence, which is required to check for data availability.
        VkFenceCreateInfo fenceCreateInfo = {};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        VkResult result = m_pProfiler->m_pDevice->Callbacks.CreateFence(
            m_pProfiler->m_pDevice->Handle,
            &fenceCreateInfo,
            nullptr,
            &submitBatch.m_DataCopyFence );

        if( result == VK_SUCCESS )
        {
            // Submit the fence, and optionally the command buffer, for execution.
            result = m_pProfiler->m_pDevice->Callbacks.QueueSubmit(
                submitBatch.m_Handle,
                submitCount,
                &submitInfo,
                submitBatch.m_DataCopyFence );
        }

        return (result == VK_SUCCESS);
    }

    /***********************************************************************************\

    Function:
        WriteQueryDataToCpuBuffer

    Description:
        Copy the data from the query pools to the buffer.

    \***********************************************************************************/
    bool ProfilerDataAggregator::WriteQueryDataToCpuBuffer( SubmitBatch& submitBatch )
    {
        // Drop the packet if the CPU buffer allocation failed.
        if( submitBatch.m_pDataBuffer->GetCpuBuffer() == nullptr )
        {
            return false;
        }

        // Write timestamp queries to the buffer using the CPU.
        DeviceProfilerQueryDataBufferWriter writer(
            *m_pProfiler,
            *submitBatch.m_pDataBuffer );

        for( ProfilerCommandBuffer* pCommandBuffer : submitBatch.m_pSubmittedCommandBuffers )
        {
            pCommandBuffer->WriteQueryData( writer );
        }

        return true;
    }
}

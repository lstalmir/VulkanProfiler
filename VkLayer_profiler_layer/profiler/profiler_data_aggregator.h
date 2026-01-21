// Copyright (c) 2019-2026 Lukasz Stalmirski
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

#pragma once
#include "profiler_data.h"
#include "profiler_command_buffer.h"
#include "profiler_command_pool.h"
#include <list>
#include <vector>
#include <shared_mutex>
#include <thread>
#include <unordered_set>
#include <unordered_map>
// Import extension structures
#include "profiler_ext/VkProfilerEXT.h"

namespace Profiler
{
    class DeviceProfiler;
    class DeviceProfilerQueryDataBuffer;

    struct DeviceProfilerSubmit
    {
        std::vector<ProfilerCommandBuffer*>             m_pCommandBuffers = {};
        std::vector<VkSemaphoreHandle>                  m_SignalSemaphores = {};
        std::vector<VkSemaphoreHandle>                  m_WaitSemaphores = {};
    };

    struct DeviceProfilerSubmitBatch
    {
        VkQueueHandle                                   m_Handle = {};
        ContainerType<DeviceProfilerSubmit>             m_Submits = {};
        uint64_t                                        m_Timestamp = {};
        uint32_t                                        m_ThreadId = {};
    };
    
    struct DeviceProfilerFrame
    {
        uint32_t                                        m_FrameIndex = 0;
        uint32_t                                        m_ThreadId = 0;
        uint64_t                                        m_Timestamp = 0;
        float                                           m_FramesPerSec = 0;
        VkProfilerFrameDelimiterEXT                     m_FrameDelimiter = {};
        DeviceProfilerSynchronizationTimestamps         m_SyncTimestamps = {};
    };

    /***********************************************************************************\

    Class:
        ProfilerDataAggregator

    Description:
        Merges data from multiple command buffers

    \***********************************************************************************/
    class ProfilerDataAggregator
    {
        struct SubmitBatch : DeviceProfilerSubmitBatch
        {
            DeviceProfilerQueryDataBuffer*              m_pDataBuffer = {};

            DeviceProfilerInternalCommandPool*          m_pDataCopyCommandPool = {};
            VkCommandBuffer                             m_DataCopyCommandBuffer = {};
            VkFence                                     m_DataCopyFence = {};

            uint32_t                                    m_SubmitBatchDataIndex = 0;
            std::unordered_set<ProfilerCommandBuffer*>  m_pSubmittedCommandBuffers = {};

            SubmitBatch( const DeviceProfilerSubmitBatch& submitBatch )
                : DeviceProfilerSubmitBatch( submitBatch )
            {}
        };

        struct Frame : DeviceProfilerFrame
        {
            std::list<SubmitBatch>                      m_PendingSubmits = {};
            std::deque<DeviceProfilerSubmitBatchData>   m_CompleteSubmits = {};

            uint64_t                                    m_EndTimestamp = {};

            Frame( const DeviceProfilerFrame& frame )
                : DeviceProfilerFrame( frame )
            {}
        };

    public:
        ProfilerDataAggregator();

        VkResult Initialize( DeviceProfiler* );
        void Destroy();

        void SetDataBufferSize( uint32_t maxFrames );

        bool IsDataCollectionThreadRunning() const { return m_DataCollectionThreadRunning; }
        void StopDataCollectionThread();

        void AppendFrame( const DeviceProfilerFrame& );
        void AppendSubmit( const DeviceProfilerSubmitBatch& );
        void Aggregate( ProfilerCommandBuffer* = nullptr );

        std::list<std::shared_ptr<DeviceProfilerFrameData>> GetAggregatedData();

    private:
        DeviceProfiler* m_pProfiler;

        std::thread m_DataCollectionThread;
        std::atomic_bool m_DataCollectionThreadRunning;

        std::list<std::shared_ptr<DeviceProfilerFrameData>> m_pResolvedFrames;
        std::list<std::shared_ptr<Frame>> m_pPendingFrames;

        std::shared_mutex m_Mutex;
        uint32_t m_FrameIndex;

        uint32_t m_MaxResolvedFrameCount;

        // Command pools used for copying query data
        std::unordered_map<VkQueue, DeviceProfilerInternalCommandPool> m_CopyCommandPools;

        void DataCollectionThreadProc();

        void LoadPerformanceMetricsProperties( uint32_t, std::vector<VkProfilerPerformanceCounterProperties2EXT>& ) const;
        void AggregatePerformanceMetrics( const Frame&, DeviceProfilerPerformanceCountersData& ) const;

        ContainerType<DeviceProfilerPipelineData> CollectTopPipelines( const Frame& ) const;
        void CollectPipelinesFromCommandBuffer( const DeviceProfilerCommandBufferData&, std::unordered_map<uint32_t, DeviceProfilerPipelineData>& ) const;
        void CollectPipeline( const DeviceProfilerPipelineData&, std::unordered_map<uint32_t, DeviceProfilerPipelineData>& ) const;

        void ResolveSubmitBatchData( SubmitBatch&, DeviceProfilerSubmitBatchData& ) const;
        void ResolveFrameData( Frame&, DeviceProfilerFrameData& ) const;

        void FreeDynamicAllocations( SubmitBatch& );
        bool WriteQueryDataToGpuBuffer( SubmitBatch& );
        bool WriteQueryDataToCpuBuffer( SubmitBatch& );
    };
}

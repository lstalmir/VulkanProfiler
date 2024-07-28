// Copyright (c) 2019-2024 Lukasz Stalmirski
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
#include <list>
#include <map>
#include <mutex>
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
        std::vector<VkSemaphore>                        m_SignalSemaphores = {};
        std::vector<VkSemaphore>                        m_WaitSemaphores = {};
    };

    struct DeviceProfilerSubmitBatch
    {
        VkQueue                                         m_Handle = {};
        ContainerType<DeviceProfilerSubmit>             m_Submits = {};
        std::chrono::high_resolution_clock::time_point  m_Timestamp = {};
        uint32_t                                        m_ThreadId = {};

        DeviceProfilerQueryDataBuffer*                  m_pDataBuffer = {};

        VkCommandPool                                   m_DataCopyCommandPool = {};
        VkCommandBuffer                                 m_DataCopyCommandBuffer = {};
        VkFence                                         m_DataCopyFence = {};

        uint32_t                                        m_SubmitBatchDataIndex = 0;
        std::unordered_set<ProfilerCommandBuffer*>      m_pSubmittedCommandBuffers = {};
    };

    /***********************************************************************************\

    Class:
        ProfilerDataAggregator

    Description:
        Merges data from multiple command buffers

    \***********************************************************************************/
    class ProfilerDataAggregator
    {
        struct Frame
        {
            uint32_t                                  m_FrameIndex;
            std::list<DeviceProfilerSubmitBatch>      m_PendingSubmits;
            std::deque<DeviceProfilerSubmitBatchData> m_CompleteSubmits;
        };

    public:
        VkResult Initialize( DeviceProfiler* );
        void Destroy();

        void SetFrameIndex( uint32_t i ) { m_FrameIndex = i; }

        void AppendSubmit( const DeviceProfilerSubmitBatch& );
        void Aggregate( ProfilerCommandBuffer* = nullptr );

        const DeviceProfilerFrameData& GetAggregatedData() const { return m_CurrentFrameData; }

    private:
        DeviceProfiler* m_pProfiler;

        DeviceProfilerFrameData m_CurrentFrameData;
        std::list<Frame> m_NextFrames;

        std::mutex m_Mutex;
        uint32_t m_FrameIndex;

        // Command pools used for copying query data
        std::unordered_map<VkQueue, VkCommandPool> m_CopyCommandPools;

        // Vendor-specific metric properties
        std::vector<VkProfilerPerformanceCounterPropertiesEXT> m_VendorMetricProperties;
        uint32_t                                               m_VendorMetricsSetIndex;

        void LoadVendorMetricsProperties();
        std::vector<VkProfilerPerformanceCounterResultEXT> AggregateVendorMetrics(
            const Frame& ) const;

        ContainerType<DeviceProfilerPipelineData> CollectTopPipelines(
            const Frame& ) const;

        void CollectPipelinesFromCommandBuffer(
            const DeviceProfilerCommandBufferData&,
            std::unordered_map<uint32_t, DeviceProfilerPipelineData>& ) const;

        void CollectPipeline(
            const DeviceProfilerPipelineData&,
            std::unordered_map<uint32_t, DeviceProfilerPipelineData>& ) const;

        void ResolveSubmitBatchData(
            const DeviceProfilerSubmitBatch&,
            DeviceProfilerSubmitBatchData& ) const;

        void ResolveFrameData(
            Frame&,
            DeviceProfilerFrameData& ) const;

        void WriteQueryDataToGpuBuffer(
            DeviceProfilerSubmitBatch& );

        void WriteQueryDataToCpuBuffer(
            DeviceProfilerSubmitBatch& );
    };
}

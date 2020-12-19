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

    struct DeviceProfilerSubmit
    {
        ContainerType<ProfilerCommandBuffer*>   m_pCommandBuffers;
    };

    struct DeviceProfilerSubmitBatch
    {
        VkQueue                                 m_Handle = {};
        ContainerType<DeviceProfilerSubmit>     m_Submits = {};
        uint64_t                                m_Timestamp = {};
    };

    /***********************************************************************************\

    Class:
        ProfilerDataAggregator

    Description:
        Merges data from multiple command buffers

    \***********************************************************************************/
    class ProfilerDataAggregator
    {
    public:
        VkResult Initialize( DeviceProfiler* );

        void AppendSubmit( const DeviceProfilerSubmitBatch& );
        void AppendData( ProfilerCommandBuffer*, std::shared_ptr<const CommandBufferData> );
        
        void Aggregate();
        void Reset();

        DeviceProfilerFrameData GetAggregatedData() const;

    private:
        DeviceProfiler* m_pProfiler;

        std::list<DeviceProfilerSubmitBatch> m_Submits;
        std::list<DeviceProfilerSubmitBatchData> m_AggregatedData;

        std::unordered_map<ProfilerCommandBuffer*, std::shared_ptr<const CommandBufferData>> m_Data;

        std::mutex m_Mutex;

        // Vendor-specific metric properties
        std::vector<VkProfilerPerformanceCounterPropertiesEXT> m_VendorMetricProperties;

        std::vector<VkProfilerPerformanceCounterResultEXT> AggregateVendorMetrics() const;

        std::vector<AggregatedPipelineData> CollectTopPipelines() const;

        std::vector<DeviceProfilerSubmitBatchData> ConstructFrameTree() const;
    };
}

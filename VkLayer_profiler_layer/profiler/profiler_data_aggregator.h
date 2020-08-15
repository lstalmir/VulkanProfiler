#pragma once
#include "profiler_data.h"
#include "profiler_command_buffer.h"
#include <list>
#include <map>
#include <unordered_set>
#include <unordered_map>
// Import extension structures
#include "profiler_ext/VkProfilerEXT.h"

namespace Profiler
{
    class DeviceProfiler;

    struct DeviceProfilerSubmit
    {
        std::list<ProfilerCommandBuffer*>   m_pCommandBuffers;
    };

    struct DeviceProfilerSubmitBatch
    {
        VkQueue                             m_Handle = {};
        std::list<DeviceProfilerSubmit>     m_Submits = {};
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
        void AppendData( ProfilerCommandBuffer*, const DeviceProfilerCommandBufferData& );
        
        void Reset();

        DeviceProfilerFrameData GetAggregatedData();

    private:
        DeviceProfiler* m_pProfiler;

        std::list<DeviceProfilerSubmitBatch> m_Submits;
        std::list<DeviceProfilerSubmitBatchData> m_AggregatedData;

        std::unordered_map<ProfilerCommandBuffer*, DeviceProfilerCommandBufferData> m_Data;

        // Vendor-specific metric properties
        std::vector<VkProfilerPerformanceCounterPropertiesEXT> m_VendorMetricProperties;

        void MergeCommandBuffers();

        std::vector<VkProfilerPerformanceCounterResultEXT> AggregateVendorMetrics() const;

        std::list<DeviceProfilerPipelineData> CollectTopPipelines();

        void CollectPipelinesFromCommandBuffer(
            const DeviceProfilerCommandBufferData&,
            std::unordered_set<DeviceProfilerPipelineData>& );

        void CollectPipeline(
            const DeviceProfilerPipelineData&,
            std::unordered_set<DeviceProfilerPipelineData>& );
    };
}

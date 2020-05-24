#pragma once
#include "profiler_command_buffer.h"
#include "profiler_pipeline.h"
#include <list>
#include <map>
#include <unordered_set>

namespace Profiler
{
    struct ProfilerAggregatedMemoryData
    {
        uint64_t m_TotalAllocationSize;
        uint64_t m_TotalAllocationCount;
        uint64_t m_DeviceLocalAllocationSize;
        uint64_t m_HostVisibleAllocationSize;
    };

    struct ProfilerAggregatedCPUData
    {
        uint64_t m_TimeNs;
    };

    /***********************************************************************************\

    Structure:
        ProfilerAggregatedData

    Description:

    \***********************************************************************************/
    struct ProfilerAggregatedData
    {
        std::vector<ProfilerSubmitData> m_Submits;
        std::vector<ProfilerPipeline> m_TopPipelines;

        ProfilerRangeStats m_Stats;

        ProfilerAggregatedMemoryData m_Memory;
        ProfilerAggregatedCPUData m_CPU;

        std::list<std::pair<std::string, float>> m_VendorMetrics;
    };

    struct ProfilerSubmit
    {
        std::vector<ProfilerCommandBuffer*> m_pCommandBuffers;
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
        void AppendSubmit( const ProfilerSubmit& );
        
        void Reset();

        ProfilerAggregatedData GetAggregatedData();

    private:
        std::list<ProfilerSubmit> m_Submits;

        std::list<ProfilerSubmitData> m_AggregatedData;

        void MergeCommandBuffers();

        //std::unordered_set<ProfilerShaderTuple> CollectShaderTuples( const ProfilerSubmitData& ) const;
        //std::unordered_set<ProfilerShaderTuple> CollectShaderTuples( const ProfilerCommandBufferData& ) const;

        std::list<ProfilerPipeline> CollectTopPipelines();
    };
}

#pragma once
#include "profiler_command_buffer.h"
#include "profiler_pipeline.h"
#include <list>
#include <map>
#include <unordered_set>

namespace Profiler
{
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
        void AppendData( const ProfilerSubmitData& );

        void Reset();

        ProfilerAggregatedData GetAggregatedData();

    private:
        std::list<ProfilerSubmitData> m_AggregatedData;

        void MergeCommandBuffers();

        //std::unordered_set<ProfilerShaderTuple> CollectShaderTuples( const ProfilerSubmitData& ) const;
        //std::unordered_set<ProfilerShaderTuple> CollectShaderTuples( const ProfilerCommandBufferData& ) const;

        std::list<ProfilerPipeline> CollectTopPipelines();
    };
}

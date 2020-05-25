#pragma once
#include "profiler_command_buffer.h"
#include "profiler_pipeline.h"
#include <list>
#include <map>
#include <unordered_set>

namespace Profiler
{
    class VkDevice_Object;

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

    static constexpr uint32_t COPY_TUPLE_HASH = 0xFFFFFFFE;
    static constexpr uint32_t CLEAR_TUPLE_HASH = 0xFFFFFFFD;
    static constexpr uint32_t BEGIN_RENDER_PASS_TUPLE_HASH = 0xFFFFFFFC;
    static constexpr uint32_t END_RENDER_PASS_TUPLE_HASH = 0xFFFFFFFB;
    static constexpr uint32_t PIPELINE_BARRIER_TUPLE_HASH = 0xFFFFFFFA;
    static constexpr uint32_t RESOLVE_TUPLE_HASH = 0xFFFFFFF9;

    /***********************************************************************************\

    Class:
        ProfilerDataAggregator

    Description:
        Merges data from multiple command buffers

    \***********************************************************************************/
    class ProfilerDataAggregator
    {
    public:
        VkResult Initialize( VkDevice_Object* );

        void AppendSubmit( const ProfilerSubmit& );
        
        void Reset();

        ProfilerAggregatedData GetAggregatedData();

    private:
        std::list<ProfilerSubmit> m_Submits;

        std::list<ProfilerSubmitData> m_AggregatedData;

        // Pipelines accumulating drawcall data
        ProfilerPipeline m_CopyPipeline;
        ProfilerPipeline m_ClearPipeline;
        ProfilerPipeline m_BeginRenderPassPipeline;
        ProfilerPipeline m_EndRenderPassPipeline;
        ProfilerPipeline m_PipelineBarrierPipeline;
        ProfilerPipeline m_ResolvePipeline;

        void InitializePipeline( VkDevice_Object*, ProfilerPipeline&, uint32_t );

        void MergeCommandBuffers();

        //std::unordered_set<ProfilerShaderTuple> CollectShaderTuples( const ProfilerSubmitData& ) const;
        //std::unordered_set<ProfilerShaderTuple> CollectShaderTuples( const ProfilerCommandBufferData& ) const;

        std::list<ProfilerPipeline> CollectTopPipelines();
    };
}

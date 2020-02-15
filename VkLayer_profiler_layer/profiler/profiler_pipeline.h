#pragma once
#include "profiler_drawcall.h"
#include "profiler_helpers.h"
#include "profiler_shader.h"
#include <vulkan.h>

namespace Profiler
{
    struct ProfilerRangeStats
    {
        uint64_t m_BeginTimestamp;
        uint64_t m_TotalTicks;

        uint32_t m_TotalDrawCount;
        uint32_t m_TotalDispatchCount;
        uint32_t m_TotalCopyCount;
        uint32_t m_TotalBarrierCount;

        inline void Clear()
        {
            ClearMemory( this );
        }
    };


    template<typename Handle, typename Subtype>
    struct ProfilerRangeStatsCollector
    {
        Handle m_Handle;
        ProfilerRangeStats m_Stats;

        // Valid only if profiling mode collects data for Subtype regions
        std::vector<Subtype> m_Subregions;

        inline void Clear()
        {
            m_Stats.Clear();
            m_Subregions.clear();
        }

        inline void OnDraw()
        {
            m_Stats.m_TotalDrawCount++;
            m_Subregions.back().OnDraw();
        }

        inline void OnDispatch()
        {
            m_Stats.m_TotalDispatchCount++;
            m_Subregions.back().OnDispatch();
        }

        inline void OnCopy()
        {
            m_Stats.m_TotalCopyCount++;
            m_Subregions.back().OnCopy();
        }
    };

    /***********************************************************************************\

    Structure:
        ProfilerPipeline

    Description:
        Contains data collected per-pipeline.

    \***********************************************************************************/
    struct ProfilerPipeline : ProfilerRangeStatsCollector<VkPipeline, ProfilerDrawcall>
    {
        ProfilerShaderTuple m_ShaderTuple;

        inline bool operator==( const ProfilerPipeline& rh ) const
        {
            return m_ShaderTuple == rh.m_ShaderTuple;
        }

        inline void OnDraw()
        {
            m_Stats.m_TotalDrawCount++;
            m_Subregions.push_back( { ProfilerDrawcallType::eDraw } );
        }

        inline void OnDispatch()
        {
            m_Stats.m_TotalDispatchCount++;
            m_Subregions.push_back( { ProfilerDrawcallType::eDispatch } );
        }

        inline void OnCopy()
        {
            m_Stats.m_TotalCopyCount++;
            m_Subregions.push_back( { ProfilerDrawcallType::eCopy } );
        }
    };
}


namespace std
{
    template<>
    struct hash<Profiler::ProfilerPipeline>
    {
        hash<Profiler::ProfilerShaderTuple> m_ShaderTupleHash;

        inline size_t operator()( const Profiler::ProfilerPipeline& pipeline ) const
        {
            return m_ShaderTupleHash( pipeline.m_ShaderTuple );
        }
    };
}

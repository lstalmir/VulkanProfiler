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

        uint32_t m_TotalDrawcallCount;
        uint32_t m_TotalDrawCount;
        uint32_t m_TotalDrawIndirectCount;
        uint32_t m_TotalDispatchCount;
        uint32_t m_TotalDispatchIndirectCount;
        uint32_t m_TotalCopyCount;
        uint32_t m_TotalClearCount;
        uint32_t m_TotalClearImplicitCount;
        uint32_t m_TotalBarrierCount;
        uint32_t m_TotalImplicitBarrierCount;

        inline void Clear()
        {
            ClearMemory( this );
        }

        inline void Add( const ProfilerRangeStats& rh )
        {
            m_TotalTicks += rh.m_TotalTicks;
            m_TotalDrawcallCount += rh.m_TotalDrawcallCount;
            m_TotalDrawCount += rh.m_TotalDrawCount;
            m_TotalDrawIndirectCount += rh.m_TotalDrawIndirectCount;
            m_TotalDispatchCount += rh.m_TotalDispatchCount;
            m_TotalDispatchIndirectCount += rh.m_TotalDispatchIndirectCount;
            m_TotalCopyCount += rh.m_TotalCopyCount;
            m_TotalClearCount += rh.m_TotalClearCount;
            m_TotalClearImplicitCount += rh.m_TotalClearImplicitCount;
            m_TotalBarrierCount += rh.m_TotalBarrierCount;
            m_TotalImplicitBarrierCount += rh.m_TotalImplicitBarrierCount;
        }

        template<size_t Stat>
        inline void IncrementStat( uint32_t count = 1 )
        {
            // Get address of the counter
            uint8_t* pCounter = reinterpret_cast<uint8_t*>(this) + Stat;

            // Convert to modifiable uint32_t reference
            uint32_t& counter = *reinterpret_cast<uint32_t*>(pCounter);

            counter += count;

            IncrementDrawcallCount<Stat>( count );
        }

    private:
        template<size_t Size>
        inline void IncrementDrawcallCount( uint32_t count )
        {
            m_TotalDrawcallCount += count;
        }
    };

    static constexpr size_t STAT_DRAW_COUNT = offsetof( ProfilerRangeStats, m_TotalDrawCount );
    static constexpr size_t STAT_DRAW_INDIRECT_COUNT = offsetof( ProfilerRangeStats, m_TotalDrawIndirectCount );
    static constexpr size_t STAT_DISPATCH_COUNT = offsetof( ProfilerRangeStats, m_TotalDispatchCount );
    static constexpr size_t STAT_DISPATCH_INDIRECT_COUNT = offsetof( ProfilerRangeStats, m_TotalDispatchIndirectCount );
    static constexpr size_t STAT_COPY_COUNT = offsetof( ProfilerRangeStats, m_TotalCopyCount );
    static constexpr size_t STAT_CLEAR_COUNT = offsetof( ProfilerRangeStats, m_TotalClearCount );
    static constexpr size_t STAT_CLEAR_IMPLICIT_COUNT = offsetof( ProfilerRangeStats, m_TotalClearImplicitCount );
    static constexpr size_t STAT_BARRIER_COUNT = offsetof( ProfilerRangeStats, m_TotalBarrierCount );
    static constexpr size_t STAT_IMPLICIT_BARRIER_COUNT = offsetof( ProfilerRangeStats, m_TotalImplicitBarrierCount );

    template<>
    inline void ProfilerRangeStats::IncrementDrawcallCount<STAT_BARRIER_COUNT>( uint32_t )
    {
    }

    template<>
    inline void ProfilerRangeStats::IncrementDrawcallCount<STAT_IMPLICIT_BARRIER_COUNT>( uint32_t )
    {
    }


    template<typename Handle, typename Subtype>
    struct ProfilerRangeStatsCollector
    {
        Handle m_Handle;
        ProfilerRangeStats m_Stats;

        std::vector<Subtype> m_Subregions;

        inline void Clear()
        {
            m_Stats.Clear();
            m_Subregions.clear();
        }

        template<size_t Stat>
        inline void IncrementStat( uint32_t count = 1 )
        {
            m_Stats.IncrementStat<Stat>( count );
            m_Subregions.back().IncrementStat<Stat>( count );
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

        template<size_t Stat>
        inline void IncrementStat( uint32_t count = 1 )
        {
            m_Stats.IncrementStat<Stat>( count );

            switch( Stat )
            {
            case STAT_DRAW_COUNT:
            case STAT_DRAW_INDIRECT_COUNT:
                m_Subregions.push_back( { ProfilerDrawcallType::eDraw } ); break;

            case STAT_DISPATCH_COUNT:
            case STAT_DISPATCH_INDIRECT_COUNT:
                m_Subregions.push_back( { ProfilerDrawcallType::eDispatch } ); break;

            case STAT_CLEAR_COUNT:
                m_Subregions.push_back( { ProfilerDrawcallType::eClear } ); break;

            case STAT_COPY_COUNT:
                m_Subregions.push_back( { ProfilerDrawcallType::eCopy } ); break;
            }
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

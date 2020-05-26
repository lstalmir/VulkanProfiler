#pragma once
#include "profiler_drawcall.h"
#include "profiler_pipeline.h"

namespace Profiler
{
    // Range duration comparator
    template<typename Data>
    inline bool DurationDesc( const Data& a, const Data& b )
    {
        return a.m_Stats.m_TotalTicks > b.m_Stats.m_TotalTicks;
    }

    template<typename Data>
    inline bool DurationAsc( const Data& a, const Data& b )
    {
        return a.m_Stats.m_TotalTicks < b.m_Stats.m_TotalTicks;
    }

    // Drawcall duration comparator
    template<>
    inline bool DurationDesc( const ProfilerDrawcall& a, const ProfilerDrawcall& b )
    {
        return a.m_Ticks > b.m_Ticks;
    }

    template<>
    inline bool DurationAsc( const ProfilerDrawcall& a, const ProfilerDrawcall& b )
    {
        return a.m_Ticks < b.m_Ticks;
    }
}

#pragma once

namespace Profiler
{
    // Range duration comparator
    template<typename Data>
    inline bool DurationDesc( const Data& a, const Data& b )
    {
        return a.m_Ticks > b.m_Ticks;
    }

    template<typename Data>
    inline bool DurationAsc( const Data& a, const Data& b )
    {
        return a.m_Ticks < b.m_Ticks;
    }
}

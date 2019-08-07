#include "counters.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        CpuCounter

    Description:
        Constructor

    \***********************************************************************************/
    CpuCounter::CpuCounter()
        : m_Value( 0 )
    {
    }

    void CpuCounter::Reset()
    {
        m_Value = 0;
    }

    void CpuCounter::Increment()
    {
        m_Value++;
    }

    void CpuCounter::Decrement()
    {
        m_Value--;
    }

    uint64_t CpuCounter::GetValue() const
    {
        return m_Value;
    }

    /***********************************************************************************\

    Function:
        CpuTimestampCounter

    Description:
        Constructor

    \***********************************************************************************/
    CpuTimestampCounter::CpuTimestampCounter()
    {
    }

    void CpuTimestampCounter::Reset()
    {
    }

    void CpuTimestampCounter::Begin()
    {
        m_BeginValue = std::chrono::high_resolution_clock::now();
    }

    void CpuTimestampCounter::End()
    {
        m_EndValue = std::chrono::high_resolution_clock::now();
    }

    uint64_t CpuTimestampCounter::GetValue() const
    {
        using namespace std::chrono;
        return duration_cast<microseconds>(m_EndValue - m_BeginValue).count();
    }
}

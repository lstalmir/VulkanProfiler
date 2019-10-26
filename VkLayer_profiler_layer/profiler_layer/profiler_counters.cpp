#include "profiler_counters.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        CpuCounter {ctor}

    Description:
        Creates new CPU counter instance.

    \***********************************************************************************/
    CpuCounter::CpuCounter()
        : m_Value( 0 )
    {
    }

    /***********************************************************************************\

    Function:
        Reset

    Description:
        Resets the CPU counter value to 0.

    \***********************************************************************************/
    void CpuCounter::Reset()
    {
        m_Value = 0;
    }

    /***********************************************************************************\

    Function:
        Increment

    Description:
        Increments the CPU counter by 1.

    \***********************************************************************************/
    void CpuCounter::Increment()
    {
        m_Value++;
    }

    /***********************************************************************************\

    Function:
        Decrement

    Description:
        Decrements the CPU counter by 1.

    \***********************************************************************************/
    void CpuCounter::Decrement()
    {
        m_Value--;
    }

    /***********************************************************************************\

    Function:
        Add

    Description:
        Increments the CPU counter by value.

    \***********************************************************************************/
    void CpuCounter::Add( uint64_t value )
    {
        m_Value += value;
    }

    /***********************************************************************************\

    Function:
        Decrement

    Description:
        Decrements the CPU counter by value.

    \***********************************************************************************/
    void CpuCounter::Subtract( uint64_t value )
    {
        m_Value -= value;
    }

    /***********************************************************************************\

    Function:
        GetValue

    Description:
        Gets current counter value.

    \***********************************************************************************/
    uint64_t CpuCounter::GetValue() const
    {
        return m_Value;
    }

    /***********************************************************************************\

    Function:
        CpuTimestampCounter {ctor}

    Description:
        Create new CPU time counter.

    \***********************************************************************************/
    CpuTimestampCounter::CpuTimestampCounter()
    {
    }

    /***********************************************************************************\

    Function:
        Reset

    Description:
        Reset the time measurement range.

    \***********************************************************************************/
    void CpuTimestampCounter::Reset()
    {
    }

    /***********************************************************************************\

    Function:
        Begin

    Description:
        Begin time measurement range.

    \***********************************************************************************/
    void CpuTimestampCounter::Begin()
    {
        m_BeginValue = std::chrono::high_resolution_clock::now();
    }

    /***********************************************************************************\

    Function:
        End

    Description:
        End time measurement range.

    \***********************************************************************************/
    void CpuTimestampCounter::End()
    {
        m_EndValue = std::chrono::high_resolution_clock::now();
    }

    /***********************************************************************************\

    Function:
        GetValue

    Description:
        Get difference between start and end of the measured time range.

    \***********************************************************************************/
    uint64_t CpuTimestampCounter::GetValue() const
    {
        using namespace std::chrono;
        return duration_cast<microseconds>(m_EndValue - m_BeginValue).count();
    }
}

#pragma once
#include <atomic>
#include <chrono>

namespace Profiler
{
    /***********************************************************************************\

    Class:
        CpuCounter

    Description:


    \***********************************************************************************/
    class CpuCounter
    {
    public:
        // Initialize new CPU counter
        inline CpuCounter() { m_Value = 0; }

        // Reset counter value
        inline void Reset() { m_Value = 0; }

        // Increment counter value
        inline void Increment() { m_Value++; }

        // Decrement counter value
        inline void Decrement() { m_Value--; }

        // Add value to the counter
        inline void Add( int64_t value ) { m_Value += value; }

        // Subtract value from the counter
        inline void Subtract( int64_t value ) { m_Value -= value; }

        // Get current counter value
        inline int64_t GetValue() const { return m_Value; }

    protected:
        std::atomic_int64_t m_Value;
    };

    /***********************************************************************************\

    Class:
        CpuTimestampCounter

    Description:


    \***********************************************************************************/
    class CpuTimestampCounter
    {
    public:
        // Initialize new CPU timestamp query counter
        inline CpuTimestampCounter() { m_BeginValue = m_EndValue = std::chrono::high_resolution_clock::now(); }

        // Reset counter values
        inline void Reset() { m_BeginValue = m_EndValue = std::chrono::high_resolution_clock::now(); }

        // Begin timestamp query
        inline void Begin() { m_BeginValue = std::chrono::high_resolution_clock::now(); }

        // End timestamp query
        inline void End() { m_EndValue = std::chrono::high_resolution_clock::now(); }

        // Get time range between begin and end
        template<typename Unit>
        inline auto GetValue() const { return std::chrono::duration_cast<Unit>(m_EndValue - m_BeginValue); }

    protected:
        std::chrono::high_resolution_clock::time_point m_BeginValue;
        std::chrono::high_resolution_clock::time_point m_EndValue;
    };
}
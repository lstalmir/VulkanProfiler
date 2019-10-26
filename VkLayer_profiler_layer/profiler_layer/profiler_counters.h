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
        CpuCounter();

        void Reset();

        void Increment();
        void Decrement();
        void Add( uint64_t value );
        void Subtract( uint64_t value );

        uint64_t GetValue() const;

    protected:
        std::atomic_uint64_t m_Value;
    };

    /***********************************************************************************\

    Class:
        CpuTimestampCounter

    Description:


    \***********************************************************************************/
    class CpuTimestampCounter
    {
    public:
        CpuTimestampCounter();
        
        void Reset();

        void Begin();
        void End();

        uint64_t GetValue() const;

    protected:
        std::chrono::high_resolution_clock::time_point m_BeginValue;
        std::chrono::high_resolution_clock::time_point m_EndValue;
    };
}

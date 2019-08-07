#pragma once
#include <atomic>
#include <chrono>

namespace Profiler
{
    /***********************************************************************************\

    Class:
        CpuCounter

    Description:
        Set of VkInstance functions which are overloaded in this layer.

    \***********************************************************************************/
    class CpuCounter
    {
    public:
        CpuCounter();

        void Reset();
        void Increment();
        void Decrement();
        uint64_t GetValue() const;

    protected:
        std::atomic_uint64_t m_Value;
    };

    /***********************************************************************************\

    Class:
        CpuTimestampCounter

    Description:
        Set of VkInstance functions which are overloaded in this layer.

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

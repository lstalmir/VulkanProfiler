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

    /***********************************************************************************\

    Class:
        CpuScopedTimestampCounter

    Description:


    \***********************************************************************************/
    template<typename Unit, bool Overwrite = false>
    class CpuScopedTimestampCounter : private CpuTimestampCounter
    {
    public:
        // Initialize new scoped CPU timestamp query counter
        inline CpuScopedTimestampCounter( uint64_t& valueOut ) : m_ValueOut( valueOut )
        {
            Begin();
        }

        // Destroy scoped CPU timestamp query counter, write result to valueOut
        inline ~CpuScopedTimestampCounter()
        {
            End();

            const Unit result = this->template GetValue<Unit>();

            if( Overwrite )
            {
                // Overwrite result of the query
                m_ValueOut = result.count();
            }
            else
            {
                // Aggregate result of the query
                m_ValueOut += result.count();
            }
        }

    private:
        uint64_t& m_ValueOut;
    };

    /***********************************************************************************\

    Class:
        CpuEventFrequencyCounter

    Description:
        Special kind of counter that reads average number of generated events in given
        timespan.

    \***********************************************************************************/
    class CpuEventFrequencyCounter
    {
        using TimePointType = typename std::chrono::high_resolution_clock::time_point;
        using DurationType = typename TimePointType::duration;

    public:
        // Initialize event counter
        template<typename Unit = std::chrono::seconds>
        inline CpuEventFrequencyCounter( Unit refreshRate = std::chrono::seconds( 1 ) )
            : m_RefreshRate( std::chrono::duration_cast<DurationType>(refreshRate) )
        {
            m_EventCount = 0;
            m_EventFrequency = 0;
            m_BeginTimestamp = std::chrono::high_resolution_clock::now();
        }

        // Update event counter
        inline void Update()
        {
            m_EventCount++;

            const TimePointType timestamp = std::chrono::high_resolution_clock::now();
            const DurationType delta = (timestamp - m_BeginTimestamp);

            if( delta > m_RefreshRate )
            {
                m_EventFrequency = (m_EventCount) / (delta.count() * REFRESH_RATE_TO_SEC);
                m_EventCount = 0;
                m_BeginTimestamp = timestamp;
            }
        }

        // Get current event frequency
        inline float GetValue() const { return m_EventFrequency; }

    protected:
        TimePointType m_BeginTimestamp;
        const DurationType m_RefreshRate;
        uint32_t m_EventCount;
        float m_EventFrequency;

        static constexpr float REFRESH_RATE_TO_SEC =
            static_cast<float>(DurationType::period::num) /
            static_cast<float>(DurationType::period::den);
    };
}

// Copyright (c) 2019-2024 Lukasz Stalmirski
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once
#include "profiler_helpers.h"

#include <vulkan/vk_layer.h>

#include <atomic>
#include <chrono>

// Include OS-specific implementation of CPU counters
#ifdef WIN32
#include "profiler_counters_windows.inl"
#else
#include "profiler_counters_linux.inl"
#endif

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
        explicit CpuTimestampCounter( VkTimeDomainEXT domain = OSGetDefaultTimeDomain() ) : m_TimeDomain( domain ) { Reset(); }

        // Set the time domain in which the timestamps should be collected
        inline void SetTimeDomain( VkTimeDomainEXT domain ) { m_TimeDomain = domain; }

        // Reset counter values
        inline void Reset() { m_BeginValue = m_EndValue = OSGetTimestamp( m_TimeDomain ); }

        // Begin timestamp query
        inline void Begin() { m_BeginValue = OSGetTimestamp( m_TimeDomain ); }

        // End timestamp query
        inline void End() { m_EndValue = OSGetTimestamp( m_TimeDomain ); }

        // Get time range between begin and end
        template<typename Unit = std::chrono::nanoseconds>
        inline auto GetValue() const
        {
            return std::chrono::duration_cast<Unit>(std::chrono::nanoseconds(
                ((m_EndValue - m_BeginValue) * 1'000'000'000) / OSGetTimestampFrequency( m_TimeDomain ) ));
        }

        inline uint64_t GetBeginValue() const { return m_BeginValue; }

        inline uint64_t GetCurrentValue() const { return OSGetTimestamp( m_TimeDomain ); }

    protected:
        uint64_t m_BeginValue;
        uint64_t m_EndValue;
        VkTimeDomainEXT m_TimeDomain;
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
    public:
        // Initialize event counter
        template<typename Unit = std::chrono::seconds>
        inline CpuEventFrequencyCounter( Unit refreshRate = std::chrono::seconds( 1 ), VkTimeDomainEXT timeDomain = OSGetDefaultTimeDomain() )
            : m_RefreshRate( std::chrono::duration_cast<std::chrono::nanoseconds>(refreshRate).count() )
            , m_TimeDomain( timeDomain )
        {
            m_EventCount = 0;
            m_EventFrequency = 0;
            m_BeginTimestamp = OSGetTimestamp( m_TimeDomain );
        }

        // Set the new time domain
        inline void SetTimeDomain( VkTimeDomainEXT timeDomain )
        {
            m_TimeDomain = timeDomain;
        }

        // Update event counter
        inline bool Update()
        {
            m_EventCount++;

            const uint64_t timestamp = OSGetTimestamp( m_TimeDomain );
            const float delta = static_cast<float>(timestamp - m_BeginTimestamp);

            if( delta > m_RefreshRate )
            {
                m_EventFrequency = (m_EventCount * OSGetTimestampFrequency( m_TimeDomain )) / delta;
                m_LastEventCount = m_EventCount;
                m_EventCount = 0;
                m_BeginTimestamp = timestamp;
                return true;
            }

            return false;
        }

        // Get current event frequency
        inline float GetValue() const { return m_EventFrequency; }

        // Get last event count
        inline uint32_t GetEventCount() const { return m_LastEventCount; }

    protected:
        uint64_t m_BeginTimestamp;
        uint64_t m_RefreshRate;
        uint32_t m_EventCount;
        uint32_t m_LastEventCount;
        float m_EventFrequency;
        VkTimeDomainEXT m_TimeDomain;
    };
}

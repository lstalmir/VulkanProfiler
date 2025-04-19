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
#include <vector>
#include <stack>

// Include OS-specific implementation of CPU counters
#ifdef WIN32
#include "profiler_counters_windows.inl"
#else
#include "profiler_counters_linux.inl"
#endif

// Enable time in profiler counters in non-release builds
#ifndef PROFILER_ENABLE_TIP
#if defined( PROFILER_CONFIG_DEBUG ) || defined( PROFILER_CONFIG_RELWITHDEBINFO )
#define PROFILER_ENABLE_TIP 1
#else
#define PROFILER_ENABLE_TIP 0
#endif
#endif // PROFILER_ENABLE_TIP

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
            : m_RefreshRate( std::chrono::nanoseconds( refreshRate ).count() * 0.000'000'001f )
            , m_TimeDomain( timeDomain )
        {
            m_EventCount = 0;
            m_EventFrequency = 0;
            m_LastEventCount = 0;
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
            const float delta = static_cast<float>(timestamp - m_BeginTimestamp) /
                OSGetTimestampFrequency( m_TimeDomain );

            if( delta > m_RefreshRate )
            {
                m_EventFrequency = m_EventCount / delta;
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
        float m_RefreshRate;
        uint32_t m_EventCount;
        uint32_t m_LastEventCount;
        float m_EventFrequency;
        VkTimeDomainEXT m_TimeDomain;
    };

    /***********************************************************************************\

    Structure:
        TipRange

    Description:
        Single time in profiler range.

    \***********************************************************************************/
    struct TipRange
    {
        const char* m_pFunctionName;
        uint32_t m_ThreadId;
        uint64_t m_CallStackSize;
        uint64_t m_BeginTimestamp;
        uint64_t m_EndTimestamp;

        inline TipRange( const char* pFunctionName, uint32_t threadId, uint64_t beginTimestamp )
            : m_pFunctionName( pFunctionName )
            , m_ThreadId( threadId )
            , m_CallStackSize( 0 )
            , m_BeginTimestamp( beginTimestamp )
            , m_EndTimestamp( beginTimestamp )
        {}
    };

    /***********************************************************************************\

    Structure:
        TipRangeId

    Description:
        Identifies a time in profiler range.

    \***********************************************************************************/
    struct TipRangeId
    {
        uint32_t m_FrameIndex;
        uint32_t m_RangeIndex;
        uint32_t m_ParentIndex;
    };

    /***********************************************************************************\

    Class:
        TipCounter

    Description:
        Time in profiler counter.

    \***********************************************************************************/
    template<bool Enabled>
    class TipCounterImpl;
    using TipCounter = TipCounterImpl<PROFILER_ENABLE_TIP>;

    template<>
    class TipCounterImpl<false>
    {
    public:
        inline void SetTimeDomain( VkTimeDomainEXT ) {}
        inline void Reset() {}
        inline TipRangeId BeginFunction( const char* ) { return TipRangeId(); }
        inline void EndFunction( TipRangeId ) {}
        inline auto GetData() { return std::vector<TipRange>(); }
    };

    template<>
    class TipCounterImpl<true>
    {
    public:
        // Sets the time domain to collect the CPU timestamps.
        inline void SetTimeDomain( VkTimeDomainEXT timeDomain )
        {
            m_CpuTimestampCounter.SetTimeDomain( timeDomain );
        }

        // Clears all collected TIP regions.
        inline void Reset()
        {
            std::scoped_lock lk( m_Mutex );
            m_Ranges.clear();
            m_FrameIndex++;
            m_CurrentRegionIndex = UINT32_MAX;
        }

        // Begins TIP region in the current call stack.
        inline TipRangeId BeginFunction( const char* pFunctionName )
        {
            std::scoped_lock lk( m_Mutex );
            m_Ranges.emplace_back(
                pFunctionName,
                ProfilerPlatformFunctions::GetCurrentThreadId(),
                m_CpuTimestampCounter.GetCurrentValue() );

            TipRangeId id = {};
            id.m_FrameIndex = m_FrameIndex;
            id.m_RangeIndex = static_cast<uint32_t>( m_Ranges.size() - 1 );
            id.m_ParentIndex = m_CurrentRegionIndex;

            m_CurrentRegionIndex = id.m_RangeIndex;

            return id;
        }

        // Ends the last TIP region in the current call stack.
        inline void EndFunction( TipRangeId id )
        {
            std::scoped_lock lk( m_Mutex );

            // If frame has changed between Begin and End, the m_Ranges no longer stores data for this region.
            if( id.m_FrameIndex == m_FrameIndex )
            {
                TipRange& range = m_Ranges.at( id.m_RangeIndex );
                range.m_EndTimestamp = m_CpuTimestampCounter.GetCurrentValue();

                if( id.m_ParentIndex != UINT32_MAX )
                {
                    m_Ranges.at( id.m_ParentIndex ).m_CallStackSize += range.m_CallStackSize;
                }

                m_CurrentRegionIndex = id.m_ParentIndex;
            }
        }

        // Returns all collected TIP ranges.
        inline std::vector<TipRange> GetData()
        {
            std::scoped_lock lk( m_Mutex );
            return m_Ranges;
        }

    private:
        CpuTimestampCounter   m_CpuTimestampCounter;
        std::mutex            m_Mutex;
        std::vector<TipRange> m_Ranges;
        uint32_t              m_FrameIndex = 0;
        uint32_t              m_CurrentRegionIndex = UINT32_MAX;
    };

    /***********************************************************************************\

    Class:
        TipGuard

    Description:
        RAII wrapper that counts time spent in the profiler.

    \***********************************************************************************/
    template<bool Enabled>
    class TipGuardImpl;
    using TipGuard = TipGuardImpl<PROFILER_ENABLE_TIP>;

    /* Disabled */
    template<>
    class TipGuardImpl<false>
    {
    public:
        inline TipGuardImpl( TipCounter&, const char* ) {}
    };

    /* Enabled */
    template<>
    class TipGuardImpl<true>
    {
    public:
        // Begins TIP measurement.
        inline TipGuardImpl( TipCounter& counter, const char* pFunctionName )
            : m_TipCounter( counter )
        {
            m_TipRangeId = m_TipCounter.BeginFunction( pFunctionName );
        }

        // Ends TIP measurement.
        inline ~TipGuardImpl()
        {
            m_TipCounter.EndFunction( m_TipRangeId );
        }

    private:
        TipCounter& m_TipCounter;
        TipRangeId m_TipRangeId;
    };
}

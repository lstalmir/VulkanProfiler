// Copyright (c) 2024 Lukasz Stalmirski
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

#include "profiler_self_time.h"
#include <utility>

namespace Profiler
{
    DeviceProfilerSelfTime::DeviceProfilerSelfTime()
        : m_FunctionTimeStack()
        , m_FunctionStats()
        , m_FrameTime( 0 )
        , m_TotalTime( 0 )
    {}

    void DeviceProfilerSelfTime::Reset()
    {
        for( auto& [_, stats] : m_FunctionStats )
        {
            stats.m_TotalCount += std::exchange( stats.m_FrameCount, 0 );
            stats.m_TotalTime += std::exchange( stats.m_FrameTime, 0 );
        }
        m_TotalTime += std::exchange( m_FrameTime, 0 );
    }

    void DeviceProfilerSelfTime::Begin( const char* pFunctionName )
    {
        CurrentFunctionTime& functionTime = m_FunctionTimeStack.emplace();
        functionTime.m_pFunctionName = pFunctionName;
        functionTime.m_BeginTime = Clock::now();
    }

    void DeviceProfilerSelfTime::Begin( NextLayerCall )
    {
        CurrentFunctionTime& externalTime = m_FunctionTimeStack.emplace();
        externalTime.m_pFunctionName = nullptr;
        externalTime.m_BeginTime = Clock::now();
    }

    void DeviceProfilerSelfTime::End()
    {
        TimePoint endTime = Clock::now();

        CurrentFunctionTime functionTime = m_FunctionTimeStack.top();
        m_FunctionTimeStack.pop();

        uint64_t time = std::chrono::nanoseconds( endTime - functionTime.m_BeginTime ).count();

        if( functionTime.m_pFunctionName )
        {
            // Accumulate the function time.
            FunctionStats& functionStats = m_FunctionStats[ functionTime.m_pFunctionName ];
            functionStats.m_FrameCount++;
            functionStats.m_FrameTime += time;

            if( m_FunctionTimeStack.empty() )
            {
                // Accumulate the frame time if this was a top-level function.
                m_FrameTime += time;
            }
        }
        else
        {
            // Subtract next layer time from the function time.
            CurrentFunctionTime& currentFunctionTime = m_FunctionTimeStack.top();
            currentFunctionTime.m_BeginTime += std::chrono::nanoseconds( time );
        }
    }

    const DeviceProfilerSelfTime::FunctionStatsMap& DeviceProfilerSelfTime::GetFunctionStats() const
    {
        return m_FunctionStats;
    }

    uint64_t DeviceProfilerSelfTime::GetFrameTime() const
    {
        return m_FrameTime;
    }

    uint64_t DeviceProfilerSelfTime::GetTotalTime() const
    {
        return m_TotalTime;
    }

    DeviceProfilerSelfTimeGuard::DeviceProfilerSelfTimeGuard( DeviceProfilerSelfTime& selfTime, const char* pFunctionName )
        : m_SelfTime( selfTime )
    {
        m_SelfTime.Begin( pFunctionName );
    }

    DeviceProfilerSelfTimeGuard::DeviceProfilerSelfTimeGuard( DeviceProfilerSelfTime& selfTime, DeviceProfilerSelfTime::NextLayerCall )
        : m_SelfTime( selfTime )
    {
        m_SelfTime.Begin( DeviceProfilerSelfTime::NextLayer );
    }

    DeviceProfilerSelfTimeGuard::~DeviceProfilerSelfTimeGuard()
    {
        m_SelfTime.End();
    }
}

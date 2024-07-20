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

#pragma once
#include <chrono>
#include <stack>
#include <unordered_map>

namespace Profiler
{
    /***********************************************************************************\

    Class:
        DeviceProfilerSelfTime

    Description:
        Counts time spent in the profiler.

    \***********************************************************************************/
    class DeviceProfilerSelfTime
    {
    public:
        inline static constexpr struct NextLayerCall {} NextLayer;

        struct FunctionStats
        {
            uint64_t m_FrameCount;
            uint64_t m_FrameTime;
            uint64_t m_TotalCount;
            uint64_t m_TotalTime;
        };

        typedef std::unordered_map<const char*, FunctionStats> FunctionStatsMap;

    public:
        DeviceProfilerSelfTime();

        void Reset();
        void Begin( const char* pFunctionName );
        void Begin( NextLayerCall );
        void End();

        const FunctionStatsMap& GetFunctionStats() const;
        uint64_t GetFrameTime() const;
        uint64_t GetTotalTime() const;

    private:
        typedef std::chrono::high_resolution_clock Clock;
        typedef typename Clock::time_point TimePoint;

        struct CurrentFunctionTime
        {
            const char*                 m_pFunctionName;
            TimePoint                   m_BeginTime;
        };

        std::stack<CurrentFunctionTime> m_FunctionTimeStack;
        FunctionStatsMap                m_FunctionStats;

        uint64_t                        m_FrameTime;
        uint64_t                        m_TotalTime;
    };

    /***********************************************************************************\

    Class:
        DeviceProfilerSelfTimeGuard

    Description:
        Starts the instrumentation of profiler on construction, and stops it when the
        object is destroyed.

    \***********************************************************************************/
    class DeviceProfilerSelfTimeGuard
    {
    public:
        DeviceProfilerSelfTimeGuard( DeviceProfilerSelfTime& selfTime, const char* pFunctionName );
        DeviceProfilerSelfTimeGuard( DeviceProfilerSelfTime& selfTime, DeviceProfilerSelfTime::NextLayerCall );
        ~DeviceProfilerSelfTimeGuard();

        DeviceProfilerSelfTimeGuard( const DeviceProfilerSelfTimeGuard& ) = delete;
        DeviceProfilerSelfTimeGuard& operator=( const DeviceProfilerSelfTimeGuard& ) = delete;

        DeviceProfilerSelfTimeGuard( DeviceProfilerSelfTimeGuard&& ) = delete;
        DeviceProfilerSelfTimeGuard& operator=( DeviceProfilerSelfTimeGuard&& ) = delete;

    private:
        DeviceProfilerSelfTime& m_SelfTime;
    };
}

#define PROFILER_SELF_TIME_NAME(line) \
    profiler_self_time_ ## line

#define PROFILER_SELF_TIME(pDevice) \
    Profiler::DeviceProfilerSelfTimeGuard PROFILER_SELF_TIME_NAME(__LINE__) ((pDevice)->m_ProfilerSelfTime, __FUNCTION__)

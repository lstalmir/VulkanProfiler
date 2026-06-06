// Copyright (c) 2019-2025 Lukasz Stalmirski
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

#include "profiler_trace_event.h"
#include "profiler/profiler_helpers.h"

namespace Profiler
{
    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize TraceEvent to JSON object.

    \*************************************************************************/
    void TraceEvent::Serialize( DeviceProfilerJsonBuilder& builder ) const
    {
        using namespace std::literals;

        if( !m_Name.empty() )
        {
            builder.AddKeyValue( "name", m_Name );
        }

        if( !m_Category.empty() )
        {
            builder.AddKeyValue( "cat", m_Category );
        }

        const char phase[] = { static_cast<char>( m_Phase ), '\0' };
        builder.AddKeyValue( "ph", std::string_view( phase ) );
        builder.AddKeyValue( "ts", m_Timestamp.count() );
        builder.AddKeyValue( "pid", 0 );

        if( m_Queue != VK_NULL_HANDLE )
        {
            char queueHexHandle[32] = {};
            ProfilerStringFunctions::Hex( queueHexHandle, reinterpret_cast<uint64_t>( m_Queue ) );

            builder.AddKeyValue( "tid", "VkQueue 0x"s + queueHexHandle );
        }

        if( m_Color )
        {
            builder.BeginKeyValue( "cname" );
            m_Color( builder );
            builder.EndKeyValue();
        }

        if( m_Args )
        {
            builder.BeginKeyValue( "args" );
            m_Args( builder );
            builder.EndKeyValue();
        }
    }

    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize TraceInstantEvent to JSON object.

    \*************************************************************************/
    void TraceInstantEvent::Serialize( DeviceProfilerJsonBuilder& builder ) const
    {
        TraceEvent::Serialize( builder );

        // Instant events contain additional 's' parameter
        const char scope[] = { static_cast<char>(m_Scope), '\0' };
        builder.AddKeyValue( "s", std::string_view( scope ) );
    }

    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize TraceAsyncEvent to JSON object.

    \*************************************************************************/
    void TraceAsyncEvent::Serialize( DeviceProfilerJsonBuilder& builder ) const
    {
        TraceEvent::Serialize( builder );

        // Async events contain additional 'id' parameter
        builder.AddKeyValue( "id", m_Id );
    }

    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize TraceCompleteEvent to JSON object.

    \*************************************************************************/
    void TraceCompleteEvent::Serialize( DeviceProfilerJsonBuilder& builder ) const
    {
        TraceEvent::Serialize( builder );

        // Complete events contain additional 'dur' parameter
        builder.AddKeyValue( "dur", m_Duration.count() );
    }

    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize TraceCounterEvent to JSON object.

    \*************************************************************************/
    void TraceCounterEvent::Serialize( DeviceProfilerJsonBuilder& builder ) const
    {
        TraceEvent::Serialize( builder );

        // Counter events contain all metrics in 'args' parameter
        builder.BeginArray( "args" );

        for( uint32_t i = 0; i < m_CounterCount; ++i )
        {
            const VkProfilerPerformanceCounterProperties2EXT& properties = m_pCounterProperties[i];
            const VkProfilerPerformanceCounterResultEXT result = m_pCounterResults ? m_pCounterResults[i] : VkProfilerPerformanceCounterResultEXT();

            switch( properties.storage )
            {
            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_INT32_EXT:
                builder.AddKeyValue( properties.shortName, result.int32 );
                break;
            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_UINT32_EXT:
                builder.AddKeyValue( properties.shortName, result.uint32 );
                break;
            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_INT64_EXT:
                builder.AddKeyValue( properties.shortName, result.int64 );
                break;
            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_UINT64_EXT:
                builder.AddKeyValue( properties.shortName, result.uint64 );
                break;
            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_FLOAT32_EXT:
                builder.AddKeyValue( properties.shortName, result.float32 );
                break;
            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_FLOAT64_EXT:
                builder.AddKeyValue( properties.shortName, result.float64 );
                break;
            }
        }

        builder.EndArray();
    }

    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize DebugTraceEvent to JSON object.

    \*************************************************************************/
    void DebugTraceEvent::Serialize( DeviceProfilerJsonBuilder& builder ) const
    {
        TraceEvent::Serialize( builder );

        // Set thread id
        builder.AddKeyValue( "tid", "Debug labels" );

        if( m_Phase == Phase::eInstant )
        {
            const char scope[] = { static_cast<char>( TraceInstantEvent::Scope::eThread ), '\0' };
            builder.AddKeyValue( "s", std::string_view( scope ) );
        }
    }

    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize ApiTraceEvent to JSON object.

    \*************************************************************************/
    void ApiTraceEvent::Serialize( DeviceProfilerJsonBuilder& builder ) const
    {
        TraceEvent::Serialize( builder );

        // Set thread id
        builder.AddKeyValue( "tid", "Thread " + std::to_string( m_ThreadId ) );
    }
}

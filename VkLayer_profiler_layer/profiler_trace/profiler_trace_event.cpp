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
    void TraceEvent::Serialize( simdjson::builder::string_builder& builder ) const
    {
        bool firstArg = true;

        using namespace std::literals;

        if( !m_Name.empty() )
        {
            builder.append_key_value( "name", m_Name );
            firstArg = false;
        }

        if( !m_Category.empty() )
        {
            if( !firstArg ) builder.append_comma();
            builder.append_key_value( "cat", m_Category );
            firstArg = false;
        }

        if( !firstArg ) builder.append_comma();
        builder.append_key_value( "ph", std::string( 1, static_cast<char>( m_Phase ) ) );
        builder.append_comma();
        builder.append_key_value( "ts", m_Timestamp.count() );
        builder.append_comma();
        builder.append_key_value( "pid", 0 );
        firstArg = false;

        if( m_Queue != VK_NULL_HANDLE )
        {
            char queueHexHandle[32] = {};
            ProfilerStringFunctions::Hex( queueHexHandle, reinterpret_cast<uint64_t>( m_Queue ) );

            if( !firstArg ) builder.append_comma();
            builder.append_key_value( "tid", "VkQueue 0x"s + queueHexHandle );
            firstArg = false;
        }

        if( m_Color )
        {
            if( !firstArg ) builder.append_comma();
            builder.escape_and_append_with_quotes( "cname" );
            builder.append_colon();
            m_Color( builder );
            firstArg = false;
        }

        if( m_Args )
        {
            if( !firstArg ) builder.append_comma();
            builder.escape_and_append_with_quotes( "args" );
            builder.append_colon();
            m_Args( builder );
            firstArg = false;
        }
    }

    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize TraceInstantEvent to JSON object.

    \*************************************************************************/
    void TraceInstantEvent::Serialize( simdjson::builder::string_builder& builder ) const
    {
        TraceEvent::Serialize( builder );

        // Instant events contain additional 's' parameter
        builder.append_comma();
        builder.append_key_value( "s", std::string( 1, static_cast<char>(m_Scope)) );
    }

    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize TraceAsyncEvent to JSON object.

    \*************************************************************************/
    void TraceAsyncEvent::Serialize( simdjson::builder::string_builder& builder ) const
    {
        TraceEvent::Serialize( builder );

        // Async events contain additional 'id' parameter
        builder.append_comma();
        builder.append_key_value( "id", m_Id );
    }

    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize TraceCompleteEvent to JSON object.

    \*************************************************************************/
    void TraceCompleteEvent::Serialize( simdjson::builder::string_builder& builder ) const
    {
        TraceEvent::Serialize( builder );

        // Complete events contain additional 'dur' parameter
        builder.append_comma();
        builder.append_key_value( "dur", m_Duration.count() );
    }

    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize TraceCounterEvent to JSON object.

    \*************************************************************************/
    void TraceCounterEvent::Serialize( simdjson::builder::string_builder& builder ) const
    {
        TraceEvent::Serialize( builder );

        // Counter events contain all metrics in 'args' parameter
        builder.append_comma();
        builder.escape_and_append_with_quotes( "args" );
        builder.append_colon();

        builder.start_array();

        for( uint32_t i = 0; i < m_CounterCount; ++i )
        {
            const VkProfilerPerformanceCounterProperties2EXT& properties = m_pCounterProperties[i];
            const VkProfilerPerformanceCounterResultEXT result = m_pCounterResults ? m_pCounterResults[i] : VkProfilerPerformanceCounterResultEXT();

            if( i > 0 ) builder.append_comma();
            switch( properties.storage )
            {
            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_INT32_EXT:
                builder.append_key_value( properties.shortName, result.int32 );
                break;
            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_UINT32_EXT:
                builder.append_key_value( properties.shortName, result.uint32 );
                break;
            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_INT64_EXT:
                builder.append_key_value( properties.shortName, result.int64 );
                break;
            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_UINT64_EXT:
                builder.append_key_value( properties.shortName, result.uint64 );
                break;
            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_FLOAT32_EXT:
                builder.append_key_value( properties.shortName, result.float32 );
                break;
            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_FLOAT64_EXT:
                builder.append_key_value( properties.shortName, result.float64 );
                break;
            }
        }

        builder.end_array();
    }

    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize DebugTraceEvent to JSON object.

    \*************************************************************************/
    void DebugTraceEvent::Serialize( simdjson::builder::string_builder& builder ) const
    {
        TraceEvent::Serialize( builder );

        // Set thread id
        builder.append_comma();
        builder.append_key_value( "tid", "Debug labels" );

        if( m_Phase == Phase::eInstant )
        {
            builder.append_comma();
            builder.append_key_value( "s", std::string( 1, static_cast<char>(TraceInstantEvent::Scope::eThread) ) );
        }
    }

    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize ApiTraceEvent to JSON object.

    \*************************************************************************/
    void ApiTraceEvent::Serialize( simdjson::builder::string_builder& builder ) const
    {
        TraceEvent::Serialize( builder );

        // Set thread id
        builder.append_comma();
        builder.append_key_value( "tid", "Thread " + std::to_string( m_ThreadId ) );
    }
}

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

#include <string>

namespace Profiler
{
    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize TraceEvent to JSON object.

    \*************************************************************************/
    void TraceEvent::Serialize( yyjson_mut_doc* pDocument, yyjson_mut_val* pValue ) const
    {
        using namespace std::literals;

        if( !m_Name.empty() )
        {
            yyjson_mut_obj_add_strcpy( pDocument, pValue, "name", m_Name.c_str() );
        }

        if( !m_Category.empty() )
        {
            yyjson_mut_obj_add_strcpy( pDocument, pValue, "cat", m_Category.c_str() );
        }

        yyjson_mut_obj_add_strcpy( pDocument, pValue, "ph", std::string( 1, static_cast<char>( m_Phase ) ).c_str() );
        yyjson_mut_obj_add_real( pDocument, pValue, "ts", m_Timestamp.count() );
        yyjson_mut_obj_add_uint( pDocument, pValue, "pid", 0 );

        if( m_Queue != VK_NULL_HANDLE )
        {
            char queueHexHandle[32] = {};
            ProfilerStringFunctions::Hex( queueHexHandle, reinterpret_cast<uint64_t>( m_Queue ) );

            yyjson_mut_obj_add_strcpy( pDocument, pValue, "tid", ( "VkQueue 0x"s + queueHexHandle ).c_str() );
        }

        if( m_pColor )
        {
            yyjson_mut_obj_add_val( pDocument, pValue, "cname", m_pColor );
        }

        if( m_pArgs )
        {
            yyjson_mut_obj_add_val( pDocument, pValue, "args", m_pArgs );
        }
    }

    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize TraceInstantEvent to JSON object.

    \*************************************************************************/
    void TraceInstantEvent::Serialize( yyjson_mut_doc* pDocument, yyjson_mut_val* pValue ) const
    {
        TraceEvent::Serialize( pDocument, pValue );

        // Instant events contain additional 's' parameter
        yyjson_mut_obj_add_strcpy( pDocument, pValue, "s", std::string( 1, static_cast<char>( m_Scope ) ).c_str() );
    }

    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize TraceAsyncEvent to JSON object.

    \*************************************************************************/
    void TraceAsyncEvent::Serialize( yyjson_mut_doc* pDocument, yyjson_mut_val* pValue ) const
    {
        TraceEvent::Serialize( pDocument, pValue );

        // Async events contain additional 'id' parameter
        yyjson_mut_obj_add_uint( pDocument, pValue, "id", m_Id );
    }

    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize TraceCompleteEvent to JSON object.

    \*************************************************************************/
    void TraceCompleteEvent::Serialize( yyjson_mut_doc* pDocument, yyjson_mut_val* pValue ) const
    {
        TraceEvent::Serialize( pDocument, pValue );

        // Complete events contain additional 'dur' parameter
        yyjson_mut_obj_add_real( pDocument, pValue, "dur", m_Duration.count() );
    }

    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize TraceCounterEvent to JSON object.

    \*************************************************************************/
    void TraceCounterEvent::Serialize( yyjson_mut_doc* pDocument, yyjson_mut_val* pValue ) const
    {
        TraceEvent::Serialize( pDocument, pValue );

        // Counter events contain all metrics in 'args' parameter
        yyjson_mut_val* pArgs = yyjson_mut_obj_add_obj( pDocument, pValue, "args" );

        for( uint32_t i = 0; i < m_CounterCount; ++i )
        {
            const VkProfilerPerformanceCounterProperties2EXT& properties = m_pCounterProperties[i];
            const VkProfilerPerformanceCounterResultEXT result = m_pCounterResults ? m_pCounterResults[i] : VkProfilerPerformanceCounterResultEXT();

            switch( properties.storage )
            {
            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_INT32_EXT:
                yyjson_mut_obj_add_int( pDocument, pArgs, properties.shortName, result.int32 );
                break;
            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_UINT32_EXT:
                yyjson_mut_obj_add_uint( pDocument, pArgs, properties.shortName, result.uint32 );
                break;
            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_INT64_EXT:
                yyjson_mut_obj_add_int( pDocument, pArgs, properties.shortName, result.int64 );
                break;
            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_UINT64_EXT:
                yyjson_mut_obj_add_uint( pDocument, pArgs, properties.shortName, result.uint64 );
                break;
            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_FLOAT32_EXT:
                yyjson_mut_obj_add_real( pDocument, pArgs, properties.shortName, result.float32 );
                break;
            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_FLOAT64_EXT:
                yyjson_mut_obj_add_real( pDocument, pArgs, properties.shortName, result.float64 );
                break;
            }
        }
    }

    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize DebugTraceEvent to JSON object.

    \*************************************************************************/
    void DebugTraceEvent::Serialize( yyjson_mut_doc* pDocument, yyjson_mut_val* pValue ) const
    {
        TraceEvent::Serialize( pDocument, pValue );

        // Set thread id
        yyjson_mut_obj_add_str( pDocument, pValue, "tid", "Debug labels" );

        if( m_Phase == Phase::eInstant )
        {
            yyjson_mut_obj_add_strcpy( pDocument, pValue, "s", std::string( 1, static_cast<char>( TraceInstantEvent::Scope::eThread ) ).c_str() );
        }
    }

    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize ApiTraceEvent to JSON object.

    \*************************************************************************/
    void ApiTraceEvent::Serialize( yyjson_mut_doc* pDocument, yyjson_mut_val* pValue ) const
    {
        TraceEvent::Serialize( pDocument, pValue );

        // Set thread id
        yyjson_mut_obj_add_strcpy( pDocument, pValue, "tid", ( "Thread " + std::to_string( m_ThreadId ) ).c_str() );
    }
}

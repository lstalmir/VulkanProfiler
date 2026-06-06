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

#include <inttypes.h>

namespace Profiler
{
    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize TraceEvent to JSON object.

    \*************************************************************************/
    void TraceEvent::Serialize( FILE* pFile ) const
    {
        bool sep = false;

        if( !m_Name.empty() )
        {
            fprintf( pFile, "\"name\":\"%s\"", m_Name.c_str() );
            sep = true;
        }

        if( !m_Category.empty() )
        {
            if( sep ) fputc( ',', pFile );
            fprintf( pFile, "\"cat\":\"%s\"", m_Category.c_str() );
            sep = true;
        }

        if( sep ) fputc( ',', pFile );
        fprintf( pFile,
            "\"ph\":\"%c\","
            "\"ts\":%f,"
            "\"pid\":0",
            static_cast<char>( m_Phase ),
            m_Timestamp.count() );
        sep = true;

        if( m_Queue != VK_NULL_HANDLE )
        {
            char queueHexHandle[32] = {};
            ProfilerStringFunctions::Hex( queueHexHandle, reinterpret_cast<uint64_t>( m_Queue ) );

            if( sep ) fputc( ',', pFile );
            fprintf( pFile,
                "\"tid\":\"VkQueue 0x%s\"", queueHexHandle );
            sep = true;
        }

        if( m_ColorCallback )
        {
            if( sep ) fputc( ',', pFile );
            fputs( "\"cname\":", pFile );
            m_ColorCallback( pFile );
            sep = true;
        }

        if( m_ArgsCallback )
        {
            if( sep ) fputc( ',', pFile );
            fputs( "\"args\":", pFile );
            m_ArgsCallback( pFile );
            sep = true;
        }
    }

    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize TraceInstantEvent to JSON object.

    \*************************************************************************/
    void TraceInstantEvent::Serialize( FILE* pFile ) const
    {
        TraceEvent::Serialize( pFile );

        // Instant events contain additional 's' parameter
        fprintf( pFile, ",\"s\":\"%c\"", static_cast<char>(m_Scope) );
    }

    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize TraceAsyncEvent to JSON object.

    \*************************************************************************/
    void TraceAsyncEvent::Serialize( FILE* pFile ) const
    {
        TraceEvent::Serialize( pFile );

        // Async events contain additional 'id' parameter
        fprintf( pFile, ",\"id\":%" PRIu64, m_Id );
    }

    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize TraceCompleteEvent to JSON object.

    \*************************************************************************/
    void TraceCompleteEvent::Serialize( FILE* pFile ) const
    {
        TraceEvent::Serialize( pFile );

        // Complete events contain additional 'dur' parameter
        fprintf( pFile, ",\"dur\":%f", m_Duration.count() );
    }

    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize TraceCounterEvent to JSON object.

    \*************************************************************************/
    void TraceCounterEvent::Serialize( FILE* pFile ) const
    {
        TraceEvent::Serialize( pFile );

        // Counter events contain all metrics in 'args' parameter
        fputs( ",\"args\":{", pFile );

        for( uint32_t i = 0; i < m_CounterCount; ++i )
        {
            const VkProfilerPerformanceCounterProperties2EXT& properties = m_pCounterProperties[i];
            const VkProfilerPerformanceCounterResultEXT result = m_pCounterResults ? m_pCounterResults[i] : VkProfilerPerformanceCounterResultEXT();

            switch( properties.storage )
            {
            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_INT32_EXT:
                fprintf( pFile, "\"%s\":%" PRId32, properties.shortName, result.int32 );
                break;
            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_UINT32_EXT:
                fprintf( pFile, "\"%s\":%" PRIu32, properties.shortName, result.uint32 );
                break;
            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_INT64_EXT:
                fprintf( pFile, "\"%s\":%" PRId64, properties.shortName, result.int64 );
                break;
            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_UINT64_EXT:
                fprintf( pFile, "\"%s\":%" PRIu64, properties.shortName, result.uint64 );
                break;
            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_FLOAT32_EXT:
                fprintf( pFile, "\"%s\":%f", properties.shortName, result.float32 );
                break;
            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_FLOAT64_EXT:
                fprintf( pFile, "\"%s\":%lf", properties.shortName, result.float64 );
                break;
            }
        }

        fputc( '}', pFile );
    }

    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize DebugTraceEvent to JSON object.

    \*************************************************************************/
    void DebugTraceEvent::Serialize( FILE* pFile ) const
    {
        TraceEvent::Serialize( pFile );

        // Set thread id
        fputs( ",\"tid\":\"Debug labels\"", pFile );

        if( m_Phase == Phase::eInstant )
        {
            fprintf( pFile, ",\"s\":\"%c\"", static_cast<char>( TraceInstantEvent::Scope::eThread ) );
        }
    }

    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize ApiTraceEvent to JSON object.

    \*************************************************************************/
    void ApiTraceEvent::Serialize( FILE* pFile ) const
    {
        TraceEvent::Serialize( pFile );

        // Set thread id
        fprintf( pFile, ",\"tid\":\"Thread %u\"", m_ThreadId );
    }
}

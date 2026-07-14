// Copyright (c) 2019-2026 Lukasz Stalmirski
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
#include "profiler_layer_objects/VkObject.h"
#include <fmt/format.h>

namespace Profiler
{
    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize TraceEvent to JSON object.

    \*************************************************************************/
    void TraceEvent::Serialize( DeviceProfilerJsonObjectBuilder& builder ) const
    {
        builder.Add( "name", m_Name );
        builder.Add( "cat", m_Category );
        builder.Add( "ph", static_cast<char>( m_Phase ) );
        builder.Add( "ts", m_Timestamp.count() );
        builder.Add( "pid", "0" );
        builder.Add( "tid", GetTrackName() );

        if( m_Color )
        {
            auto cnameBuilder = builder.Add( "cname" );
            m_Color( cnameBuilder );
        }

        if( m_Args )
        {
            auto argsBuilder = builder.Add( "args" );
            m_Args( argsBuilder );
        }
    }

    /*************************************************************************\

    Function:
        GetTrackName

    Description:
        Return track name for host trace events.

    \*************************************************************************/
    std::string TraceHostEvent::GetTrackName() const
    {
        return "CPU Thread " + std::to_string( m_ThreadId );
    }

    /*************************************************************************\

    Function:
        GetTrackName

    Description:
        Return track name for host trace events.

    \*************************************************************************/
    std::string TraceDeviceEvent::GetTrackName() const
    {
        if( m_Queue != VK_NULL_HANDLE )
        {
            return fmt::format( "GPU Queue 0x{:016x} [{}]",
                VkObjectTraits<VkQueue>::GetObjectHandleAsUint64( m_Queue ),
                m_Track );
        }

        return fmt::format( "GPU [{}]", m_Track );
    }

    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize TraceCounterEvent to JSON object.

    \*************************************************************************/
    void TraceCounterEvent::Serialize( DeviceProfilerJsonObjectBuilder& builder ) const
    {
        TraceEvent::Serialize( builder );

        // Counter events contain all metrics in 'args' parameter
        auto args = builder.AddObject( "args" );
        for( uint32_t i = 0; i < m_CounterCount; ++i )
        {
            const VkProfilerPerformanceCounterProperties2EXT& properties = m_pCounterProperties[i];
            const VkProfilerPerformanceCounterResultEXT result = m_pCounterResults ? m_pCounterResults[i] : VkProfilerPerformanceCounterResultEXT();

            switch( properties.storage )
            {
            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_INT32_EXT:
                args.Add( properties.shortName, result.int32 );
                break;
            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_UINT32_EXT:
                args.Add( properties.shortName, result.uint32 );
                break;
            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_INT64_EXT:
                args.Add( properties.shortName, result.int64 );
                break;
            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_UINT64_EXT:
                args.Add( properties.shortName, result.uint64 );
                break;
            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_FLOAT32_EXT:
                args.Add( properties.shortName, result.float32 );
                break;
            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_FLOAT64_EXT:
                args.Add( properties.shortName, result.float64 );
                break;
            }
        }
        args.End();
    }

    /*************************************************************************\

    Function:
        GetTrackName

    Description:
        Get track name for counter trace events.

    \*************************************************************************/
    std::string TraceCounterEvent::GetTrackName() const
    {
        return "Counters";
    }

    /*************************************************************************\

    Function:
        GetTrackName

    Description:
        Get track name for debug trace events.

    \*************************************************************************/
    std::string TraceDebugEvent::GetTrackName() const
    {
        return "Debug labels";
    }

    /*************************************************************************\

    Function:
        TraceMetadataEvent

    Description:
        Constructor.

    \*************************************************************************/
    TraceMetadataEvent::TraceMetadataEvent(
        MetadataType type,
        std::string_view track,
        std::string_view stringValue )
        : TraceEvent( Phase::eMetadata, GetEventName( type ), "Metadata", Microseconds( 0 ) )
        , m_Type( type )
        , m_Track( track )
        , m_Args( stringValue )
    {
    }

    /*************************************************************************\

    Function:
        TraceMetadataEvent

    Description:
        Constructor.

    \*************************************************************************/
    TraceMetadataEvent::TraceMetadataEvent(
        MetadataType type,
        std::string_view track,
        uint32_t intValue )
        : TraceEvent( Phase::eMetadata, GetEventName( type ), "Metadata", Microseconds( 0 ) )
        , m_Type( type )
        , m_Track( track )
        , m_Args( intValue )
    {
    }

    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize TraceMetadataEvent to JSON object.

    \*************************************************************************/
    void TraceMetadataEvent::Serialize( DeviceProfilerJsonObjectBuilder& builder ) const
    {
        TraceEvent::Serialize( builder );

        // The metadata is contained in 'args' parameter
        auto args = builder.AddObject( "args" );
        switch( m_Type )
        {
        case MetadataType::eTrackName:
        {
            args.Add( "name", std::get<std::string_view>( m_Args ) );
            break;
        }
        case MetadataType::eTrackSortIndex:
        {
            args.Add( "sort_index", std::get<uint32_t>( m_Args ) );
            break;
        }
        }
    }

    /*************************************************************************\

    Function:
        GetTrackName

    Description:
        Return track name for metadata trace events.

    \*************************************************************************/
    std::string TraceMetadataEvent::GetTrackName() const
    {
        return std::string( m_Track );
    }

    /*************************************************************************\

    Function:
        GetTrackName

    Description:
        Return track name for metadata trace events.

    \*************************************************************************/
    std::string_view TraceMetadataEvent::GetEventName( MetadataType type )
    {
        switch( type )
        {
        case MetadataType::eTrackName:
            return "thread_name";
        case MetadataType::eTrackSortIndex:
            return "thread_sort_index";
        default:
            return std::string_view();
        }
    }
}

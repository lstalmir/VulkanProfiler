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

#pragma once
#include "profiler/profiler_helpers.h"
#include "profiler_helpers/profiler_json_builder.h"
#include "profiler_helpers/profiler_time_helpers.h"
#include <vulkan/vulkan.h>
#include <functional>
#include <variant>

#include "profiler_ext/VkProfilerEXT.h"

namespace Profiler
{
    /*************************************************************************\

    Structure:
        TraceEvent

    Description:
        Contains data common for all trace event types.

    See:
        https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU

    \*************************************************************************/
    struct TraceEvent
    {
        typedef std::function<void( DeviceProfilerJsonValueBuilder& )> BuildCallback;

        enum class Phase
        {
            eDurationBegin = 'B',
            eDurationEnd = 'E',
            eComplete = 'X',
            eInstant = 'i',
            eCounter = 'C',
            eAsyncStart = 'b',
            eAsyncInstant = 'n',
            eAsyncEnd = 'e',
            eFlowStart = 's',
            eFlowStep = 't',
            eFlowEnd = 'f',
            eSample = 'P',
            eObjectCreated = 'N',
            eObjectSnapshot = 'O',
            eObjectDestroyed = 'D',
            eMetadata = 'M',
            eMemoryDumpGlobal = 'V',
            eMemoryDumpProcess = 'v',
            eMark = 'R',
            eClockSync = 'c',
            eContextBegin = '(',
            eContextEnd = ')'
        };

        Phase            m_Phase;
        std::string_view m_Name;
        std::string_view m_Category;
        Microseconds     m_Timestamp;
        BuildCallback    m_Color;
        BuildCallback    m_Args;

        TraceEvent() = default;

        template<typename TimestampType>
        inline TraceEvent(
            Phase phase,
            std::string_view name,
            std::string_view category,
            TimestampType timestamp,
            BuildCallback color = {},
            BuildCallback args = {} )
            : m_Phase( phase )
            , m_Name( name )
            , m_Category( category )
            , m_Timestamp( std::chrono::duration_cast<decltype( m_Timestamp )>( timestamp ) )
            , m_Color( std::move( color ) )
            , m_Args( std::move( args ) )
        {
        }

        virtual void Serialize( DeviceProfilerJsonObjectBuilder& builder ) const;

        virtual std::string GetTrackName() const = 0;
    };

    /*************************************************************************\

    Structure:
        TraceHostEvent

    Description:
        Base class for all events that are generated on the host (CPU).

    \*************************************************************************/
    struct TraceHostEvent : TraceEvent
    {
        uint64_t m_ThreadId;

        TraceHostEvent() = default;

        template<typename TimestampType>
        inline TraceHostEvent(
            Phase phase,
            uint64_t threadId,
            std::string_view name,
            std::string_view category,
            TimestampType timestamp,
            BuildCallback color = {},
            BuildCallback args = {} )
            : TraceEvent( phase, name, category, timestamp, std::move( color ), std::move( args ) )
            , m_ThreadId( threadId )
        {
        }

        std::string GetTrackName() const override;
    };

    /*************************************************************************\

    Structure:
        TraceDeviceEvent

    Description:
        Base class for all events that are generated on the device (GPU).

    \*************************************************************************/
    struct TraceDeviceEvent : TraceEvent
    {
        VkQueue  m_Queue;
        uint64_t m_Track;

        TraceDeviceEvent() = default;

        template<typename TimestampType>
        inline TraceDeviceEvent(
            Phase phase,
            VkQueue queue,
            uint64_t track,
            std::string_view name,
            std::string_view category,
            TimestampType timestamp,
            BuildCallback color = {},
            BuildCallback args = {} )
            : TraceEvent( phase, name, category, timestamp, std::move( color ), std::move( args ) )
            , m_Queue( queue )
            , m_Track( track )
        {
        }

        std::string GetTrackName() const override;
    };

    /*************************************************************************\

    Structure:
        TraceCounterEvent

    Description:
        Counter events are used to report performance counters in the trace.

    See:
        https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU

    \*************************************************************************/
    struct TraceCounterEvent : TraceEvent
    {
        size_t m_CounterCount;
        const VkProfilerPerformanceCounterProperties2EXT* m_pCounterProperties;
        const VkProfilerPerformanceCounterResultEXT* m_pCounterResults;

        TraceCounterEvent() = default;

        template<typename TimestampType>
        inline TraceCounterEvent(
            std::string_view name,
            TimestampType timestamp,
            size_t counterCount,
            const VkProfilerPerformanceCounterProperties2EXT* pCounterProperties,
            const VkProfilerPerformanceCounterResultEXT* pCounterResults,
            BuildCallback color = {} )
            : TraceEvent( Phase::eCounter, name, "Counters", timestamp, std::move( color ) )
            , m_CounterCount( counterCount )
            , m_pCounterProperties( pCounterProperties )
            , m_pCounterResults( pCounterResults )
        {
        }

        void Serialize( DeviceProfilerJsonObjectBuilder& builder ) const override;

        std::string GetTrackName() const override;
    };

    /*************************************************************************\

    Structure:
        DebugTraceEvent

    Description:
        Debug trace events are debug labels inserted into queues and command
        buffers.

    \*************************************************************************/
    struct TraceDebugEvent : TraceEvent
    {
        TraceDebugEvent() = default;

        template<typename TimestampType>
        inline TraceDebugEvent(
            Phase phase,
            std::string_view name,
            TimestampType timestamp,
            BuildCallback color = {},
            BuildCallback args = {} )
            : TraceEvent( phase, name, "Debug", timestamp, std::move( color ), std::move( args ) )
        {
        }

        std::string GetTrackName() const override;
    };

    /*************************************************************************\

    Structure:
        TraceMetadataEvent

    Description:
        Metadata trace events provide additional information about the trace,
        such as labels or annotations, and don't belong to any queue.

    \*************************************************************************/
    struct TraceMetadataEvent : TraceEvent
    {
        enum class MetadataType
        {
            eTrackName,
            eTrackSortIndex
        };

        MetadataType m_Type;
        std::string_view m_Track;
        std::variant<std::string_view, uint32_t> m_Args;

        TraceMetadataEvent( MetadataType type, std::string_view track, std::string_view stringValue );
        TraceMetadataEvent( MetadataType type, std::string_view track, uint32_t intValue );

        void Serialize( DeviceProfilerJsonObjectBuilder& builder ) const override;

        std::string GetTrackName() const override;

        static std::string_view GetEventName( MetadataType type );
    };
}

// Copyright (c) 2020 Lukasz Stalmirski
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
#include "profiler_helpers/profiler_time_helpers.h"
#include <vulkan/vulkan.h>
#include <nlohmann/json.hpp>

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

        Phase          m_Phase;
        std::string    m_Name;
        std::string    m_Category;
        Microseconds   m_Timestamp;
        VkQueue        m_Queue;
        nlohmann::json m_Color;
        nlohmann::json m_Args;

        TraceEvent() = default;

        template<typename TimestampType>
        inline TraceEvent(
            Phase phase,
            std::string_view name,
            std::string_view category,
            TimestampType timestamp,
            VkQueue queue,
            const nlohmann::json& color = {},
            const nlohmann::json& args = {} )
            : m_Phase( phase )
            , m_Name( name )
            , m_Category( category )
            , m_Timestamp( std::chrono::duration_cast<decltype(m_Timestamp)>(timestamp) )
            , m_Queue( queue )
            , m_Color( color )
            , m_Args( args )
        {
        }

        virtual void Serialize( nlohmann::json& j ) const;
    };

    /*************************************************************************\

    Structure:
        TraceInstantEvent

    Description:
        Instant events contain additional 's' field with scope of the event.

    See:
        https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU

    \*************************************************************************/
    struct TraceInstantEvent : TraceEvent
    {
        enum class Scope
        {
            eGlobal = 'g',
            eProcess = 'p',
            eThread = 't',
        };

        Scope m_Scope;

        TraceInstantEvent() = default;

        template<typename TimestampType>
        inline TraceInstantEvent(
            Scope scope,
            std::string_view name,
            std::string_view category,
            TimestampType timestamp,
            VkQueue queue,
            const nlohmann::json& color = {},
            const nlohmann::json& args = {} )
            : TraceEvent( Phase::eInstant, name, category, timestamp, queue, color, args )
            , m_Scope( scope )
        {
        }

        void Serialize( nlohmann::json& j ) const override;
    };

    /*************************************************************************\

    Structure:
        TraceAsyncEvent

    Description:
        Async events contain additional 'id' field with scope of the event.

    See:
        https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU

    \*************************************************************************/
    struct TraceAsyncEvent : TraceEvent
    {
        uint64_t m_Id;

        TraceAsyncEvent() = default;

        template<typename TimestampType>
        inline TraceAsyncEvent(
            Phase phase,
            uint64_t id,
            std::string_view name,
            std::string_view category,
            TimestampType timestamp,
            VkQueue queue,
            const nlohmann::json& color = {},
            const nlohmann::json& args = {} )
            : TraceEvent( phase, name, category, timestamp, queue, color, args )
            , m_Id( id )
        {
            assert( (m_Phase == Phase::eAsyncStart)
                || (m_Phase == Phase::eAsyncEnd)
                || (m_Phase == Phase::eAsyncInstant) );
        }

        void Serialize( nlohmann::json& j ) const override;
    };

    /*************************************************************************\

    Structure:
        TraceCompleteEvent

    Description:
        Complete events contain additional 'dur' field with duration of the event.

    See:
        https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU

    \*************************************************************************/
    struct TraceCompleteEvent : TraceEvent
    {
        Microseconds m_Duration;

        TraceCompleteEvent() = default;

        template<typename TimestampType, typename DurationType>
        inline TraceCompleteEvent(
            std::string_view name,
            std::string_view category,
            TimestampType timestamp,
            DurationType duration,
            VkQueue queue,
            const nlohmann::json& color = {},
            const nlohmann::json& args = {} )
            : TraceEvent( Phase::eComplete, name, category, timestamp, queue, color, args )
            , m_Duration( std::chrono::duration_cast<decltype(m_Duration)>(duration) )
        {
        }

        void Serialize( nlohmann::json& j ) const override;
    };

    /*************************************************************************\

    Structure:
        ApiTraceEvent

    Description:
        API trace events mark events that happen only on CPU and don't belong
        to any queue (even if they submit work to the queue).

    \*************************************************************************/
    struct ApiTraceEvent : TraceInstantEvent
    {
        uint32_t m_ThreadId;

        ApiTraceEvent() = default;

        template<typename TimestampType>
        inline ApiTraceEvent(
            Scope scope,
            std::string_view name,
            uint32_t threadId,
            TimestampType timestamp,
            const nlohmann::json& color = {},
            const nlohmann::json& args = {} )
            : TraceInstantEvent( scope, name, "API", timestamp, VK_NULL_HANDLE, color, args )
            , m_ThreadId( threadId )
        {
        }

        void Serialize( nlohmann::json& j ) const override;
    };

    // Conversion functions
    void to_json( nlohmann::json& j, const TraceEvent& event );
}

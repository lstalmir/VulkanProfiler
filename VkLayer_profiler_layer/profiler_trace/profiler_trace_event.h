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
    enum class TraceEventPhase
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
        TraceEventPhase           m_Phase;
        std::string               m_Name;
        std::string               m_Category;
        Microseconds              m_Timestamp;
        VkQueue                   m_Queue;
        nlohmann::json::object_t  m_Args;

        TraceEvent() = default;

        template<typename TimestampType>
        inline TraceEvent(
            TraceEventPhase phase,
            std::string_view name,
            std::string_view category,
            TimestampType timestamp,
            VkQueue queue,
            nlohmann::json::object_t&& args = {} )
            : m_Phase( phase )
            , m_Name( name )
            , m_Category( category )
            , m_Timestamp( std::chrono::duration_cast<decltype(m_Timestamp)>(timestamp) )
            , m_Queue( queue )
            , m_Args( args )
        {
        }

        virtual void Serialize( nlohmann::json& j ) const;
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
            nlohmann::json::object_t&& args = {} )
            : TraceEvent( TraceEventPhase::eComplete, name, category, timestamp, queue, std::move( args ) )
            , m_Duration( std::chrono::duration_cast<decltype(m_Duration)>(duration) )
        {
        }

        void Serialize( nlohmann::json& j ) const override;
    };

    // Conversion functions
    void to_json( nlohmann::json& j, const TraceEvent& event );
}

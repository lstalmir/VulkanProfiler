// Copyright (c) 2019-2021 Lukasz Stalmirski
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
    void TraceEvent::Serialize( nlohmann::json& jsonObject ) const
    {
        using namespace std::literals;

        char queueHexHandle[ 32 ] = {};
        ProfilerStringFunctions::Hex( queueHexHandle, reinterpret_cast<uint64_t>(m_Queue) );

        jsonObject = {
            { "name", m_Name },
            { "cat", m_Category },
            { "ph", std::string( 1, static_cast<char>(m_Phase) ) },
            { "ts", m_Timestamp.count() },
            { "pid", 0 },
            { "tid", "VkQueue 0x"s + queueHexHandle } };

        if( !m_Color.empty() )
        {
            jsonObject[ "cname" ] = m_Color;
        }

        if( !m_Args.empty() )
        {
            jsonObject[ "args" ] = m_Args;
        }
    }

    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize TraceInstantEvent to JSON object.

    \*************************************************************************/
    void TraceInstantEvent::Serialize( nlohmann::json& jsonObject ) const
    {
        TraceEvent::Serialize( jsonObject );

        // Instant events contain additional 's' parameter
        jsonObject[ "s" ] = std::string( 1, static_cast<char>(m_Scope));
    }

    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize TraceAsyncEvent to JSON object.

    \*************************************************************************/
    void TraceAsyncEvent::Serialize( nlohmann::json& jsonObject ) const
    {
        TraceEvent::Serialize( jsonObject );

        // Async events contain additional 'id' parameter
        jsonObject[ "id" ] = m_Id;
    }

    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize TraceCompleteEvent to JSON object.

    \*************************************************************************/
    void TraceCompleteEvent::Serialize( nlohmann::json& jsonObject ) const
    {
        TraceEvent::Serialize( jsonObject );

        // Complete events contain additional 'dur' parameter
        jsonObject[ "dur" ] = m_Duration.count();
    }

    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize DebugTraceEvent to JSON object.

    \*************************************************************************/
    void DebugTraceEvent::Serialize( nlohmann::json& jsonObject ) const
    {
        TraceEvent::Serialize( jsonObject );

        // Set thread id
        jsonObject[ "tid" ] = "Debug labels";

        if( m_Phase == Phase::eInstant )
        {
            jsonObject[ "s" ] = std::string( 1, static_cast<char>(TraceInstantEvent::Scope::eThread) );
        }
    }

    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize ApiTraceEvent to JSON object.

    \*************************************************************************/
    void ApiTraceEvent::Serialize( nlohmann::json& jsonObject ) const
    {
        TraceInstantEvent::Serialize( jsonObject );

        // Set thread id
        jsonObject[ "tid" ] = "Thread " + std::to_string( m_ThreadId );
    }

    /*************************************************************************\

    Function:
        to_json

    Description:
        Serialize TraceEvent to JSON object.

    \*************************************************************************/
    void to_json( nlohmann::json& jsonObject, const TraceEvent& event )
    {
        event.Serialize( jsonObject );
    }
}

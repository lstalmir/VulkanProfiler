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

#include "profiler_trace_event.h"
#include "profiler/profiler_helpers.h"

namespace Profiler
{
    /*************************************************************************\

    Function:
        to_json

    Description:
        Serialize TraceEvent to JSON object.

    \*************************************************************************/
    void to_json( nlohmann::json& jsonObject, const TraceEvent& event )
    {
        using namespace std::literals;

        char queueHexHandle[ 32 ] = {};
        Profiler::u64tohex( queueHexHandle, reinterpret_cast<uint64_t>(event.m_Queue) );

        jsonObject = {
            { "name", event.m_Name },
            { "cat", event.m_Category },
            { "ph", std::string( 1, static_cast<char>(event.m_Phase) ) },
            { "ts", static_cast<uint64_t>(event.m_Timestamp * 1000000) },
            { "pid", 0 },
            { "tid", "VkQueue 0x"s + queueHexHandle },
            { "args", event.m_Args } };
    }
}

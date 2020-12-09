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

    // Intermediate types
    struct TraceEvent
    {
        std::string              m_Name;
        std::string              m_Category;
        TraceEventPhase          m_Phase;
        double                   m_Timestamp;
        VkQueue                  m_Queue;
        VkCommandBuffer          m_CommandBuffer;
        nlohmann::json::object_t m_Args;
    };

    // Conversion functions
    void to_json( nlohmann::json& j, const TraceEvent& event );
}

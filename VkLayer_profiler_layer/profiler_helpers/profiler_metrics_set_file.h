// Copyright (c) 2025 Lukasz Stalmirski
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
#include "profiler_ext/VkProfilerEXT.h"
#include <stdint.h>
#include <fstream>
#include <string>
#include <vector>

namespace Profiler
{
    class DeviceProfilerFrontend;

    /***********************************************************************************\

    Class:
        DeviceProfilerMetricsSetFile

    Description:
        Serializes performance metrics set into JSON file.

    \***********************************************************************************/
    class DeviceProfilerMetricsSetFile
    {
    public:
        DeviceProfilerMetricsSetFile();

        bool Read( const std::string& filename );
        bool Write( const std::string& filename ) const;

        void SetName( const std::string_view& name );
        void SetDescription( const std::string_view& description );
        void SetCounters( uint32_t count, const VkProfilerPerformanceCounterProperties2EXT* pProperties );

        const std::string& GetName() const;
        const std::string& GetDescription() const;
        std::vector<uint32_t> GetCounters( DeviceProfilerFrontend& frontend ) const;
        uint32_t GetCounterCount() const;

    private:
        std::string m_Name;
        std::string m_Description;
        std::vector<std::string> m_Counters;
    };
}

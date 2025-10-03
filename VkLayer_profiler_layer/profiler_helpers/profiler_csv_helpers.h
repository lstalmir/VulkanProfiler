// Copyright (c) 2024-2024 Lukasz Stalmirski
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
#include <stdint.h>
#include <fstream>
#include <string>
#include <vector>

struct VkProfilerPerformanceCounterProperties2EXT;
union VkProfilerPerformanceCounterResultEXT;

namespace Profiler
{
    /***********************************************************************************\

    Class:
        DeviceProfilerCsvSerializer

    Description:
        Serializes performance counter results into CSV file.

    \***********************************************************************************/
    class DeviceProfilerCsvSerializer
    {
    public:
        DeviceProfilerCsvSerializer();
        ~DeviceProfilerCsvSerializer();

        bool Open( const std::string& filename );
        void Close();

        void WriteHeader( uint32_t count, const VkProfilerPerformanceCounterProperties2EXT* pProperties );
        void WriteRow( uint32_t count, const VkProfilerPerformanceCounterResultEXT* pValues );

    private:
        std::ofstream m_File;
        std::vector<VkProfilerPerformanceCounterProperties2EXT> m_Properties;
    };

    /***********************************************************************************\

    Class:
        DeviceProfilerCsvDeserializer

    Description:
        Deserializes performance counter results from CSV file.

    \***********************************************************************************/
    class DeviceProfilerCsvDeserializer
    {
    public:
        DeviceProfilerCsvDeserializer();
        ~DeviceProfilerCsvDeserializer();

        bool Open( const std::string& filename );
        void Close();

        std::vector<VkProfilerPerformanceCounterProperties2EXT> ReadHeader();
        std::vector<VkProfilerPerformanceCounterResultEXT> ReadRow();

    private:
        std::ifstream m_File;
        std::vector<VkProfilerPerformanceCounterProperties2EXT> m_Properties;
    };
}

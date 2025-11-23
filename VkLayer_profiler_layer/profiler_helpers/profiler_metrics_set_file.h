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
#include <string>
#include <vector>

namespace Profiler
{
    class DeviceProfilerFrontend;

    /***********************************************************************************\

    Class:
        DeviceProfilerMetricsSetFileEntry

    Description:
        Represents a single metrics set entry in the performance metrics set file.

    \***********************************************************************************/
    struct DeviceProfilerMetricsSetFileEntry
    {
    public:
        DeviceProfilerMetricsSetFileEntry();

        void SetName( const std::string_view& name );
        void SetDescription( const std::string_view& description );
        void SetCounters( const std::vector<VkProfilerPerformanceCounterProperties2EXT>& properties );
        void SetCounters( const std::vector<std::string>& counters );
        void SetCounters( std::vector<std::string>&& counters );

        const std::string& GetName() const;
        const std::string& GetDescription() const;
        const std::vector<uint32_t>& GetCounterIndices() const;
        const std::vector<std::string>& GetCounterNames() const;
        uint32_t GetCounterCount() const;

        void ResolveCounterIndices( const std::vector<VkProfilerPerformanceCounterProperties2EXT>& supportedCounters );

    private:
        std::string m_Name;
        std::string m_Description;
        std::vector<std::string> m_Counters;
        std::vector<uint32_t> m_CounterIndices;
    };

    /***********************************************************************************\

    Class:
        DeviceProfilerMetricsSetFile

    Description:
        Represents a performance metrics set file.

        The file may contain one or more metrics set entries, each defining a set of
        performance counters.

    \***********************************************************************************/
    class DeviceProfilerMetricsSetFile
    {
    public:
        DeviceProfilerMetricsSetFile();

        bool Read( const std::string& filename );
        bool Write( const std::string& filename ) const;

        void AddEntry( const DeviceProfilerMetricsSetFileEntry& entry );
        void RemoveEntry( uint32_t index );
        void RemoveAllEntries();

        uint32_t GetEntryCount() const;
        const DeviceProfilerMetricsSetFileEntry& GetEntry( uint32_t index ) const;

        void ResolveCounterIndices( DeviceProfilerFrontend& frontend );

    private:
        std::vector<DeviceProfilerMetricsSetFileEntry> m_Entries;
    };
}

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

#include "profiler_metrics_set_file.h"
#include "profiler/profiler_frontend.h"

#include <assert.h>
#include <fstream>
#include <nlohmann/json.hpp>

namespace Profiler
{
    /***********************************************************************************\

    Function:
        DeviceProfilerMetricsSetFileEntry

    Description:
        Constructor.

    \***********************************************************************************/
    DeviceProfilerMetricsSetFileEntry::DeviceProfilerMetricsSetFileEntry()
        : m_Name()
        , m_Description()
        , m_Counters()
    {
    }

    /***********************************************************************************\

    Function:
        SetName

    Description:
        Set metrics set name.

    \***********************************************************************************/
    void DeviceProfilerMetricsSetFileEntry::SetName( const std::string_view& name )
    {
        m_Name = name;
    }

    /***********************************************************************************\

    Function:
        SetDescription

    Description:
        Set metrics set description.

    \***********************************************************************************/
    void DeviceProfilerMetricsSetFileEntry::SetDescription( const std::string_view& description )
    {
        m_Description = description;
    }
    
    /***********************************************************************************\

    Function:
        SetCounters

    Description:
        Set metrics set counters.

    \***********************************************************************************/
    void DeviceProfilerMetricsSetFileEntry::SetCounters( const std::vector<VkProfilerPerformanceCounterProperties2EXT>& properties )
    {
        m_Counters.clear();
        m_Counters.reserve( properties.size() );

        for( const VkProfilerPerformanceCounterProperties2EXT& property : properties )
        {
            m_Counters.push_back( property.shortName );
        }
    }

    /***********************************************************************************\

    Function:
        SetCounters

    Description:
        Set metrics set counters.

    \***********************************************************************************/
    void DeviceProfilerMetricsSetFileEntry::SetCounters( const std::vector<std::string>& counters )
    {
        m_Counters = counters;
    }

    /***********************************************************************************\

    Function:
        SetCounters

    Description:
        Set metrics set counters.

    \***********************************************************************************/
    void DeviceProfilerMetricsSetFileEntry::SetCounters( std::vector<std::string>&& counters )
    {
        m_Counters = std::move( counters );
    }

    /***********************************************************************************\

    Function:
        GetName

    Description:
        Get metrics set name.

    \***********************************************************************************/
    const std::string& DeviceProfilerMetricsSetFileEntry::GetName() const
    {
        return m_Name;
    }

    /***********************************************************************************\

    Function:
        GetDescription

    Description:
        Get metrics set description.

    \***********************************************************************************/
    const std::string& DeviceProfilerMetricsSetFileEntry::GetDescription() const
    {
        return m_Description;
    }

    /***********************************************************************************\

    Function:
        GetCounterCount

    Description:
        Get number of saved counters.

    \***********************************************************************************/
    uint32_t DeviceProfilerMetricsSetFileEntry::GetCounterCount() const
    {
        return static_cast<uint32_t>( m_Counters.size() );
    }

    /***********************************************************************************\

    Function:
        GetCounterIndices

    Description:
        Get resolved metrics set counter indices.

    \***********************************************************************************/
    const std::vector<uint32_t>& DeviceProfilerMetricsSetFileEntry::GetCounterIndices() const
    {
        return m_CounterIndices;
    }

    /***********************************************************************************\

    Function:
        GetCounterNames

    Description:
        Get metrics set counter names.

    \***********************************************************************************/
    const std::vector<std::string>& DeviceProfilerMetricsSetFileEntry::GetCounterNames() const
    {
        return m_Counters;
    }

    /***********************************************************************************\

    Function:
        ResolveCounterIndices

    Description:
        Resolve counter indices based on supported counters.

    \***********************************************************************************/
    void DeviceProfilerMetricsSetFileEntry::ResolveCounterIndices( const std::vector<VkProfilerPerformanceCounterProperties2EXT>& supportedCounters )
    {
        m_CounterIndices.clear();
        m_CounterIndices.reserve( m_Counters.size() );

        for( const auto& counterName : m_Counters )
        {
            for( size_t i = 0; i < supportedCounters.size(); i++ )
            {
                if( counterName == supportedCounters[i].shortName )
                {
                    m_CounterIndices.push_back( static_cast<uint32_t>( i ) );
                    break;
                }
            }
        }
    }

    /***********************************************************************************\

    Function:
        DeviceProfilerMetricsSetFile

    Description:
        Constructor.

    \***********************************************************************************/
    DeviceProfilerMetricsSetFile::DeviceProfilerMetricsSetFile()
        : m_Entries()
    {
    }

    /***********************************************************************************\

    Function:
        Read

    Description:
        Read metrics set from JSON file.

    \***********************************************************************************/
    bool DeviceProfilerMetricsSetFile::Read( const std::string& filename )
    {
        try
        {
            std::ifstream file( filename );
            nlohmann::json json = nlohmann::json::parse( file );

            auto ReadEntry = [this]( const nlohmann::json& json ) -> bool
            {
                try
                {
                    DeviceProfilerMetricsSetFileEntry entry;
                    entry.SetName( json.at( "name" ).get<std::string>() );
                    entry.SetDescription( json.at( "description" ).get<std::string>() );
                    entry.SetCounters( json.at( "counters" ).get<std::vector<std::string>>() );
                    m_Entries.push_back( std::move( entry ) );
                    return true;
                }
                catch( ... )
                {
                    return false;
                }
            };

            if( json.is_object() )
            {
                // Single-set file.
                return ReadEntry( json );
            }

            if( json.is_array() )
            {
                // Collection of multiple metrics sets.
                for( const nlohmann::json& entry : json )
                {
                    if( !ReadEntry( entry ) )
                    {
                        return false;
                    }
                }

                return true;
            }

            return false;
        }
        catch( ... )
        {
            return false;
        }
    }

    /***********************************************************************************\

    Function:
        Write

    Description:
        Write metrics set to JSON file.

    \***********************************************************************************/
    bool DeviceProfilerMetricsSetFile::Write( const std::string& filename ) const
    {
        try
        {
            nlohmann::json json = nlohmann::json::array();

            for( const DeviceProfilerMetricsSetFileEntry& entry : m_Entries )
            {
                nlohmann::json jsonEntry;
                jsonEntry["name"] = entry.GetName();
                jsonEntry["description"] = entry.GetDescription();
                jsonEntry["counters"] = entry.GetCounterNames();

                json.push_back( jsonEntry );
            }

            std::ofstream file( filename );
            file << std::setw( 4 ) << json;

            return true;
        }
        catch( ... )
        {
            return false;
        }
    }

    /***********************************************************************************\

    Function:
        AddEntry

    Description:
        Add a new entry to the metrics set library.

    \***********************************************************************************/
    void DeviceProfilerMetricsSetFile::AddEntry( const DeviceProfilerMetricsSetFileEntry& entry )
    {
        m_Entries.push_back( entry );
    }

    /***********************************************************************************\

    Function:
        RemoveEntry

    Description:
        Remove an entry from the metrics set library.

    \***********************************************************************************/
    void DeviceProfilerMetricsSetFile::RemoveEntry( uint32_t index )
    {
        assert( index < m_Entries.size() );
        if( index < m_Entries.size() )
        {
            m_Entries.erase( m_Entries.begin() + index );
        }
    }

    /***********************************************************************************\

    Function:
        RemoveAllEntries

    Description:
        Remove all entries from the metrics set library.

    \***********************************************************************************/
    void DeviceProfilerMetricsSetFile::RemoveAllEntries()
    {
        m_Entries.clear();
    }

    /***********************************************************************************\

    Function:
        GetEntryCount

    Description:
        Get number of metrics sets in the library.

    \***********************************************************************************/
    uint32_t DeviceProfilerMetricsSetFile::GetEntryCount() const
    {
        return static_cast<uint32_t>( m_Entries.size() );
    }

    /***********************************************************************************\

    Function:
        GetEntry

    Description:
        Get metrics set entry by index.

    \***********************************************************************************/
    const DeviceProfilerMetricsSetFileEntry& DeviceProfilerMetricsSetFile::GetEntry( uint32_t index ) const
    {
        assert( index < m_Entries.size() );
        return m_Entries.at( index );
    }

    /***********************************************************************************\

    Function:
        ResolveCounterIndices

    Description:
        Resolve counter indices based on supported counters.

    \***********************************************************************************/
    void DeviceProfilerMetricsSetFile::ResolveCounterIndices( DeviceProfilerFrontend& frontend )
    {
        const uint32_t supportedCounterCount = frontend.GetPerformanceCounterProperties( 0, nullptr );
        if( supportedCounterCount )
        {
            std::vector<VkProfilerPerformanceCounterProperties2EXT> supportedCounters( supportedCounterCount );
            frontend.GetPerformanceCounterProperties( supportedCounterCount, supportedCounters.data() );

            for( DeviceProfilerMetricsSetFileEntry& entry : m_Entries )
            {
                entry.ResolveCounterIndices( supportedCounters );
            }
        }
    }
}

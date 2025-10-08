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
#include <nlohmann/json.hpp>

namespace Profiler
{
    /***********************************************************************************\

    Function:
        DeviceProfilerMetricsSetFile

    Description:
        Constructor.

    \***********************************************************************************/
    DeviceProfilerMetricsSetFile::DeviceProfilerMetricsSetFile()
        : m_Name()
        , m_Description()
        , m_Counters()
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

            m_Name = json["name"].get<std::string>();
            m_Description = json["description"].get<std::string>();

            m_Counters.clear();

            for( const auto& counter : json["counters"] )
            {
                m_Counters.push_back( counter.get<std::string>() );
            }
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
            nlohmann::json json;
            json["name"] = m_Name;
            json["description"] = m_Description;
            json["counters"] = m_Counters;

            std::ofstream file( filename );
            file << json;
        }
        catch( ... )
        {
            return false;
        }
    }

    /***********************************************************************************\

    Function:
        SetName

    Description:
        Set metrics set name.

    \***********************************************************************************/
    void DeviceProfilerMetricsSetFile::SetName( const std::string_view& name )
    {
        m_Name = name;
    }

    /***********************************************************************************\

    Function:
        SetDescription

    Description:
        Set metrics set description.

    \***********************************************************************************/
    void DeviceProfilerMetricsSetFile::SetDescription( const std::string_view& description )
    {
        m_Description = description;
    }

    /***********************************************************************************\

    Function:
        SetCounters

    Description:
        Set metrics set counters.

    \***********************************************************************************/
    void DeviceProfilerMetricsSetFile::SetCounters( uint32_t count, const VkProfilerPerformanceCounterProperties2EXT* pProperties )
    {
        m_Counters.clear();
        m_Counters.reserve( count );

        for( uint32_t i = 0; i < count; i++ )
        {
            m_Counters.push_back( pProperties[i].shortName );
        }
    }

    /***********************************************************************************\

    Function:
        GetName

    Description:
        Get metrics set name.

    \***********************************************************************************/
    const std::string& DeviceProfilerMetricsSetFile::GetName() const
    {
        return m_Name;
    }

    /***********************************************************************************\

    Function:
        GetDescription

    Description:
        Get metrics set description.

    \***********************************************************************************/
    const std::string& DeviceProfilerMetricsSetFile::GetDescription() const
    {
        return m_Description;
    }

    /***********************************************************************************\

    Function:
        GetCounterCount

    Description:
        Get number of saved counters.

    \***********************************************************************************/
    uint32_t DeviceProfilerMetricsSetFile::GetCounterCount() const
    {
        return static_cast<uint32_t>( m_Counters.size() );
    }

    /***********************************************************************************\

    Function:
        GetCounters

    Description:
        Get metrics set counters.

    \***********************************************************************************/
    std::vector<uint32_t> DeviceProfilerMetricsSetFile::GetCounters( DeviceProfilerFrontend& frontend ) const
    {
        std::vector<uint32_t> result( 0 );

        const uint32_t supportedCounterCount = frontend.GetPerformanceCounterProperties( 0, nullptr );
        if( supportedCounterCount )
        {
            std::vector<VkProfilerPerformanceCounterProperties2EXT> supportedCounters( supportedCounterCount );
            frontend.GetPerformanceCounterProperties( supportedCounterCount, supportedCounters.data() );

            result.reserve( m_Counters.size() );

            for( const auto& counterName : m_Counters )
            {
                for( uint32_t i = 0; i < supportedCounterCount; i++ )
                {
                    if( counterName == supportedCounters[i].shortName )
                    {
                        result.push_back( i );
                        break;
                    }
                }
            }
        }

        return result;
    }
}

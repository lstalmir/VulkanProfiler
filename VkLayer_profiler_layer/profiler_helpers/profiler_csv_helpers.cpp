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

#include "profiler_csv_helpers.h"
#include "profiler/profiler_helpers.h"
#include "profiler_ext/VkProfilerEXT.h"

#include <assert.h>

#define sep( i ) ( ( ( i ) > 0 ) ? "," : "" )

namespace Profiler
{
    static const std::pair<VkProfilerPerformanceCounterStorageEXT, const char*> g_StorageNames[] = {
        { VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_INT32_EXT, "int32" },
        { VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_INT64_EXT, "int64" },
        { VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_UINT32_EXT, "uint32" },
        { VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_UINT64_EXT, "uint64" },
        { VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_FLOAT32_EXT, "float32" },
        { VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_FLOAT64_EXT, "float64" },
    };

    /***********************************************************************************\

    Function:
        GetPerformanceCounterStorageName

    Description:
        Get performance counter storage name.

    \***********************************************************************************/
    static const char* GetPerformanceCounterStorageName( VkProfilerPerformanceCounterStorageEXT storage )
    {
        for( const auto& pair : g_StorageNames )
        {
            if( pair.first == storage )
            {
                return pair.second;
            }
        }

        return "?";
    }

    /***********************************************************************************\

    Function:
        GetPerformanceCounterStorageType

    Description:
        Get performance counter storage type by its name.

    \***********************************************************************************/
    static VkProfilerPerformanceCounterStorageEXT GetPerformanceCounterStorageType( const char* name )
    {
        for( const auto& pair : g_StorageNames )
        {
            if( strcmp( pair.second, name ) == 0 )
            {
                return pair.first;
            }
        }

        return VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_MAX_ENUM_EXT;
    }

    /***********************************************************************************\

    Function:
        DeviceProfilerCsvSerializer

    Description:
        Constructor.

    \***********************************************************************************/
    DeviceProfilerCsvSerializer::DeviceProfilerCsvSerializer()
        : m_File()
        , m_Properties( 0 )
    {
    }

    /***********************************************************************************\

    Function:
        ~DeviceProfilerCsvSerializer

    Description:
        Destructor.

    \***********************************************************************************/
    DeviceProfilerCsvSerializer::~DeviceProfilerCsvSerializer()
    {
        Close();
    }

    /***********************************************************************************\

    Function:
        Open

    Description:
        Open CSV file for writing.

    \***********************************************************************************/
    bool DeviceProfilerCsvSerializer::Open( const std::string& filename )
    {
        try
        {
            m_File.open( filename, std::ios::out | std::ios::trunc );
            return m_File.is_open();
        }
        catch( ... )
        {
            return false;
        }
    }

    /***********************************************************************************\

    Function:
        Close

    Description:
        Close CSV file.

    \***********************************************************************************/
    void DeviceProfilerCsvSerializer::Close()
    {
        if( m_File.is_open() )
        {
            m_File.close();
        }
    }

    /***********************************************************************************\

    Function:
        WriteHeader

    Description:
        Write CSV header and save performance counter properties for the following rows.

    \***********************************************************************************/
    void DeviceProfilerCsvSerializer::WriteHeader( uint32_t count, const VkProfilerPerformanceCounterPropertiesEXT* pProperties )
    {
        m_Properties.resize( count );
        memcpy( m_Properties.data(), pProperties, count * sizeof( VkProfilerPerformanceCounterPropertiesEXT ) );

        for( uint32_t i = 0; i < count; ++i )
        {
            m_Properties[i] = pProperties[i];
            m_File << sep( i )
                   << GetPerformanceCounterStorageName( pProperties[i].storage ) << ":"
                   << pProperties[i].shortName;
        }

        m_File << std::endl;
    }

    /***********************************************************************************\

    Function:
        WriteRow

    Description:
        Write CSV row with performance counter results.

    \***********************************************************************************/
    void DeviceProfilerCsvSerializer::WriteRow( uint32_t count, const VkProfilerPerformanceCounterResultEXT* pValues )
    {
        assert( count == m_Properties.size() );

        for( uint32_t i = 0; i < count; ++i )
        {
            m_File << sep( i );

            switch( m_Properties[i].storage )
            {
            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_INT32_EXT:
                m_File << pValues[i].int32;
                break;

            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_INT64_EXT:
                m_File << pValues[i].int64;
                break;

            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_UINT32_EXT:
                m_File << pValues[i].uint32;
                break;

            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_UINT64_EXT:
                m_File << pValues[i].uint64;
                break;

            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_FLOAT32_EXT:
                m_File << pValues[i].float32;
                break;

            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_FLOAT64_EXT:
                m_File << pValues[i].float64;
                break;
            }
        }

        m_File << std::endl;
    }

    /***********************************************************************************\

    Function:
        DeviceProfilerCsvDeserializer

    Description:
        Constructor.

    \***********************************************************************************/
    DeviceProfilerCsvDeserializer::DeviceProfilerCsvDeserializer()
        : m_File()
        , m_Properties( 0 )
    {
    }

    /***********************************************************************************\

    Function:
        ~DeviceProfilerCsvDeserializer

    Description:
        Destructor.

    \***********************************************************************************/
    DeviceProfilerCsvDeserializer::~DeviceProfilerCsvDeserializer()
    {
        Close();
    }

    /***********************************************************************************\

    Function:
        Open

    Description:
        Open CSV file for reading.

    \***********************************************************************************/
    bool DeviceProfilerCsvDeserializer::Open( const std::string& filename )
    {
        try
        {
            m_File.open( filename, std::ios::in );
            return m_File.is_open();
        }
        catch( ... )
        {
            return false;
        }
    }

    /***********************************************************************************\

    Function:
        Close

    Description:
        Close CSV file.

    \***********************************************************************************/
    void DeviceProfilerCsvDeserializer::Close()
    {
        if( m_File.is_open() )
        {
            m_File.close();
        }
    }

    /***********************************************************************************\

    Function:
        ReadHeader

    Description:
        Read CSV header and return performance counter properties.

    \***********************************************************************************/
    std::vector<VkProfilerPerformanceCounterPropertiesEXT> DeviceProfilerCsvDeserializer::ReadHeader()
    {
        m_Properties.clear();

        std::string line;
        if( std::getline( m_File, line ) )
        {
            size_t start = 0;
            size_t end = 0;
            uint32_t i = 0;

            while( end != std::string::npos )
            {
                end = line.find( ",", start );
                std::string header = line.substr( start, end - start );

                size_t colon = header.find( ":" );
                if( colon != std::string::npos )
                {
                    VkProfilerPerformanceCounterPropertiesEXT property = {};
                    property.storage = GetPerformanceCounterStorageType( header.substr( 0, colon ).c_str() );

                    std::string name = header.substr( colon + 1 );
                    ProfilerStringFunctions::CopyString( property.shortName, name.c_str(), name.length() + 1 );

                    m_Properties.push_back( property );
                }

                start = end + 1;
                ++i;
            }
        }

        return m_Properties;
    }

    /***********************************************************************************\

    Function:
        ReadRow

    Description:
        Read CSV row with performance counter results.

    \***********************************************************************************/
    std::vector<VkProfilerPerformanceCounterResultEXT> DeviceProfilerCsvDeserializer::ReadRow()
    {
        std::vector<VkProfilerPerformanceCounterResultEXT> values( m_Properties.size() );

        std::string line;
        if( std::getline( m_File, line ) )
        {
            size_t start = 0;
            size_t end = 0;
            uint32_t i = 0;

            while( end != std::string::npos )
            {
                if( i >= m_Properties.size() )
                {
                    break;
                }

                end = line.find( ",", start );
                std::string value = line.substr( start, end - start );

                switch( m_Properties[i].storage )
                {
                case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_INT32_EXT:
                    values[i].int32 = std::stoi( value );
                    break;

                case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_INT64_EXT:
                    values[i].int64 = std::stoll( value );
                    break;

                case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_UINT32_EXT:
                    values[i].uint32 = std::stoul( value );
                    break;

                case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_UINT64_EXT:
                    values[i].uint64 = std::stoull( value );
                    break;

                case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_FLOAT32_EXT:
                    values[i].float32 = std::stof( value );
                    break;

                case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_FLOAT64_EXT:
                    values[i].float64 = std::stod( value );
                    break;
                }

                start = end + 1;
                ++i;
            }
        }

        return values;
    }
}

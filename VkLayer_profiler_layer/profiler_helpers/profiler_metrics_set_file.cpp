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
#include <yyjson.h>

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
        m_Name.clear();
        m_Description.clear();
        m_Counters.clear();

        yyjson_doc* pDocument = yyjson_read_file(
            filename.c_str(),
            YYJSON_READ_NOFLAG,
            nullptr,
            nullptr );

        if( !pDocument )
        {
            return false;
        }

        yyjson_val* pRoot = yyjson_doc_get_root( pDocument );
        if( !pRoot || !yyjson_is_obj( pRoot ) )
        {
            yyjson_doc_free( pDocument );
            return false;
        }

        yyjson_obj_iter iter;
        if( yyjson_obj_iter_init( pRoot, &iter ) )
        {
            yyjson_val* pName = yyjson_obj_iter_get( &iter, "name" );
            if( pName && yyjson_is_str( pName ) )
                m_Name = yyjson_get_str( pName );

            yyjson_val* pDescription = yyjson_obj_iter_get( &iter, "description" );
            if( pDescription && yyjson_is_str( pDescription ) )
                m_Description = yyjson_get_str( pDescription );

            yyjson_val* pCounters = yyjson_obj_iter_get( &iter, "counters" );
            if( pCounters && yyjson_is_arr( pCounters ) )
            {
                yyjson_arr_iter countersIter;
                yyjson_arr_iter_init( pCounters, &countersIter );

                while( yyjson_arr_iter_has_next( &countersIter ) )
                {
                    yyjson_val* pCounter = yyjson_arr_iter_next( &countersIter );
                    if( pCounter && yyjson_is_str( pCounter ) )
                        m_Counters.push_back( yyjson_get_str( pCounter ) );
                }
            }
        }

        yyjson_doc_free( pDocument );
        return true;
    }

    /***********************************************************************************\

    Function:
        Write

    Description:
        Write metrics set to JSON file.

    \***********************************************************************************/
    bool DeviceProfilerMetricsSetFile::Write( const std::string& filename ) const
    {
        yyjson_mut_doc* pDocument = yyjson_mut_doc_new( nullptr );
        if( !pDocument )
        {
            return false;
        }

        yyjson_mut_val* pRoot = yyjson_mut_obj( pDocument );
        yyjson_mut_doc_set_root( pDocument, pRoot );

        yyjson_mut_obj_add_str( pDocument, pRoot, "name", m_Name.c_str() );
        yyjson_mut_obj_add_str( pDocument, pRoot, "description", m_Description.c_str() );

        yyjson_mut_val* pCounters =
            yyjson_mut_obj_add_arr( pDocument, pRoot, "counters" );

        for( const std::string& counter : m_Counters )
        {
            yyjson_mut_arr_add_str( pDocument, pCounters, counter.c_str() );
        }

        bool success = yyjson_mut_write_file(
            filename.c_str(),
            pDocument,
            YYJSON_WRITE_NOFLAG,
            nullptr,
            nullptr );

        yyjson_mut_doc_free( pDocument );
        return success;
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

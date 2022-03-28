// Copyright (c) 2022 Lukasz Stalmirski
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

#include "profiler_config.h"

#include <fstream>

#define VKPROF_DISABLE_OVERLAY_CVAR_NAME "disable_overlay"
#define VKPROF_DISABLE_PERFORMANCE_QUERY_EXT_CVAR_NAME "disable_performance_query_ext"
#define VKPROF_ENABLE_RENDER_PASS_BEGIN_END_PROFILING_CVAR_NAME "enable_render_pass_begin_end_profiling"
#define VKPROF_SAMPLING_MODE_CVAR_NAME "sampling_mode"
#define VKPROF_SYNC_MODE_CVAR_NAME "sync_mode"

namespace Profiler
{
    void DeviceProfilerConfig::SaveToFile( const std::filesystem::path& filename ) const
    {
        std::ofstream out( filename );
        out << VKPROF_DISABLE_OVERLAY_CVAR_NAME " " <<
            static_cast<int>( (m_Flags & VK_PROFILER_CREATE_NO_OVERLAY_BIT_EXT) != 0 ) << "\n";
        out << VKPROF_DISABLE_PERFORMANCE_QUERY_EXT_CVAR_NAME " " <<
            static_cast<int>( (m_Flags & VK_PROFILER_CREATE_NO_PERFORMANCE_QUERY_EXTENSION_BIT_EXT) != 0 ) << "\n";
        out << VKPROF_ENABLE_RENDER_PASS_BEGIN_END_PROFILING_CVAR_NAME " " <<
            static_cast<int>( (m_Flags & VK_PROFILER_CREATE_RENDER_PASS_BEGIN_END_PROFILING_ENABLED_BIT_EXT) != 0 ) << "\n";
        out << VKPROF_SAMPLING_MODE_CVAR_NAME " " << static_cast<int>( m_SamplingMode ) << "\n";
        out << VKPROF_SYNC_MODE_CVAR_NAME " " << static_cast<int>( m_SyncMode ) << "\n";
    }

    void DeviceProfilerConfig::LoadFromFile( const std::filesystem::path& filename )
    {
        std::ifstream in( filename );
        std::string name, value;

        while( in )
        {
            in >> name >> value;

            if( !name.empty() )
            {
                if( strcmp( name.c_str(), VKPROF_DISABLE_OVERLAY_CVAR_NAME ) == 0 )
                {
                    if( atoi( value.c_str() ) != 0 )
                    {
                        m_Flags |= VK_PROFILER_CREATE_NO_OVERLAY_BIT_EXT;
                    }
                    continue;
                }

                if( strcmp( name.c_str(), VKPROF_DISABLE_PERFORMANCE_QUERY_EXT_CVAR_NAME ) == 0 )
                {
                    if( atoi( value.c_str() ) != 0 )
                    {
                        m_Flags |= VK_PROFILER_CREATE_NO_PERFORMANCE_QUERY_EXTENSION_BIT_EXT;
                    }
                    continue;
                }

                if( strcmp( name.c_str(), VKPROF_ENABLE_RENDER_PASS_BEGIN_END_PROFILING_CVAR_NAME ) == 0 )
                {
                    if( atoi( value.c_str() ) != 0 )
                    {
                        m_Flags |= VK_PROFILER_CREATE_RENDER_PASS_BEGIN_END_PROFILING_ENABLED_BIT_EXT;
                    }
                    continue;
                }

                if( strcmp( name.c_str(), VKPROF_SAMPLING_MODE_CVAR_NAME ) == 0 )
                {
                    m_SamplingMode = static_cast<VkProfilerModeEXT>( atoi( value.c_str() ) );
                    continue;
                }

                if( strcmp( name.c_str(), VKPROF_SYNC_MODE_CVAR_NAME ) == 0 )
                {
                    m_SyncMode = static_cast<VkProfilerSyncModeEXT>( atoi( value.c_str() ) );
                    continue;
                }
            }
        }
    }

    void DeviceProfilerConfig::LoadFromCreateInfo( const VkProfilerCreateInfoEXT* pCreateInfo )
    {
        m_Flags = pCreateInfo->flags;
        m_SamplingMode = pCreateInfo->samplingMode;
        m_SyncMode = pCreateInfo->syncMode;
    }

    void DeviceProfilerConfig::LoadFromEnvironment()
    {
        if( const char* pDisableOverlay = getenv( "VKPROF_DISABLE_OVERLAY" ) )
        {
            if( atoi( pDisableOverlay ) )
            {
                m_Flags |= VK_PROFILER_CREATE_NO_OVERLAY_BIT_EXT;
            }
            else
            {
                m_Flags &= ~VK_PROFILER_CREATE_NO_OVERLAY_BIT_EXT;
            }
        }

        if( const char* pDisablePerformanceQueryExt = getenv( "VKPROF_DISABLE_PERFORMANCE_QUERY_EXT" ) )
        {
            if( atoi( pDisablePerformanceQueryExt ) )
            {
                m_Flags |= VK_PROFILER_CREATE_NO_PERFORMANCE_QUERY_EXTENSION_BIT_EXT;
            }
            else
            {
                m_Flags &= ~VK_PROFILER_CREATE_NO_PERFORMANCE_QUERY_EXTENSION_BIT_EXT;
            }
        }

        if( const char* pEnableRenderPassBeginEndProfiling = getenv( "VKPROF_ENABLE_RENDER_PASS_BEGIN_END_PROFILING" ) )
        {
            if( atoi( pEnableRenderPassBeginEndProfiling ) )
            {
                m_Flags |= VK_PROFILER_CREATE_RENDER_PASS_BEGIN_END_PROFILING_ENABLED_BIT_EXT;
            }
            else
            {
                m_Flags &= ~VK_PROFILER_CREATE_RENDER_PASS_BEGIN_END_PROFILING_ENABLED_BIT_EXT;
            }
        }

        if( const char* pSamplingMode = getenv( "VKPROF_SAMPLING_MODE" ) )
        {
            m_SamplingMode = static_cast<VkProfilerModeEXT>( atoi( pSamplingMode ) );
        }

        if( const char* pSyncMode = getenv( "VKPROF_SYNC_MODE" ) )
        {
            m_SyncMode = static_cast<VkProfilerSyncModeEXT>( atoi( pSyncMode ) );
        }
    }
}

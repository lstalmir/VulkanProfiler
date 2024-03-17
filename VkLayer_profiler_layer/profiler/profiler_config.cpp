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
#include "profiler_helpers.h"

#include <fstream>
#include <string>

#define VKPROF_ENABLE_OVERLAY_CVAR_NAME "enable_overlay"
#define VKPROF_ENABLE_PERFORMANCE_QUERY_EXT_CVAR_NAME "enable_performance_query_ext"
#define VKPROF_ENABLE_RENDER_PASS_BEGIN_END_PROFILING_CVAR_NAME "enable_render_pass_begin_end_profiling"
#define VKPROF_SET_STABLE_POWER_STATE "set_stable_power_state"
#define VKPROF_SAMPLING_MODE_CVAR_NAME "sampling_mode"
#define VKPROF_SYNC_MODE_CVAR_NAME "sync_mode"

#define VKPROF_GET_ENV_CVAR_NAME(cvar) "VKPROF_" cvar

namespace Profiler
{
    void DeviceProfilerConfig::SaveToFile( const std::filesystem::path& filename ) const
    {
        std::ofstream out( filename );
        out << VKPROF_ENABLE_OVERLAY_CVAR_NAME " " << m_EnableOverlay << "\n";
        out << VKPROF_ENABLE_PERFORMANCE_QUERY_EXT_CVAR_NAME " " << m_EnablePerformanceQueryExtension << "\n";
        out << VKPROF_ENABLE_RENDER_PASS_BEGIN_END_PROFILING_CVAR_NAME " " << m_EnableRenderPassBeginEndProfiling << "\n";
        out << VKPROF_SET_STABLE_POWER_STATE " " << m_SetStablePowerState << "\n";
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
                if( strcmp( name.c_str(), VKPROF_ENABLE_OVERLAY_CVAR_NAME ) == 0 )
                {
                    m_EnableOverlay = atoi( value.c_str() );
                    continue;
                }

                if( strcmp( name.c_str(), VKPROF_ENABLE_PERFORMANCE_QUERY_EXT_CVAR_NAME ) == 0 )
                {
                    m_EnablePerformanceQueryExtension = atoi( value.c_str() );
                    continue;
                }

                if( strcmp( name.c_str(), VKPROF_ENABLE_RENDER_PASS_BEGIN_END_PROFILING_CVAR_NAME ) == 0 )
                {
                    m_EnableRenderPassBeginEndProfiling = atoi( value.c_str() );
                    continue;
                }

                if( strcmp( name.c_str(), VKPROF_SET_STABLE_POWER_STATE ) == 0 )
                {
                    m_SetStablePowerState = atoi( value.c_str() );
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
        m_EnableOverlay = (pCreateInfo->flags & VK_PROFILER_CREATE_NO_OVERLAY_BIT_EXT) == 0;
        m_EnablePerformanceQueryExtension = (pCreateInfo->flags & VK_PROFILER_CREATE_NO_PERFORMANCE_QUERY_EXTENSION_BIT_EXT) == 0;
        m_EnableRenderPassBeginEndProfiling = (pCreateInfo->flags & VK_PROFILER_CREATE_RENDER_PASS_BEGIN_END_PROFILING_ENABLED_BIT_EXT) != 0;
        m_SetStablePowerState = (pCreateInfo->flags & VK_PROFILER_CREATE_NO_STABLE_POWER_STATE) == 0;
        m_SamplingMode = pCreateInfo->samplingMode;
        m_SyncMode = pCreateInfo->syncMode;
    }

    void DeviceProfilerConfig::LoadFromEnvironment()
    {
        if( auto enableOverlay = ProfilerPlatformFunctions::GetEnvironmentVar( VKPROF_GET_ENV_CVAR_NAME( VKPROF_ENABLE_OVERLAY_CVAR_NAME ) ) )
        {
            m_EnableOverlay = std::stoi( enableOverlay.value() );
        }

        if( auto enablePerformanceQueryExt = ProfilerPlatformFunctions::GetEnvironmentVar( VKPROF_GET_ENV_CVAR_NAME( VKPROF_ENABLE_PERFORMANCE_QUERY_EXT_CVAR_NAME ) ) )
        {
            m_EnablePerformanceQueryExtension = std::stoi( enablePerformanceQueryExt.value() );
        }

        if( auto enableRenderPassBeginEndProfiling = ProfilerPlatformFunctions::GetEnvironmentVar( VKPROF_GET_ENV_CVAR_NAME( VKPROF_ENABLE_RENDER_PASS_BEGIN_END_PROFILING_CVAR_NAME ) ) )
        {
            m_EnableRenderPassBeginEndProfiling = std::stoi( enableRenderPassBeginEndProfiling.value() );
        }
        
        if( auto setStablePowerState = ProfilerPlatformFunctions::GetEnvironmentVar( VKPROF_GET_ENV_CVAR_NAME( VKPROF_SET_STABLE_POWER_STATE ) ) )
        {
            m_SetStablePowerState = std::stoi( setStablePowerState.value() );
        }

        if( auto samplingMode = ProfilerPlatformFunctions::GetEnvironmentVar( VKPROF_GET_ENV_CVAR_NAME( VKPROF_SAMPLING_MODE_CVAR_NAME ) ) )
        {
            m_SamplingMode = static_cast<VkProfilerModeEXT>( std::stoi( samplingMode.value() ) );
        }

        if( auto syncMode = ProfilerPlatformFunctions::GetEnvironmentVar( VKPROF_GET_ENV_CVAR_NAME( VKPROF_SYNC_MODE_CVAR_NAME ) ) )
        {
            m_SyncMode = static_cast<VkProfilerSyncModeEXT>( std::stoi( syncMode.value() ) );
        }
    }

    void DeviceProfilerConfig::LoadFromLayerSettings( const ProfilerLayerSettings& settings )
    {
        m_EnableOverlay = settings.enable_overlay;
        m_EnablePerformanceQueryExtension = settings.enable_performance_query_ext;
        m_EnableRenderPassBeginEndProfiling = settings.enable_render_pass_begin_end_profiling;
        m_SetStablePowerState = settings.set_stable_power_state;
        m_SamplingMode = static_cast<VkProfilerModeEXT>(settings.sampling_mode.value);
        m_SyncMode = static_cast<VkProfilerSyncModeEXT>(settings.sync_mode.value);
    }
}

// Copyright (c) 2022-2025 Lukasz Stalmirski
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

namespace Profiler
{
    DeviceProfilerConfig::DeviceProfilerConfig( const ProfilerLayerSettings& settings )
        : ProfilerLayerSettings( settings )
    {}

    void DeviceProfilerConfig::LoadFromCreateInfo( const VkProfilerCreateInfoEXT* pCreateInfo )
    {
        for( const VkBaseInStructure& s : PNextIterator( pCreateInfo->pNext ) )
        {
            switch( s.sType )
            {
            case VK_STRUCTURE_TYPE_PROFILER_PERFORMANCE_COUNTERS_CREATE_INFO_EXT:
            {
                const VkProfilerPerformanceCountersCreateInfoEXT& performanceCountersCreateInfo =
                    *reinterpret_cast<const VkProfilerPerformanceCountersCreateInfoEXT*>( s.pNext );
                m_EnablePerformanceQueryExt = performanceCountersCreateInfo.backend;
                m_PerformanceQueryMode = performanceCountersCreateInfo.samplingMode;
                break;
            }
            }
        }

        if( (m_Output == output_t::overlay) && (pCreateInfo->flags & VK_PROFILER_CREATE_NO_OVERLAY_BIT_EXT) )
        {
            m_Output = output_t::none;
        }

        if( pCreateInfo->flags & VK_PROFILER_CREATE_NO_PERFORMANCE_QUERY_EXTENSION_BIT_EXT )
        {
            m_EnablePerformanceQueryExt = enable_performance_query_ext_t::none;
        }

        m_EnableRenderPassBeginEndProfiling = (pCreateInfo->flags & VK_PROFILER_CREATE_RENDER_PASS_BEGIN_END_PROFILING_ENABLED_BIT_EXT) != 0;
        m_SetStablePowerState = (pCreateInfo->flags & VK_PROFILER_CREATE_NO_STABLE_POWER_STATE_BIT_EXT) == 0;
        m_EnableThreading = (pCreateInfo->flags & VK_PROFILER_CREATE_NO_THREADING_BIT_EXT) == 0;
        m_EnableMemoryProfiling = (pCreateInfo->flags & VK_PROFILER_CREATE_NO_MEMORY_PROFILING_BIT_EXT) == 0;
        m_SamplingMode = pCreateInfo->samplingMode;
        m_FrameDelimiter = pCreateInfo->frameDelimiter;
    }
}

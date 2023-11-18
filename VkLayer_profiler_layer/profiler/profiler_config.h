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

#pragma once
#include "profiler_ext/VkProfilerEXT.h"

#include <filesystem>

namespace Profiler
{
    class DeviceProfilerConfig
    {
    public:
        // Whether to display the interactive overlay on the application's window.
        bool m_EnableOverlay = true;

        // Whether to enable VK_INTEL_performance_query extension.
        bool m_EnablePerformanceQueryExtension = true;

        // Whether to enable profiling of vkCmdBeginRenderPass and vkCmdEndRenderPass in per render pass sampling mode.
        bool m_EnableRenderPassBeginEndProfiling = false;

        // Whether to try to stabilize GPU frequency by setting stable power state via D3D12 device (Windows 10+ only).
        bool m_SetStablePowerState = true;

        // Frequency of sending timestamp queries in command buffers recorded by the application.
        VkProfilerModeEXT m_SamplingMode = VK_PROFILER_MODE_PER_DRAWCALL_EXT;

        // Frequency of reading the timestamp queries.
        VkProfilerSyncModeEXT m_SyncMode = VK_PROFILER_SYNC_MODE_PRESENT_EXT;

        // Pipeline stage at which begin timestamps are sent.
        VkPipelineStageFlagBits m_BeginTimestampStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

        // Pipeline stage at which end timestamps are sent.
        VkPipelineStageFlagBits m_EndTimestampStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

    public:
        void SaveToFile( const std::filesystem::path& filename ) const;
        void LoadFromFile( const std::filesystem::path& filename );
        void LoadFromCreateInfo( const VkProfilerCreateInfoEXT* pCreateInfo );
        void LoadFromEnvironment();
    };
}

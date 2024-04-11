// Copyright (c) 2024 Lukasz Stalmirski
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

#define PROFILER_MENU "##Menu"
#define PROFILER_MENU_ITEM "##MenuItem"

namespace Profiler
{
    struct Lang
    {
        enum ID
        {
            eEnglish,
            ePolish,
            eCount,
            eDefault = eEnglish
        };

        const char* LanguageName;

        const char* WindowName;
        const char* Device;
        const char* Instance;
        const char* Pause;
        const char* Save;

        // Menu
        const char* FileMenu;
        const char* WindowMenu;
        const char* PerformanceMenuItem;
        const char* PerformanceCountersMenuItem;
        const char* TopPipelinesMenuItem;
        const char* MemoryMenuItem;
        const char* StatisticsMenuItem;
        const char* SettingsMenuItem;

        // Tabs
        const char* Performance;
        const char* PerformanceCounters;
        const char* TopPipelines;
        const char* FrameBrowser;
        const char* Memory;
        const char* Statistics;
        const char* Settings;

        // Performance tab
        const char* GPUTime;
        const char* CPUTime;
        const char* FPS;
        const char* RenderPasses;
        const char* Pipelines;
        const char* Drawcalls;
        const char* HistogramGroups;
        const char* GPUCycles;
        const char* Metric;
        const char* Frame;
        const char* SubmissionOrder;
        const char* DurationDescending;
        const char* DurationAscending;
        const char* Sort;
        const char* CustomStatistics;
        const char* Container;

        const char* ShaderCapabilityTooltipFmt;

        const char* PerformanceCountersFilter;
        const char* PerformanceCountersRange;
        const char* PerformanceCountersSet;
        const char* PerformanceCountesrNotAvailable;
        const char* PerformanceCountersNotAvailableForCommandBuffer;

        // Memory tab
        const char* MemoryHeapUsage;
        const char* MemoryHeap;
        const char* Allocations;
        const char* MemoryTypeIndex;

        // Statistics tab
        const char* DrawCalls;
        const char* DrawCallsIndirect;
        const char* DispatchCalls;
        const char* DispatchCallsIndirect;
        const char* TraceRaysCalls;
        const char* TraceRaysIndirectCalls;
        const char* CopyBufferCalls;
        const char* CopyBufferToImageCalls;
        const char* CopyImageCalls;
        const char* CopyImageToBufferCalls;
        const char* PipelineBarriers;
        const char* ColorClearCalls;
        const char* DepthStencilClearCalls;
        const char* ResolveCalls;
        const char* BlitCalls;
        const char* FillBufferCalls;
        const char* UpdateBufferCalls;

        // Settings tab
        const char* Language;
        const char* Present;
        const char* Submit;
        const char* SamplingMode;
        const char* SyncMode;
        const char* InterfaceScale;
        const char* ShowDebugLabels;
        const char* ShowShaderCapabilities;
        const char* TimeUnit;

        const char* Milliseconds;
        const char* Microseconds;
        const char* Nanoseconds;

        const char* Unknown;
    };

    const Lang& GetLanguage( Lang::ID id = Lang::eDefault );
}

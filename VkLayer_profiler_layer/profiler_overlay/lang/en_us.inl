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

#include "lang.h"

namespace Profiler
{
    inline const Lang& GetLanguage_en_us()
    {
        static Lang en_us;
        if( !en_us.Language )
        {
            en_us.LanguageName = "English (United States)";

            en_us.WindowName = "VkProfiler";
            en_us.Device = "Device";
            en_us.Instance = "Instance";
            en_us.Pause = "Pause";
            en_us.Save = "Save trace";

            en_us.FileMenu = "File" PROFILER_MENU;
            en_us.WindowMenu = "Window" PROFILER_MENU;
            en_us.PerformanceMenuItem = "Performance" PROFILER_MENU_ITEM;
            en_us.PerformanceCountersMenuItem = "Performance counters" PROFILER_MENU_ITEM;
            en_us.TopPipelinesMenuItem = "Top pipelines" PROFILER_MENU_ITEM;
            en_us.MemoryMenuItem = "Memory" PROFILER_MENU_ITEM;
            en_us.StatisticsMenuItem = "Statistics" PROFILER_MENU_ITEM;
            en_us.SettingsMenuItem = "Settings" PROFILER_MENU_ITEM;

            en_us.Performance = "Performance";
            en_us.Memory = "Memory";
            en_us.Statistics = "Statistics";
            en_us.Settings = "Settings";

            en_us.GPUTime = "GPU Time";
            en_us.CPUTime = "CPU Time";
            en_us.FPS = "fps";
            en_us.RenderPasses = "Render passes";
            en_us.Pipelines = "Pipelines";
            en_us.Drawcalls = "Drawcalls";
            en_us.HistogramGroups = "Histogram groups";
            en_us.GPUCycles = "GPU Cycles";
            en_us.TopPipelines = "Top pipelines";
            en_us.PerformanceCounters = "Performance counters";
            en_us.Metric = "Metric";
            en_us.Frame = "Frame";
            en_us.FrameBrowser = "Frame browser";
            en_us.SubmissionOrder = "Submission order";
            en_us.DurationDescending = "Duration descending";
            en_us.DurationAscending = "Duration ascending";
            en_us.Sort = "Sort";
            en_us.CustomStatistics = "Custom statistics";
            en_us.Container = "Container";

            en_us.ShaderCapabilityTooltipFmt = "At least one shader in the pipeline uses '%s' capability.";

            en_us.PerformanceCountersFilter = "Filter";
            en_us.PerformanceCountersRange = "Range";
            en_us.PerformanceCountersSet = "Metrics set";
            en_us.PerformanceCountesrNotAvailable = "Performance metrics are not available.";
            en_us.PerformanceCountersNotAvailableForCommandBuffer = "Performance metrics are not available for the selected command buffer.";

            en_us.MemoryHeapUsage = "Memory heap usage";
            en_us.MemoryHeap = "Heap";
            en_us.Allocations = "Allocations";
            en_us.MemoryTypeIndex = "Memory type index";

            en_us.DrawCalls = "Draw calls";
            en_us.DrawCallsIndirect = "Draw calls (indirect)";
            en_us.DispatchCalls = "Dispatch calls";
            en_us.DispatchCallsIndirect = "Dispatch calls (indirect)";
            en_us.TraceRaysCalls = "Trace rays calls";
            en_us.TraceRaysIndirectCalls = "Trace rays calls (indirect)";
            en_us.CopyBufferCalls = "Copy buffer calls";
            en_us.CopyBufferToImageCalls = "Copy buffer-to-image calls";
            en_us.CopyImageCalls = "Copy image calls";
            en_us.CopyImageToBufferCalls = "Copy image-to-buffer calls";
            en_us.PipelineBarriers = "Pipeline barriers";
            en_us.ColorClearCalls = "Color clear calls";
            en_us.DepthStencilClearCalls = "Depth-stencil clear calls";
            en_us.ResolveCalls = "Resolve calls";
            en_us.BlitCalls = "Blit calls";
            en_us.FillBufferCalls = "Fill buffer calls";
            en_us.UpdateBufferCalls = "Update buffer calls";

            en_us.Language = "Language";
            en_us.Present = "Present";
            en_us.Submit = "Submit";
            en_us.SamplingMode = "Sampling mode";
            en_us.SyncMode = "Sync mode";
            en_us.InterfaceScale = "Interface scale";
            en_us.ShowDebugLabels = "Show debug labels";
            en_us.ShowShaderCapabilities = "Show shader capabilities";
            en_us.TimeUnit = "Time unit";

            en_us.Milliseconds = "ms";
            en_us.Microseconds = "us";
            en_us.Nanoseconds = "ns";

            en_us.Unknown = "Unknown";
        }
        return en_us;
    }
}

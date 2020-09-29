// Copyright (c) 2020 Lukasz Stalmirski
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

namespace Profiler
{
    struct DeviceProfilerOverlayLanguage_Base
    {
        inline static constexpr char Language[] = "English (United States)";

        inline static constexpr char WindowName[] = "VkProfiler";
        inline static constexpr char Device[] = "Device";
        inline static constexpr char Instance[] = "Instance";
        inline static constexpr char Pause[] = "Pause";

        // Tabs
        inline static constexpr char Performance[] = "Performance";
        inline static constexpr char Memory[] = "Memory";
        inline static constexpr char Statistics[] = "Statistics";
        inline static constexpr char Settings[] = "Settings";

        // Performance tab
        inline static constexpr char GPUTime[] = "GPU Time";
        inline static constexpr char CPUTime[] = "CPU Time";
        inline static constexpr char FPS[] = "fps";
        inline static constexpr char RenderPasses[] = "Render passes";
        inline static constexpr char Pipelines[] = "Pipelines";
        inline static constexpr char Drawcalls[] = "Drawcalls";
        inline static constexpr char HistogramGroups[] = "Histogram groups";
        inline static constexpr char GPUCycles[] = "GPU Cycles";
        inline static constexpr char TopPipelines[] = "Top pipelines";
        inline static constexpr char PerformanceCounters[] = "Performance counters";
        inline static constexpr char Metric[] = "Metric";
        inline static constexpr char Frame[] = "Frame";
        inline static constexpr char FrameBrowser[] = "Frame browser";
        inline static constexpr char SubmissionOrder[] = "Submission order";
        inline static constexpr char DurationDescending[] = "Duration descending";
        inline static constexpr char DurationAscending[] = "Duration ascending";
        inline static constexpr char Sort[] = "Sort";
        inline static constexpr char CustomStatistics[] = "Custom statistics";
        inline static constexpr char Container[] = "Container";

        // Memory tab
        inline static constexpr char MemoryHeapUsage[] = "Memory heap usage";
        inline static constexpr char MemoryHeap[] = "Heap";
        inline static constexpr char Allocations[] = "Allocations";
        inline static constexpr char MemoryTypeIndex[] = "Memory type index";

        // Statistics tab
        inline static constexpr char DrawCalls[] = "Draw calls";
        inline static constexpr char DrawCallsIndirect[] = "Draw calls (indirect)";
        inline static constexpr char DispatchCalls[] = "Dispatch calls";
        inline static constexpr char DispatchCallsIndirect[] = "Dispatch calls (indirect)";
        inline static constexpr char CopyBufferCalls[] = "Copy buffer calls";
        inline static constexpr char CopyBufferToImageCalls[] = "Copy buffer-to-image calls";
        inline static constexpr char CopyImageCalls[] = "Copy image calls";
        inline static constexpr char CopyImageToBufferCalls[] = "Copy image-to-buffer calls";
        inline static constexpr char PipelineBarriers[] = "Pipeline barriers";
        inline static constexpr char ColorClearCalls[] = "Color clear calls";
        inline static constexpr char DepthStencilClearCalls[] = "Depth-stencil clear calls";
        inline static constexpr char ResolveCalls[] = "Resolve calls";
        inline static constexpr char BlitCalls[] = "Blit calls";
        inline static constexpr char FillBufferCalls[] = "Fill buffer calls";
        inline static constexpr char UpdateBufferCalls[] = "Update buffer calls";

        // Settings tab
        inline static constexpr char Present[] = "Present";
        inline static constexpr char Submit[] = "Submit";
        inline static constexpr char SyncMode[] = "Sync mode";
        inline static constexpr char ShowDebugLabels[] = "Show debug labels";

        inline static constexpr char Unknown[] = "Unknown";
    };
}

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

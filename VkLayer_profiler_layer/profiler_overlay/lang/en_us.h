// Copyright (c) 2019-2021 Lukasz Stalmirski
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

#define PROFILER_MENU      "##Menu"
#define PROFILER_MENU_ITEM "##MenuItem"

namespace Profiler
{
    struct DeviceProfilerOverlayLanguage_Base
    {
        inline static constexpr char Language[] = "English (United States)";

        inline static constexpr char WindowName[] = "VkProfiler";
        inline static constexpr char Device[] = "Device";
        inline static constexpr char Instance[] = "Instance";
        inline static constexpr char Pause[] = "Pause";
        inline static constexpr char Save[] = "Save";
        inline static constexpr char Load[] = "Load";
        inline static constexpr char SaveTrace[] = "Save trace";

        // Menu
        inline static constexpr char FileMenu[] = "File" PROFILER_MENU;
        inline static constexpr char WindowMenu[] = "Window" PROFILER_MENU;
        inline static constexpr char PerformanceMenuItem[] = "Performance" PROFILER_MENU_ITEM;
        inline static constexpr char PerformanceCountersMenuItem[] = "Performance counters" PROFILER_MENU_ITEM;
        inline static constexpr char QueueUtilizationMenuItem[] = "Queue utilization" PROFILER_MENU_ITEM;
        inline static constexpr char TopPipelinesMenuItem[] = "Top pipelines" PROFILER_MENU_ITEM;
        inline static constexpr char MemoryMenuItem[] = "Memory" PROFILER_MENU_ITEM;
        inline static constexpr char InspectorMenuItem[] = "Inspector" PROFILER_MENU_ITEM;
        inline static constexpr char StatisticsMenuItem[] = "Statistics" PROFILER_MENU_ITEM;
        inline static constexpr char SettingsMenuItem[] = "Settings" PROFILER_MENU_ITEM;
        inline static constexpr char ApplicationInfoMenuItem[] = "Application info" PROFILER_MENU_ITEM;
        inline static constexpr char Fullscreen[] = "Fullscreen";

        // Tabs
        inline static constexpr char Performance[] = "Performance###Performance";
        inline static constexpr char Memory[] = "Memory###Memory";
        inline static constexpr char Inspector[] = "Inspector###Inspector";
        inline static constexpr char Statistics[] = "Statistics###Statistics";
        inline static constexpr char Settings[] = "Settings###Settings";
        inline static constexpr char ApplicationInfo[] = "Application info###ApplicationInfo";

        // Performance tab
        inline static constexpr char GPUTime[] = "GPU Time";
        inline static constexpr char CPUTime[] = "CPU Time";
        inline static constexpr char FPS[] = "fps";
        inline static constexpr char Frames[] = "Frames###Frames";
        inline static constexpr char Submits[] = "Submissions###Frames";
        inline static constexpr char Snapshots[] = "Snapshots###Snapshots";
        inline static constexpr char RenderPasses[] = "Render passes";
        inline static constexpr char Pipelines[] = "Pipelines";
        inline static constexpr char Drawcalls[] = "Drawcalls";
        inline static constexpr char Constant[] = "Constant";
        inline static constexpr char Duration[] = "Duration";
        inline static constexpr char Height[] = "Height";
        inline static constexpr char ShowIdle[] = "Show idle";
        inline static constexpr char ShowActiveFrame[] = "Show active frame";
        inline static constexpr char HistogramGroups[] = "Histogram groups";
        inline static constexpr char GPUCycles[] = "GPU Cycles";
        inline static constexpr char QueueUtilization[] = "Queue utilization###QueueUtilization";
        inline static constexpr char TopPipelines[] = "Top pipelines###Top pipelines";
        inline static constexpr char PerformanceCounters[] = "Performance counters###Performance counters";
        inline static constexpr char Metric[] = "Metric";
        inline static constexpr char Contrib[] = "Contrib";
        inline static constexpr char Frame[] = "Frame";
        inline static constexpr char Pipeline[] = "Pipeline";
        inline static constexpr char Stages[] = "Stages";
        inline static constexpr char Capabilities[] = "Capabilities";
        inline static constexpr char Ref[] = "Ref";
        inline static constexpr char Delta[] = "Delta";
        inline static constexpr char Value[] = "Value";
        inline static constexpr char SetRef[] = "Set ref";
        inline static constexpr char ClearRef[] = "Clear ref";
        inline static constexpr char FrameBrowser[] = "Frame browser###Frame browser";
        inline static constexpr char SubmissionOrder[] = "Submission order";
        inline static constexpr char DurationDescending[] = "Duration descending";
        inline static constexpr char DurationAscending[] = "Duration ascending";
        inline static constexpr char Sort[] = "Sort";
        inline static constexpr char CustomStatistics[] = "Custom statistics";
        inline static constexpr char Container[] = "Container";
        inline static constexpr char Inspect[] = "Inspect";
        inline static constexpr char ShowPerformanceMetrics[] = "Show performance metrics";
        inline static constexpr char SelectedFrame[] = "Selected frame";
        inline static constexpr char CopyToClipboard[] = "Copy to clipboard";
        inline static constexpr char CopyName[] = "Copy name";
        inline static constexpr char ShowMore[] = "Show more...";
        inline static constexpr char ShowLess[] = "Show less";

        inline static constexpr char ShaderCapabilityTooltipFmt[] = "At least one shader in the pipeline uses '%s' capability.";
        inline static constexpr char ShaderObjectsTooltip[] = "Pipeline constructed from VkShaderEXT objects passed via vkCmdBindShadersEXT.";

        inline static constexpr char PerformanceCountersFilter[] = "Filter";
        inline static constexpr char PerformanceCountersRange[] = "Range";
        inline static constexpr char PerformanceCountersSet[] = "Metrics set";
        inline static constexpr char PerformanceCountesrNotAvailable[] = "Performance metrics are not available.";
        inline static constexpr char PerformanceCountersNotAvailableForCommandBuffer[] = "Performance metrics are not available for the selected command buffer.";

        // Memory tab
        inline static constexpr char ResourceBrowser[] = "Resource browser###Resource browser";
        inline static constexpr char ResourceInspector[] = "Resource inspector###Resource inspector";
        inline static constexpr char MemoryHeapUsage[] = "Memory heap usage";
        inline static constexpr char MemoryHeap[] = "Heap";
        inline static constexpr char Allocations[] = "Allocations";
        inline static constexpr char MemoryTypeIndex[] = "Memory type index";
        inline static constexpr char Buffers[] = "Buffers";
        inline static constexpr char Images[] = "Images";

        // Statistics tab
        inline static constexpr char ShowEmptyStatistics[] = "Show empty statistics...";
        inline static constexpr char HideEmptyStatistics[] = "Hide empty statistics";
        inline static constexpr char StatName[] = "Name";
        inline static constexpr char StatCount[] = "Count";
        inline static constexpr char StatTotal[] = "Total";
        inline static constexpr char StatMin[] = "Min";
        inline static constexpr char StatMax[] = "Max";
        inline static constexpr char StatAvg[] = "Avg";
        inline static constexpr char DrawCalls[] = "Draw calls";
        inline static constexpr char DrawCallsIndirect[] = "Draw calls (indirect)";
        inline static constexpr char DrawMeshTasksCalls[] = "Draw mesh tasks calls";
        inline static constexpr char DrawMeshTasksIndirectCalls[] = "Draw mesh tasks calls (indirect)";
        inline static constexpr char DispatchCalls[] = "Dispatch calls";
        inline static constexpr char DispatchCallsIndirect[] = "Dispatch calls (indirect)";
        inline static constexpr char TraceRaysCalls[] = "Trace rays calls";
        inline static constexpr char TraceRaysIndirectCalls[] = "Trace rays calls (indirect)";
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

        // Inspector tab
        inline static constexpr char PipelineState[] = "Pipeline state";
        inline static constexpr char PipelineStateNotAvailable[] = "Pipeline state info is not available for this pipeline.";
        inline static constexpr char PipelineStateVertexInput[] = "Vertex input";
        inline static constexpr char PipelineStateInputAssembly[] = "Input assembly";
        inline static constexpr char PipelineStateTessellation[] = "Tessellation";
        inline static constexpr char PipelineStateViewport[] = "Viewport";
        inline static constexpr char PipelineStateRasterization[] = "Rasterization";
        inline static constexpr char PipelineStateMultisampling[] = "Multisampling";
        inline static constexpr char PipelineStateDepthStencil[] = "Depth and stencil";
        inline static constexpr char PipelineStateColorBlend[] = "Color blend";

        // Settings tab
        inline static constexpr char Present[] = "Present";
        inline static constexpr char Submit[] = "Submit";
        inline static constexpr char SamplingMode[] = "Sampling mode";
        inline static constexpr char FrameDelimiter[] = "Frame delimiter";
        inline static constexpr char InterfaceScale[] = "Interface scale";
        inline static constexpr char CollectedFrameCount[] = "Collected frame count";
        inline static constexpr char ShowDebugLabels[] = "Show debug labels";
        inline static constexpr char ShowShaderCapabilities[] = "Show shader capabilities";
        inline static constexpr char TimeUnit[] = "Time unit";

        inline static constexpr char Milliseconds[] = "ms";
        inline static constexpr char Microseconds[] = "us";
        inline static constexpr char Nanoseconds[] = "ns";

        // Application info window
        inline static constexpr char VulkanVersion[] = "Vulkan version";
        inline static constexpr char ApplicationName[] = "Application name";
        inline static constexpr char ApplicationVersion[] = "Application version";
        inline static constexpr char EngineName[] = "Engine name";
        inline static constexpr char EngineVersion[] = "Engine version";

        inline static constexpr char Unknown[] = "Unknown";
    };
}

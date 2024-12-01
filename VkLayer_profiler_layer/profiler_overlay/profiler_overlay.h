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
#include "profiler/profiler_data_aggregator.h"
#include "profiler/profiler_helpers.h"
#include "profiler/profiler_stat_comparators.h"
#include "profiler_layer_objects/VkDevice_object.h"
#include "profiler_layer_objects/VkQueue_object.h"
#include "profiler_layer_objects/VkSwapchainKhr_object.h"
#include "profiler_helpers/profiler_time_helpers.h"
#include "profiler_overlay_settings.h"
#include "profiler_overlay_resources.h"
#include "profiler_overlay_shader_view.h"
#include <vulkan/vk_layer.h>
#include <list>
#include <vector>
#include <stack>
#include <mutex>
#include <functional>

// Public interface
#include "profiler_ext/VkProfilerEXT.h"

struct ImGuiContext;
class ImGui_ImplVulkan_Context;
class ImGui_Window_Context;

namespace ImGuiX
{
    struct HistogramColumnData;
}

namespace Profiler
{
    class DeviceProfiler;
    struct ProfilerSubmitData;

    /***********************************************************************************\

    Class:
        ProfilerOverlayOutput

    Description:
        Writes profiling output to the overlay.

    \***********************************************************************************/
    class ProfilerOverlayOutput final
    {
    public:
        ProfilerOverlayOutput();
        ~ProfilerOverlayOutput();

        VkResult Initialize(
            VkDevice_Object& device,
            VkQueue_Object& graphicsQueue,
            VkSwapchainKhr_Object& swapchain,
            const VkSwapchainCreateInfoKHR* pCreateInfo );

        void Destroy();

        bool IsAvailable() const;

        VkSwapchainKHR GetSwapchain() const;

        VkResult ResetSwapchain(
            VkSwapchainKhr_Object& swapchain,
            const VkSwapchainCreateInfoKHR* pCreateInfo );

        void Present(
            const std::shared_ptr<DeviceProfilerFrameData>& pData,
            const VkQueue_Object& presentQueue,
            VkPresentInfoKHR* pPresentInfo );

    private:
        OverlaySettings m_Settings;

        VkDevice_Object* m_pDevice;
        VkQueue_Object* m_pGraphicsQueue;
        VkSwapchainKhr_Object* m_pSwapchain;

        OSWindowHandle m_Window;

        ImGuiContext* m_pImGuiContext;
        ImGui_ImplVulkan_Context* m_pImGuiVulkanContext;
        ImGui_Window_Context* m_pImGuiWindowContext;

        OverlayResources m_Resources;

        VkDescriptorPool m_DescriptorPool;

        VkRenderPass m_RenderPass;
        VkExtent2D m_RenderArea;
        VkFormat m_ImageFormat;
        std::vector<VkImage> m_Images;
        std::vector<VkImageView> m_ImageViews;
        std::vector<VkFramebuffer> m_Framebuffers;

        VkCommandPool m_CommandPool;
        std::vector<VkCommandBuffer> m_CommandBuffers;
        std::vector<VkFence> m_CommandFences;
        std::vector<VkSemaphore> m_CommandSemaphores;

        std::string m_Title;

        uint32_t m_ActiveMetricsSetIndex;
        std::vector<bool> m_ActiveMetricsVisibility;

        struct VendorMetricsSet
        {
            VkProfilerPerformanceMetricsSetPropertiesEXT           m_Properties;
            std::vector<VkProfilerPerformanceCounterPropertiesEXT> m_Metrics;
        };

        std::vector<VendorMetricsSet> m_VendorMetricsSets;
        std::vector<bool>             m_VendorMetricsSetVisibility;

        char m_VendorMetricFilter[ 128 ] = {};

        Milliseconds m_TimestampPeriod;
        float m_TimestampDisplayUnit;
        const char* m_pTimestampDisplayUnitStr;

        enum class FrameBrowserSortMode
        {
            eSubmissionOrder,
            eDurationDescending,
            eDurationAscending
        };

        FrameBrowserSortMode m_FrameBrowserSortMode;

        enum class HistogramGroupMode
        {
            eRenderPass,
            ePipeline,
            eDrawcall,
            eRenderPassBegin,
            eRenderPassEnd,
        };

        enum class HistogramValueMode
        {
            eConstant,
            eDuration,
        };

        HistogramGroupMode m_HistogramGroupMode;
        HistogramValueMode m_HistogramValueMode;
        bool m_HistogramShowIdle;

        typedef std::vector<uint16_t> FrameBrowserTreeNodeIndex;

        std::shared_ptr<DeviceProfilerFrameData> m_pData;
        bool m_Pause;
        bool m_ShowDebugLabels;
        bool m_ShowShaderCapabilities;
        bool m_ShowEmptyStatistics;

        enum class TimeUnit
        {
            eMilliseconds,
            eMicroseconds,
            eNanoseconds
        };

        float m_FrameTime;

        TimeUnit m_TimeUnit;
        VkProfilerModeEXT m_SamplingMode;
        VkProfilerSyncModeEXT m_SyncMode;

        FrameBrowserTreeNodeIndex m_SelectedFrameBrowserNodeIndex;
        bool m_ScrollToSelectedFrameBrowserNode;
        bool ScrollToSelectedFrameBrowserNode( const FrameBrowserTreeNodeIndex& index ) const;

        std::vector<char> m_FrameBrowserNodeIndexStr;
        const char* GetFrameBrowserNodeIndexStr( const FrameBrowserTreeNodeIndex& index );

        std::chrono::high_resolution_clock::time_point m_SelectionUpdateTimestamp;
        std::chrono::high_resolution_clock::time_point m_SerializationFinishTimestamp;

        // Cached inspector tab state.
        DeviceProfilerPipeline m_InspectorPipeline;
        OverlayShaderView m_InspectorShaderView;

        struct InspectorTab
        {
            std::string Name;
            std::function<void()> Select;
            std::function<void()> Draw;
        };

        std::vector<InspectorTab> m_InspectorTabs;
        size_t m_InspectorTabIndex;

        // Performance metrics filter.
        // The profiler will show only metrics for the selected command buffer.
        // If no command buffer is selected, the aggregated stats for the whole frame will be displayed.
        VkCommandBuffer m_PerformanceQueryCommandBufferFilter;
        std::string     m_PerformanceQueryCommandBufferFilterName;

        std::unordered_map<std::string, VkProfilerPerformanceCounterResultEXT> m_ReferencePerformanceCounters;

        // Performance counter serialization
        struct PerformanceCounterExporter;
        std::unique_ptr<PerformanceCounterExporter> m_pPerformanceCounterExporter;

        // Trace serialization output
        bool m_SerializationSucceeded;
        bool m_SerializationWindowVisible;
        std::string m_SerializationMessage;
        VkExtent2D m_SerializationOutputWindowSize;
        std::chrono::milliseconds m_SerializationOutputWindowDuration;
        std::chrono::milliseconds m_SerializationOutputWindowFadeOutDuration;

        struct TraceExporter;
        std::unique_ptr<TraceExporter> m_pTraceExporter;

        // Performance graph colors
        uint32_t m_RenderPassColumnColor;
        uint32_t m_GraphicsPipelineColumnColor;
        uint32_t m_ComputePipelineColumnColor;
        uint32_t m_RayTracingPipelineColumnColor;
        uint32_t m_InternalPipelineColumnColor;

        class DeviceProfilerStringSerializer* m_pStringSerializer;

        // Dock space ids
        int m_MainDockSpaceId;
        int m_PerformanceTabDockSpaceId;
        int m_QueueUtilizationTabDockSpaceId;
        int m_TopPipelinesTabDockSpaceId;
        int m_FrameBrowserDockSpaceId;

        void PerformanceTabDockSpace( int flags = 0 );

        struct WindowState
        {
            bool* pOpen;
            bool Docked;
            bool Focus = false;
            void SetFocus();
        };

        WindowState m_PerformanceWindowState;
        WindowState m_QueueUtilizationWindowState;
        WindowState m_TopPipelinesWindowState;
        WindowState m_PerformanceCountersWindowState;
        WindowState m_MemoryWindowState;
        WindowState m_InspectorWindowState;
        WindowState m_StatisticsWindowState;
        WindowState m_SettingsWindowState;

        VkResult InitializeImGuiWindowHooks( const VkSwapchainCreateInfoKHR* );
        VkResult InitializeImGuiVulkanContext( const VkSwapchainCreateInfoKHR* );

        void InitializeImGuiStyle();

        void Update( const std::shared_ptr<DeviceProfilerFrameData>& );
        void UpdatePerformanceTab();
        void UpdateQueueUtilizationTab();
        void UpdateTopPipelinesTab();
        void UpdatePerformanceCountersTab();
        void UpdateMemoryTab();
        void UpdateInspectorTab();
        void UpdateStatisticsTab();
        void UpdateSettingsTab();

        // Performance graph helpers
        struct PerformanceGraphColumn;
        void GetPerformanceGraphColumns( std::vector<PerformanceGraphColumn>& ) const;
        void GetPerformanceGraphColumns( const DeviceProfilerCommandBufferData&, FrameBrowserTreeNodeIndex&, std::vector<PerformanceGraphColumn>& ) const;
        void GetPerformanceGraphColumns( const DeviceProfilerRenderPassData&, FrameBrowserTreeNodeIndex&, std::vector<PerformanceGraphColumn>& ) const;
        void GetPerformanceGraphColumns( const DeviceProfilerPipelineData&, FrameBrowserTreeNodeIndex&, std::vector<PerformanceGraphColumn>& ) const;
        void GetPerformanceGraphColumns( const DeviceProfilerDrawcall&, FrameBrowserTreeNodeIndex&, std::vector<PerformanceGraphColumn>& ) const;
        void DrawPerformanceGraphLabel( const ImGuiX::HistogramColumnData& );
        void SelectPerformanceGraphColumn( const ImGuiX::HistogramColumnData& );

        struct QueueGraphColumn;
        void GetQueueGraphColumns( VkQueue, std::vector<QueueGraphColumn>& ) const;
        float GetQueueUtilization( const std::vector<QueueGraphColumn>& ) const;

        // Performance counter helpers
        std::string GetDefaultPerformanceCountersFileName( uint32_t ) const;
        void UpdatePerformanceCounterExporter();
        void SavePerformanceCountersToFile( const std::string&, uint32_t, const std::vector<VkProfilerPerformanceCounterResultEXT>&, const std::vector<bool>& );
        void LoadPerformanceCountersFromFile( const std::string& );

        // Trace serialization helpers
        void UpdateTraceExporter();
        void SaveTraceToFile( const std::string&, const DeviceProfilerFrameData& );

        // Notifications
        void UpdateNotificationWindow();

        // Inspector helpers
        void Inspect( const DeviceProfilerPipeline& );
        void SelectInspectorShaderStage( size_t );
        void DrawInspectorShaderStage();
        void DrawInspectorPipelineState();
        void DrawInspectorGraphicsPipelineState();
        void SetInspectorTabIndex( size_t );
        void ShaderRepresentationSaved( bool, const std::string& );

        // Frame browser helpers
        void PrintCommandBuffer( const DeviceProfilerCommandBufferData&, FrameBrowserTreeNodeIndex& );
        void PrintRenderPass( const DeviceProfilerRenderPassData&, FrameBrowserTreeNodeIndex& );
        void PrintSubpass( const DeviceProfilerSubpassData&, FrameBrowserTreeNodeIndex&, bool );
        void PrintPipeline( const DeviceProfilerPipelineData&, FrameBrowserTreeNodeIndex& );
        void PrintDrawcall( const DeviceProfilerDrawcall&, FrameBrowserTreeNodeIndex& );
        void PrintDebugLabel( const char*, const float[ 4 ] );

        template<typename Data>
        void PrintRenderPassCommand( const Data& data, bool dynamic, FrameBrowserTreeNodeIndex& index, uint32_t drawcallIndex );

        template<typename Data>
        void DrawSignificanceRect( const Data& data, const FrameBrowserTreeNodeIndex& index );
        void DrawSignificanceRect( float significance, const FrameBrowserTreeNodeIndex& index );
        void DrawBadge( uint32_t color, const char* shortName, const char* fmt, ... );

        template<typename Data>
        void PrintDuration( const Data& data );

        template<typename Data>
        float GetDuration( const Data& data ) const;
        float GetDuration( uint64_t from, uint64_t to ) const;

        // Sort frame browser data
        template<typename TypeHint = std::nullptr_t, typename Data>
        auto SortFrameBrowserData( const Data& data ) const
        {
            using Subdata = typename Data::value_type;
            std::list<const Subdata*> pSortedData;

            for( const auto& subdata : data )
                pSortedData.push_back( &subdata );

            switch( m_FrameBrowserSortMode )
            {
            case FrameBrowserSortMode::eSubmissionOrder:
                break; // No sorting in submission order view

            case FrameBrowserSortMode::eDurationDescending:
                pSortedData.sort( []( const Subdata* a, const Subdata* b )
                    { return DurationDesc<TypeHint>( *a, *b ); } ); break;

            case FrameBrowserSortMode::eDurationAscending:
                pSortedData.sort( []( const Subdata* a, const Subdata* b )
                    { return DurationAsc<TypeHint>( *a, *b ); } ); break;
            }

            return pSortedData;
        }
    };
}

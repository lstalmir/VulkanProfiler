// Copyright (c) 2019-2025 Lukasz Stalmirski
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
#include "profiler_helpers/profiler_time_helpers.h"
#include "profiler_overlay_backend.h"
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
    class DeviceProfilerFrontend;

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

        bool Initialize( DeviceProfilerFrontend& frontend, OverlayBackend& backend );
        void Destroy();

        bool IsAvailable() const;

        void Update();

        void LoadPerformanceCountersFromFile( const std::string& );
        void LoadTopPipelinesFromFile( const std::string& );

        void SetMaxFrameCount( uint32_t maxFrameCount );

    private:
        OverlaySettings m_Settings;

        DeviceProfilerFrontend* m_pFrontend;
        OverlayBackend* m_pBackend;

        ImGuiContext* m_pImGuiContext;
        OverlayResources m_Resources;

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
            eFrame,
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

        struct FrameBrowserTreeNodeIndex : std::vector<uint16_t>
        {
            using std::vector<uint16_t>::vector;

            void SetFrameIndex( uint32_t frameIndex );
            uint32_t GetFrameIndex() const;

            const uint16_t* GetTreeNodeIndex() const;
            size_t GetTreeNodeIndexSize() const;
        };

        std::list<std::shared_ptr<DeviceProfilerFrameData>> m_pFrames;
         // 0 - current frame, 1 - previous frame, etc.
        uint32_t m_SelectedFrameIndex;
        uint32_t m_MaxFrameCount;

        std::shared_ptr<DeviceProfilerFrameData> m_pData;
        bool m_Pause;
        bool m_Fullscreen;
        bool m_ShowDebugLabels;
        bool m_ShowShaderCapabilities;
        bool m_ShowEmptyStatistics;
        bool m_ShowAllTopPipelines;
        bool m_ShowActiveFrame;

        bool m_SetLastMainWindowPos;
        Float2* m_pLastMainWindowPos;
        Float2* m_pLastMainWindowSize;

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

        // Frame browser state.
        struct FrameBrowserContext
        {
            const DeviceProfilerCommandBufferData* pCommandBuffer;
            const DeviceProfilerRenderPassData* pRenderPass;
            const DeviceProfilerPipelineData* pPipeline;
        };

        FrameBrowserTreeNodeIndex m_SelectedFrameBrowserNodeIndex;
        bool m_ScrollToSelectedFrameBrowserNode;
        bool ScrollToSelectedFrameBrowserNode( const FrameBrowserTreeNodeIndex& index ) const;

        std::vector<char> m_FrameBrowserNodeIndexStr;
        const char* GetFrameBrowserNodeIndexStr( const FrameBrowserTreeNodeIndex& index );

        std::chrono::high_resolution_clock::time_point m_SelectionUpdateTimestamp;
        std::chrono::high_resolution_clock::time_point m_SerializationFinishTimestamp;

        // Queue utilization state.
        std::unordered_set<VkSemaphore> m_SelectedSemaphores;

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

        // Top pipelines serialization
        struct TopPipelinesExporter;
        std::unique_ptr<TopPipelinesExporter> m_pTopPipelinesExporter;
        std::unordered_map<std::string, float> m_ReferenceTopPipelines;
        std::string m_ReferenceTopPipelinesShortDescription;
        std::string m_ReferenceTopPipelinesFullDescription;

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

        std::unique_ptr<class DeviceProfilerStringSerializer> m_pStringSerializer;

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

        void ResetMembers();

        void InitializeImGuiStyle();

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
        void DrawQueueGraphLabel( const ImGuiX::HistogramColumnData& );
        void SelectQueueGraphColumn( const ImGuiX::HistogramColumnData& );

        // Performance counter helpers
        std::string GetDefaultPerformanceCountersFileName( uint32_t ) const;
        void UpdatePerformanceCounterExporter();
        void SavePerformanceCountersToFile( const std::string&, uint32_t, const std::vector<VkProfilerPerformanceCounterResultEXT>&, const std::vector<bool>& );

        // Top pipelines helpers
        void UpdateTopPipelinesExporter();
        void SaveTopPipelinesToFile( const std::string&, const DeviceProfilerFrameData& );

        // Trace serialization helpers
        void UpdateTraceExporter();
        void SaveTraceToFile( const std::string&, const DeviceProfilerFrameData& );

        // Notifications
        void UpdateNotificationWindow();
        void UpdateApplicationInfoWindow();

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
        void PrintRenderPass( const DeviceProfilerRenderPassData&, FrameBrowserTreeNodeIndex&, const FrameBrowserContext& );
        void PrintSubpass( const DeviceProfilerSubpassData&, FrameBrowserTreeNodeIndex&, bool, const FrameBrowserContext& );
        void PrintPipeline( const DeviceProfilerPipelineData&, FrameBrowserTreeNodeIndex&, const FrameBrowserContext& );
        void PrintDrawcall( const DeviceProfilerDrawcall&, FrameBrowserTreeNodeIndex&, const FrameBrowserContext& );
        void PrintDrawcallIndirectPayload( const DeviceProfilerDrawcall&, const FrameBrowserContext& );
        void PrintDebugLabel( const char*, const float[ 4 ] );

        template<typename Data>
        void PrintRenderPassCommand( const Data& data, bool dynamic, FrameBrowserTreeNodeIndex& index, uint32_t drawcallIndex );

        template<typename Data>
        void DrawSignificanceRect( const Data& data, const FrameBrowserTreeNodeIndex& index );
        void DrawSignificanceRect( float significance, const FrameBrowserTreeNodeIndex& index );
        void DrawBadge( uint32_t color, const char* shortName, const char* fmt, ... );
        void DrawPipelineCapabilityBadges( const DeviceProfilerPipelineData& pipeline );
        void DrawPipelineStageBadge( const DeviceProfilerPipelineData& pipeline, VkShaderStageFlagBits stage, const char* pStageName );
        void DrawPipelineContextMenu( const DeviceProfilerPipelineData& pipeline, const char* id = nullptr );

        template<typename Data>
        void PrintDuration( const Data& data );
        void PrintDuration( uint64_t from, uint64_t to );

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

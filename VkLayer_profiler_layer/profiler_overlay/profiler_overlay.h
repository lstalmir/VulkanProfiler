// Copyright (c) 2019-2026 Lukasz Stalmirski
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
#include "profiler/profiler_frontend.h"
#include "profiler/profiler_data_aggregator.h"
#include "profiler/profiler_counters.h"
#include "profiler/profiler_helpers.h"
#include "profiler/profiler_stat_comparators.h"
#include "profiler_helpers/profiler_memory_comparator.h"
#include "profiler_helpers/profiler_time_helpers.h"
#include "profiler_overlay_backend.h"
#include "profiler_overlay_settings.h"
#include "profiler_overlay_resources.h"
#include "profiler_overlay_shader_view.h"
#include <vulkan/vk_layer.h>
#include <list>
#include <vector>
#include <stack>
#include <memory>
#include <mutex>
#include <functional>
#include <regex>

// Public interface
#include "profiler_ext/VkProfilerEXT.h"

struct ImGuiContext;
struct ImPlotContext;
class ImGui_ImplVulkan_Context;
class ImGui_Window_Context;

namespace ImGuiX
{
    struct HistogramColumnData;
}

namespace Profiler
{
    /***********************************************************************************\

    Class:
        ProfilerOverlayOutput

    Description:
        Writes profiling output to the overlay.

    \***********************************************************************************/
    class ProfilerOverlayOutput final : public DeviceProfilerOutput
    {
    public:
        ProfilerOverlayOutput( DeviceProfilerFrontend& frontend, OverlayBackend& backend );
        ~ProfilerOverlayOutput();

        bool Initialize() override;
        void Destroy() override;

        bool IsAvailable() override;

        void Update() override;
        void Present() override;

        void LoadPerformanceCountersFromFile( const std::string& );
        void LoadPerformanceQueryMetricsSetsFromFile( const std::string& );
        void SavePerformanceQueryMetricsSetsToFile( const std::string& );
        void LoadTopPipelinesFromFile( const std::string& );

        void SetMaxFrameCount( uint32_t maxFrameCount );

    private:
        OverlaySettings m_Settings;

        OverlayBackend& m_Backend;

        ImGuiContext* m_pImGuiContext;
        ImPlotContext* m_pImPlotContext;
        OverlayResources m_Resources;

        std::string m_Title;

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

        // Used to distinguish indexes between m_pFrames and m_pSnapshots.
        inline static const uint32_t SnapshotFrameIndexFlag = 0x80000000;
        inline static const uint32_t FrameIndexFlagsMask = 0x80000000;
        inline static const uint32_t FrameIndexMask = ~FrameIndexFlagsMask;
        inline static const uint32_t InvalidFrameIndex = UINT32_MAX;
        inline static const uint32_t CurrentFrameIndex = UINT32_MAX - 1;
        static uint32_t MakeFrameIndex( size_t, uint32_t );

        typedef std::list<std::shared_ptr<DeviceProfilerFrameData>>
            FrameDataList;

        std::shared_mutex m_DataMutex;
        FrameDataList m_pFrames;
        FrameDataList m_pSnapshots;
         // 0 - current frame, 1 - previous frame, etc.
        uint32_t m_SelectedFrameIndex;
        uint32_t m_MaxFrameCount;
        const char* m_pFramesStr;
        const char* m_pFrameStr;
        bool m_HasNewSnapshots;

        std::shared_ptr<DeviceProfilerFrameData> m_pData;
        bool m_Pause;
        bool m_Fullscreen;
        bool m_ShowDebugLabels;
        bool m_ShowShaderCapabilities;
        bool m_ShowEmptyStatistics;
        bool m_ShowAllTopPipelines;
        bool m_ShowActiveFrame;
        bool m_ShowEntryPoints;

        bool GetShowActiveFrame() const;
        const FrameDataList& GetActiveFramesList() const;
        std::shared_ptr<DeviceProfilerFrameData> GetFrameData( uint32_t frameIndex ) const;
        std::string GetFrameName( const char* pContextName, uint32_t frameIndex, bool indent = false ) const;
        std::string GetFrameName( const std::shared_ptr<DeviceProfilerFrameData>& pFrameData, const char* pContextName, uint32_t frameIndex, bool indent = false ) const;

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
        VkProfilerFrameDelimiterEXT m_FrameDelimiter;

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
        std::unordered_set<VkSemaphoreHandle> m_SelectedSemaphores;

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

        // Memory inspector state.
        DeviceProfilerMemoryComparator m_MemoryComparator;
        uint32_t m_MemoryCompareRefFrameIndex;
        uint32_t m_MemoryCompareSelFrameIndex;

        bool m_MemoryConsumptionHistoryVisible;
        bool m_MemoryConsumptionHistoryAutoScroll;
        double m_MemoryConsumptionHistoryTimeRangeMin;
        double m_MemoryConsumptionHistoryTimeRangeMax;
        float m_MemoryConsumptionHistoryUpdatePeriod;
        CpuTimestampCounter m_MemoryConsumptionHistoryUpdateCounter;
        std::vector<float> m_MemoryConsumptionHistoryTimePoints;
        std::vector<float> m_MemoryConsumptionHistory[VK_MAX_MEMORY_HEAPS];
        float m_MemoryConsumptionHistoryMax[VK_MAX_MEMORY_HEAPS];

        std::string m_ResourceBrowserNameFilter;
        VkBufferUsageFlags m_ResourceBrowserBufferUsageFilter;
        VkImageUsageFlags m_ResourceBrowserImageUsageFilter;
        VkFlags m_ResourceBrowserAccelerationStructureTypeFilter;
        VkFlags m_ResourceBrowserMicromapTypeFilter;
        bool m_ResourceBrowserShowDifferences;

        VkBufferHandle m_ResourceInspectorBuffer;
        DeviceProfilerBufferMemoryData m_ResourceInspectorBufferData;

        VkImageHandle m_ResourceInspectorImage;
        DeviceProfilerImageMemoryData m_ResourceInspectorImageData;
        VkImageSubresource m_ResourceInspectorImageMapSubresource;
        float m_ResourceInspectorImageMapBlockSize;

        VkAccelerationStructureKHRHandle m_ResourceInspectorAccelerationStructure;
        DeviceProfilerAccelerationStructureMemoryData m_ResourceInspectorAccelerationStructureData;
        DeviceProfilerBufferMemoryData m_ResourceInspectorAccelerationStructureBufferData;

        VkMicromapEXTHandle m_ResourceInspectorMicromap;
        DeviceProfilerMicromapMemoryData m_ResourceInspectorMicromapData;
        DeviceProfilerBufferMemoryData m_ResourceInspectorMicromapBufferData;

        struct ResourceListExporter;
        std::unique_ptr<ResourceListExporter> m_pResourceListExporter;

        // Performance counters.
        struct PerformanceQueryMetricsSet
            : public std::enable_shared_from_this<PerformanceQueryMetricsSet>
        {
            uint32_t m_MetricsSetIndex;
            bool m_FilterResult;
            std::vector<VkProfilerPerformanceCounterProperties2EXT> m_Metrics;
            VkProfilerPerformanceMetricsSetProperties2EXT m_Properties;
        };

        std::shared_ptr<PerformanceQueryMetricsSet> m_pActivePerformanceQueryMetricsSet;
        std::vector<std::shared_ptr<PerformanceQueryMetricsSet>> m_pPerformanceQueryMetricsSets;
        std::vector<bool> m_ActivePerformanceQueryMetricsFilterResults;
        std::string m_PerformanceQueryMetricsFilter;
        std::regex m_PerformanceQueryMetricsFilterRegex;
        bool m_PerformanceQueryMetricsFilterRegexValid;
        bool m_PerformanceQueryMetricsFilterRegexMode;
        bool m_PerformanceQueryMetricsSetPropertiesExpanded;

        void SelectPerformanceQueryMetricsSet( const std::shared_ptr<PerformanceQueryMetricsSet>& );

        bool CompilePerformanceQueryMetricsFilterRegex();
        void UpdatePerformanceQueryEditorMetricsFilterResults();
        void UpdatePerformanceQueryActiveMetricsFilterResults();
        void UpdatePerformanceQueryMetricsSetFilterResults( const std::shared_ptr<PerformanceQueryMetricsSet>& );
        void UpdatePerformanceQueryMetricsSetsFilterResults();

        // Performance metrics filter.
        // The profiler will show only metrics for the selected command buffer.
        // If no command buffer is selected, the aggregated stats for the whole frame will be displayed.
        struct PerformanceQueryRangeData
        {
            const DeviceProfilerPerformanceCountersData* m_pPerformanceCountersData;
            uint64_t m_BeginTimestamp;
            uint64_t m_EndTimestamp;
        };

        VkCommandBufferHandle m_PerformanceQueryCommandBufferFilter;
        std::string m_PerformanceQueryCommandBufferFilterName;

        std::unordered_map<std::string, VkProfilerPerformanceCounterResultEXT> m_ReferencePerformanceQueryData;

        // Performance counter sets editor.
        std::vector<VkProfilerPerformanceCounterProperties2EXT> m_PerformanceQueryEditorCounterProperties;
        std::vector<uint32_t> m_PerformanceQueryEditorCounterIndices;
        std::vector<uint32_t> m_PerformanceQueryEditorCounterVisibileIndices;
        std::vector<bool> m_PerformanceQueryEditorCounterAvailability;
        std::vector<bool> m_PerformanceQueryEditorCounterAvailabilityKnown;
        std::string m_PerformanceQueryEditorFilter;
        std::string m_PerformanceQueryEditorSetName;
        std::string m_PerformanceQueryEditorSetDescription;

        struct PerformanceQueryMetricsSetExporter;
        std::unique_ptr<PerformanceQueryMetricsSetExporter> m_pPerformanceQueryMetricsSetExporter;

        uint32_t FindPerformanceQueryCounterIndexByUUID( const uint8_t uuid[VK_UUID_SIZE] ) const;
        void SetPerformanceQueryEditorCounterSelected( uint32_t counterIndex, bool selected, bool refresh );
        void RefreshPerformanceQueryEditorCountersSet( bool countersOnly = false );
        std::shared_ptr<PerformanceQueryMetricsSet> CreatePerformanceQueryMetricsSet( const char*, const char*, const std::vector<uint32_t>& );

        // Performance counter serialization
        struct PerformanceQueryExporter;
        std::unique_ptr<PerformanceQueryExporter> m_pPerformanceQueryExporter;

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
        int m_MemoryTabDockSpaceId;
        int m_ResourceBrowserDockSpaceId;
        int m_ResourceInspectorDockSpaceId;

        void PerformanceTabDockSpace( int flags = 0 );
        void MemoryTabDockSpace( int flags = 0 );

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
        std::string GetDefaultPerformanceCountersFileName( const std::shared_ptr<PerformanceQueryMetricsSet>& ) const;
        void UpdatePerformanceCounterExporter();
        void SavePerformanceCountersToFile( const std::string&, const std::shared_ptr<PerformanceQueryMetricsSet>&, const std::vector<VkProfilerPerformanceCounterResultEXT>&, const std::vector<bool>& );

        std::string GetDefaultPerformanceQueryMetricsSetFileName( const std::shared_ptr<PerformanceQueryMetricsSet>& ) const;
        void UpdatePerformanceQueryMetricsSetExporter();
        void SavePerformanceQueryMetricsSetToFile( const std::string&, const std::shared_ptr<PerformanceQueryMetricsSet>& );

        void DrawPerformanceCountersQueryData( const PerformanceQueryRangeData& );
        void DrawPerformanceCountersStreamData( const PerformanceQueryRangeData& );

        // Top pipelines helpers
        void UpdateTopPipelinesExporter();
        void SaveTopPipelinesToFile( const std::string&, const DeviceProfilerFrameData& );

        // Trace serialization helpers
        void UpdateTraceExporter();
        void SaveTraceToFile( const std::string&, const DeviceProfilerFrameData& );

        // Notifications
        void UpdateNotificationWindow();
        void UpdateApplicationInfoWindow();

        // Resource inspector helpers
        void ResetResourceInspector();
        void DrawResourceInspectorAccelerationStructureInfo( VkAccelerationStructureKHRHandle, const DeviceProfilerAccelerationStructureMemoryData&, const DeviceProfilerBufferMemoryData& );
        void DrawResourceInspectorMicromapInfo( VkMicromapEXTHandle, const DeviceProfilerMicromapMemoryData&, const DeviceProfilerBufferMemoryData& );
        void DrawResourceInspectorBufferInfo( VkBufferHandle, const DeviceProfilerBufferMemoryData& );
        void DrawResourceInspectorImageInfo( VkImageHandle, const DeviceProfilerImageMemoryData& );
        void DrawResourceInspectorImageMemoryMap();

        std::string GetDefaultResourceListFileName() const;
        void UpdateResourceListExporter();
        void SaveResourceListToFile( const std::string&, const std::shared_ptr<DeviceProfilerFrameData>&, const std::string&, VkBufferUsageFlags, VkImageUsageFlags, VkFlags, VkFlags );

        // Pipeline inspector helpers
        void Inspect( const DeviceProfilerPipeline& );
        void SelectInspectorShaderStage( size_t );
        void DrawInspectorShaderStage();
        void DrawInspectorPipelineState();
        void DrawInspectorGraphicsPipelineState();
        void DrawInspectorRayTracingPipelineState();
        void SetInspectorTabIndex( size_t );
        void ShaderRepresentationSaved( bool, const std::string& );

        // Frame browser helpers
        void PrintFramesList( const FrameDataList&, uint32_t = 0 );
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

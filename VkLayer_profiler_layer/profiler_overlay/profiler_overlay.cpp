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

#include "profiler_overlay.h"
#include "profiler_overlay_shader_view.h"
#include "profiler/profiler_frontend.h"
#include "profiler_trace/profiler_trace.h"
#include "profiler_helpers/profiler_string_serializer.h"
#include "profiler_helpers/profiler_csv_helpers.h"
#include "profiler_layer_objects/VkObject.h"
#include "profiler_layer_objects/VkQueue_object.h"

#include <string>
#include <sstream>
#include <stack>
#include <fstream>
#include <regex>

#include <imgui_internal.h>
#include <ImGuiFileDialog.h>

#include <fmt/format.h>

#include "utils/lockable_unordered_map.h"

#include "imgui_widgets/imgui_breakdown_ex.h"
#include "imgui_widgets/imgui_histogram_ex.h"
#include "imgui_widgets/imgui_table_ex.h"
#include "imgui_widgets/imgui_ex.h"

// Languages
#include "lang/en_us.h"
#include "lang/pl_pl.h"

#if 1
using Lang = Profiler::DeviceProfilerOverlayLanguage_Base;
#else
using Lang = Profiler::DeviceProfilerOverlayLanguage_PL;
#endif

namespace Profiler
{
    // Define static members
    std::mutex s_ImGuiMutex;

    struct ProfilerOverlayOutput::PerformanceGraphColumn : ImGuiX::HistogramColumnData
    {
        HistogramGroupMode groupMode;
        FrameBrowserTreeNodeIndex nodeIndex;
    };

    struct ProfilerOverlayOutput::QueueGraphColumn : ImGuiX::HistogramColumnData
    {
        enum DataType
        {
            eIdle,
            eCommandBuffer,
            eSignalSemaphores,
            eWaitSemaphores,
        };

        DataType userDataType;
        FrameBrowserTreeNodeIndex nodeIndex;
    };

    struct ProfilerOverlayOutput::PerformanceCounterExporter
    {
        enum class Action
        {
            eExport,
            eImport
        };

        IGFD::FileDialog m_FileDialog;
        IGFD::FileDialogConfig m_FileDialogConfig;
        std::vector<VkProfilerPerformanceCounterResultEXT> m_Data;
        std::vector<bool> m_DataMask;
        uint32_t m_MetricsSetIndex;
        Action m_Action;
    };

    struct ProfilerOverlayOutput::TopPipelinesExporter
    {
        enum class Action
        {
            eExport,
            eImport
        };

        IGFD::FileDialog m_FileDialog;
        IGFD::FileDialogConfig m_FileDialogConfig;
        std::shared_ptr<DeviceProfilerFrameData> m_pData;
        Action m_Action;
    };

    struct ProfilerOverlayOutput::TraceExporter
    {
        IGFD::FileDialog m_FileDialog;
        IGFD::FileDialogConfig m_FileDialogConfig;
        std::shared_ptr<DeviceProfilerFrameData> m_pData;
    };

    static bool DisplayFileDialog(
        const std::string& fileDialogId,
        IGFD::FileDialog& fileDialog,
        IGFD::FileDialogConfig& fileDialogConfig,
        const char* pTitle,
        const char* pFilters )
    {
        // Initialize the file dialog on the first call to this function.
        if( !fileDialog.IsOpened() )
        {
            // Set initial size and position of the dialog.
            ImGuiIO& io = ImGui::GetIO();
            ImVec2 size = io.DisplaySize;
            float scale = io.FontGlobalScale;
            size.x = std::min( size.x / 1.5f, 640.f * scale );
            size.y = std::min( size.y / 1.25f, 480.f * scale );
            ImGui::SetNextWindowSize( size );

            ImVec2 pos = io.DisplaySize;
            pos.x = ( pos.x - size.x ) / 2.0f;
            pos.y = ( pos.y - size.y ) / 2.0f;
            ImGui::SetNextWindowPos( pos );

            fileDialog.OpenDialog(
                fileDialogId,
                pTitle,
                pFilters,
                fileDialogConfig );
        }

        // Display the file dialog until user closes it.
        return fileDialog.Display(
            fileDialogId,
            ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoSavedSettings );
    }

    template<typename T, typename U>
    static float CalcPerformanceCounterDelta( T ref, U val )
    {
        if( ref != 0 )
        {
            return 100.f * ( static_cast<float>( val ) - static_cast<float>( ref ) ) / static_cast<float>( ref );
        }
        else if( val != 0 )
        {
            return ( val > 0 ) ? 100.f : -100.f;
        }
        else
        {
            return 0.f;
        }
    }

    static ImU32 GetPerformanceCounterDeltaColor( float delta )
    {
        float deltaAbs = std::abs( delta );
        if( deltaAbs < 1.f ) return IM_COL32( 128, 128, 128, 255 );
        if( deltaAbs < 5.f ) return IM_COL32( 192, 192, 192, 255 );
        if( deltaAbs < 15.f ) return IM_COL32( 255, 255, 255, 255 );
        if( deltaAbs < 30.f ) return IM_COL32( 255, 255, 128, 255 );
        if( deltaAbs < 50.f ) return IM_COL32( 255, 192, 128, 255 );
        return IM_COL32( 255, 128, 128, 255 );
    }

    void ProfilerOverlayOutput::FrameBrowserTreeNodeIndex::SetFrameIndex( uint32_t frameIndex )
    {
        if( size() < 2 ) resize( 2 );
        uint16_t* pIndex = data();
        pIndex[0] = static_cast<uint16_t>( frameIndex & 0xffff );
        pIndex[1] = static_cast<uint16_t>( ( frameIndex >> 16 ) & 0xffff );
    }

    uint32_t ProfilerOverlayOutput::FrameBrowserTreeNodeIndex::GetFrameIndex() const
    {
        if( size() < 2 ) return 0;
        const uint16_t* pIndex = data();
        return ( static_cast<uint32_t>( pIndex[0] ) ) |
               ( static_cast<uint32_t>( pIndex[1] ) << 16 );
    }

    const uint16_t* ProfilerOverlayOutput::FrameBrowserTreeNodeIndex::GetTreeNodeIndex() const
    {
        if( size() < 2 ) return nullptr;
        return data() + 2;
    }

    size_t ProfilerOverlayOutput::FrameBrowserTreeNodeIndex::GetTreeNodeIndexSize() const
    {
        if( size() < 2 ) return 0;
        return size() - 2;
    }

    /***********************************************************************************\

    Function:
        ProfilerOverlayOutput

    Description:
        Constructor.

    \***********************************************************************************/
    ProfilerOverlayOutput::ProfilerOverlayOutput( DeviceProfilerFrontend& frontend, OverlayBackend& backend )
        : DeviceProfilerOutput( frontend )
        , m_Backend( backend )
        , m_InspectorShaderView( m_Resources )
        , m_pLastMainWindowPos( m_Settings.AddFloat2( "LastMainWindowPos", Float2() ) )
        , m_pLastMainWindowSize( m_Settings.AddFloat2( "LastMainWindowSize", Float2() ) )
        , m_PerformanceWindowState{ m_Settings.AddBool( "PerformanceWindowOpen", true ), true }
        , m_QueueUtilizationWindowState{ m_Settings.AddBool( "QueueUtilizationWindowOpen", true ), true }
        , m_TopPipelinesWindowState{ m_Settings.AddBool( "TopPipelinesWindowOpen", true ), true }
        , m_PerformanceCountersWindowState{ m_Settings.AddBool( "PerformanceCountersWindowOpen", true ), true }
        , m_MemoryWindowState{ m_Settings.AddBool( "MemoryWindowOpen", true ), true }
        , m_InspectorWindowState{ m_Settings.AddBool( "InspectorWindowOpen", true ), true }
        , m_StatisticsWindowState{ m_Settings.AddBool( "StatisticsWindowOpen", true ), true }
        , m_SettingsWindowState{ m_Settings.AddBool( "SettingsWindowOpen", true ), true }
    {
        ResetMembers();
    }

    /***********************************************************************************\

    Function:
        ~ProfilerOverlayOutput

    Description:
        Destructor.

    \***********************************************************************************/
    ProfilerOverlayOutput::~ProfilerOverlayOutput()
    {
    }

    /***********************************************************************************\

    Function:
        Initialize

    Description:
        Initializes profiler overlay.

    \***********************************************************************************/
    bool ProfilerOverlayOutput::Initialize()
    {
        bool success = true;

        const VkPhysicalDeviceProperties& deviceProperties = m_Frontend.GetPhysicalDeviceProperties();

        // Set main window title
        m_Title = fmt::format( "{0} - {1}###VkProfiler",
            Lang::WindowName,
            deviceProperties.deviceName );

        // Get timestamp query period
        m_TimestampPeriod = Nanoseconds( deviceProperties.limits.timestampPeriod );

        // Init ImGui
        if( success )
        {
            std::scoped_lock lk( s_ImGuiMutex );
            IMGUI_CHECKVERSION();
            m_pImGuiContext = ImGui::CreateContext();

            ImGui::SetCurrentContext( m_pImGuiContext );

            // Register settings handler to the new context
            m_Settings.InitializeHandlers();

            ImGuiIO& io = ImGui::GetIO();
            io.DisplaySize = m_Backend.GetRenderArea();
            io.DeltaTime = 1.0f / 60.0f;
            io.IniFilename = "VK_LAYER_profiler_imgui.ini";
            io.ConfigFlags = ImGuiConfigFlags_DockingEnable;

            m_Settings.Validate( io.IniFilename );
            ImGui::LoadIniSettingsFromDisk( io.IniFilename );

            m_Resources.InitializeFonts();
            InitializeImGuiStyle();

            // Initialize ImGui window size and position
            if( m_pLastMainWindowSize->x != 0 || m_pLastMainWindowSize->y != 0 )
            {
                m_SetLastMainWindowPos = true;
            }

            // Initialize ImGui backends
            success = m_Backend.PrepareImGuiBackend();
        }

        if( success )
        {
            // Initialize backend-dependent config
            float dpiScale = m_Backend.GetDPIScale();
            ImGuiIO& io = ImGui::GetIO();
            io.FontGlobalScale = (dpiScale > 1e-3f) ? dpiScale : 1.0f;

            // Initialize resources
            success = m_Resources.InitializeImages( &m_Backend );
        }

        // Get vendor metrics sets
        if( success )
        {
            const std::vector<VkProfilerPerformanceMetricsSetPropertiesEXT>& metricsSets =
                m_Frontend.GetPerformanceMetricsSets();

            const uint32_t vendorMetricsSetCount = static_cast<uint32_t>( metricsSets.size() );
            m_VendorMetricsSets.reserve( vendorMetricsSetCount );
            m_VendorMetricsSetVisibility.reserve( vendorMetricsSetCount );

            for( uint32_t i = 0; i < vendorMetricsSetCount; ++i )
            {
                VendorMetricsSet& metricsSet = m_VendorMetricsSets.emplace_back();
                memcpy( &metricsSet.m_Properties, &metricsSets[i], sizeof( metricsSet.m_Properties ) );

                // Get metrics belonging to this set.
                metricsSet.m_Metrics = m_Frontend.GetPerformanceCounterProperties( i );

                m_VendorMetricsSetVisibility.push_back( true );
            }

            m_ActiveMetricsSetIndex = m_Frontend.GetPerformanceMetricsSetIndex();

            if( m_ActiveMetricsSetIndex < m_VendorMetricsSets.size() )
            {
                m_ActiveMetricsVisibility.resize(
                    m_VendorMetricsSets[ m_ActiveMetricsSetIndex ].m_Metrics.size(), true );
            }
        }

        // Initialize the disassembler in the shader view
        if( success )
        {
            m_InspectorShaderView.Initialize( m_Frontend );
            m_InspectorShaderView.SetShaderSavedCallback( std::bind(
                &ProfilerOverlayOutput::ShaderRepresentationSaved,
                this,
                std::placeholders::_1,
                std::placeholders::_2 ) );
        }

        // Initialize serializer
        if( success )
        {
            m_pStringSerializer.reset( new (std::nothrow) DeviceProfilerStringSerializer( m_Frontend ) );
            success = (m_pStringSerializer != nullptr);
        }

        // Initialize settings
        if( success )
        {
            m_SamplingMode = m_Frontend.GetProfilerSamplingMode();
            m_FrameDelimiter = m_Frontend.GetProfilerFrameDelimiter();

            switch( m_FrameDelimiter )
            {
            case VK_PROFILER_FRAME_DELIMITER_PRESENT_EXT:
                m_pFrameStr = Lang::Frame;
                m_pFramesStr = Lang::Frames;
                break;

            case VK_PROFILER_FRAME_DELIMITER_SUBMIT_EXT:
                m_pFrameStr = Lang::Submit;
                m_pFramesStr = Lang::Submits;
                break;
            }
        }

        // Initialize the overlay according to the configuration
        if( success )
        {
            const DeviceProfilerConfig& config = m_Frontend.GetProfilerConfig();
            
            if( !config.m_RefMetrics.empty() )
            {
                LoadPerformanceCountersFromFile( config.m_RefMetrics );
            }

            if( !config.m_RefPipelines.empty() )
            {
                LoadTopPipelinesFromFile( config.m_RefPipelines );
            }

            SetMaxFrameCount( std::max( config.m_FrameCount, 0 ) );
        }

        // Don't leave object in partly-initialized state if something went wrong
        if( !success )
        {
            Destroy();
        }

        return success;
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:
        Destructor.

    \***********************************************************************************/
    void ProfilerOverlayOutput::Destroy()
    {
        if( m_pImGuiContext )
        {
            std::scoped_lock imGuiLock( s_ImGuiMutex );
            ImGui::SetCurrentContext( m_pImGuiContext );

            // Destroy resources created for the ImGui overlay.
            m_Resources.Destroy();

            // Destroy ImGui backends.
            m_Backend.DestroyImGuiBackend();

            ImGui::DestroyContext();
        }

        // Reset members to initial values
        ResetMembers();
    }

    /***********************************************************************************\

    Function:
        ResetMembers

    Description:
        Set all members to initial values.

    \***********************************************************************************/
    void ProfilerOverlayOutput::ResetMembers()
    {
        m_pImGuiContext = nullptr;

        m_Title.clear();

        m_ActiveMetricsSetIndex = UINT32_MAX;
        m_VendorMetricsSetVisibility.clear();
        m_VendorMetricsSets.clear();
        memset( m_VendorMetricFilter, 0, sizeof( m_VendorMetricFilter ) );

        m_TimestampPeriod = Milliseconds( 0 );
        m_TimestampDisplayUnit = 1.0f;
        m_pTimestampDisplayUnitStr = Lang::Milliseconds;

        m_FrameBrowserSortMode = FrameBrowserSortMode::eSubmissionOrder;

        m_HistogramGroupMode = HistogramGroupMode::eRenderPass;
        m_HistogramValueMode = HistogramValueMode::eDuration;
        m_HistogramShowIdle = false;

        m_pFrames.clear();
        m_SelectedFrameIndex = 0;
        m_MaxFrameCount = 1;

        m_pFrameStr = Lang::Frame;
        m_pFramesStr = Lang::Frames;

        m_pData = nullptr;
        m_Pause = false;
        m_Fullscreen = false;
        m_ShowDebugLabels = true;
        m_ShowShaderCapabilities = true;
        m_ShowEmptyStatistics = false;
        m_ShowAllTopPipelines = false;
        m_ShowActiveFrame = false;

        m_SetLastMainWindowPos = false;

        m_FrameTime = 0.0f;

        m_TimeUnit = TimeUnit::eMilliseconds;
        m_SamplingMode = VK_PROFILER_MODE_PER_DRAWCALL_EXT;
        m_FrameDelimiter = VK_PROFILER_FRAME_DELIMITER_PRESENT_EXT;

        m_SelectedFrameBrowserNodeIndex = { 0, 0, 0xFFFF };
        m_ScrollToSelectedFrameBrowserNode = false;
        m_FrameBrowserNodeIndexStr.clear();
        m_SelectionUpdateTimestamp = std::chrono::high_resolution_clock::time_point();
        m_SerializationFinishTimestamp = std::chrono::high_resolution_clock::time_point();

        m_SelectedSemaphores.clear();

        m_InspectorPipeline = DeviceProfilerPipeline();
        m_InspectorShaderView.Clear();
        m_InspectorTabs.clear();
        m_InspectorTabIndex = 0;

        m_PerformanceQueryCommandBufferFilter = VK_NULL_HANDLE;
        m_PerformanceQueryCommandBufferFilterName = m_pFrameStr;
        m_ReferencePerformanceCounters.clear();
        m_pPerformanceCounterExporter = nullptr;

        m_pTopPipelinesExporter = nullptr;
        m_ReferenceTopPipelines.clear();
        m_ReferenceTopPipelinesShortDescription.clear();
        m_ReferenceTopPipelinesFullDescription.clear();

        m_SerializationSucceeded = false;
        m_SerializationWindowVisible = false;
        m_SerializationMessage.clear();
        m_SerializationOutputWindowSize = { 0, 0 };
        m_SerializationOutputWindowDuration = std::chrono::seconds( 4 );
        m_SerializationOutputWindowFadeOutDuration = std::chrono::seconds( 1 );

        m_pTraceExporter = nullptr;

        m_RenderPassColumnColor = 0;
        m_GraphicsPipelineColumnColor = 0;
        m_ComputePipelineColumnColor = 0;
        m_RayTracingPipelineColumnColor = 0;
        m_InternalPipelineColumnColor = 0;

        m_pStringSerializer = nullptr;

        m_MainDockSpaceId = 0;
        m_PerformanceTabDockSpaceId = 0;
        m_QueueUtilizationTabDockSpaceId = 0;
        m_TopPipelinesTabDockSpaceId = 0;
        m_FrameBrowserDockSpaceId = 0;
    }

    /***********************************************************************************\

    Function:
        IsAvailable

    Description:
        Check if profiler overlay is ready for presenting.

    \***********************************************************************************/
    bool ProfilerOverlayOutput::IsAvailable()
    {
        return (m_pImGuiContext != nullptr);
    }

    /***********************************************************************************\

    Function:
        SetMaxFrameCount

    Description:
        Set maximum number of frames to be displayed in the overlay.

    \***********************************************************************************/
    void ProfilerOverlayOutput::SetMaxFrameCount( uint32_t maxFrameCount )
    {
        m_MaxFrameCount = maxFrameCount;

        // Update buffers in the frontend to avoid dropping data.
        m_Frontend.SetDataBufferSize( maxFrameCount + 1 );
    }

    /***********************************************************************************\

    Function:
        Update

    Description:
        Consume available data from the frontend.

    \***********************************************************************************/
    void ProfilerOverlayOutput::Update()
    {
        std::scoped_lock lk( m_DataMutex );

        // Update data
        if( !m_Pause || m_pFrames.empty() )
        {
            auto pData = m_Frontend.GetData();
            if( pData )
            {
                m_pFrames.push_back( pData );
            }
        }

        if( m_MaxFrameCount )
        {
            while( m_pFrames.size() > m_MaxFrameCount )
            {
                m_pFrames.pop_front();
            }
        }

        m_SelectedFrameIndex = std::min<uint32_t>( m_SelectedFrameIndex, m_pFrames.size() - 1 );
        m_pData = GetNthElement( m_pFrames, m_pFrames.size() - m_SelectedFrameIndex - 1 );

        m_FrameTime = GetDuration( 0, m_pData->m_Ticks );
    }

    /***********************************************************************************\

    Function:
        Present

    Description:
        Draw profiler overlay before presenting the image to screen.

    \***********************************************************************************/
    void ProfilerOverlayOutput::Present()
    {
        std::scoped_lock lk( s_ImGuiMutex );
        ImGui::SetCurrentContext( m_pImGuiContext );

        // Must be set before calling NewFrame to avoid clipping on window resize.
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = m_Backend.GetRenderArea();

        if( !m_Backend.NewFrame() )
        {
            return;
        }

        ImGui::NewFrame();

        // Prevent data modification during presentation.
        std::shared_lock dataLock( m_DataMutex );

        // Initialize IDs of the popup windows before entering the main window scope
        uint32_t applicationInfoPopupID = ImGui::GetID( Lang::ApplicationInfo );

        // Configure main window
        int mainWindowFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_MenuBar;

        float defaultWindowRounding = ImGui::GetStyle().WindowRounding;

        bool fullscreen = m_Fullscreen;
        if( fullscreen )
        {
            // Disable title bar and resizing in fullscreen mode
            mainWindowFlags |=
                ImGuiWindowFlags_NoDecoration |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoBringToFrontOnFocus;

            // Fix position and size of the window
            ImGui::SetNextWindowPos( ImVec2( 0, 0 ) );
            ImGui::SetNextWindowSize( ImGui::GetIO().DisplaySize );

            // Disable rounding
            ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, 0.f );
        }
        else
        {
            if( m_SetLastMainWindowPos )
            {
                ImGui::SetNextWindowPos( *m_pLastMainWindowPos );
                ImGui::SetNextWindowSize( *m_pLastMainWindowSize );

                m_SetLastMainWindowPos = false;
                *m_pLastMainWindowPos = Float2();
                *m_pLastMainWindowSize = Float2();
            }
        }

        // Begin main window
        ImGui::PushFont( m_Resources.GetDefaultFont() );
        ImGui::Begin( m_Title.c_str(), nullptr, mainWindowFlags );

        if( !m_Fullscreen )
        {
            // Save current window position and size to restore it when user exits fullscreen mode
            *m_pLastMainWindowPos = ImGui::GetWindowPos();
            *m_pLastMainWindowSize = ImGui::GetWindowSize();
        }

        if( m_Fullscreen )
        {
            // Keep the main window always at the back when in fullscreen mode
            ImGui::BringWindowToDisplayBack( ImGui::GetCurrentWindow() );
        }

        if( ImGui::BeginMenuBar() )
        {
            if( ImGui::BeginMenu( Lang::FileMenu ) )
            {
                if( ImGui::MenuItem( Lang::SaveTrace ) )
                {
                    m_pTraceExporter = std::make_unique<TraceExporter>();
                    m_pTraceExporter->m_pData = m_pData;
                }
                ImGui::EndMenu();
            }

            if( ImGui::BeginMenu( Lang::WindowMenu ) )
            {
                if( ImGui::MenuItem( Lang::Fullscreen, nullptr, &m_Fullscreen ) )
                {
                    // Restore pre-fullscreen position and size
                    m_SetLastMainWindowPos = !m_Fullscreen;
                }

                ImGui::Separator();
                ImGui::MenuItem( Lang::PerformanceMenuItem, nullptr, m_PerformanceWindowState.pOpen );
                ImGui::MenuItem( Lang::QueueUtilizationMenuItem, nullptr, m_QueueUtilizationWindowState.pOpen );
                ImGui::MenuItem( Lang::TopPipelinesMenuItem, nullptr, m_TopPipelinesWindowState.pOpen );
                ImGui::MenuItem( Lang::PerformanceCountersMenuItem, nullptr, m_PerformanceCountersWindowState.pOpen );
                ImGui::MenuItem( Lang::MemoryMenuItem, nullptr, m_MemoryWindowState.pOpen );
                ImGui::MenuItem( Lang::InspectorMenuItem, nullptr, m_InspectorWindowState.pOpen );
                ImGui::MenuItem( Lang::StatisticsMenuItem, nullptr, m_StatisticsWindowState.pOpen );
                ImGui::MenuItem( Lang::SettingsMenuItem, nullptr, m_SettingsWindowState.pOpen );
                ImGui::EndMenu();
            }

            if( ImGui::MenuItem( Lang::ApplicationInfoMenuItem ) )
            {
                ImGui::OpenPopup( applicationInfoPopupID );
            }

            ImGui::EndMenuBar();
        }

        // Save results to file
        if( ImGui::Button( Lang::SaveTrace ) )
        {
            m_pTraceExporter = std::make_unique<TraceExporter>();
            m_pTraceExporter->m_pData = m_pData;
        }
        if( ImGui::IsItemHovered() )
        {
            ImGui::SetTooltip( "Save trace of the current %s to file", m_pFrameStr );
        }

        // Keep results
        ImGui::SameLine();
        ImGui::Checkbox( Lang::Pause, &m_Pause );

        const VkApplicationInfo& applicationInfo = m_Frontend.GetApplicationInfo();
        ImGuiX::TextAlignRight( "Vulkan %u.%u",
            VK_API_VERSION_MAJOR( applicationInfo.apiVersion ),
            VK_API_VERSION_MINOR( applicationInfo.apiVersion ) );

        // Add padding
        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + 5 );

        m_MainDockSpaceId = ImGui::GetID( "##m_MainDockSpaceId" );
        m_PerformanceTabDockSpaceId = ImGui::GetID( "##m_PerformanceTabDockSpaceId_2" );

        ImU32 defaultWindowBg = ImGui::GetColorU32( ImGuiCol_WindowBg );
        ImU32 defaultTitleBg = ImGui::GetColorU32( ImGuiCol_TitleBg );
        ImU32 defaultTitleBgActive = ImGui::GetColorU32( ImGuiCol_TitleBgActive );
        int numPushedColors = 0;
        int numPushedVars = 0;
        bool isOpen = false;
        bool isExpanded = false;

        auto BeginDockingWindow = [&]( const char* pTitle, int dockSpaceId, WindowState& state )
            {
                isExpanded = false;
                if( isOpen = (!state.pOpen || *state.pOpen) )
                {
                    if( !state.Docked )
                    {
                        ImGui::PushStyleColor( ImGuiCol_WindowBg, defaultWindowBg );
                        ImGui::PushStyleColor( ImGuiCol_TitleBg, defaultTitleBg );
                        ImGui::PushStyleColor( ImGuiCol_TitleBgActive, defaultTitleBgActive );
                        numPushedColors = 3;

                        ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, defaultWindowRounding );
                        numPushedVars = 1;
                    }

                    if( state.Focus )
                    {
                        ImGui::SetNextWindowFocus();
                    }

                    ImGui::SetNextWindowDockID( dockSpaceId, ImGuiCond_FirstUseEver );

                    isExpanded = ImGui::Begin( pTitle, state.pOpen );

                    ImGuiID dockSpaceId = ImGuiX::GetWindowDockSpaceID();
                    state.Docked = ImGui::IsWindowDocked() &&
                        (dockSpaceId == m_MainDockSpaceId ||
                            dockSpaceId == m_PerformanceTabDockSpaceId);

                    state.Focus = false;
                }
                return isExpanded;
            };

        auto EndDockingWindow = [&]()
            {
                if( isOpen )
                {
                    ImGui::End();
                    ImGui::PopStyleColor( numPushedColors );
                    ImGui::PopStyleVar( numPushedVars );
                    numPushedColors = 0;
                    numPushedVars = 0;
                }
            };

        ImU32 transparentColor = ImGui::GetColorU32( { 0, 0, 0, 0 } );
        ImGui::PushStyleColor( ImGuiCol_WindowBg, transparentColor );
        ImGui::PushStyleColor( ImGuiCol_TitleBg, transparentColor );
        ImGui::PushStyleColor( ImGuiCol_TitleBgActive, transparentColor );

        ImGui::DockSpace( m_MainDockSpaceId );

        if( BeginDockingWindow( Lang::Performance, m_MainDockSpaceId, m_PerformanceWindowState ) )
        {
            UpdatePerformanceTab();
        }
        else
        {
            PerformanceTabDockSpace( ImGuiDockNodeFlags_KeepAliveOnly );
        }
        EndDockingWindow();

        if( BeginDockingWindow( Lang::QueueUtilization, m_PerformanceTabDockSpaceId, m_QueueUtilizationWindowState ) )
        {
            UpdateQueueUtilizationTab();
        }
        EndDockingWindow();

        // Top pipelines
        if( BeginDockingWindow( Lang::TopPipelines, m_PerformanceTabDockSpaceId, m_TopPipelinesWindowState ) )
        {
            UpdateTopPipelinesTab();
        }
        EndDockingWindow();

        if( BeginDockingWindow( Lang::PerformanceCounters, m_PerformanceTabDockSpaceId, m_PerformanceCountersWindowState ) )
        {
            UpdatePerformanceCountersTab();
        }
        EndDockingWindow();

        if( BeginDockingWindow( Lang::Memory, m_MainDockSpaceId, m_MemoryWindowState ) )
        {
            UpdateMemoryTab();
        }
        EndDockingWindow();

        if( BeginDockingWindow( Lang::Inspector, m_MainDockSpaceId, m_InspectorWindowState ) )
        {
            UpdateInspectorTab();
        }
        EndDockingWindow();

        if( BeginDockingWindow( Lang::Statistics, m_MainDockSpaceId, m_StatisticsWindowState ) )
        {
            UpdateStatisticsTab();
        }
        EndDockingWindow();

        if( BeginDockingWindow( Lang::Settings, m_MainDockSpaceId, m_SettingsWindowState ) )
        {
            UpdateSettingsTab();
        }
        EndDockingWindow();

        ImGui::PopStyleColor( 3 );
        ImGui::End();

        if( fullscreen )
        {
            // Re-enable window rounding
            ImGui::PopStyleVar();
        }

        // Draw other windows
        UpdatePerformanceCounterExporter();
        UpdateTopPipelinesExporter();
        UpdateTraceExporter();
        UpdateNotificationWindow();
        UpdateApplicationInfoWindow();

        // Data not used from now on
        dataLock.unlock();

        // Set initial tab
        if( ImGui::GetFrameCount() == 1 )
        {
            ImGui::SetWindowFocus( Lang::Performance );
        }

        // Draw foreground overlay
        ImDrawList* pForegroundDrawList = ImGui::GetForegroundDrawList();
        if( pForegroundDrawList != nullptr )
        {
            // Draw cursor pointer in case the application doesn't render one.
            // It is also needed when the app uses raw input because relative movements may be
            // translated differently by the application and by the layer.
            pForegroundDrawList->AddCircleFilled( ImGui::GetIO().MousePos, 2.f, 0xffffffff, 4 );
        }

        ImGui::PopFont();
        ImGui::Render();

        m_Backend.RenderDrawData( ImGui::GetDrawData() );
    }

    /***********************************************************************************\

    Function:
        InitializeImGuiColors

    Description:

    \***********************************************************************************/
    void ProfilerOverlayOutput::InitializeImGuiStyle()
    {
        ImGui::StyleColorsDark();

        ImGuiStyle& style = ImGui::GetStyle();
        // Round window corners
        style.WindowRounding = 7.f;

        // Performance graph colors
        m_RenderPassColumnColor = ImGui::GetColorU32( { 0.9f, 0.7f, 0.0f, 1.0f } ); // #e6b200
        m_GraphicsPipelineColumnColor = ImGui::GetColorU32( { 0.9f, 0.7f, 0.0f, 1.0f } ); // #e6b200
        m_ComputePipelineColumnColor = ImGui::GetColorU32( { 0.9f, 0.55f, 0.0f, 1.0f } ); // #ffba42
        m_RayTracingPipelineColumnColor = ImGui::GetColorU32( { 0.2f, 0.73f, 0.92f, 1.0f } ); // #34baeb
        m_InternalPipelineColumnColor = ImGui::GetColorU32( { 0.5f, 0.22f, 0.9f, 1.0f } ); // #9e30ff

        m_InspectorShaderView.InitializeStyles();
    }

    /***********************************************************************************\

    Function:
        PerformanceTabDockSpace

    Description:
        Defines dock spaces of the "Performance" tab.

    \***********************************************************************************/
    void ProfilerOverlayOutput::PerformanceTabDockSpace( int flags )
    {
        bool requiresInitialization = ( ImGui::DockBuilderGetNode( m_PerformanceTabDockSpaceId ) == nullptr );
        ImGui::DockSpace( m_PerformanceTabDockSpaceId, ImVec2( 0, 0 ), flags );

        if( requiresInitialization )
        {
            ImGui::DockBuilderRemoveNode( m_PerformanceTabDockSpaceId );
            ImGui::DockBuilderAddNode( m_PerformanceTabDockSpaceId, ImGuiDockNodeFlags_None );
            ImGui::DockBuilderSetNodeSize( m_PerformanceTabDockSpaceId, ImGui::GetMainViewport()->Size );

            ImGuiID dockMain = m_PerformanceTabDockSpaceId;
            ImGuiID dockLeft;
            ImGui::DockBuilderSplitNode( dockMain, ImGuiDir_Left, 0.3f, &dockLeft, &dockMain );
            ImGuiID dockQueueUtilization, dockTopPipelines;
            ImGui::DockBuilderSplitNode( dockMain, ImGuiDir_Up, 0.12f, &dockQueueUtilization, &dockMain );
            ImGui::DockBuilderSplitNode( dockMain, ImGuiDir_Up, 0.2f, &dockTopPipelines, &dockMain );
            ImGuiID dockFrames;
            ImGui::DockBuilderSplitNode( dockLeft, ImGuiDir_Up, 0.2f, &dockFrames, &dockLeft );

            ImGui::DockBuilderDockWindow( m_pFramesStr, dockFrames );
            ImGui::DockBuilderDockWindow( Lang::QueueUtilization, dockQueueUtilization );
            ImGui::DockBuilderDockWindow( Lang::TopPipelines, dockTopPipelines );
            ImGui::DockBuilderDockWindow( Lang::FrameBrowser, dockLeft );
            ImGui::DockBuilderDockWindow( Lang::PerformanceCounters, dockMain );
            ImGui::DockBuilderFinish( m_PerformanceTabDockSpaceId );
        }
    }

    /***********************************************************************************\

    Function:
        UpdatePerformanceTab

    Description:
        Updates "Performance" tab.

    \***********************************************************************************/
    void ProfilerOverlayOutput::UpdatePerformanceTab()
    {
        // Header
        {
            const uint64_t cpuTimestampFreq = OSGetTimestampFrequency( m_pData->m_SyncTimestamps.m_HostTimeDomain );
            const Milliseconds gpuTimeMs = m_pData->m_Ticks * m_TimestampPeriod;
            const Milliseconds cpuTimeMs = std::chrono::nanoseconds(
                ((m_pData->m_CPU.m_EndTimestamp - m_pData->m_CPU.m_BeginTimestamp) * 1'000'000'000) / cpuTimestampFreq );

            ImGui::Text( "%s: %.2f ms", Lang::GPUTime, gpuTimeMs.count() );
            ImGui::PushStyleColor( ImGuiCol_Text, { 0.55f, 0.55f, 0.55f, 1.0f } );
            ImGuiX::TextAlignRight( "%s %u", m_pFrameStr, m_pData->m_CPU.m_FrameIndex );
            ImGui::PopStyleColor();
            ImGui::Text( "%s: %.2f ms", Lang::CPUTime, cpuTimeMs.count() );
            ImGuiX::TextAlignRight( "%.1f %s", m_pData->m_CPU.m_FramesPerSec, Lang::FPS );
        }

        // Histogram
        {
            static const char* groupOptions[] = {
                m_pFramesStr,
                Lang::RenderPasses,
                Lang::Pipelines,
                Lang::Drawcalls };

            const float interfaceScale = ImGui::GetIO().FontGlobalScale;

            // Select group mode
            {
                if( ImGui::BeginCombo( Lang::HistogramGroups, nullptr, ImGuiComboFlags_NoPreview ) )
                {
                    ImGuiX::TSelectable( m_pFramesStr, m_HistogramGroupMode, HistogramGroupMode::eFrame );

                    ImGui::BeginDisabled( m_SamplingMode > VK_PROFILER_MODE_PER_RENDER_PASS_EXT );
                    ImGuiX::TSelectable( Lang::RenderPasses, m_HistogramGroupMode, HistogramGroupMode::eRenderPass );
                    ImGui::EndDisabled();

                    ImGui::BeginDisabled( m_SamplingMode > VK_PROFILER_MODE_PER_PIPELINE_EXT );
                    ImGuiX::TSelectable( Lang::Pipelines, m_HistogramGroupMode, HistogramGroupMode::ePipeline );
                    ImGui::EndDisabled();

                    ImGui::BeginDisabled( m_SamplingMode > VK_PROFILER_MODE_PER_DRAWCALL_EXT );
                    ImGuiX::TSelectable( Lang::Drawcalls, m_HistogramGroupMode, HistogramGroupMode::eDrawcall );
                    ImGui::EndDisabled();

                    ImGui::EndCombo();
                }

                ImGui::SameLine( 0, 20 * interfaceScale );
                ImGui::PushItemWidth( 100 * interfaceScale );

                if( ImGui::BeginCombo( Lang::Height, nullptr, ImGuiComboFlags_NoPreview ) )
                {
                    ImGuiX::TSelectable( Lang::Constant, m_HistogramValueMode, HistogramValueMode::eConstant );
                    ImGuiX::TSelectable( Lang::Duration, m_HistogramValueMode, HistogramValueMode::eDuration );
                    ImGui::EndCombo();
                }

                ImGui::SameLine( 0, 20 * interfaceScale );
                ImGui::PushItemWidth( 100 * interfaceScale );
                ImGui::Checkbox( Lang::ShowIdle, &m_HistogramShowIdle );

                ImGui::SameLine( 0, 20 * interfaceScale );
                ImGui::PushItemWidth( 100 * interfaceScale );
                ImGui::Checkbox( Lang::ShowActiveFrame, &m_ShowActiveFrame );
            }

            float histogramHeight = (m_HistogramValueMode == HistogramValueMode::eConstant) ? 30.f : 110.0f;
            histogramHeight *= interfaceScale;

            // Enumerate columns for selected group mode
            std::vector<PerformanceGraphColumn> columns;
            GetPerformanceGraphColumns( columns );

            char pHistogramDescription[ 32 ];
            snprintf( pHistogramDescription, sizeof( pHistogramDescription ),
                "%s (%s)",
                Lang::GPUTime,
                groupOptions[(size_t)m_HistogramGroupMode] );

            ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, { 1, 1 } );

            ImGui::PushItemWidth( -1 );
            ImGui::PushStyleColor( ImGuiCol_FrameBg, { 0.0f, 0.0f, 0.0f, 0.0f } );
            ImGuiX::PlotHistogramEx(
                "",
                columns.data(),
                static_cast<int>( columns.size() ),
                0,
                sizeof( columns.front() ),
                pHistogramDescription, 0, FLT_MAX, { 0, histogramHeight },
                ImGuiX::HistogramFlags_None,
                std::bind( &ProfilerOverlayOutput::DrawPerformanceGraphLabel, this, std::placeholders::_1 ),
                std::bind( &ProfilerOverlayOutput::SelectPerformanceGraphColumn, this, std::placeholders::_1 ) );

            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
            ImGui::SetCursorPosY( ImGui::GetCursorPosY() + 5 * interfaceScale );
        }

        PerformanceTabDockSpace();

        // Frames list
        if( ImGui::Begin( m_pFramesStr, nullptr, ImGuiWindowFlags_NoMove ) )
        {
            uint32_t frameIndex = static_cast<uint32_t>( m_pFrames.size() - 1 );

            for( const auto& pFrame : m_pFrames )
            {
                std::string frameName = fmt::format(
                    "{} #{} ({:.2f} {})###Selectable_frame_{}",
                    m_pFrameStr,
                    pFrame->m_CPU.m_FrameIndex,
                    GetDuration( 0, pFrame->m_Ticks ),
                    m_pTimestampDisplayUnitStr,
                    frameIndex );

                bool selected = ( frameIndex == m_SelectedFrameIndex );
                if( ImGui::Selectable( frameName.c_str(), &selected, ImGuiSelectableFlags_SpanAvailWidth ) )
                {
                    m_SelectedFrameIndex = frameIndex;
                }

                frameIndex--;
            }
        }

        ImGui::End();

        // m_pData may be temporarily replaced by another frame to correctly scroll to its node.
        // Save pointer to the current frame to restore it later.
        std::shared_ptr<DeviceProfilerFrameData> pCurrentFrameData = m_pData;

        // Force frame browser open
        if( m_ScrollToSelectedFrameBrowserNode )
        {
            ImGui::SetNextItemOpen( true );

            // Update frame index when scrolling to a node from a different frame
            m_SelectedFrameIndex = std::clamp<uint32_t>(
                m_SelectedFrameBrowserNodeIndex.GetFrameIndex(), 0, m_pFrames.size() - 1 );

            // Temporarily replace pointer to the current frame data
            m_pData = GetNthElement( m_pFrames, m_SelectedFrameIndex );
        }

        // Frame browser
        if( ImGui::Begin( Lang::FrameBrowser, nullptr, ImGuiWindowFlags_NoMove ) )
        {
            // Select sort mode
            {
                static const char* sortOptions[] = {
                    Lang::SubmissionOrder,
                    Lang::DurationDescending,
                    Lang::DurationAscending };

                const char* selectedOption = sortOptions[(size_t)m_FrameBrowserSortMode];

                ImGui::Text( Lang::Sort );
                ImGui::SameLine();

                if( ImGui::BeginCombo( "##FrameBrowserSortMode", selectedOption ) )
                {
                    for( size_t i = 0; i < std::extent_v<decltype( sortOptions )>; ++i )
                    {
                        if( ImGuiX::TSelectable( sortOptions[i], selectedOption, sortOptions[i] ) )
                        {
                            // Selection changed
                            m_FrameBrowserSortMode = FrameBrowserSortMode( i );
                        }
                    }

                    ImGui::EndCombo();
                }
            }

            FrameBrowserTreeNodeIndex index;
            index.SetFrameIndex( m_SelectedFrameIndex );

            ImGui::Text( "%s #%u", m_pFrameStr, m_pData->m_CPU.m_FrameIndex );
            PrintDuration( m_pData->m_BeginTimestamp, m_pData->m_EndTimestamp );

            index.emplace_back( 0 );

            // Enumerate submits in frame
            for( const auto& submitBatch : m_pData->m_Submits )
            {
                const std::string queueName = m_pStringSerializer->GetName( submitBatch.m_Handle );

                if( ScrollToSelectedFrameBrowserNode( index ) )
                {
                    ImGui::SetNextItemOpen( true );
                }

                const char* indexStr = GetFrameBrowserNodeIndexStr( index );
                if( ImGui::TreeNode( indexStr, "vkQueueSubmit(%s, %u)",
                        queueName.c_str(),
                        static_cast<uint32_t>( submitBatch.m_Submits.size() ) ) )
                {
                    index.emplace_back( 0 );

                    for( const auto& submit : submitBatch.m_Submits )
                    {
                        if( ScrollToSelectedFrameBrowserNode( index ) )
                        {
                            ImGui::SetNextItemOpen( true );
                        }

                        const char* indexStr = GetFrameBrowserNodeIndexStr( index );
                        const bool inSubmitSubtree =
                            ( submitBatch.m_Submits.size() > 1 ) &&
                            ( ImGui::TreeNode( indexStr, "VkSubmitInfo #%u", index.back() ) );

                        if( ( inSubmitSubtree ) || ( submitBatch.m_Submits.size() == 1 ) )
                        {
                            index.emplace_back( 0 );

                            // Sort frame browser data
                            std::list<const DeviceProfilerCommandBufferData*> pCommandBuffers =
                                SortFrameBrowserData( submit.m_CommandBuffers );

                            // Enumerate command buffers in submit
                            for( const auto* pCommandBuffer : pCommandBuffers )
                            {
                                PrintCommandBuffer( *pCommandBuffer, index );
                                index.back()++;
                            }

                            index.pop_back();
                        }

                        if( inSubmitSubtree )
                        {
                            // Finish submit subtree
                            ImGui::TreePop();
                        }

                        index.back()++;
                    }

                    // Finish submit batch subtree
                    ImGui::TreePop();

                    // Invalidate submit index
                    index.pop_back();
                }

                index.back()++;
            }
        }

        ImGui::End();

        m_ScrollToSelectedFrameBrowserNode = false;
        m_pData = pCurrentFrameData;
    }

    /***********************************************************************************\

    Function:
        UpdateQueueUtilizationTab

    Description:
        Updates "Queue utilization" tab.

    \***********************************************************************************/
    void ProfilerOverlayOutput::UpdateQueueUtilizationTab()
    {
        const float interfaceScale = ImGui::GetIO().FontGlobalScale;

        ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, { 1, 1 } );
        ImGui::PushStyleColor( ImGuiCol_FrameBg, { 1.0f, 1.0f, 1.0f, 0.02f } );

        // Select first and last frame for queue utilization calculation.
        std::shared_ptr<DeviceProfilerFrameData> pFirstFrame = m_ShowActiveFrame ? m_pData : m_pFrames.front();
        std::shared_ptr<DeviceProfilerFrameData> pLastFrame = m_ShowActiveFrame ? m_pData : m_pFrames.back();

        // m_FrameTime is active time, queue utilization calculation should take idle time into account as well.
        const float frameDuration = GetDuration( pFirstFrame->m_BeginTimestamp, pLastFrame->m_EndTimestamp );

        std::vector<QueueGraphColumn> queueGraphColumns;
        for( const auto& [_, queue] : m_Frontend.GetDeviceQueues() )
        {
            // Enumerate columns for command queue activity graph.
            queueGraphColumns.clear();
            GetQueueGraphColumns( queue.Handle, queueGraphColumns );

            if( !queueGraphColumns.empty() )
            {
                char queueGraphId[32];
                snprintf( queueGraphId, sizeof( queueGraphId ), "##QueueGraph%p", queue.Handle );

                const std::string queueName = m_pStringSerializer->GetName( queue.Handle );
                ImGui::Text( "%s %s", m_pStringSerializer->GetQueueTypeName( queue.Flags ).c_str(), queueName.c_str() );

                const float queueUtilization = GetQueueUtilization( queueGraphColumns );
                ImGuiX::TextAlignRight(
                    "%.2f %s, %.2f %%",
                    queueUtilization,
                    m_pTimestampDisplayUnitStr,
                    queueUtilization * 100.f / frameDuration );

                ImGui::PushItemWidth( -1 );
                ImGuiX::PlotHistogramEx(
                    queueGraphId,
                    queueGraphColumns.data(),
                    static_cast<int>( queueGraphColumns.size() ),
                    0,
                    sizeof( queueGraphColumns.front() ),
                    "", 0, FLT_MAX, { 0, 8 * interfaceScale },
                    ImGuiX::HistogramFlags_NoScale,
                    std::bind( &ProfilerOverlayOutput::DrawQueueGraphLabel, this, std::placeholders::_1 ),
                    std::bind( &ProfilerOverlayOutput::SelectQueueGraphColumn, this, std::placeholders::_1 ) );
            }
        }

        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + 5 * interfaceScale );
    }

    /***********************************************************************************\

    Function:
        DrawQueueGraphLabel

    Description:
        Show a tooltip with queue submit description.

    \***********************************************************************************/
    void ProfilerOverlayOutput::DrawQueueGraphLabel( const ImGuiX::HistogramColumnData& data )
    {
        const QueueGraphColumn& column = static_cast<const QueueGraphColumn&>( data );

        switch( column.userDataType )
        {
        case QueueGraphColumn::eIdle:
        {
            ImGui::SetTooltip( "Idle\n%.2f %s", column.x, m_pTimestampDisplayUnitStr );
            break;
        }
        case QueueGraphColumn::eCommandBuffer:
        {
            const DeviceProfilerCommandBufferData& commandBufferData =
                *reinterpret_cast<const DeviceProfilerCommandBufferData*>( column.userData );

            if( ImGui::BeginTooltip() )
            {
                ImGui::Text( "%s\n%.2f %s",
                    m_pStringSerializer->GetName( commandBufferData.m_Handle ).c_str(),
                    column.x,
                    m_pTimestampDisplayUnitStr );

                ImGui::PushStyleColor( ImGuiCol_Text, { 0.55f, 0.55f, 0.55f, 1.0f } );
                ImGui::TextUnformatted( "Click to show in Frame Browser" );
                ImGui::PopStyleColor();

                ImGui::EndTooltip();
            }
            break;
        }
        case QueueGraphColumn::eSignalSemaphores:
        {
            const std::vector<VkSemaphore>& semaphores =
                *reinterpret_cast<const std::vector<VkSemaphore>*>( column.userData );

            if( ImGui::BeginTooltip() )
            {
                ImGui::Text( "Signal semaphores:" );

                ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, { 0, 0 } );
                for( VkSemaphore semaphore : semaphores )
                {
                    ImGui::Text( " - %s", m_pStringSerializer->GetName( semaphore ).c_str() );
                }
                ImGui::PopStyleVar();
                ImGui::SetCursorPosY( ImGui::GetCursorPosY() + ImGui::GetStyle().ItemSpacing.y );

                ImGui::PushStyleColor( ImGuiCol_Text, { 0.55f, 0.55f, 0.55f, 1.0f } );
                ImGui::TextUnformatted( "Click to highlight all occurrences in frame" );
                ImGui::PopStyleColor();

                ImGui::EndTooltip();
            }
            break;
        }
        case QueueGraphColumn::eWaitSemaphores:
        {
            const std::vector<VkSemaphore>& semaphores =
                *reinterpret_cast<const std::vector<VkSemaphore>*>( column.userData );

            if( ImGui::BeginTooltip() )
            {
                ImGui::Text( "Wait semaphores:" );

                ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, { 0, 0 } );
                for( VkSemaphore semaphore : semaphores )
                {
                    ImGui::Text( " - %s", m_pStringSerializer->GetName( semaphore ).c_str() );
                }
                ImGui::PopStyleVar();
                ImGui::SetCursorPosY( ImGui::GetCursorPosY() + ImGui::GetStyle().ItemSpacing.y );

                ImGui::PushStyleColor( ImGuiCol_Text, { 0.55f, 0.55f, 0.55f, 1.0f } );
                ImGui::TextUnformatted( "Click to highlight all occurrences in frame" );
                ImGui::PopStyleColor();

                ImGui::EndTooltip();
            }
            break;
        }
        }
    }

    /***********************************************************************************\

    Function:
        SelectQueueGraphColumn

    Description:
        Select a queue graph column and scroll to it in the frame browser.

    \***********************************************************************************/
    void ProfilerOverlayOutput::SelectQueueGraphColumn( const ImGuiX::HistogramColumnData& data )
    {
        const QueueGraphColumn& column = static_cast<const QueueGraphColumn&>( data );

        switch( column.userDataType )
        {
        case QueueGraphColumn::eCommandBuffer:
        {
            m_SelectedFrameBrowserNodeIndex = column.nodeIndex;
            m_ScrollToSelectedFrameBrowserNode = true;

            m_SelectionUpdateTimestamp = std::chrono::high_resolution_clock::now();
            break;
        }
        case QueueGraphColumn::eSignalSemaphores:
        case QueueGraphColumn::eWaitSemaphores:
        {
            const std::vector<VkSemaphore>& semaphores =
                *reinterpret_cast<const std::vector<VkSemaphore>*>( column.userData );

            // Unselect the semaphores if they are already selected.
            bool unselect = false;
            for( VkSemaphore semaphore : semaphores )
            {
                if( m_SelectedSemaphores.count( semaphore ) )
                {
                    unselect = true;
                    break;
                }
            }

            m_SelectedSemaphores.clear();

            if( !unselect )
            {
                m_SelectedSemaphores.insert( semaphores.begin(), semaphores.end() );
            }
            break;
        }
        }
    }

    /***********************************************************************************\

    Function:
        UpdateTopPipelinesTab

    Description:
        Updates "Top pipelines" tab.

    \***********************************************************************************/
    void ProfilerOverlayOutput::UpdateTopPipelinesTab()
    {
        const ImGuiStyle& style = ImGui::GetStyle();
        const float interfaceScale = ImGui::GetIO().FontGlobalScale;
        const float badgeSpacing = 3.f * interfaceScale;

        const float ellipsisWidth = ImGui::CalcTextSize( "..." ).x;

        // Calculate width of badges to align them.
        const float meshPipelineBadgesWidth =
            ImGui::CalcTextSize( "AS" ).x + badgeSpacing +
            ImGui::CalcTextSize( "MS" ).x + badgeSpacing;

        const float traditional3DPipelineBadgesWidth =
            ImGui::CalcTextSize( "VS" ).x + badgeSpacing +
            ImGui::CalcTextSize( "HS" ).x + badgeSpacing +
            ImGui::CalcTextSize( "DS" ).x + badgeSpacing +
            ImGui::CalcTextSize( "GS" ).x + badgeSpacing;

        const float meshPipelineBadgesOffset =
            std::max( 0.f, traditional3DPipelineBadgesWidth - meshPipelineBadgesWidth );

        const float traditional3DPipelineBadgesOffset =
            std::max( 0.f, meshPipelineBadgesWidth - traditional3DPipelineBadgesWidth );

        // Toolbar with save and load options.
        ImGui::BeginDisabled( m_pTopPipelinesExporter != nullptr );
        if( ImGui::Button( Lang::Save ) )
        {
            m_pTopPipelinesExporter = std::make_unique<TopPipelinesExporter>();
            m_pTopPipelinesExporter->m_pData = m_pData;
            m_pTopPipelinesExporter->m_Action = TopPipelinesExporter::Action::eExport;
        }
        ImGui::EndDisabled();

        ImGui::SameLine( 0.0f, 1.5f * interfaceScale );
        ImGui::BeginDisabled( m_pTopPipelinesExporter != nullptr );
        if( ImGui::Button( Lang::Load ) )
        {
            m_pTopPipelinesExporter = std::make_unique<TopPipelinesExporter>();
            m_pTopPipelinesExporter->m_Action = TopPipelinesExporter::Action::eImport;
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        if( ImGui::Button( Lang::SetRef ) )
        {
            m_ReferenceTopPipelines.clear();

            const uint32_t frameIndex = m_pData->m_CPU.m_FrameIndex;
            m_ReferenceTopPipelinesShortDescription = fmt::format( "{} #{}", m_pFrameStr, frameIndex );

            for( const DeviceProfilerPipelineData& pipeline : m_pData->m_TopPipelines )
            {
                const float pipelineTimeMs = Profiler::GetDuration( pipeline ) * m_TimestampPeriod.count();

                m_ReferenceTopPipelines.try_emplace(
                    m_pStringSerializer->GetName( pipeline ), pipelineTimeMs );
            }
        }

        ImGui::SameLine( 0.0f, 1.5f * interfaceScale );
        ImGui::BeginDisabled( m_ReferenceTopPipelines.empty() );
        if( ImGui::Button( Lang::ClearRef ) )
        {
            m_ReferenceTopPipelines.clear();
            m_ReferenceTopPipelinesShortDescription.clear();
            m_ReferenceTopPipelinesFullDescription.clear();
        }
        ImGui::EndDisabled();

        if( !m_ReferenceTopPipelines.empty() )
        {
            ImGuiX::TextAlignRight( "Ref: %s", m_ReferenceTopPipelinesShortDescription.c_str() );

            if( !m_ReferenceTopPipelinesFullDescription.empty() )
            {
                if( ImGui::IsItemHovered() )
                {
                    ImGui::SetTooltip( "%s", m_ReferenceTopPipelinesFullDescription.c_str() );
                }
            }
        }

        // Draw the table with top pipelines.
        if( ImGui::BeginTable( "TopPipelinesTable", 8,
                ImGuiTableFlags_Hideable |
                ImGuiTableFlags_PadOuterX |
                ImGuiTableFlags_NoClip ) )
        {
            // Hide reference columns if there are no reference pipelines captured.
            int referenceColumnFlags = ImGuiTableColumnFlags_WidthStretch;
            if( m_ReferenceTopPipelines.empty() )
            {
                referenceColumnFlags |= ImGuiTableColumnFlags_Disabled;
            }

            // Headers
            ImGui::TableSetupColumn( "#", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_NoHide );
            ImGui::TableSetupColumn( Lang::Pipeline, ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoHide );
            ImGui::TableSetupColumn( Lang::Capabilities, ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_NoHeaderLabel );
            ImGui::TableSetupColumn( Lang::Stages, ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize );
            ImGuiX::TableSetupColumn( Lang::Contrib, ImGuiTableColumnFlags_WidthStretch, ImGuiXTableColumnFlags_AlignHeaderRight, 0.25f );
            ImGuiX::TableSetupColumn( Lang::StatTotal, ImGuiTableColumnFlags_WidthStretch, ImGuiXTableColumnFlags_AlignHeaderRight, 0.25f );
            ImGuiX::TableSetupColumn( Lang::Ref, referenceColumnFlags, ImGuiXTableColumnFlags_AlignHeaderRight, 0.25f );
            ImGuiX::TableSetupColumn( Lang::Delta, referenceColumnFlags, ImGuiXTableColumnFlags_AlignHeaderRight, 0.25f );
            ImGuiX::TableHeadersRow( m_Resources.GetBoldFont() );

            uint32_t pipelineIndex = 0;
            char pipelineIndexStr[32];

            for( const auto& pipeline : m_pData->m_TopPipelines )
            {
                // Skip debug pipelines.
                if( ( pipeline.m_Type == DeviceProfilerPipelineType::eNone ) ||
                    ( pipeline.m_Type == DeviceProfilerPipelineType::eDebug ) )
                {
                    continue;
                }

                ImGui::TableNextRow();

                pipelineIndex++;
                snprintf( pipelineIndexStr, sizeof( pipelineIndexStr ), "TopPipeline_%u", pipelineIndex );

                const float pipelineTime = GetDuration( pipeline );
                std::string pipelineName = m_pStringSerializer->GetName( pipeline );

                if( ImGui::TableNextColumn() )
                {
                    ImGui::Text( "%u", pipelineIndex );
                }

                if( ImGui::TableNextColumn() )
                {
                    // Ellide the pipeline name if it's too long.
                    const float availableWidth = ImGuiX::TableGetColumnWidth();
                    float pipelineNameWidth = ImGui::CalcTextSize( pipelineName.c_str() ).x;

                    if( pipelineNameWidth > availableWidth )
                    {
                        while( !pipelineName.empty() && ( pipelineNameWidth + ellipsisWidth ) > availableWidth )
                        {
                            pipelineName.pop_back();
                            pipelineNameWidth = ImGui::CalcTextSize( pipelineName.c_str() ).x;
                        }

                        if( !pipelineName.empty() )
                        {
                            ImGui::Text( "%s...", pipelineName.c_str() );
                        }
                    }
                    else
                    {
                        ImGui::TextUnformatted( pipelineName.c_str() );
                    }

                    DrawPipelineContextMenu( pipeline, pipelineIndexStr );
                }

                if( ImGui::TableNextColumn() )
                {
                    DrawPipelineCapabilityBadges( pipeline );

                    ImGui::SameLine( 0.f, 5.f );
                    ImGui::Dummy( ImVec2() );
                }

                if( ImGui::TableNextColumn() )
                {
                    if( !pipeline.m_ShaderTuple.m_Shaders.empty() )
                    {
                        ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( badgeSpacing, 0 ) );

                        if( pipeline.m_UsesMeshShading )
                        {
                            // Mesh shading pipeline.
                            ImGui::SameLine( 0.f, meshPipelineBadgesOffset );
                            DrawPipelineStageBadge( pipeline, VK_SHADER_STAGE_TASK_BIT_EXT, "AS" );
                            DrawPipelineStageBadge( pipeline, VK_SHADER_STAGE_MESH_BIT_EXT, "MS" );
                        }
                        else
                        {
                            // Traditional 3D pipeline.
                            ImGui::SameLine( 0.f, traditional3DPipelineBadgesOffset );
                            DrawPipelineStageBadge( pipeline, VK_SHADER_STAGE_VERTEX_BIT, "VS" );
                            DrawPipelineStageBadge( pipeline, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, "HS" );
                            DrawPipelineStageBadge( pipeline, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, "DS" );
                            DrawPipelineStageBadge( pipeline, VK_SHADER_STAGE_GEOMETRY_BIT, "GS" );
                        }

                        DrawPipelineStageBadge( pipeline, VK_SHADER_STAGE_FRAGMENT_BIT, "PS" );
                        DrawPipelineStageBadge( pipeline, VK_SHADER_STAGE_COMPUTE_BIT, "CS" );
                        DrawPipelineStageBadge( pipeline, VK_SHADER_STAGE_RAYGEN_BIT_KHR, "RT" );

                        ImGui::PopStyleVar();
                    }
                }

                if( ImGui::TableNextColumn() )
                {
                    ImGuiX::TextAlignRight(
                        ImGuiX::TableGetColumnWidth(),
                        "%.1f %%",
                        pipelineTime * 100.f / m_FrameTime );
                }

                if( ImGui::TableNextColumn() )
                {
                    ImGuiX::TextAlignRight(
                        ImGuiX::TableGetColumnWidth(),
                        "%.2f %s",
                        pipelineTime,
                        m_pTimestampDisplayUnitStr );
                }

                // Show reference time if available.
                if( !m_ReferenceTopPipelines.empty() )
                {
                    auto ref = m_ReferenceTopPipelines.find( pipelineName );
                    if( ref != m_ReferenceTopPipelines.end() )
                    {
                        // Convert saved reference time to the same unit as the pipeline time.
                        float refPipelineTime = ref->second * m_TimestampDisplayUnit;

                        if( ImGui::TableNextColumn() )
                        {
                            ImGui::PushStyleColor( ImGuiCol_Text, IM_COL32( 128, 128, 128, 255 ) );
                            ImGuiX::TextAlignRight(
                                ImGuiX::TableGetColumnWidth(),
                                "%.2f %s",
                                refPipelineTime,
                                m_pTimestampDisplayUnitStr );
                            ImGui::PopStyleColor();
                        }

                        if( ImGui::TableNextColumn() )
                        {
                            const float delta = CalcPerformanceCounterDelta( refPipelineTime, pipelineTime );
                            ImGui::PushStyleColor( ImGuiCol_Text, GetPerformanceCounterDeltaColor( delta ) );
                            ImGuiX::TextAlignRight(
                                ImGuiX::TableGetColumnWidth(),
                                "%+.1f%%",
                                delta );
                            ImGui::PopStyleColor();
                        }
                    }
                }

                if( !m_ShowAllTopPipelines && pipelineIndex == 10 )
                {
                    break;
                }
            }

            // Show more/less button if there is more than 10 pipelines.
            if( pipelineIndex >= 10 )
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                if( ImGui::TableNextColumn() )
                {
                    if( ImGui::TextLink( m_ShowAllTopPipelines ? Lang::ShowLess : Lang::ShowMore ) )
                    {
                        m_ShowAllTopPipelines = !m_ShowAllTopPipelines;
                    }
                }
            }

            ImGui::EndTable();
        }
    }

    /***********************************************************************************\

    Function:
        UpdatePerformanceCountersTab

    Description:
        Updates "Performance Counters" tab.

    \***********************************************************************************/
    void ProfilerOverlayOutput::UpdatePerformanceCountersTab()
    {
        // Vendor-specific
        if( !m_pData->m_VendorMetrics.empty() )
        {
            std::unordered_set<VkCommandBuffer> uniqueCommandBuffers;

            // Data source
            const std::vector<VkProfilerPerformanceCounterResultEXT>* pVendorMetrics = &m_pData->m_VendorMetrics;

            bool performanceQueryResultsFiltered = false;
            auto regexFilterFlags =
                std::regex::ECMAScript | std::regex::icase | std::regex::optimize;

            auto UpdateActiveMetricsVisiblityWithRegex = [&]( const std::regex& regex ) {
                const VendorMetricsSet& activeMetricsSet = m_VendorMetricsSets[ m_ActiveMetricsSetIndex ];
                assert( activeMetricsSet.m_Metrics.size() == m_ActiveMetricsVisibility.size() );

                for( size_t metricIndex = 0; metricIndex < m_ActiveMetricsVisibility.size(); ++metricIndex )
                {
                    const auto& metric = activeMetricsSet.m_Metrics[ metricIndex ];
                    m_ActiveMetricsVisibility[ metricIndex ] =
                        std::regex_search( metric.shortName, regex );
                }
            };

            auto UpdateActiveMetricsVisiblity = [&]() {
                try
                {
                    // Try to compile the regex filter.
                    UpdateActiveMetricsVisiblityWithRegex(
                        std::regex( m_VendorMetricFilter, regexFilterFlags ) );
                }
                catch( ... )
                {
                    // Regex compilation failed, don't change the visibility of the metrics.
                }
            };

            // Find the first command buffer that matches the filter.
            // TODO: Aggregation.
            for( const auto& submitBatch : m_pData->m_Submits )
            {
                for( const auto& submit : submitBatch.m_Submits )
                {
                    for( const auto& commandBuffer : submit.m_CommandBuffers )
                    {
                        if( (performanceQueryResultsFiltered == false) &&
                            (commandBuffer.m_Handle != VK_NULL_HANDLE) &&
                            (commandBuffer.m_Handle == m_PerformanceQueryCommandBufferFilter) )
                        {
                            // Use the data from this command buffer.
                            pVendorMetrics = &commandBuffer.m_PerformanceQueryResults;
                            performanceQueryResultsFiltered = true;
                        }

                        uniqueCommandBuffers.insert( commandBuffer.m_Handle );
                    }
                }
            }

            const float interfaceScale = ImGui::GetIO().FontGlobalScale;

            // Toolbar with save and load options.
            ImGui::BeginDisabled( m_pPerformanceCounterExporter != nullptr || pVendorMetrics->empty() );
            if( ImGui::Button( Lang::Save ) )
            {
                m_pPerformanceCounterExporter = std::make_unique<PerformanceCounterExporter>();
                m_pPerformanceCounterExporter->m_Data = *pVendorMetrics;
                m_pPerformanceCounterExporter->m_DataMask = m_ActiveMetricsVisibility;
                m_pPerformanceCounterExporter->m_MetricsSetIndex = m_ActiveMetricsSetIndex;
                m_pPerformanceCounterExporter->m_Action = PerformanceCounterExporter::Action::eExport;
            }
            ImGui::EndDisabled();

            ImGui::SameLine( 0.0f, 1.5f * interfaceScale );
            ImGui::BeginDisabled( m_pPerformanceCounterExporter != nullptr || pVendorMetrics->empty() );
            if( ImGui::Button( Lang::Load ) )
            {
                m_pPerformanceCounterExporter = std::make_unique<PerformanceCounterExporter>();
                m_pPerformanceCounterExporter->m_Action = PerformanceCounterExporter::Action::eImport;
            }
            ImGui::EndDisabled();

            ImGui::SameLine();
            ImGui::BeginDisabled( pVendorMetrics->empty() );
            if( ImGui::Button( Lang::SetRef ) )
            {
                m_ReferencePerformanceCounters.clear();

                const auto& activeMetricsSet = m_VendorMetricsSets[m_ActiveMetricsSetIndex];
                if( pVendorMetrics->size() == activeMetricsSet.m_Metrics.size() )
                {
                    for( size_t i = 0; i < pVendorMetrics->size(); ++i )
                    {
                        m_ReferencePerformanceCounters.try_emplace( activeMetricsSet.m_Metrics[i].shortName, ( *pVendorMetrics )[i] );
                    }
                }
            }
            ImGui::EndDisabled();

            ImGui::SameLine( 0.0f, 1.5f * interfaceScale );
            ImGui::BeginDisabled( pVendorMetrics->empty() || m_ReferencePerformanceCounters.empty() );
            if( ImGui::Button( Lang::ClearRef ) )
            {
                m_ReferencePerformanceCounters.clear();
            }
            ImGui::EndDisabled();

            // Show a search box for filtering metrics sets to find specific metrics.
            ImGui::SameLine();
            ImGui::Text( "%s:", Lang::PerformanceCountersFilter );
            ImGui::SameLine();
            ImGui::SetNextItemWidth( std::clamp( 200.f * interfaceScale, 50.f, ImGui::GetContentRegionAvail().x ) );
            if( ImGui::InputText( "##PerformanceQueryMetricsFilter", m_VendorMetricFilter, std::extent_v<decltype( m_VendorMetricFilter )> ) )
            {
                try
                {
                    // Text changed, construct a regex from the string and find the matching metrics sets.
                    std::regex regexFilter( m_VendorMetricFilter, regexFilterFlags );

                    // Enumerate only sets that match the query.
                    for( uint32_t metricsSetIndex = 0; metricsSetIndex < m_VendorMetricsSets.size(); ++metricsSetIndex )
                    {
                        const auto& metricsSet = m_VendorMetricsSets[metricsSetIndex];

                        m_VendorMetricsSetVisibility[metricsSetIndex] = false;

                        // Match by metrics set name.
                        if( std::regex_search( metricsSet.m_Properties.name, regexFilter ) )
                        {
                            m_VendorMetricsSetVisibility[metricsSetIndex] = true;
                            continue;
                        }

                        // Match by metric name.
                        for( const auto& metric : metricsSet.m_Metrics )
                        {
                            if( std::regex_search( metric.shortName, regexFilter ) )
                            {
                                m_VendorMetricsSetVisibility[metricsSetIndex] = true;
                                break;
                            }
                        }
                    }

                    // Update visibility of metrics in the active metrics set.
                    UpdateActiveMetricsVisiblityWithRegex( regexFilter );
                }
                catch( ... )
                {
                    // Regex compilation failed, don't change the visibility of the sets.
                }
            }

            ImGui::SetCursorPosY( ImGui::GetCursorPosY() + 5 * interfaceScale );

            // Show a combo box that allows the user to select the filter the profiled range.
            ImGui::TextUnformatted( Lang::PerformanceCountersRange );
            ImGui::SameLine( 100.f * interfaceScale );
            ImGui::PushItemWidth( -1 );
            if( ImGui::BeginCombo( "##PerformanceQueryFilter", m_PerformanceQueryCommandBufferFilterName.c_str() ) )
            {
                if( ImGuiX::TSelectable( m_pFrameStr, m_PerformanceQueryCommandBufferFilter, VkCommandBuffer() ) )
                {
                    // Selection changed.
                    m_PerformanceQueryCommandBufferFilterName = m_pFrameStr;
                }

                // Enumerate command buffers.
                for( VkCommandBuffer commandBuffer : uniqueCommandBuffers )
                {
                    std::string commandBufferName = m_pStringSerializer->GetName( commandBuffer );

                    if( ImGuiX::TSelectable( commandBufferName.c_str(), m_PerformanceQueryCommandBufferFilter, commandBuffer ) )
                    {
                        // Selection changed.
                        m_PerformanceQueryCommandBufferFilterName = std::move( commandBufferName );
                    }
                }

                ImGui::EndCombo();
            }

            // Show a combo box that allows the user to change the active metrics set.
            ImGui::TextUnformatted( Lang::PerformanceCountersSet );
            ImGui::SameLine( 100.f * interfaceScale );
            ImGui::PushItemWidth( -1 );
            if( ImGui::BeginCombo( "##PerformanceQueryMetricsSet", m_VendorMetricsSets[ m_ActiveMetricsSetIndex ].m_Properties.name ) )
            {
                // Enumerate metrics sets.
                for( uint32_t metricsSetIndex = 0; metricsSetIndex < m_VendorMetricsSets.size(); ++metricsSetIndex )
                {
                    if( m_VendorMetricsSetVisibility[ metricsSetIndex ] )
                    {
                        const auto& metricsSet = m_VendorMetricsSets[ metricsSetIndex ];

                        if( ImGuiX::Selectable( metricsSet.m_Properties.name, (m_ActiveMetricsSetIndex == metricsSetIndex) ) )
                        {
                            // Notify the profiler.
                            if( m_Frontend.SetPreformanceMetricsSetIndex( metricsSetIndex ) == VK_SUCCESS )
                            {
                                // Refresh the performance metric properties.
                                m_ActiveMetricsSetIndex = metricsSetIndex;
                                m_ActiveMetricsVisibility.resize( m_VendorMetricsSets[ m_ActiveMetricsSetIndex ].m_Properties.metricsCount, true );
                                UpdateActiveMetricsVisiblity();
                            }
                        }
                    }
                }

                ImGui::EndCombo();
            }

            if( pVendorMetrics->empty() )
            {
                // Vendor metrics not available.
                ImGui::TextUnformatted( Lang::PerformanceCountersNotAvailableForCommandBuffer );
            }

            const auto& activeMetricsSet = m_VendorMetricsSets[ m_ActiveMetricsSetIndex ];
            if( pVendorMetrics->size() == activeMetricsSet.m_Metrics.size() )
            {
                const auto& vendorMetrics = *pVendorMetrics;

                ImGui::BeginTable( "Performance counters table",
                    /* columns_count */ 5,
                    ImGuiTableFlags_NoClip |
                    (ImGuiTableFlags_Borders & ~ImGuiTableFlags_BordersInnerV) );

                // Headers
                ImGui::TableSetupColumn( Lang::Metric, ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoResize, 0.5f );
                ImGui::TableSetupColumn( Lang::Ref, ImGuiTableColumnFlags_WidthStretch, 0.25f );
                ImGui::TableSetupColumn( Lang::Delta, ImGuiTableColumnFlags_WidthStretch, 0.15f );
                ImGui::TableSetupColumn( Lang::Value, ImGuiTableColumnFlags_WidthStretch, 0.25f );
                ImGui::TableSetupColumn( "", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize );
                ImGui::TableHeadersRow();

                for( uint32_t i = 0; i < vendorMetrics.size(); ++i )
                {
                    const VkProfilerPerformanceCounterResultEXT& metric = vendorMetrics[ i ];
                    const VkProfilerPerformanceCounterPropertiesEXT& metricProperties = activeMetricsSet.m_Metrics[ i ];

                    if( !m_ActiveMetricsVisibility[ i ] )
                    {
                        continue;
                    }

                    ImGui::TableNextColumn();
                    {
                        ImGui::Text( "%s", metricProperties.shortName );

                        if( ImGui::IsItemHovered() &&
                            metricProperties.description[ 0 ] )
                        {
                            ImGui::BeginTooltip();
                            ImGui::PushTextWrapPos( 350.f * interfaceScale );
                            ImGui::TextUnformatted( metricProperties.description );
                            ImGui::PopTextWrapPos();
                            ImGui::EndTooltip();
                        }
                    }

                    float delta = 0.0f;
                    bool deltaValid = false;

                    ImGui::TableNextColumn();
                    {
                        auto it = m_ReferencePerformanceCounters.find( metricProperties.shortName );
                        if( it != m_ReferencePerformanceCounters.end() )
                        {
                            const float columnWidth = ImGuiX::TableGetColumnWidth();
                            switch( metricProperties.storage )
                            {
                            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_INT32_EXT:
                                ImGuiX::TextAlignRight( columnWidth, "%d", it->second.int32 );
                                delta = CalcPerformanceCounterDelta( it->second.int32, metric.int32 );
                                deltaValid = true;
                                break;

                            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_INT64_EXT:
                                ImGuiX::TextAlignRight( columnWidth, "%lld", it->second.int64 );
                                delta = CalcPerformanceCounterDelta( it->second.int64, metric.int64 );
                                deltaValid = true;
                                break;

                            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_UINT32_EXT:
                                ImGuiX::TextAlignRight( columnWidth, "%u", it->second.uint32 );
                                delta = CalcPerformanceCounterDelta( it->second.uint32, metric.uint32 );
                                deltaValid = true;
                                break;

                            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_UINT64_EXT:
                                ImGuiX::TextAlignRight( columnWidth, "%llu", it->second.uint64 );
                                delta = CalcPerformanceCounterDelta( it->second.uint64, metric.uint64 );
                                deltaValid = true;
                                break;

                            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_FLOAT32_EXT:
                                ImGuiX::TextAlignRight( columnWidth, "%.2f", it->second.float32 );
                                delta = CalcPerformanceCounterDelta( it->second.float32, metric.float32 );
                                deltaValid = true;
                                break;

                            case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_FLOAT64_EXT:
                                ImGuiX::TextAlignRight( columnWidth, "%.2lf", it->second.float64 );
                                delta = CalcPerformanceCounterDelta( it->second.float64, metric.float64 );
                                deltaValid = true;
                                break;
                            }
                        }
                    }

                    ImGui::TableNextColumn();
                    if( deltaValid )
                    {
                        const float columnWidth = ImGuiX::TableGetColumnWidth();
                        ImGui::PushStyleColor( ImGuiCol_Text, GetPerformanceCounterDeltaColor( delta ) );
                        ImGuiX::TextAlignRight( columnWidth, "%+.1f%%", delta );
                        ImGui::PopStyleColor();
                    }

                    ImGui::TableNextColumn();
                    {
                        const float columnWidth = ImGuiX::TableGetColumnWidth();
                        switch( metricProperties.storage )
                        {
                        case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_INT32_EXT:
                            ImGuiX::TextAlignRight( columnWidth, "%d", metric.int32 );
                            break;

                        case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_INT64_EXT:
                            ImGuiX::TextAlignRight( columnWidth, "%lld", metric.int64 );
                            break;

                        case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_UINT32_EXT:
                            ImGuiX::TextAlignRight( columnWidth, "%u", metric.uint32 );
                            break;

                        case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_UINT64_EXT:
                            ImGuiX::TextAlignRight( columnWidth, "%llu", metric.uint64 );
                            break;

                        case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_FLOAT32_EXT:
                            ImGuiX::TextAlignRight( columnWidth, "%.2f", metric.float32 );
                            break;

                        case VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_FLOAT64_EXT:
                            ImGuiX::TextAlignRight( columnWidth, "%.2lf", metric.float64 );
                            break;
                        }
                    }

                    ImGui::TableNextColumn();
                    {
                        const char* pUnitString = "???";

                        static const char* const ppUnitString[ 11 ] =
                        {
                            "" /* VK_PERFORMANCE_COUNTER_UNIT_GENERIC_KHR */,
                            "%" /* VK_PERFORMANCE_COUNTER_UNIT_PERCENTAGE_KHR */,
                            "ns" /* VK_PERFORMANCE_COUNTER_UNIT_NANOSECONDS_KHR */,
                            "B" /* VK_PERFORMANCE_COUNTER_UNIT_BYTES_KHR */,
                            "B/s" /* VK_PERFORMANCE_COUNTER_UNIT_BYTES_PER_SECOND_KHR */,
                            "K" /* VK_PERFORMANCE_COUNTER_UNIT_KELVIN_KHR */,
                            "W" /* VK_PERFORMANCE_COUNTER_UNIT_WATTS_KHR */,
                            "V" /* VK_PERFORMANCE_COUNTER_UNIT_VOLTS_KHR */,
                            "A" /* VK_PERFORMANCE_COUNTER_UNIT_AMPS_KHR */,
                            "Hz" /* VK_PERFORMANCE_COUNTER_UNIT_HERTZ_KHR */,
                            "clk" /* VK_PERFORMANCE_COUNTER_UNIT_CYCLES_KHR */
                        };

                        if( (metricProperties.unit >= 0) && (metricProperties.unit < 11) )
                        {
                            pUnitString = ppUnitString[ metricProperties.unit ];
                        }

                        ImGui::TextUnformatted( pUnitString );
                    }
                }

                ImGui::EndTable();
            }
        }
        else
        {
            ImGui::TextUnformatted( Lang::PerformanceCountesrNotAvailable );
        }
    }

    /***********************************************************************************\

    Function:
        UpdateMemoryTab

    Description:
        Updates "Memory" tab.

    \***********************************************************************************/
    void ProfilerOverlayOutput::UpdateMemoryTab()
    {
        const VkPhysicalDeviceMemoryProperties& memoryProperties =
            m_Frontend.GetPhysicalDeviceMemoryProperties();

        if( ImGui::CollapsingHeader( Lang::MemoryHeapUsage ) )
        {
            for( uint32_t i = 0; i < memoryProperties.memoryHeapCount; ++i )
            {
                ImGui::Text( "%s %u", Lang::MemoryHeap, i );

                ImGuiX::TextAlignRight( "%u %s", m_pData->m_Memory.m_Heaps[i].m_AllocationCount, Lang::Allocations );

                float usage = 0.f;
                char usageStr[ 64 ] = {};

                if( memoryProperties.memoryHeaps[ i ].size != 0 )
                {
                    usage = (float)m_pData->m_Memory.m_Heaps[i].m_AllocationSize / memoryProperties.memoryHeaps[i].size;

                    snprintf( usageStr, sizeof( usageStr ),
                        "%.2f/%.2f MB (%.1f%%)",
                        m_pData->m_Memory.m_Heaps[i].m_AllocationSize / 1048576.f,
                        memoryProperties.memoryHeaps[ i ].size / 1048576.f,
                        usage * 100.f );
                }

                ImGui::ProgressBar( usage, { -1, 0 }, usageStr );

                if( ImGui::IsItemHovered() && (memoryProperties.memoryHeaps[ i ].flags != 0) )
                {
                    ImGui::BeginTooltip();

                    if( memoryProperties.memoryHeaps[ i ].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT )
                    {
                        ImGui::TextUnformatted( "VK_MEMORY_HEAP_DEVICE_LOCAL_BIT" );
                    }

                    if( memoryProperties.memoryHeaps[ i ].flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT )
                    {
                        ImGui::TextUnformatted( "VK_MEMORY_HEAP_MULTI_INSTANCE_BIT" );
                    }

                    ImGui::EndTooltip();
                }

                std::vector<float> memoryTypeUsages( memoryProperties.memoryTypeCount );
                std::vector<std::string> memoryTypeDescriptors( memoryProperties.memoryTypeCount );

                for( uint32_t typeIndex = 0; typeIndex < memoryProperties.memoryTypeCount; ++typeIndex )
                {
                    if( memoryProperties.memoryTypes[ typeIndex ].heapIndex == i )
                    {
                        memoryTypeUsages[ typeIndex ] = static_cast<float>( m_pData->m_Memory.m_Types[ typeIndex ].m_AllocationSize );

                        // Prepare descriptor for memory type
                        std::stringstream sstr;

                        sstr << Lang::MemoryTypeIndex << " " << typeIndex << "\n"
                             << m_pData->m_Memory.m_Types[ typeIndex ].m_AllocationCount << " " << Lang::Allocations << "\n";

                        if( memoryProperties.memoryTypes[ typeIndex ].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT )
                        {
                            sstr << "VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT\n";
                        }

                        if( memoryProperties.memoryTypes[ typeIndex ].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD )
                        {
                            sstr << "VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD\n";
                        }

                        if( memoryProperties.memoryTypes[ typeIndex ].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD )
                        {
                            sstr << "VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD\n";
                        }

                        if( memoryProperties.memoryTypes[ typeIndex ].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT )
                        {
                            sstr << "VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT\n";
                        }

                        if( memoryProperties.memoryTypes[ typeIndex ].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT )
                        {
                            sstr << "VK_MEMORY_PROPERTY_HOST_COHERENT_BIT\n";
                        }

                        if( memoryProperties.memoryTypes[ typeIndex ].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT )
                        {
                            sstr << "VK_MEMORY_PROPERTY_HOST_CACHED_BIT\n";
                        }

                        if( memoryProperties.memoryTypes[ typeIndex ].propertyFlags & VK_MEMORY_PROPERTY_PROTECTED_BIT )
                        {
                            sstr << "VK_MEMORY_PROPERTY_PROTECTED_BIT\n";
                        }

                        if( memoryProperties.memoryTypes[ typeIndex ].propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT )
                        {
                            sstr << "VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT\n";
                        }

                        memoryTypeDescriptors[ typeIndex ] = sstr.str();
                    }
                }

                // Get descriptor pointers
                std::vector<const char*> memoryTypeDescriptorPointers( memoryProperties.memoryTypeCount );

                for( uint32_t typeIndex = 0; typeIndex < memoryProperties.memoryTypeCount; ++typeIndex )
                {
                    memoryTypeDescriptorPointers[ typeIndex ] = memoryTypeDescriptors[ typeIndex ].c_str();
                }

                ImGuiX::PlotBreakdownEx(
                    "HEAP_BREAKDOWN",
                    memoryTypeUsages.data(),
                    memoryProperties.memoryTypeCount, 0,
                    memoryTypeDescriptorPointers.data() );
            }
        }
    }

    /***********************************************************************************\

    Function:
        UpdateInspectorTab

    Description:
        Updates "Inspector" tab.

    \***********************************************************************************/
    void ProfilerOverlayOutput::UpdateInspectorTab()
    {
        // Early out if no valid pipeline is selected.
        if( !m_InspectorPipeline.m_Handle && !m_InspectorPipeline.m_UsesShaderObjects )
        {
            ImGui::TextUnformatted( "No pipeline selected for inspection." );
            return;
        }

        // Enumerate inspector tabs.
        ImGui::PushItemWidth( -1 );

        if( ImGui::BeginCombo( "##InspectorTabs", m_InspectorTabs[m_InspectorTabIndex].Name.c_str() ) )
        {
            const size_t tabCount = m_InspectorTabs.size();
            for( size_t i = 0; i < tabCount; ++i )
            {
                if( ImGuiX::TSelectable( m_InspectorTabs[ i ].Name.c_str(), m_InspectorTabIndex, i ) )
                {
                    // Change tab.
                    SetInspectorTabIndex( i );
                }
            }

            ImGui::EndCombo();
        }

        // Render the inspector tab.
        const InspectorTab& tab = m_InspectorTabs[m_InspectorTabIndex];
        if( tab.Draw )
        {
            tab.Draw();
        }
    }

    /***********************************************************************************\

    Function:
        Inspect

    Description:
        Sets the inspected pipeline and switches the view to the "Inspector" tab.

    \***********************************************************************************/
    void ProfilerOverlayOutput::Inspect( const DeviceProfilerPipeline& pipeline )
    {
        m_InspectorPipeline = pipeline;

        // Resolve inspected pipeline shader stage names.
        m_InspectorTabs.clear();
        m_InspectorTabs.push_back( { Lang::PipelineState,
            nullptr,
            std::bind( &ProfilerOverlayOutput::DrawInspectorPipelineState, this ) } );

        const size_t shaderCount = m_InspectorPipeline.m_ShaderTuple.m_Shaders.size();
        const ProfilerShader* pShaders = m_InspectorPipeline.m_ShaderTuple.m_Shaders.data();
        for( size_t shaderIndex = 0; shaderIndex < shaderCount; ++shaderIndex )
        {
            m_InspectorTabs.push_back( { m_pStringSerializer->GetShaderName( pShaders[shaderIndex] ),
                std::bind( &ProfilerOverlayOutput::SelectInspectorShaderStage, this, shaderIndex ),
                std::bind( &ProfilerOverlayOutput::DrawInspectorShaderStage, this ) } );
        }

        SetInspectorTabIndex( 0 );

        // Switch to the inspector tab.
        m_InspectorWindowState.SetFocus();
    }

    /***********************************************************************************\

    Function:
        SelectInspectorShaderStage

    Description:
        Sets the inspected shader stage and updates the view.

    \***********************************************************************************/
    void ProfilerOverlayOutput::SelectInspectorShaderStage( size_t shaderIndex )
    {
        const ProfilerShader& shader = m_InspectorPipeline.m_ShaderTuple.m_Shaders[ shaderIndex ];

        m_InspectorShaderView.Clear();
        m_InspectorShaderView.SetShaderName( m_pStringSerializer->GetShortShaderName( shader ) );
        m_InspectorShaderView.SetEntryPointName( shader.m_EntryPoint );

        // Shader module may not be available if the VkShaderEXT has been created directly from a binary.
        if( shader.m_pShaderModule )
        {
            m_InspectorShaderView.SetShaderIdentifier(
                shader.m_pShaderModule->m_IdentifierSize,
                shader.m_pShaderModule->m_Identifier );

            const auto& bytecode = shader.m_pShaderModule->m_Bytecode;
            m_InspectorShaderView.AddBytecode( bytecode.data(), bytecode.size() );
        }

        // Enumerate shader internal representations associated with the selected stage.
        for( const ProfilerShaderExecutable& executable : m_InspectorPipeline.m_ShaderTuple.m_ShaderExecutables )
        {
            if( executable.GetStages() & shader.m_Stage )
            {
                m_InspectorShaderView.AddShaderExecutable( executable );
            }
        }
    }

    /***********************************************************************************\

    Function:
        DrawInspectorShaderStage

    Description:
        Draws the inspected shader stage.

    \***********************************************************************************/
    void ProfilerOverlayOutput::DrawInspectorShaderStage()
    {
        m_InspectorShaderView.Draw();
    }

    /***********************************************************************************\

    Function:
        DrawInspectorPipelineState

    Description:
        Draws the inspected pipeline state.

    \***********************************************************************************/
    void ProfilerOverlayOutput::DrawInspectorPipelineState()
    {
        if( m_InspectorPipeline.m_pCreateInfo == nullptr )
        {
            ImGui::TextUnformatted( Lang::PipelineStateNotAvailable );
            return;
        }

        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + 5 );

        switch( m_InspectorPipeline.m_Type )
        {
        case DeviceProfilerPipelineType::eGraphics:
            DrawInspectorGraphicsPipelineState();
            break;
        }
    }

    /***********************************************************************************\

    Function:
        DrawInspectorGraphicsPipelineState

    Description:
        Draws the inspected graphics pipeline state.

    \***********************************************************************************/
    static bool IsPipelineStateDynamic( const VkPipelineDynamicStateCreateInfo* pDynamicStateInfo, VkDynamicState dynamicState )
    {
        if( pDynamicStateInfo != nullptr )
        {
            for( uint32_t i = 0; i < pDynamicStateInfo->dynamicStateCount; ++i )
            {
                if( pDynamicStateInfo->pDynamicStates[i] == dynamicState )
                {
                    return true;
                }
            }
        }
        return false;
    }

    template<typename T>
    static void DrawPipelineStateValue( const char* pName, const char* pFormat, T&& value, const VkPipelineDynamicStateCreateInfo* pDynamicStateInfo = nullptr, VkDynamicState dynamicState = {} )
    {
        ImGui::TableNextRow();

        if( ImGui::TableNextColumn() )
        {
            ImGui::TextUnformatted( pName );
        }

        if( ImGui::TableNextColumn() )
        {
            if( IsPipelineStateDynamic( pDynamicStateInfo, dynamicState ) )
            {
                ImGui::PushStyleColor( ImGuiCol_Text, IM_COL32( 128, 128, 128, 255 ) );
                ImGui::TextUnformatted( "Dynamic" );
                ImGui::PopStyleColor();

                if( ImGui::IsItemHovered() )
                {
                    ImGui::SetTooltip( "This state is set dynamically." );
                }
            }
        }

        if( ImGui::TableNextColumn() )
        {
            ImGui::Text( pFormat, value );
        }
    }

    void ProfilerOverlayOutput::DrawInspectorGraphicsPipelineState()
    {
        assert( m_InspectorPipeline.m_Type == DeviceProfilerPipelineType::eGraphics );
        assert( m_InspectorPipeline.m_pCreateInfo != nullptr );
        const VkGraphicsPipelineCreateInfo& gci = m_InspectorPipeline.m_pCreateInfo->m_GraphicsPipelineCreateInfo;

        const ImGuiTableFlags tableFlags =
            //ImGuiTableFlags_BordersInnerH |
            ImGuiTableFlags_PadOuterX |
            ImGuiTableFlags_SizingStretchSame;

        const float contentPaddingTop = 2.0f;
        const float contentPaddingLeft = 5.0f;
        const float contentPaddingRight = 10.0f;
        const float contentPaddingBottom = 10.0f;

        const float dynamicColumnWidth = ImGui::CalcTextSize( "Dynamic" ).x + 5;

        auto SetupDefaultPipelineStateColumns = [&]() {
            ImGui::TableSetupColumn( "Name", 0, 1.5f );
            ImGui::TableSetupColumn( "Dynamic", ImGuiTableColumnFlags_WidthFixed, dynamicColumnWidth );
        };

        ImGui::PushStyleColor( ImGuiCol_Header, IM_COL32( 40, 40, 43, 128 ) );

        // VkPipelineVertexInputStateCreateInfo
        ImGui::BeginDisabled( gci.pVertexInputState == nullptr );
        if( ImGui::CollapsingHeader( Lang::PipelineStateVertexInput ) &&
            ( gci.pVertexInputState != nullptr ) )
        {
            const VkPipelineVertexInputStateCreateInfo& state = *gci.pVertexInputState;

            ImGuiX::BeginPadding( contentPaddingTop, contentPaddingRight, contentPaddingLeft );
            if( ImGui::BeginTable( "##VertexInputState", 6, tableFlags ) )
            {
                ImGui::TableSetupColumn( "Location" );
                ImGui::TableSetupColumn( "Binding" );
                ImGui::TableSetupColumn( "Format", 0, 3.0f );
                ImGui::TableSetupColumn( "Offset" );
                ImGui::TableSetupColumn( "Stride" );
                ImGui::TableSetupColumn( "Input rate", 0, 1.5f );
                ImGuiX::TableHeadersRow( m_Resources.GetBoldFont() );

                for( uint32_t i = 0; i < state.vertexAttributeDescriptionCount; ++i )
                {
                    const VkVertexInputAttributeDescription* pAttribute = &state.pVertexAttributeDescriptions[ i ];
                    const VkVertexInputBindingDescription* pBinding = nullptr;

                    // Find the binding description of the current attribute.
                    for( uint32_t j = 0; j < state.vertexBindingDescriptionCount; ++j )
                    {
                        if( state.pVertexBindingDescriptions[ j ].binding == pAttribute->binding )
                        {
                            pBinding = &state.pVertexBindingDescriptions[ j ];
                            break;
                        }
                    }

                    ImGui::TableNextRow();
                    ImGuiX::TableTextColumn( "%u", pAttribute->location );
                    ImGuiX::TableTextColumn( "%u", pAttribute->binding );
                    ImGuiX::TableTextColumn( "%s", m_pStringSerializer->GetFormatName( pAttribute->format ).c_str() );
                    ImGuiX::TableTextColumn( "%u", pAttribute->offset );

                    if( pBinding != nullptr )
                    {
                        ImGuiX::TableTextColumn( "%u", pBinding->stride );
                        ImGuiX::TableTextColumn( "%s", m_pStringSerializer->GetVertexInputRateName( pBinding->inputRate ).c_str() );
                    }
                }

                ImGui::EndTable();
            }

            if( state.vertexAttributeDescriptionCount == 0 )
            {
                ImGuiX::BeginPadding( 0, 0, contentPaddingLeft + 4 );
                ImGui::TextUnformatted( "No vertex data on input." );
            }

            ImGuiX::EndPadding( contentPaddingBottom );
        }
        ImGui::EndDisabled();

        // VkPipelineInputAssemblyStateCreateInfo
        ImGui::BeginDisabled( gci.pInputAssemblyState == nullptr );
        if( ImGui::CollapsingHeader( Lang::PipelineStateInputAssembly ) &&
            ( gci.pInputAssemblyState != nullptr ) )
        {
            ImGuiX::BeginPadding( contentPaddingTop, contentPaddingRight, contentPaddingLeft );
            if( ImGui::BeginTable( "##InputAssemblyState", 3, tableFlags ) )
            {
                SetupDefaultPipelineStateColumns();

                const VkPipelineInputAssemblyStateCreateInfo& state = *gci.pInputAssemblyState;
                DrawPipelineStateValue( "Topology", "%s", m_pStringSerializer->GetPrimitiveTopologyName( state.topology ).c_str(), gci.pDynamicState, VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY_EXT );
                DrawPipelineStateValue( "Primitive restart", "%s", m_pStringSerializer->GetBool( state.primitiveRestartEnable ).c_str(), gci.pDynamicState, VK_DYNAMIC_STATE_PRIMITIVE_RESTART_ENABLE_EXT );
                ImGui::EndTable();
            }
            ImGuiX::EndPadding( contentPaddingBottom );
        }
        ImGui::EndDisabled();

        // VkPipelineTessellationStateCreateInfo
        ImGui::BeginDisabled( gci.pTessellationState == nullptr );
        if( ImGui::CollapsingHeader( Lang::PipelineStateTessellation ) &&
            ( gci.pTessellationState != nullptr ) )
        {
            ImGuiX::BeginPadding( 5, 10, 10 );

            if( ImGui::BeginTable( "##TessellationState", 3, tableFlags ) )
            {
                SetupDefaultPipelineStateColumns();

                const VkPipelineTessellationStateCreateInfo& state = *gci.pTessellationState;
                DrawPipelineStateValue( "Patch control points", "%u", state.patchControlPoints, gci.pDynamicState, VK_DYNAMIC_STATE_PATCH_CONTROL_POINTS_EXT );
                ImGui::EndTable();
            }
            ImGuiX::EndPadding( contentPaddingBottom );
        }
        ImGui::EndDisabled();

        // VkPipelineViewportStateCreateInfo
        ImGui::BeginDisabled( gci.pViewportState == nullptr );
        if( ImGui::CollapsingHeader( Lang::PipelineStateViewport ) &&
            ( gci.pViewportState != nullptr ) )
        {
            const float firstColumnWidth = ImGui::CalcTextSize( "00 (Dynamic)" ).x + 5;

            ImGuiX::BeginPadding( contentPaddingTop, contentPaddingRight, contentPaddingLeft );
            if( ImGui::BeginTable( "##Viewports", 7, tableFlags ) )
            {
                ImGui::TableSetupColumn( "Viewport", ImGuiTableColumnFlags_WidthFixed, firstColumnWidth );
                ImGui::TableSetupColumn( "X" );
                ImGui::TableSetupColumn( "Y" );
                ImGui::TableSetupColumn( "Width" );
                ImGui::TableSetupColumn( "Height" );
                ImGui::TableSetupColumn( "Min Z" );
                ImGui::TableSetupColumn( "Max Z" );
                ImGuiX::TableHeadersRow( m_Resources.GetBoldFont() );

                const char* format = "%u";
                if( IsPipelineStateDynamic( gci.pDynamicState, VK_DYNAMIC_STATE_VIEWPORT ) )
                {
                    format = "%u (Dynamic)";
                }

                const VkPipelineViewportStateCreateInfo& state = *gci.pViewportState;
                for( uint32_t i = 0; i < state.viewportCount; ++i )
                {
                    ImGui::TableNextRow();
                    ImGuiX::TableTextColumn( format, i );

                    const VkViewport* pViewport = (state.pViewports ? &state.pViewports[i] : nullptr);
                    if( pViewport )
                    {
                        ImGuiX::TableTextColumn( "%.2f", pViewport->x );
                        ImGuiX::TableTextColumn( "%.2f", pViewport->y );
                        ImGuiX::TableTextColumn( "%.2f", pViewport->width );
                        ImGuiX::TableTextColumn( "%.2f", pViewport->height );
                        ImGuiX::TableTextColumn( "%.2f", pViewport->minDepth );
                        ImGuiX::TableTextColumn( "%.2f", pViewport->maxDepth );
                    }
                }

                ImGui::EndTable();
            }
            ImGuiX::EndPadding( contentPaddingBottom );

            ImGuiX::BeginPadding( contentPaddingTop, contentPaddingRight, contentPaddingLeft );
            if( ImGui::BeginTable( "##Scissors", 7, tableFlags ) )
            {
                ImGui::TableSetupColumn( "Scissor", ImGuiTableColumnFlags_WidthFixed, firstColumnWidth );
                ImGui::TableSetupColumn( "X" );
                ImGui::TableSetupColumn( "Y" );
                ImGui::TableSetupColumn( "Width" );
                ImGui::TableSetupColumn( "Height" );
                ImGuiX::TableHeadersRow( m_Resources.GetBoldFont() );

                const char* format = "%u";
                if( IsPipelineStateDynamic( gci.pDynamicState, VK_DYNAMIC_STATE_SCISSOR ) )
                {
                    format = "%u (Dynamic)";
                }

                const VkPipelineViewportStateCreateInfo& state = *gci.pViewportState;
                for( uint32_t i = 0; i < state.scissorCount; ++i )
                {
                    ImGui::TableNextRow();
                    ImGuiX::TableTextColumn( format, i );

                    const VkRect2D* pScissor = (state.pScissors ? &state.pScissors[i] : nullptr);
                    if( pScissor )
                    {
                        ImGuiX::TableTextColumn( "%u", pScissor->offset.x );
                        ImGuiX::TableTextColumn( "%u", pScissor->offset.y );
                        ImGuiX::TableTextColumn( "%u", pScissor->extent.width );
                        ImGuiX::TableTextColumn( "%u", pScissor->extent.height );
                    }
                }

                ImGui::EndTable();
            }
            ImGuiX::EndPadding( contentPaddingBottom );
        }
        ImGui::EndDisabled();

        // VkPipelineRasterizationStateCreateInfo
        ImGui::BeginDisabled( gci.pRasterizationState == nullptr );
        if( ImGui::CollapsingHeader( Lang::PipelineStateRasterization ) &&
            ( gci.pRasterizationState != nullptr ) )
        {
            ImGuiX::BeginPadding( contentPaddingTop, contentPaddingRight, contentPaddingLeft );
            if( ImGui::BeginTable( "##RasterizationState", 3, tableFlags ) )
            {
                SetupDefaultPipelineStateColumns();

                const VkPipelineRasterizationStateCreateInfo& state = *gci.pRasterizationState;
                DrawPipelineStateValue( "Depth clamp enable", "%s", m_pStringSerializer->GetBool( state.depthClampEnable ).c_str(), gci.pDynamicState, VK_DYNAMIC_STATE_DEPTH_CLAMP_ENABLE_EXT );
                DrawPipelineStateValue( "Rasterizer discard enable", "%s", m_pStringSerializer->GetBool( state.rasterizerDiscardEnable ).c_str(), gci.pDynamicState, VK_DYNAMIC_STATE_RASTERIZER_DISCARD_ENABLE_EXT );
                DrawPipelineStateValue( "Polygon mode", "%s", m_pStringSerializer->GetPolygonModeName( state.polygonMode ).c_str(), gci.pDynamicState, VK_DYNAMIC_STATE_POLYGON_MODE_EXT );
                DrawPipelineStateValue( "Cull mode", "%s", m_pStringSerializer->GetCullModeName( state.cullMode ).c_str(), gci.pDynamicState, VK_DYNAMIC_STATE_CULL_MODE_EXT );
                DrawPipelineStateValue( "Front face", "%s", m_pStringSerializer->GetFrontFaceName( state.frontFace ).c_str(), gci.pDynamicState, VK_DYNAMIC_STATE_FRONT_FACE_EXT );
                DrawPipelineStateValue( "Depth bias enable", "%s", m_pStringSerializer->GetBool( state.depthBiasEnable ).c_str(), gci.pDynamicState, VK_DYNAMIC_STATE_DEPTH_BIAS_ENABLE_EXT );
                DrawPipelineStateValue( "Depth bias constant factor", "%f", state.depthBiasConstantFactor, gci.pDynamicState, VK_DYNAMIC_STATE_DEPTH_BIAS );
                DrawPipelineStateValue( "Depth bias clamp", "%f", state.depthBiasClamp, gci.pDynamicState, VK_DYNAMIC_STATE_DEPTH_BIAS );
                DrawPipelineStateValue( "Depth bias slope factor", "%f", state.depthBiasSlopeFactor, gci.pDynamicState, VK_DYNAMIC_STATE_DEPTH_BIAS );
                DrawPipelineStateValue( "Line width", "%f", state.lineWidth, gci.pDynamicState, VK_DYNAMIC_STATE_LINE_WIDTH );
                ImGui::EndTable();
            }
            ImGuiX::EndPadding( contentPaddingBottom );
        }
        ImGui::EndDisabled();

        // VkPipelineMultisampleStateCreateInfo
        ImGui::BeginDisabled( gci.pMultisampleState == nullptr );
        if( ImGui::CollapsingHeader( Lang::PipelineStateMultisampling ) &&
            ( gci.pMultisampleState != nullptr ) )
        {
            ImGuiX::BeginPadding( contentPaddingTop, contentPaddingRight, contentPaddingLeft );
            if( ImGui::BeginTable( "##MultisampleState", 3, tableFlags ) )
            {
                SetupDefaultPipelineStateColumns();

                const VkPipelineMultisampleStateCreateInfo& state = *gci.pMultisampleState;
                DrawPipelineStateValue( "Rasterization samples", "%u", state.rasterizationSamples, gci.pDynamicState, VK_DYNAMIC_STATE_RASTERIZATION_SAMPLES_EXT );
                DrawPipelineStateValue( "Sample shading enable", "%s", m_pStringSerializer->GetBool( state.sampleShadingEnable ).c_str() );
                DrawPipelineStateValue( "Min sample shading", "%u", state.minSampleShading );
                DrawPipelineStateValue( "Sample mask", "0x%08X", state.pSampleMask ? *state.pSampleMask : 0xFFFFFFFF, gci.pDynamicState, VK_DYNAMIC_STATE_SAMPLE_MASK_EXT );
                DrawPipelineStateValue( "Alpha to coverage enable", "%s", m_pStringSerializer->GetBool( state.alphaToCoverageEnable ).c_str(), gci.pDynamicState, VK_DYNAMIC_STATE_ALPHA_TO_COVERAGE_ENABLE_EXT );
                DrawPipelineStateValue( "Alpha to one enable", "%s", m_pStringSerializer->GetBool( state.alphaToOneEnable ).c_str(), gci.pDynamicState, VK_DYNAMIC_STATE_ALPHA_TO_ONE_ENABLE_EXT );
                ImGui::EndTable();
            }
            ImGuiX::EndPadding( contentPaddingBottom );
        }
        ImGui::EndDisabled();

        // VkPipelineDepthStencilStateCreateInfo
        ImGui::BeginDisabled( gci.pDepthStencilState == nullptr );
        if( ImGui::CollapsingHeader( Lang::PipelineStateDepthStencil ) &&
            ( gci.pDepthStencilState != nullptr ) )
        {
            ImGuiX::BeginPadding( contentPaddingTop, contentPaddingRight, contentPaddingLeft );
            if( ImGui::BeginTable( "##DepthStencilState", 3, tableFlags ) )
            {
                SetupDefaultPipelineStateColumns();

                const VkPipelineDepthStencilStateCreateInfo& state = *gci.pDepthStencilState;
                DrawPipelineStateValue( "Depth test enable", "%s", m_pStringSerializer->GetBool( state.depthTestEnable ).c_str(), gci.pDynamicState, VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE_EXT );
                DrawPipelineStateValue( "Depth write enable", "%s", m_pStringSerializer->GetBool( state.depthWriteEnable ).c_str(), gci.pDynamicState, VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE_EXT );
                DrawPipelineStateValue( "Depth compare op", "%s", m_pStringSerializer->GetCompareOpName( state.depthCompareOp ).c_str(), gci.pDynamicState, VK_DYNAMIC_STATE_DEPTH_COMPARE_OP_EXT );
                DrawPipelineStateValue( "Depth bounds test enable", "%s", m_pStringSerializer->GetBool( state.depthBoundsTestEnable ).c_str(), gci.pDynamicState, VK_DYNAMIC_STATE_DEPTH_BOUNDS_TEST_ENABLE_EXT );
                DrawPipelineStateValue( "Min depth bounds", "%f", state.minDepthBounds, gci.pDynamicState, VK_DYNAMIC_STATE_DEPTH_BOUNDS );
                DrawPipelineStateValue( "Max depth bounds", "%f", state.maxDepthBounds, gci.pDynamicState, VK_DYNAMIC_STATE_DEPTH_BOUNDS );
                DrawPipelineStateValue( "Stencil test enable", "%s", m_pStringSerializer->GetBool( state.stencilTestEnable ).c_str(), gci.pDynamicState, VK_DYNAMIC_STATE_STENCIL_TEST_ENABLE_EXT );

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if( ImGui::TreeNodeEx( "Front face stencil op", ImGuiTreeNodeFlags_SpanAllColumns ) )
                {
                    DrawPipelineStateValue( "Fail op", "%u", state.front.failOp );
                    DrawPipelineStateValue( "Pass op", "%u", state.front.passOp );
                    DrawPipelineStateValue( "Depth fail op", "%u", state.front.depthFailOp );
                    DrawPipelineStateValue( "Compare op", "%s", m_pStringSerializer->GetCompareOpName( state.front.compareOp ).c_str() );
                    DrawPipelineStateValue( "Compare mask", "0x%02X", state.front.compareMask, gci.pDynamicState, VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK );
                    DrawPipelineStateValue( "Write mask", "0x%02X", state.front.writeMask, gci.pDynamicState, VK_DYNAMIC_STATE_STENCIL_WRITE_MASK );
                    DrawPipelineStateValue( "Reference", "0x%02X", state.front.reference, gci.pDynamicState, VK_DYNAMIC_STATE_STENCIL_REFERENCE );
                    ImGui::TreePop();
                }

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if( ImGui::TreeNodeEx( "Back face stencil op", ImGuiTreeNodeFlags_SpanAllColumns ) )
                {
                    DrawPipelineStateValue( "Fail op", "%u", state.back.failOp );
                    DrawPipelineStateValue( "Pass op", "%u", state.back.passOp );
                    DrawPipelineStateValue( "Depth fail op", "%u", state.back.depthFailOp );
                    DrawPipelineStateValue( "Compare op", "%s", m_pStringSerializer->GetCompareOpName( state.back.compareOp ).c_str() );
                    DrawPipelineStateValue( "Compare mask", "0x%02X", state.back.compareMask, gci.pDynamicState, VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK );
                    DrawPipelineStateValue( "Write mask", "0x%02X", state.back.writeMask, gci.pDynamicState, VK_DYNAMIC_STATE_STENCIL_WRITE_MASK );
                    DrawPipelineStateValue( "Reference", "0x%02X", state.back.reference, gci.pDynamicState, VK_DYNAMIC_STATE_STENCIL_REFERENCE );
                    ImGui::TreePop();
                }

                ImGui::EndTable();
            }
            ImGuiX::EndPadding( contentPaddingBottom );
        }
        ImGui::EndDisabled();

        // VkPipelineColorBlendStateCreateInfo
        ImGui::BeginDisabled( gci.pColorBlendState == nullptr );
        if( ImGui::CollapsingHeader( Lang::PipelineStateColorBlend ) &&
            ( gci.pColorBlendState != nullptr ) )
        {
            const VkPipelineColorBlendStateCreateInfo& state = *gci.pColorBlendState;

            ImGuiX::BeginPadding( contentPaddingTop, contentPaddingRight, contentPaddingLeft );
            if( ImGui::BeginTable( "##ColorBlendState", 3, tableFlags ) )
            {
                SetupDefaultPipelineStateColumns();
                DrawPipelineStateValue( "Logic op enable", "%s", m_pStringSerializer->GetBool( state.logicOpEnable ).c_str(), gci.pDynamicState, VK_DYNAMIC_STATE_LOGIC_OP_ENABLE_EXT );
                DrawPipelineStateValue( "Logic op", "%s", m_pStringSerializer->GetLogicOpName( state.logicOp ).c_str(), gci.pDynamicState, VK_DYNAMIC_STATE_LOGIC_OP_EXT );
                DrawPipelineStateValue( "Blend constants", "%s", m_pStringSerializer->GetVec4( state.blendConstants ).c_str(), gci.pDynamicState, VK_DYNAMIC_STATE_BLEND_CONSTANTS );
                ImGui::EndTable();
            }
            ImGuiX::EndPadding( contentPaddingBottom );

            ImGuiX::BeginPadding( contentPaddingTop, contentPaddingRight, contentPaddingLeft );
            if( ImGui::BeginTable( "##ColorBlendAttachments", 9, tableFlags ) )
            {
                const float indexColumnWidth = ImGui::CalcTextSize( "000" ).x + 5;
                const float maskColumnWidth = ImGui::CalcTextSize( "RGBA" ).x + 5;

                ImGui::TableSetupColumn( "#", ImGuiTableColumnFlags_WidthFixed, indexColumnWidth );
                ImGui::TableSetupColumn( "Enable" );
                ImGui::TableSetupColumn( "Src color" );
                ImGui::TableSetupColumn( "Dst color" );
                ImGui::TableSetupColumn( "Color op" );
                ImGui::TableSetupColumn( "Src alpha" );
                ImGui::TableSetupColumn( "Dst alpha" );
                ImGui::TableSetupColumn( "Alpha op" );
                ImGui::TableSetupColumn( "Mask", ImGuiTableColumnFlags_WidthFixed, maskColumnWidth );
                ImGuiX::TableHeadersRow( m_Resources.GetBoldFont() );

                for( uint32_t i = 0; i < state.attachmentCount; ++i )
                {
                    const VkPipelineColorBlendAttachmentState& attachment = state.pAttachments[ i ];
                    ImGui::TableNextRow();
                    ImGuiX::TableTextColumn( "%u", i );
                    ImGuiX::TableTextColumn( "%s", m_pStringSerializer->GetBool( attachment.blendEnable ).c_str() );
                    ImGuiX::TableTextColumn( "%s", m_pStringSerializer->GetBlendFactorName( attachment.srcColorBlendFactor ).c_str() );
                    ImGuiX::TableTextColumn( "%s", m_pStringSerializer->GetBlendFactorName( attachment.dstColorBlendFactor ).c_str() );
                    ImGuiX::TableTextColumn( "%s", m_pStringSerializer->GetBlendOpName( attachment.colorBlendOp ).c_str() );
                    ImGuiX::TableTextColumn( "%s", m_pStringSerializer->GetBlendFactorName( attachment.dstAlphaBlendFactor ).c_str() );
                    ImGuiX::TableTextColumn( "%s", m_pStringSerializer->GetBlendFactorName( attachment.dstAlphaBlendFactor ).c_str() );
                    ImGuiX::TableTextColumn( "%s", m_pStringSerializer->GetBlendOpName( attachment.alphaBlendOp ).c_str() );
                    ImGuiX::TableTextColumn( "%s", m_pStringSerializer->GetColorComponentFlagNames( attachment.colorWriteMask ).c_str() );
                }

                ImGui::EndTable();
            }

            if( state.attachmentCount == 0 )
            {
                ImGuiX::BeginPadding( 0, 0, contentPaddingLeft + 4 );
                ImGui::TextUnformatted( "No color attachments on output." );
            }

            ImGuiX::EndPadding( contentPaddingBottom );
        }
        ImGui::EndDisabled();

        ImGui::PopStyleColor();
    }

    /***********************************************************************************\

    Function:
        SetInspectorTabIndex

    Description:
        Switches the inspector to another tab.

    \***********************************************************************************/
    void ProfilerOverlayOutput::SetInspectorTabIndex( size_t index )
    {
        const InspectorTab& tab = m_InspectorTabs[index];
        if( tab.Select )
        {
            // Call tab-specific setup callback.
            tab.Select();
        }

        m_InspectorTabIndex = index;
    }

    /***********************************************************************************\

    Function:
        ShaderRepresentationSaved

    Description:
        Called when a shader is saved.

    \***********************************************************************************/
    void ProfilerOverlayOutput::ShaderRepresentationSaved( bool succeeded, const std::string& message )
    {
        m_SerializationSucceeded = succeeded;
        m_SerializationMessage = message;

        // Display message box
        m_SerializationFinishTimestamp = std::chrono::high_resolution_clock::now();
        m_SerializationOutputWindowSize = { 0, 0 };
        m_SerializationWindowVisible = false;
    }

    /***********************************************************************************\

    Function:
        UpdateStatisticsTab

    Description:
        Updates "Statistics" tab.

    \***********************************************************************************/
    void ProfilerOverlayOutput::UpdateStatisticsTab()
    {
        // Draw count statistics
        {
            auto PrintStatsDuration = [&]( const DeviceProfilerDrawcallStats::Stats& stats, uint64_t ticks )
                {
                    if( stats.m_TicksSum > 0 )
                    {
                        ImGuiX::TextAlignRight(
                            ImGuiX::TableGetColumnWidth(),
                            "%.2f %s",
                            m_TimestampDisplayUnit * ticks * m_TimestampPeriod.count(),
                            m_pTimestampDisplayUnitStr );
                    }
                    else
                    {
                        ImGuiX::TextAlignRight(
                            ImGuiX::TableGetColumnWidth(),
                            "-" );
                    }
                };

            auto PrintStats = [&]( const char* pName, const DeviceProfilerDrawcallStats::Stats& stats )
                {
                    if( stats.m_Count == 0 && !m_ShowEmptyStatistics )
                    {
                        return;
                    }

                    ImGui::TableNextRow();

                    // Stat name
                    if( ImGui::TableNextColumn() )
                    {
                        ImGui::TextUnformatted( pName );
                    }

                    // Count
                    if( ImGui::TableNextColumn() )
                    {
                        ImGuiX::TextAlignRight(
                            ImGuiX::TableGetColumnWidth(),
                            "%u",
                            stats.m_Count );
                    }

                    // Total duration
                    if( ImGui::TableNextColumn() )
                    {
                        PrintStatsDuration( stats, stats.m_TicksSum );
                    }

                    // Min duration
                    if( ImGui::TableNextColumn() )
                    {
                        PrintStatsDuration( stats, stats.m_TicksMin );
                    }

                    // Max duration
                    if( ImGui::TableNextColumn() )
                    {
                        PrintStatsDuration( stats, stats.m_TicksMax );
                    }

                    // Average duration
                    if( ImGui::TableNextColumn() )
                    {
                        PrintStatsDuration( stats, stats.GetTicksAvg() );
                    }
                };

            if( ImGui::BeginTable( "##StatisticsTable", 6,
                    ImGuiTableFlags_BordersInnerH |
                    ImGuiTableFlags_PadOuterX |
                    ImGuiTableFlags_Hideable |
                    ImGuiTableFlags_ContextMenuInBody |
                    ImGuiTableFlags_NoClip |
                    ImGuiTableFlags_SizingStretchProp ) )
            {
                ImGui::TableSetupColumn( Lang::StatName, ImGuiTableColumnFlags_NoHide, 3.0f );
                ImGui::TableSetupColumn( Lang::StatCount, 0, 1.0f );
                ImGui::TableSetupColumn( Lang::StatTotal, 0, 1.0f );
                ImGui::TableSetupColumn( Lang::StatMin, 0, 1.0f );
                ImGui::TableSetupColumn( Lang::StatMax, 0, 1.0f );
                ImGui::TableSetupColumn( Lang::StatAvg, 0, 1.0f );
                ImGui::TableNextRow();

                ImGui::PushFont( m_Resources.GetBoldFont() );
                ImGui::TableNextColumn();
                ImGui::TextUnformatted( Lang::StatName );
                ImGui::TableNextColumn();
                ImGuiX::TextAlignRight( ImGuiX::TableGetColumnWidth(), Lang::StatCount );
                ImGui::TableNextColumn();
                ImGuiX::TextAlignRight( ImGuiX::TableGetColumnWidth(), Lang::StatTotal );
                ImGui::TableNextColumn();
                ImGuiX::TextAlignRight( ImGuiX::TableGetColumnWidth(), Lang::StatMin );
                ImGui::TableNextColumn();
                ImGuiX::TextAlignRight( ImGuiX::TableGetColumnWidth(), Lang::StatMax );
                ImGui::TableNextColumn();
                ImGuiX::TextAlignRight( ImGuiX::TableGetColumnWidth(), Lang::StatAvg );
                ImGui::PopFont();

                PrintStats( Lang::DrawCalls, m_pData->m_Stats.m_DrawStats );
                PrintStats( Lang::DrawCallsIndirect, m_pData->m_Stats.m_DrawIndirectStats );
                PrintStats( Lang::DrawMeshTasksCalls, m_pData->m_Stats.m_DrawMeshTasksStats );
                PrintStats( Lang::DrawMeshTasksIndirectCalls, m_pData->m_Stats.m_DrawMeshTasksIndirectStats );
                PrintStats( Lang::DispatchCalls, m_pData->m_Stats.m_DispatchStats );
                PrintStats( Lang::DispatchCallsIndirect, m_pData->m_Stats.m_DispatchIndirectStats );
                PrintStats( Lang::TraceRaysCalls, m_pData->m_Stats.m_TraceRaysStats );
                PrintStats( Lang::TraceRaysIndirectCalls, m_pData->m_Stats.m_TraceRaysIndirectStats );
                PrintStats( Lang::CopyBufferCalls, m_pData->m_Stats.m_CopyBufferStats );
                PrintStats( Lang::CopyBufferToImageCalls, m_pData->m_Stats.m_CopyBufferToImageStats );
                PrintStats( Lang::CopyImageCalls, m_pData->m_Stats.m_CopyImageStats );
                PrintStats( Lang::CopyImageToBufferCalls, m_pData->m_Stats.m_CopyImageToBufferStats );
                PrintStats( Lang::PipelineBarriers, m_pData->m_Stats.m_PipelineBarrierStats );
                PrintStats( Lang::ColorClearCalls, m_pData->m_Stats.m_ClearColorStats );
                PrintStats( Lang::DepthStencilClearCalls, m_pData->m_Stats.m_ClearDepthStencilStats );
                PrintStats( Lang::ResolveCalls, m_pData->m_Stats.m_ResolveStats );
                PrintStats( Lang::BlitCalls, m_pData->m_Stats.m_BlitImageStats );
                PrintStats( Lang::FillBufferCalls, m_pData->m_Stats.m_FillBufferStats );
                PrintStats( Lang::UpdateBufferCalls, m_pData->m_Stats.m_UpdateBufferStats );

                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                if( m_ShowEmptyStatistics )
                {
                    if( ImGui::TextLink( Lang::HideEmptyStatistics ) )
                    {
                        m_ShowEmptyStatistics = false;
                    }
                }
                else
                {
                    if( ImGui::TextLink( Lang::ShowEmptyStatistics ) )
                    {
                        m_ShowEmptyStatistics = true;
                    }
                }

                ImGui::EndTable();
            }
        }
    }

    /***********************************************************************************\

    Function:
        UpdateSettingsTab

    Description:
        Updates "Settings" tab.

    \***********************************************************************************/
    void ProfilerOverlayOutput::UpdateSettingsTab()
    {
        // Set interface scaling.
        float interfaceScale = ImGui::GetIO().FontGlobalScale;
        if( ImGui::InputFloat( Lang::InterfaceScale, &interfaceScale ) )
        {
            ImGui::GetIO().FontGlobalScale = std::clamp( interfaceScale, 0.25f, 4.0f );
        }

        // Set number of collected frames
        int maxFrameCount = static_cast<int>( m_MaxFrameCount );
        if( ImGui::InputInt( Lang::CollectedFrameCount, &maxFrameCount ) )
        {
            SetMaxFrameCount( std::max<uint32_t>( 0, maxFrameCount ) );
        }

        // Select sampling mode (constant in runtime for now)
        ImGui::BeginDisabled();
        {
            static const char* samplingGroupOptions[] = {
                "Drawcall",
                "Pipeline",
                "Render pass",
                "Command buffer"
            };

            int samplingModeSelectedOption = static_cast<int>(m_SamplingMode);
            if( ImGui::Combo( Lang::SamplingMode, &samplingModeSelectedOption, samplingGroupOptions, 4 ) )
            {
                assert( false );
            }
        }
        ImGui::EndDisabled();

        // Select frame delimiter (constant in runtime)
        ImGui::BeginDisabled();
        {
            static const char* frameDelimiterOptions[] = {
                Lang::Present,
                Lang::Submit };

            int frameDelimiterSelectedOption = static_cast<int>(m_FrameDelimiter);
            if( ImGui::Combo( Lang::FrameDelimiter, &frameDelimiterSelectedOption, frameDelimiterOptions, 2 ) )
            {
                assert( false );
            }
        }
        ImGui::EndDisabled();

        // Select time display unit.
        {
            static const char* timeUnitGroupOptions[] = {
                Lang::Milliseconds,
                Lang::Microseconds,
                Lang::Nanoseconds };

            int timeUnitSelectedOption = static_cast<int>(m_TimeUnit);
            if( ImGui::Combo( Lang::TimeUnit, &timeUnitSelectedOption, timeUnitGroupOptions, 3 ) )
            {
                static float timeUnitFactors[] = {
                    1.0f,
                    1'000.0f,
                    1'000'000.0f
                };

                m_TimeUnit = static_cast<TimeUnit>(timeUnitSelectedOption);
                m_TimestampDisplayUnit = timeUnitFactors[ timeUnitSelectedOption ];
                m_pTimestampDisplayUnitStr = timeUnitGroupOptions[ timeUnitSelectedOption ];
            }
        }

        // Display debug labels in frame browser.
        ImGui::Checkbox( Lang::ShowDebugLabels, &m_ShowDebugLabels );

        // Display shader capability badges in frame browser.
        ImGui::Checkbox( Lang::ShowShaderCapabilities, &m_ShowShaderCapabilities );
    }

    /***********************************************************************************\

    Function:
        GetQueueGraphColumns

    Description:
        Enumerate queue utilization graph columns.

    \***********************************************************************************/
    void ProfilerOverlayOutput::GetQueueGraphColumns( VkQueue queue, std::vector<QueueGraphColumn>& columns ) const
    {
        FrameBrowserTreeNodeIndex index;
        index.SetFrameIndex( static_cast<uint32_t>( m_pFrames.size() - 1 ) );

        uint64_t lastTimestamp;

        std::shared_ptr<DeviceProfilerFrameData> pFirstFrame = m_ShowActiveFrame ? m_pData : m_pFrames.front();
        std::shared_ptr<DeviceProfilerFrameData> pLastFrame = m_ShowActiveFrame ? m_pData : m_pFrames.back();
        lastTimestamp = pFirstFrame->m_BeginTimestamp;

        auto AppendSemaphoreEvent = [&]( const std::vector<VkSemaphore>& semaphores, QueueGraphColumn::DataType type ) {
            QueueGraphColumn& column = columns.emplace_back();
            column.flags = ImGuiX::HistogramColumnFlags_Event;
            column.color = IM_COL32( 128, 128, 128, 255 );
            column.userDataType = type;
            column.userData = &semaphores;

            // Highlight events with selected semaphores.
            for( VkSemaphore semaphore : semaphores )
            {
                if( m_SelectedSemaphores.count( semaphore ) )
                {
                    column.color = IM_COL32( 255, 32, 16, 255 );
                    break;
                }
            }
        };

        for( const auto& pFrame : m_pFrames )
        {
            const uint32_t frameIndex = index.GetFrameIndex();

            // Skip other frames if requested
            if( m_ShowActiveFrame )
            {
                if( frameIndex != m_SelectedFrameIndex )
                {
                    index.SetFrameIndex( frameIndex - 1 );
                    continue;
                }
            }

            const bool isActiveFrame = ( frameIndex == m_SelectedFrameIndex );

            // Count queue submits in the frame.
            index.emplace_back( 0 );

            for( const auto& submitBatch : pFrame->m_Submits )
            {
                if( submitBatch.m_Handle != queue )
                {
                    // Index must be incremented to account for the submissions on the other queues.
                    index.back()++;
                    continue;
                }

                // Count submit infos.
                index.emplace_back( 0 );

                for( const auto& submit : submitBatch.m_Submits )
                {
                    // Count command buffers.
                    index.emplace_back( 0 );

                    bool firstCommandBuffer = true;

                    for( const auto& commandBuffer : submit.m_CommandBuffers )
                    {
                        if( !commandBuffer.m_DataValid )
                        {
                            // Take command buffers with no data into account.
                            index.back()++;
                            continue;
                        }

                        if( lastTimestamp != commandBuffer.m_BeginTimestamp.m_Value )
                        {
                            QueueGraphColumn& idle = columns.emplace_back();
                            idle.x = GetDuration( lastTimestamp, commandBuffer.m_BeginTimestamp.m_Value );
                            idle.y = 1;
                            idle.color = 0;
                            idle.userDataType = QueueGraphColumn::eIdle;
                            idle.userData = nullptr;
                        }

                        if( firstCommandBuffer && !submit.m_WaitSemaphores.empty() )
                        {
                            // Enumerate wait semaphores before the first executed command buffer.
                            AppendSemaphoreEvent( submit.m_WaitSemaphores, QueueGraphColumn::eWaitSemaphores );
                        }

                        QueueGraphColumn& column = columns.emplace_back();
                        column.x = GetDuration( commandBuffer );
                        column.y = 1;
                        column.color = ImGuiX::ColorAlpha( m_GraphicsPipelineColumnColor, isActiveFrame ? 1.0f : 0.2f );
                        column.userDataType = QueueGraphColumn::eCommandBuffer;
                        column.userData = &commandBuffer;
                        column.nodeIndex = index;

                        lastTimestamp = commandBuffer.m_EndTimestamp.m_Value;
                        firstCommandBuffer = false;

                        index.back()++;
                    }

                    // Insert wait semaphores if no command buffers were submitted.
                    if( firstCommandBuffer && !submit.m_WaitSemaphores.empty() )
                    {
                        AppendSemaphoreEvent( submit.m_WaitSemaphores, QueueGraphColumn::eWaitSemaphores );
                    }

                    // Enumerate signal semaphores after the last executed command buffer.
                    if( !submit.m_SignalSemaphores.empty() )
                    {
                        AppendSemaphoreEvent( submit.m_SignalSemaphores, QueueGraphColumn::eSignalSemaphores );
                    }

                    index.pop_back();
                    index.back()++;
                }

                index.pop_back();
                index.back()++;
            }

            index.pop_back();
            index.SetFrameIndex( frameIndex - 1 );
        }

        if( ( lastTimestamp != pFirstFrame->m_BeginTimestamp ) &&
            ( lastTimestamp != pLastFrame->m_EndTimestamp ) )
        {
            QueueGraphColumn& idle = columns.emplace_back();
            idle.x = GetDuration( lastTimestamp, pLastFrame->m_EndTimestamp );
            idle.y = 1;
            idle.color = 0;
            idle.userDataType = QueueGraphColumn::eIdle;
            idle.userData = nullptr;
        }
    }

    /***********************************************************************************\

    Function:
        GetQueueUtilization

    Description:
        Calculate queue utilization.

    \***********************************************************************************/
    float ProfilerOverlayOutput::GetQueueUtilization( const std::vector<QueueGraphColumn>& columns ) const
    {
        float utilization = 0.0f;
        for( const auto& column : columns )
        {
            if( column.userDataType == QueueGraphColumn::eCommandBuffer )
            {
                utilization += column.x * column.y;
            }
        }

        return utilization;
    }

    /***********************************************************************************\

    Function:
        GetPerformanceGraphColumns

    Description:
        Enumerate performance graph columns.

    \***********************************************************************************/
    void ProfilerOverlayOutput::GetPerformanceGraphColumns( std::vector<PerformanceGraphColumn>& columns ) const
    {
        FrameBrowserTreeNodeIndex index;
        index.SetFrameIndex( static_cast<uint32_t>( m_pFrames.size() - 1 ) );

        using QueueTimestampPair = std::pair<VkQueue, uint64_t>;
        const auto& queues = m_Frontend.GetDeviceQueues();
        const size_t queueCount = queues.size();

        QueueTimestampPair* pLastTimestampsPerQueue = nullptr;
        bool lastTimstampsPerQueueUsesHeapAllocation = false;

        // Allocate a timestamp per each queue in the profiled device
        if( m_HistogramShowIdle )
        {
            const size_t allocationSize = queueCount * sizeof( QueueTimestampPair );
            if( allocationSize > 1024 )
            {
                // Switch to heap allocations when number of queues is large
                lastTimstampsPerQueueUsesHeapAllocation = true;
                pLastTimestampsPerQueue =
                    static_cast<QueueTimestampPair*>( malloc( allocationSize ) );
            }
            else
            {
                // Prefer allocation on stack when array is small enough
                pLastTimestampsPerQueue =
                    static_cast<QueueTimestampPair*>( alloca( queueCount * sizeof( QueueTimestampPair ) ) );
            }

            // Initialize the array
            if( pLastTimestampsPerQueue != nullptr )
            {
                size_t i = 0;
                for( const auto& queue : queues )
                {
                    pLastTimestampsPerQueue[i].first = queue.second.Handle;
                    pLastTimestampsPerQueue[i].second = 0;
                    i++;
                }
            }
        }

        for( std::shared_ptr<DeviceProfilerFrameData> pFrame : m_pFrames )
        {
            const uint32_t frameIndex = index.GetFrameIndex();

            // Skip other frames if requested
            if( m_ShowActiveFrame )
            {
                if( frameIndex != m_SelectedFrameIndex )
                {
                    index.SetFrameIndex( frameIndex - 1 );
                    continue;
                }
            }

            // Enumerate frames only
            if( m_HistogramGroupMode == HistogramGroupMode::eFrame )
            {
                PerformanceGraphColumn& column = columns.emplace_back();
                column.x = GetDuration( pFrame->m_BeginTimestamp, pFrame->m_EndTimestamp );
                column.y = ( m_HistogramValueMode == HistogramValueMode::eDuration ? column.x : 1 );
                column.color = ImGuiX::ColorAlpha( m_RenderPassColumnColor, frameIndex == m_SelectedFrameIndex ? 1.0f : 0.2f );
                column.userData = pFrame.get();
                column.groupMode = HistogramGroupMode::eFrame;
                column.nodeIndex = index;

                index.SetFrameIndex( frameIndex - 1 );
                continue;
            }

            index.emplace_back( 0 );

            // Enumerate submits batches in frame
            for( const auto& submitBatch : pFrame->m_Submits )
            {
                index.emplace_back( 0 );

                // End timestamp of the last executed command buffer on this queue
                QueueTimestampPair* pLastQueueTimestamp = nullptr;

                if( m_HistogramShowIdle && pLastTimestampsPerQueue != nullptr )
                {
                    for( size_t i = 0; i < queueCount; ++i )
                    {
                        if( pLastTimestampsPerQueue[i].first == submitBatch.m_Handle )
                        {
                            pLastQueueTimestamp = &pLastTimestampsPerQueue[i];
                            break;
                        }
                    }
                }

                // Enumerate submits in submit batch
                for( const auto& submit : submitBatch.m_Submits )
                {
                    index.emplace_back( 0 );

                    // Enumerate command buffers in submit
                    for( const auto& commandBuffer : submit.m_CommandBuffers )
                    {
                        // Insert idle time since last command buffer
                        if( m_HistogramShowIdle &&
                            ( pLastQueueTimestamp != nullptr ) &&
                            ( commandBuffer.m_BeginTimestamp.m_Index != UINT64_MAX ) &&
                            ( commandBuffer.m_EndTimestamp.m_Index != UINT64_MAX ) )
                        {
                            if( pLastQueueTimestamp->second != 0 )
                            {
                                PerformanceGraphColumn& column = columns.emplace_back();
                                column.x = GetDuration( pLastQueueTimestamp->second, commandBuffer.m_BeginTimestamp.m_Value );
                                column.y = 0;
                            }

                            pLastQueueTimestamp->second = commandBuffer.m_EndTimestamp.m_Value;
                        }

                        GetPerformanceGraphColumns( commandBuffer, index, columns );
                        index.back()++;
                    }

                    index.pop_back();
                    index.back()++;
                }

                index.pop_back();
                index.back()++;
            }

            index.pop_back();
            index.SetFrameIndex( index.GetFrameIndex() - 1 );
        }

        // Free memory allocated on heap
        if( lastTimstampsPerQueueUsesHeapAllocation )
        {
            free( pLastTimestampsPerQueue );
        }

        assert( index.size() == 2 );
    }

    /***********************************************************************************\

    Function:
        GetPerformanceGraphColumns

    Description:
        Enumerate performance graph columns.

    \***********************************************************************************/
    void ProfilerOverlayOutput::GetPerformanceGraphColumns(
        const DeviceProfilerCommandBufferData& data,
        FrameBrowserTreeNodeIndex& index,
        std::vector<PerformanceGraphColumn>& columns ) const
    {
        index.emplace_back( 0 );

        // Enumerate render passes in command buffer
        for( const auto& renderPass : data.m_RenderPasses )
        {
            GetPerformanceGraphColumns( renderPass, index, columns );
            index.back()++;
        }

        index.pop_back();
    }

    /***********************************************************************************\

    Function:
        GetPerformanceGraphColumns

    Description:
        Enumerate performance graph columns.

    \***********************************************************************************/
    void ProfilerOverlayOutput::GetPerformanceGraphColumns(
        const DeviceProfilerRenderPassData& data,
        FrameBrowserTreeNodeIndex& index,
        std::vector<PerformanceGraphColumn>& columns ) const
    {
        const bool isActiveFrame = ( index.GetFrameIndex() == m_SelectedFrameIndex );

        if( (m_HistogramGroupMode <= HistogramGroupMode::eRenderPass) &&
            ((data.m_Handle != VK_NULL_HANDLE) ||
             (data.m_Dynamic == true) ||
             (m_SamplingMode == VK_PROFILER_MODE_PER_RENDER_PASS_EXT)) )
        {
            const float cycleCount = GetDuration( data );

            PerformanceGraphColumn& column = columns.emplace_back();
            column.x = cycleCount;
            column.y = (m_HistogramValueMode == HistogramValueMode::eDuration ? cycleCount : 1);
            column.color = ImGuiX::ColorAlpha( m_RenderPassColumnColor, isActiveFrame ? 1.0f : 0.2f );
            column.userData = &data;
            column.groupMode = HistogramGroupMode::eRenderPass;
            column.nodeIndex = index;
        }
        else
        {
            index.emplace_back( 0 );
            if( data.HasBeginCommand() )
            {
                const float cycleCount = GetDuration( data.m_Begin );

                PerformanceGraphColumn& column = columns.emplace_back();
                column.x = cycleCount;
                column.y = (m_HistogramValueMode == HistogramValueMode::eDuration ? cycleCount : 1);
                column.color = ImGuiX::ColorAlpha( m_GraphicsPipelineColumnColor, isActiveFrame ? 1.0f : 0.2f );
                column.userData = &data;
                column.groupMode = HistogramGroupMode::eRenderPassBegin;
                column.nodeIndex = index;

                index.back()++;
            }

            // Enumerate subpasses in render pass
            for( const auto& subpass : data.m_Subpasses )
            {
                index.emplace_back( 0 );

                // Treat data as pipelines if subpass contents are inline-only.
                if( subpass.m_Contents == VK_SUBPASS_CONTENTS_INLINE )
                {
                    for( const auto& data : subpass.m_Data )
                    {
                        GetPerformanceGraphColumns( std::get<DeviceProfilerPipelineData>( data ), index, columns );
                        index.back()++;
                    }
                }

                // Treat data as secondary command buffers if subpass contents are secondary command buffers only.
                else if( subpass.m_Contents == VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS )
                {
                    for( const auto& data : subpass.m_Data )
                    {
                        GetPerformanceGraphColumns( std::get<DeviceProfilerCommandBufferData>( data ), index, columns );
                        index.back()++;
                    }
                }

                // With VK_EXT_nested_command_buffer, it is possible to insert both command buffers and inline commands in the same subpass.
                else if( subpass.m_Contents == VK_SUBPASS_CONTENTS_INLINE_AND_SECONDARY_COMMAND_BUFFERS_EXT )
                {
                    for( const auto& data : subpass.m_Data )
                    {
                        switch( data.GetType() )
                        {
                        case DeviceProfilerSubpassDataType::ePipeline:
                            GetPerformanceGraphColumns( std::get<DeviceProfilerPipelineData>( data ), index, columns );
                            break;

                        case DeviceProfilerSubpassDataType::eCommandBuffer:
                            GetPerformanceGraphColumns( std::get<DeviceProfilerCommandBufferData>( data ), index, columns );
                            break;
                        }
                        index.back()++;
                    }
                }

                index.pop_back();
                index.back()++;
            }

            if( data.HasEndCommand() )
            {
                const float cycleCount = GetDuration( data.m_End );

                PerformanceGraphColumn& column = columns.emplace_back();
                column.x = cycleCount;
                column.y = (m_HistogramValueMode == HistogramValueMode::eDuration ? cycleCount : 1);
                column.color = ImGuiX::ColorAlpha( m_GraphicsPipelineColumnColor, isActiveFrame ? 1.0f : 0.2f );
                column.userData = &data;
                column.groupMode = HistogramGroupMode::eRenderPassEnd;
                column.nodeIndex = index;
            }

            index.pop_back();
        }
    }

    /***********************************************************************************\

    Function:
        GetPerformanceGraphColumns

    Description:
        Enumerate performance graph columns.

    \***********************************************************************************/
    void ProfilerOverlayOutput::GetPerformanceGraphColumns(
        const DeviceProfilerPipelineData& data,
        FrameBrowserTreeNodeIndex& index,
        std::vector<PerformanceGraphColumn>& columns ) const
    {
        if( (m_HistogramGroupMode <= HistogramGroupMode::ePipeline) &&
            ((((data.m_ShaderTuple.m_Hash & 0xFFFF) != 0) &&
              (data.m_Handle != VK_NULL_HANDLE)) ||
             (m_SamplingMode == VK_PROFILER_MODE_PER_PIPELINE_EXT)) )
        {
            const bool isActiveFrame = ( index.GetFrameIndex() == m_SelectedFrameIndex );
            const float cycleCount = GetDuration( data );

            PerformanceGraphColumn& column = columns.emplace_back();
            column.x = cycleCount;
            column.y = (m_HistogramValueMode == HistogramValueMode::eDuration ? cycleCount : 1);
            column.userData = &data;
            column.groupMode = HistogramGroupMode::ePipeline;
            column.nodeIndex = index;

            switch( data.m_BindPoint )
            {
            case VK_PIPELINE_BIND_POINT_GRAPHICS:
                column.color = ImGuiX::ColorAlpha( m_GraphicsPipelineColumnColor, isActiveFrame ? 1.0f : 0.2f );
                break;

            case VK_PIPELINE_BIND_POINT_COMPUTE:
                column.color = ImGuiX::ColorAlpha( m_ComputePipelineColumnColor, isActiveFrame ? 1.0f : 0.2f );
                break;

            case VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR:
                column.color = ImGuiX::ColorAlpha( m_RayTracingPipelineColumnColor, isActiveFrame ? 1.0f : 0.2f );
                break;

            default:
                assert( !"Unsupported pipeline type" );
                break;
            }
        }
        else
        {
            index.emplace_back( 0 );

            // Enumerate drawcalls in pipeline
            for( const auto& drawcall : data.m_Drawcalls )
            {
                GetPerformanceGraphColumns( drawcall, index, columns );
                index.back()++;
            }

            index.pop_back();
        }
    }

    /***********************************************************************************\

    Function:
        GetPerformanceGraphColumns

    Description:
        Enumerate performance graph columns.

    \***********************************************************************************/
    void ProfilerOverlayOutput::GetPerformanceGraphColumns(
        const DeviceProfilerDrawcall& data,
        FrameBrowserTreeNodeIndex& index,
        std::vector<PerformanceGraphColumn>& columns ) const
    {
        const bool isActiveFrame = ( index.GetFrameIndex() == m_SelectedFrameIndex );
        const float cycleCount = GetDuration( data );

        PerformanceGraphColumn& column = columns.emplace_back();
        column.x = cycleCount;
        column.y = (m_HistogramValueMode == HistogramValueMode::eDuration ? cycleCount : 1);
        column.userData = &data;
        column.groupMode = HistogramGroupMode::eDrawcall;
        column.nodeIndex = index;

        switch( data.GetPipelineType() )
        {
        case DeviceProfilerPipelineType::eGraphics:
            column.color = ImGuiX::ColorAlpha( m_GraphicsPipelineColumnColor, isActiveFrame ? 1.0f : 0.2f );
            break;

        case DeviceProfilerPipelineType::eCompute:
            column.color = ImGuiX::ColorAlpha( m_ComputePipelineColumnColor, isActiveFrame ? 1.0f : 0.2f );
            break;

        default:
            column.color = ImGuiX::ColorAlpha( m_InternalPipelineColumnColor, isActiveFrame ? 1.0f : 0.2f );
            break;
        }
    }

    /***********************************************************************************\

    Function:
        DrawPerformanceGraphLabel

    Description:
        Draw label for hovered column.

    \***********************************************************************************/
    void ProfilerOverlayOutput::DrawPerformanceGraphLabel( const ImGuiX::HistogramColumnData& data_ )
    {
        const PerformanceGraphColumn& data = reinterpret_cast<const PerformanceGraphColumn&>(data_);

        std::string regionName = "";
        float regionDuration = 0;

        switch( data.groupMode )
        {
        case HistogramGroupMode::eFrame:
        {
            const DeviceProfilerFrameData& frameData =
                *reinterpret_cast<const DeviceProfilerFrameData*>( data.userData );

            regionName = fmt::format( "{} #{}", m_pFrameStr, frameData.m_CPU.m_FrameIndex );
            regionDuration = GetDuration( frameData.m_BeginTimestamp, frameData.m_EndTimestamp );
            break;
        }

        case HistogramGroupMode::eRenderPass:
        {
            const DeviceProfilerRenderPassData& renderPassData =
                *reinterpret_cast<const DeviceProfilerRenderPassData*>(data.userData);

            regionName = m_pStringSerializer->GetName( renderPassData );
            regionDuration = GetDuration( renderPassData );
            break;
        }

        case HistogramGroupMode::ePipeline:
        {
            const DeviceProfilerPipelineData& pipelineData =
                *reinterpret_cast<const DeviceProfilerPipelineData*>(data.userData);

            regionName = m_pStringSerializer->GetName( pipelineData );
            regionDuration = GetDuration( pipelineData );
            break;
        }

        case HistogramGroupMode::eDrawcall:
        {
            const DeviceProfilerDrawcall& drawcallData =
                *reinterpret_cast<const DeviceProfilerDrawcall*>(data.userData);

            regionName = m_pStringSerializer->GetName( drawcallData );
            regionDuration = GetDuration( drawcallData );
            break;
        }

        case HistogramGroupMode::eRenderPassBegin:
        {
            const DeviceProfilerRenderPassData& renderPassData =
                *reinterpret_cast<const DeviceProfilerRenderPassData*>( data.userData );

            regionName = m_pStringSerializer->GetName( renderPassData.m_Begin, renderPassData.m_Dynamic );
            regionDuration = GetDuration( renderPassData.m_Begin );
            break;
        }

        case HistogramGroupMode::eRenderPassEnd:
        {
            const DeviceProfilerRenderPassData& renderPassData =
                *reinterpret_cast<const DeviceProfilerRenderPassData*>( data.userData );

            regionName = m_pStringSerializer->GetName( renderPassData.m_End, renderPassData.m_Dynamic );
            regionDuration = GetDuration( renderPassData.m_End );
            break;
        }
        }

        ImGui::SetTooltip( "%s\n%.2f %s",
            regionName.c_str(),
            regionDuration,
            m_pTimestampDisplayUnitStr );
    }

    /***********************************************************************************\

    Function:
        SelectPerformanceGraphColumn

    Description:
        Scroll frame browser to node selected in performance graph.

    \***********************************************************************************/
    void ProfilerOverlayOutput::SelectPerformanceGraphColumn( const ImGuiX::HistogramColumnData& data_ )
    {
        const PerformanceGraphColumn& data = reinterpret_cast<const PerformanceGraphColumn&>(data_);

        m_SelectedFrameBrowserNodeIndex = data.nodeIndex;
        m_ScrollToSelectedFrameBrowserNode = true;

        m_SelectionUpdateTimestamp = std::chrono::high_resolution_clock::now();
    }

    /***********************************************************************************\

    Function:
        ScrollToSelectedFrameBrowserNode

    Description:
        Checks if the frame browser should scroll to the node at the given index
        (or its child).

    \***********************************************************************************/
    bool ProfilerOverlayOutput::ScrollToSelectedFrameBrowserNode( const FrameBrowserTreeNodeIndex& index ) const
    {
        if( !m_ScrollToSelectedFrameBrowserNode )
        {
            return false;
        }

        if( m_SelectedFrameBrowserNodeIndex.size() < index.size() )
        {
            return false;
        }

        return memcmp( m_SelectedFrameBrowserNodeIndex.data(),
            index.data(),
            index.size() * sizeof( FrameBrowserTreeNodeIndex::value_type ) ) == 0;
    }

    /***********************************************************************************\

    Function:
        GetFrameBrowserNodeIndexStr

    Description:
        Returns pointer to a string representation of the index.
        The pointer remains valid until the next call to this function.

    \***********************************************************************************/
    const char* ProfilerOverlayOutput::GetFrameBrowserNodeIndexStr( const FrameBrowserTreeNodeIndex& index )
    {
        // Allocate size for the string.
        m_FrameBrowserNodeIndexStr.resize(
            (index.GetTreeNodeIndexSize() * sizeof( FrameBrowserTreeNodeIndex::value_type ) * 2) + 1 );

        ProfilerStringFunctions::Hex(
            m_FrameBrowserNodeIndexStr.data(),
            index.GetTreeNodeIndex(),
            index.GetTreeNodeIndexSize() );

        return m_FrameBrowserNodeIndexStr.data();
    }

    /***********************************************************************************\

    Function:
        GetDefaultPerformanceCountersFileName

    Description:
        Returns the default file name for performance counters.

    \***********************************************************************************/
    std::string ProfilerOverlayOutput::GetDefaultPerformanceCountersFileName( uint32_t metricsSetIndex ) const
    {
        std::stringstream stringBuilder;
        stringBuilder << ProfilerPlatformFunctions::GetProcessName() << "_";
        stringBuilder << ProfilerPlatformFunctions::GetCurrentProcessId() << "_";

        if( metricsSetIndex < m_VendorMetricsSets.size() )
        {
            std::string metricsSetName = m_VendorMetricsSets[metricsSetIndex].m_Properties.name;
            std::replace( metricsSetName.begin(), metricsSetName.end(), ' ', '_' );
            stringBuilder << metricsSetName << "_";
        }

        stringBuilder << "counters.csv";

        return stringBuilder.str();
    }

    /***********************************************************************************\

    Function:
        UpdatePerformanceCounterExporter

    Description:
        Shows a file dialog if performance counter save or load was requested and
        saves/loads them when OK is pressed.

    \***********************************************************************************/
    void ProfilerOverlayOutput::UpdatePerformanceCounterExporter()
    {
        static const std::string scFileDialogId = "#PerformanceCountersSaveFileDialog";

        if( m_pPerformanceCounterExporter != nullptr )
        {
            // Initialize the file dialog on the first call to this function.
            if( !m_pPerformanceCounterExporter->m_FileDialog.IsOpened() )
            {
                m_pPerformanceCounterExporter->m_FileDialogConfig.flags =
                    ImGuiFileDialogFlags_Default;

                if( m_pPerformanceCounterExporter->m_Action == PerformanceCounterExporter::Action::eImport )
                {
                    // Don't ask for overwrite when selecting file to load.
                    m_pPerformanceCounterExporter->m_FileDialogConfig.flags ^=
                        ImGuiFileDialogFlags_ConfirmOverwrite;
                }

                if( m_pPerformanceCounterExporter->m_Action == PerformanceCounterExporter::Action::eExport )
                {
                    m_pPerformanceCounterExporter->m_FileDialogConfig.fileName =
                        GetDefaultPerformanceCountersFileName( m_pPerformanceCounterExporter->m_MetricsSetIndex );
                }
            }

            // Draw the file dialog until the user closes it.
            bool closed = DisplayFileDialog(
                scFileDialogId,
                m_pPerformanceCounterExporter->m_FileDialog,
                m_pPerformanceCounterExporter->m_FileDialogConfig,
                "Select performance counters file path",
                ".csv" );

            if( closed )
            {
                if( m_pPerformanceCounterExporter->m_FileDialog.IsOk() )
                {
                    switch (m_pPerformanceCounterExporter->m_Action)
                    {
                    case PerformanceCounterExporter::Action::eExport:
                        SavePerformanceCountersToFile(
                            m_pPerformanceCounterExporter->m_FileDialog.GetFilePathName(),
                            m_pPerformanceCounterExporter->m_MetricsSetIndex,
                            m_pPerformanceCounterExporter->m_Data,
                            m_pPerformanceCounterExporter->m_DataMask );
                        break;

                    case PerformanceCounterExporter::Action::eImport:
                        LoadPerformanceCountersFromFile(
                            m_pPerformanceCounterExporter->m_FileDialog.GetFilePathName() );
                        break;
                    }
                }

                // Destroy the exporter.
                m_pPerformanceCounterExporter.reset();
            }
        }
    }

    /***********************************************************************************\

    Function:
        SavePerformanceCountersToFile

    Description:
        Writes performance counters data to a CSV file.

    \***********************************************************************************/
    void ProfilerOverlayOutput::SavePerformanceCountersToFile(
        const std::string& fileName,
        uint32_t metricsSetIndex,
        const std::vector<VkProfilerPerformanceCounterResultEXT>& data,
        const std::vector<bool>& mask )
    {
        DeviceProfilerCsvSerializer serializer;

        if( serializer.Open( fileName ) )
        {
            const std::vector<VkProfilerPerformanceCounterPropertiesEXT>& properties =
                m_VendorMetricsSets[metricsSetIndex].m_Metrics;

            std::vector<VkProfilerPerformanceCounterPropertiesEXT> exportedProperties;
            std::vector<VkProfilerPerformanceCounterResultEXT> exportedData;

            for( size_t i = 0; i < data.size(); ++i )
            {
                if( mask[i] )
                {
                    exportedData.push_back( data[i] );
                    exportedProperties.push_back( properties[i] );
                }
            }

            serializer.WriteHeader( static_cast<uint32_t>( exportedProperties.size() ), exportedProperties.data() );
            serializer.WriteRow( static_cast<uint32_t>( exportedData.size() ), exportedData.data() );
            serializer.Close();

            m_SerializationSucceeded = true;
            m_SerializationMessage = "Performance counters saved successfully.\n" + fileName;
        }
        else
        {
            m_SerializationSucceeded = false;
            m_SerializationMessage = "Failed to open file for writing.\n" + fileName;
        }

        // Display message box
        m_SerializationFinishTimestamp = std::chrono::high_resolution_clock::now();
        m_SerializationOutputWindowSize = { 0, 0 };
        m_SerializationWindowVisible = false;
    }

    /***********************************************************************************\

    Function:
        LoadPerformanceCountersFromFile

    Description:
        Loads performance counters data from a CSV file.

    \***********************************************************************************/
    void ProfilerOverlayOutput::LoadPerformanceCountersFromFile( const std::string& fileName )
    {
        DeviceProfilerCsvDeserializer deserializer;

        if( deserializer.Open( fileName ) )
        {
            std::vector<VkProfilerPerformanceCounterPropertiesEXT> properties = deserializer.ReadHeader();
            std::vector<VkProfilerPerformanceCounterResultEXT> results = deserializer.ReadRow();

            m_ReferencePerformanceCounters.clear();

            const size_t performanceCounterCount = std::min( properties.size(), results.size() );
            for( size_t i = 0; i < performanceCounterCount; ++i )
            {
                m_ReferencePerformanceCounters.try_emplace( properties[i].shortName, results[i] );
            }

            m_SerializationSucceeded = true;
            m_SerializationMessage = "Performance counters loaded successfully.\n" + fileName;
        }
        else
        {
            m_SerializationSucceeded = false;
            m_SerializationMessage = "Failed to open file for reading.\n" + fileName;
        }

        // Display message box
        m_SerializationFinishTimestamp = std::chrono::high_resolution_clock::now();
        m_SerializationOutputWindowSize = { 0, 0 };
        m_SerializationWindowVisible = false;
    }

    /***********************************************************************************\

    Function:
        UpdateTopPipelinesExporter

    Description:
        Shows a file dialog if top pipelines list save or load was requested and
        saves/loads them when OK is pressed.

    \***********************************************************************************/
    void ProfilerOverlayOutput::UpdateTopPipelinesExporter()
    {
        static const std::string scFileDialogId = "#TopPipelinesSaveFileDialog";

        if( m_pTopPipelinesExporter != nullptr )
        {
            // Initialize the file dialog on the first call to this function.
            if( !m_pTopPipelinesExporter->m_FileDialog.IsOpened() )
            {
                m_pTopPipelinesExporter->m_FileDialogConfig.flags =
                    ImGuiFileDialogFlags_Default;

                if( m_pTopPipelinesExporter->m_Action == TopPipelinesExporter::Action::eImport )
                {
                    // Don't ask for overwrite when selecting file to load.
                    m_pTopPipelinesExporter->m_FileDialogConfig.flags ^=
                        ImGuiFileDialogFlags_ConfirmOverwrite;
                }

                if( m_pTopPipelinesExporter->m_Action == TopPipelinesExporter::Action::eExport )
                {
                    m_pTopPipelinesExporter->m_FileDialogConfig.fileName =
                        "top_pipelines.csv";
                }
            }

            // Draw the file dialog until the user closes it.
            bool closed = DisplayFileDialog(
                scFileDialogId,
                m_pTopPipelinesExporter->m_FileDialog,
                m_pTopPipelinesExporter->m_FileDialogConfig,
                "Select top pipelines file path",
                ".csv" );

            if( closed )
            {
                if( m_pTopPipelinesExporter->m_FileDialog.IsOk() )
                {
                    switch( m_pTopPipelinesExporter->m_Action )
                    {
                    case TopPipelinesExporter::Action::eExport:
                        SaveTopPipelinesToFile(
                            m_pTopPipelinesExporter->m_FileDialog.GetFilePathName(),
                            *m_pTopPipelinesExporter->m_pData );
                        break;

                    case TopPipelinesExporter::Action::eImport:
                        LoadTopPipelinesFromFile(
                            m_pTopPipelinesExporter->m_FileDialog.GetFilePathName() );
                        break;
                    }
                }

                // Destroy the exporter.
                m_pTopPipelinesExporter.reset();
            }
        }
    }

    /***********************************************************************************\

    Function:
        SaveTopPipelinesToFile

    Description:
        Writes top pipelines data to a CSV file.

    \***********************************************************************************/
    void ProfilerOverlayOutput::SaveTopPipelinesToFile( const std::string& fileName, const DeviceProfilerFrameData& data )
    {
        DeviceProfilerCsvSerializer serializer;

        if( serializer.Open( fileName ) )
        {
            // Convert top pipelines to performance counter format to reuse existing CSV serializer implementation.
            std::vector<VkProfilerPerformanceCounterPropertiesEXT> pipelineNames;
            pipelineNames.reserve( data.m_TopPipelines.size() );

            std::vector<VkProfilerPerformanceCounterResultEXT> pipelineDurations;
            pipelineDurations.reserve( data.m_TopPipelines.size() );

            for( const DeviceProfilerPipelineData& pipeline : data.m_TopPipelines )
            {
                const std::string pipelineName = m_pStringSerializer->GetName( pipeline );

                VkProfilerPerformanceCounterPropertiesEXT& pipelineNameInfo = pipelineNames.emplace_back();
                ProfilerStringFunctions::CopyString( pipelineNameInfo.shortName, pipelineName.c_str(), pipelineName.length() );
                pipelineNameInfo.storage = VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_FLOAT32_EXT;

                VkProfilerPerformanceCounterResultEXT& pipelineDuration = pipelineDurations.emplace_back();
                pipelineDuration.float32 = GetDuration( pipeline );
            }

            // Write converted data to file.
            serializer.WriteHeader( static_cast<uint32_t>( pipelineNames.size() ), pipelineNames.data() );
            serializer.WriteRow( static_cast<uint32_t>( pipelineDurations.size() ), pipelineDurations.data() );
            serializer.Close();

            std::filesystem::path filePath( fileName );
            m_ReferenceTopPipelinesShortDescription = filePath.filename().string();
            m_ReferenceTopPipelinesFullDescription = filePath.string();

            m_SerializationSucceeded = true;
            m_SerializationMessage = "Top pipelines saved successfully.\n" + fileName;
        }
        else
        {
            m_SerializationSucceeded = false;
            m_SerializationMessage = "Failed to open file for writing.\n" + fileName;
        }

        // Display message box
        m_SerializationFinishTimestamp = std::chrono::high_resolution_clock::now();
        m_SerializationOutputWindowSize = { 0, 0 };
        m_SerializationWindowVisible = false;
    }

    /***********************************************************************************\

    Function:
        LoadTopPipelinesFromFile

    Description:
        Loads top pipelines data from a CSV file.

    \***********************************************************************************/
    void ProfilerOverlayOutput::LoadTopPipelinesFromFile( const std::string& fileName )
    {
        DeviceProfilerCsvDeserializer deserializer;

        if( deserializer.Open( fileName ) )
        {
            std::vector<VkProfilerPerformanceCounterPropertiesEXT> properties = deserializer.ReadHeader();
            std::vector<VkProfilerPerformanceCounterResultEXT> results = deserializer.ReadRow();

            m_ReferenceTopPipelines.clear();

            const size_t topPipelineCount = std::min( properties.size(), results.size() );
            for( size_t i = 0; i < topPipelineCount; ++i )
            {
                // Only float32 storage is supported for top pipelines for now.
                if( properties[i].storage == VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_FLOAT32_EXT )
                {
                    m_ReferenceTopPipelines.try_emplace( properties[i].shortName, results[i].float32 );
                }
            }

            std::filesystem::path filePath( fileName );
            m_ReferenceTopPipelinesShortDescription = filePath.filename().string();
            m_ReferenceTopPipelinesFullDescription = filePath.string();

            m_SerializationSucceeded = true;
            m_SerializationMessage = "Top pipelines loaded successfully.\n" + fileName;
        }
        else
        {
            m_SerializationSucceeded = false;
            m_SerializationMessage = "Failed to open file for reading.\n" + fileName;
        }

        // Display message box
        m_SerializationFinishTimestamp = std::chrono::high_resolution_clock::now();
        m_SerializationOutputWindowSize = { 0, 0 };
        m_SerializationWindowVisible = false;
    }

    /***********************************************************************************\

    Function:
        UpdateTraceExporter

    Description:
        Shows a file dialog if trace save was requested and saves it when OK is pressed.

    \***********************************************************************************/
    void ProfilerOverlayOutput::UpdateTraceExporter()
    {
        static const std::string scFileDialogId = "#TraceSaveFileDialog";

        // Early-out if not requested.
        if( m_pTraceExporter == nullptr )
        {
            return;
        }

        if( !m_pTraceExporter->m_FileDialog.IsOpened() )
        {
            // Initialize the file dialog on the first call to this function.
            m_pTraceExporter->m_FileDialogConfig.fileName =
                DeviceProfilerTraceSerializer::GetDefaultTraceFileName( m_SamplingMode );

            m_pTraceExporter->m_FileDialogConfig.flags =
                ImGuiFileDialogFlags_Default;
        }

        // Draw the file dialog until the user closes it.
        bool closed = DisplayFileDialog(
            scFileDialogId,
            m_pTraceExporter->m_FileDialog,
            m_pTraceExporter->m_FileDialogConfig,
            "Select trace save path",
            ".json" );

        if( closed )
        {
            if( m_pTraceExporter->m_FileDialog.IsOk() )
            {
                SaveTraceToFile(
                    m_pTraceExporter->m_FileDialog.GetFilePathName(),
                    *m_pTraceExporter->m_pData );
            }

            // Destroy the exporter.
            m_pTraceExporter.reset();
        }
    }

    /***********************************************************************************\

    Function:
        SaveTraceToFile

    Description:
        Saves frame trace to a file.

    \***********************************************************************************/
    void ProfilerOverlayOutput::SaveTraceToFile( const std::string& fileName, const DeviceProfilerFrameData& data )
    {
        DeviceProfilerTraceSerializer serializer( m_pStringSerializer.get(), m_TimestampPeriod);
        DeviceProfilerTraceSerializationResult result = serializer.Serialize( fileName, data );

        m_SerializationSucceeded = result.m_Succeeded;
        m_SerializationMessage = result.m_Message;

        // Display message box
        m_SerializationFinishTimestamp = std::chrono::high_resolution_clock::now();
        m_SerializationOutputWindowSize = { 0, 0 };
        m_SerializationWindowVisible = false;
    }

    /***********************************************************************************\

    Function:
        UpdateNotificationWindow

    Description:
        Display window with serialization output.

    \***********************************************************************************/
    void ProfilerOverlayOutput::UpdateNotificationWindow()
    {
        using namespace std::chrono;
        using namespace std::chrono_literals;

        const auto& now = std::chrono::high_resolution_clock::now();

        if( (now - m_SerializationFinishTimestamp) < 4s )
        {
            const ImVec2 outputSize = m_Backend.GetRenderArea();
            const ImVec2 windowPos = {
                static_cast<float>(outputSize.x - m_SerializationOutputWindowSize.width),
                static_cast<float>(outputSize.y - m_SerializationOutputWindowSize.height) };

            const float fadeOutStep =
                1.f - std::max( 0.f, std::min( 1.f,
                                         duration_cast<milliseconds>(now - (m_SerializationFinishTimestamp + 3s)).count() / 1000.f ) );

            ImGui::PushStyleVar( ImGuiStyleVar_Alpha, fadeOutStep );

            if( !m_SerializationSucceeded )
            {
                ImGui::PushStyleColor( ImGuiCol_WindowBg, { 1, 0, 0, 1 } );
            }

            ImGui::SetNextWindowPos( windowPos );
            ImGui::Begin( "Trace Export", nullptr,
                ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoTitleBar |
                    ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoDocking |
                    ImGuiWindowFlags_NoFocusOnAppearing |
                    ImGuiWindowFlags_NoSavedSettings |
                    ImGuiWindowFlags_AlwaysAutoResize );

            ImGui::Text( "%s", m_SerializationMessage.c_str() );

            // Save final size of the window
            if ( m_SerializationWindowVisible &&
                (m_SerializationOutputWindowSize.width == 0) )
            {
                const ImVec2 windowSize = ImGui::GetWindowSize();
                m_SerializationOutputWindowSize.width = static_cast<uint32_t>(windowSize.x);
                m_SerializationOutputWindowSize.height = static_cast<uint32_t>(windowSize.y);
            }

            ImGui::End();
            ImGui::PopStyleVar();

            if( !m_SerializationSucceeded )
            {
                ImGui::PopStyleColor();
            }

            m_SerializationWindowVisible = true;
        }
    }

    /***********************************************************************************\

    Function:
        UpdateApplicationInfoWindow

    Description:
        Display window with application information.

    \***********************************************************************************/
    void ProfilerOverlayOutput::UpdateApplicationInfoWindow()
    {
        const uint32_t applicationInfoWindowFlags = 
            ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoMove;

        if( ImGui::BeginPopup( Lang::ApplicationInfo, applicationInfoWindowFlags ) )
        {
            const float interfaceScale = ImGui::GetIO().FontGlobalScale;
            const float headerColumnWidth = 150.f * interfaceScale;
            const ImVec2 iconSize = { 12.f * interfaceScale, 12.f * interfaceScale };

            const VkApplicationInfo& applicationInfo = m_Frontend.GetApplicationInfo();

            ImGui::PushStyleColor( ImGuiCol_Button, { 0, 0, 0, 0 } );

            ImGui::TextUnformatted( Lang::VulkanVersion );
            ImGui::SameLine( headerColumnWidth );
            ImGui::Text( "%u.%u",
                VK_API_VERSION_MAJOR( applicationInfo.apiVersion ),
                VK_API_VERSION_MINOR( applicationInfo.apiVersion ) );

            ImGui::TextUnformatted( Lang::ApplicationName );
            if( applicationInfo.pApplicationName )
            {
                ImGui::SameLine( headerColumnWidth );
                ImGui::TextUnformatted( applicationInfo.pApplicationName );
                
                ImGui::SameLine();
                if( ImGui::ImageButton( "##CopyApplicationName", m_Resources.GetCopyIconImage(), iconSize ) )
                {
                    ImGui::SetClipboardText( applicationInfo.pApplicationName );
                }
                if( ImGui::IsItemHovered( ImGuiHoveredFlags_DelayNormal ) )
                {
                    ImGui::SetTooltip( Lang::CopyToClipboard );
                }
            }

            ImGui::TextUnformatted( Lang::ApplicationVersion );
            ImGui::SameLine( headerColumnWidth );
            ImGui::Text( "%u.%u.%u",
                VK_API_VERSION_MAJOR( applicationInfo.applicationVersion ),
                VK_API_VERSION_MINOR( applicationInfo.applicationVersion ),
                VK_API_VERSION_PATCH( applicationInfo.applicationVersion ) );

            ImGui::TextUnformatted( Lang::EngineName );
            if( applicationInfo.pEngineName )
            {
                ImGui::SameLine( headerColumnWidth );
                ImGui::TextUnformatted( applicationInfo.pEngineName );

                ImGui::SameLine();
                if( ImGui::ImageButton( "##CopyEngineName", m_Resources.GetCopyIconImage(), iconSize ) )
                {
                    ImGui::SetClipboardText( applicationInfo.pEngineName );
                }
                if( ImGui::IsItemHovered( ImGuiHoveredFlags_DelayNormal ) )
                {
                    ImGui::SetTooltip( Lang::CopyToClipboard );
                }
            }

            ImGui::TextUnformatted( Lang::EngineVersion );
            ImGui::SameLine( headerColumnWidth );
            ImGui::Text( "%u.%u.%u",
                VK_API_VERSION_MAJOR( applicationInfo.engineVersion ),
                VK_API_VERSION_MINOR( applicationInfo.engineVersion ),
                VK_API_VERSION_PATCH( applicationInfo.engineVersion ) );

            ImGui::PopStyleColor();
            ImGui::EndPopup();
        }
    }

    /***********************************************************************************\

    Function:
        PrintCommandBuffer

    Description:
        Writes command buffer data to the overlay.

    \***********************************************************************************/
    void ProfilerOverlayOutput::PrintCommandBuffer( const DeviceProfilerCommandBufferData& cmdBuffer, FrameBrowserTreeNodeIndex& index )
    {
        // Mark hotspots with color
        DrawSignificanceRect( cmdBuffer, index );

        if( ScrollToSelectedFrameBrowserNode( index ) )
        {
            // Tree contains selected node
            ImGui::SetNextItemOpen( true );
            ImGui::SetScrollHereY();
        }

        const char* indexStr = GetFrameBrowserNodeIndexStr( index );
        std::string commandBufferName = m_pStringSerializer->GetName( cmdBuffer.m_Handle );
        bool commandBufferTreeExpanded = ImGui::TreeNode( indexStr, "%s",
            commandBufferName.c_str() );

        if( ImGui::BeginPopupContextItem() )
        {
            if( ImGui::MenuItem( Lang::ShowPerformanceMetrics, nullptr, nullptr, !cmdBuffer.m_PerformanceQueryResults.empty() ) )
            {
                m_PerformanceQueryCommandBufferFilter = cmdBuffer.m_Handle;
                m_PerformanceQueryCommandBufferFilterName = std::move( commandBufferName );
                m_PerformanceCountersWindowState.SetFocus();
            }
            ImGui::EndPopup();
        }

        // Print duration next to the node
        PrintDuration( cmdBuffer );

        if( commandBufferTreeExpanded )
        {
            FrameBrowserContext commandBufferContext = {};
            commandBufferContext.pCommandBuffer = &cmdBuffer;

            // Sort frame browser data
            std::list<const DeviceProfilerRenderPassData*> pRenderPasses =
                SortFrameBrowserData( cmdBuffer.m_RenderPasses );

            index.emplace_back( 0 );

            // Enumerate render passes in command buffer
            for( const DeviceProfilerRenderPassData* pRenderPass : pRenderPasses )
            {
                PrintRenderPass( *pRenderPass, index, commandBufferContext );
                index.back()++;
            }

            index.pop_back();
            ImGui::TreePop();
        }
    }

    /***********************************************************************************\

    Function:
        PrintRenderPassCommand

    Description:
        Writes render pass command data to the overlay.
        Render pass commands include vkCmdBeginRenderPass, vkCmdEndRenderPass, as well as
        dynamic rendering counterparts: vkCmdBeginRendering, etc.

    \***********************************************************************************/
    template<typename Data>
    void ProfilerOverlayOutput::PrintRenderPassCommand( const Data& data, bool dynamic, FrameBrowserTreeNodeIndex& index, uint32_t drawcallIndex )
    {
        index.emplace_back( drawcallIndex );

        if( (m_ScrollToSelectedFrameBrowserNode) &&
            (m_SelectedFrameBrowserNodeIndex == index) )
        {
            ImGui::SetScrollHereY();
        }

        // Mark hotspots with color
        DrawSignificanceRect( data, index );

        index.pop_back();

        // Print command's name
        ImGui::TextUnformatted( m_pStringSerializer->GetName( data, dynamic ).c_str() );

        PrintDuration( data );
    }

    /***********************************************************************************\

    Function:
        PrintRenderPass

    Description:
        Writes render pass data to the overlay.

    \***********************************************************************************/
    void ProfilerOverlayOutput::PrintRenderPass( const DeviceProfilerRenderPassData& renderPass, FrameBrowserTreeNodeIndex& index, const FrameBrowserContext& context )
    {
        const bool isValidRenderPass = (renderPass.m_Type != DeviceProfilerRenderPassType::eNone);

        if( isValidRenderPass )
        {
            // Mark hotspots with color
            DrawSignificanceRect( renderPass, index );
        }

        // At least one subpass must be present
        assert( !renderPass.m_Subpasses.empty() );

        if( ScrollToSelectedFrameBrowserNode( index ) )
        {
            // Tree contains selected node
            ImGui::SetNextItemOpen( true );
            ImGui::SetScrollHereY();
        }

        bool inRenderPassSubtree;
        if( isValidRenderPass )
        {
            const char* indexStr = GetFrameBrowserNodeIndexStr( index );
            inRenderPassSubtree = ImGui::TreeNode( indexStr, "%s",
                m_pStringSerializer->GetName( renderPass ).c_str() );

            // Print duration next to the node
            PrintDuration( renderPass );
        }
        else
        {
            // Print render pass inline.
            inRenderPassSubtree = true;
        }

        if( inRenderPassSubtree )
        {
            FrameBrowserContext renderPassContext = context;
            renderPassContext.pRenderPass = &renderPass;

            index.emplace_back( 0 );

            // Render pass subtree opened
            if( isValidRenderPass )
            {
                if( renderPass.HasBeginCommand() )
                {
                    PrintRenderPassCommand( renderPass.m_Begin, renderPass.m_Dynamic, index, 0 );
                    index.back()++;
                }
            }

            // Sort frame browser data
            std::list<const DeviceProfilerSubpassData*> pSubpasses =
                SortFrameBrowserData( renderPass.m_Subpasses );

            // Enumerate subpasses
            for( const DeviceProfilerSubpassData* pSubpass : pSubpasses )
            {
                PrintSubpass( *pSubpass, index, ( pSubpasses.size() == 1 ), renderPassContext );
                index.back()++;
            }

            if( isValidRenderPass )
            {
                if( renderPass.HasEndCommand() )
                {
                    PrintRenderPassCommand( renderPass.m_End, renderPass.m_Dynamic, index, 1 );
                }

                ImGui::TreePop();
            }

            index.pop_back();
        }
    }

    /***********************************************************************************\

    Function:
        PrintSubpass

    Description:
        Writes subpass data to the overlay.

    \***********************************************************************************/
    void ProfilerOverlayOutput::PrintSubpass( const DeviceProfilerSubpassData& subpass, FrameBrowserTreeNodeIndex& index, bool isOnlySubpass, const FrameBrowserContext& context )
    {
        bool inSubpassSubtree = false;
        bool printSubpassInline =
            (isOnlySubpass == true) ||
            (subpass.m_Index == DeviceProfilerSubpassData::ImplicitSubpassIndex);

        if( !printSubpassInline )
        {
            // Mark hotspots with color
            DrawSignificanceRect( subpass, index );

            if( ScrollToSelectedFrameBrowserNode( index ) )
            {
                // Tree contains selected node
                ImGui::SetNextItemOpen( true );
                ImGui::SetScrollHereY();
            }

            const char* indexStr = GetFrameBrowserNodeIndexStr( index );
            inSubpassSubtree = ImGui::TreeNode( indexStr, "Subpass #%u",
                subpass.m_Index );

            // Print duration next to the node
            PrintDuration( subpass );
        }

        if( inSubpassSubtree || printSubpassInline )
        {
            index.emplace_back( 0 );

            // Treat data as pipelines if subpass contents are inline-only.
            if( subpass.m_Contents == VK_SUBPASS_CONTENTS_INLINE )
            {
                // Sort frame browser data
                std::list<const DeviceProfilerSubpassData::Data*> pDataSorted =
                    SortFrameBrowserData<DeviceProfilerPipelineData>( subpass.m_Data );

                for( const DeviceProfilerSubpassData::Data* pData : pDataSorted )
                {
                    PrintPipeline( std::get<DeviceProfilerPipelineData>( *pData ), index, context );
                    index.back()++;
                }
            }

            // Treat data as secondary command buffers if subpass contents are secondary command buffers only.
            else if( subpass.m_Contents == VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS )
            {
                // Sort frame browser data
                std::list<const DeviceProfilerSubpassData::Data*> pDataSorted =
                    SortFrameBrowserData<DeviceProfilerCommandBufferData>( subpass.m_Data );

                for( const DeviceProfilerSubpassData::Data* pData : pDataSorted )
                {
                    PrintCommandBuffer( std::get<DeviceProfilerCommandBufferData>( *pData ), index );
                    index.back()++;
                }
            }

            // With VK_EXT_nested_command_buffer, it is possible to insert both command buffers and inline commands in the same subpass.
            else if( subpass.m_Contents == VK_SUBPASS_CONTENTS_INLINE_AND_SECONDARY_COMMAND_BUFFERS_EXT )
            {
                // Sort frame browser data
                std::list<const DeviceProfilerSubpassData::Data*> pDataSorted =
                    SortFrameBrowserData( subpass.m_Data );

                for( const DeviceProfilerSubpassData::Data* pData : pDataSorted )
                {
                    switch( pData->GetType() )
                    {
                    case DeviceProfilerSubpassDataType::ePipeline:
                        PrintPipeline( std::get<DeviceProfilerPipelineData>( *pData ), index, context );
                        break;

                    case DeviceProfilerSubpassDataType::eCommandBuffer:
                        PrintCommandBuffer( std::get<DeviceProfilerCommandBufferData>( *pData ), index );
                        break;
                    }
                    index.back()++;
                }
            }

            index.pop_back();
        }

        if( inSubpassSubtree )
        {
            // Finish subpass tree
            ImGui::TreePop();
        }
    }

    /***********************************************************************************\

    Function:
        PrintPipeline

    Description:
        Writes pipeline data to the overlay.

    \***********************************************************************************/
    void ProfilerOverlayOutput::PrintPipeline( const DeviceProfilerPipelineData& pipeline, FrameBrowserTreeNodeIndex& index, const FrameBrowserContext& context )
    {
        const bool printPipelineInline =
            ((pipeline.m_Handle == VK_NULL_HANDLE) &&
                !pipeline.m_UsesShaderObjects) ||
            ((pipeline.m_ShaderTuple.m_Hash & 0xFFFF) == 0);

        bool inPipelineSubtree = false;

        if( !printPipelineInline )
        {
            // Mark hotspots with color
            DrawSignificanceRect( pipeline, index );

            if( ScrollToSelectedFrameBrowserNode( index ) )
            {
                // Tree contains selected node
                ImGui::SetNextItemOpen( true );
                ImGui::SetScrollHereY();
            }

            const char* indexStr = GetFrameBrowserNodeIndexStr( index );
            inPipelineSubtree =
                (ImGui::TreeNode( indexStr, "%s", m_pStringSerializer->GetName( pipeline ).c_str() ));

            DrawPipelineContextMenu( pipeline );
        }

        DrawPipelineCapabilityBadges( pipeline );

        if( !printPipelineInline )
        {
            // Print duration next to the node
            PrintDuration( pipeline );
        }

        if( inPipelineSubtree || printPipelineInline )
        {
            FrameBrowserContext pipelineContext = context;
            pipelineContext.pPipeline = &pipeline;

            // Sort frame browser data
            std::list<const DeviceProfilerDrawcall*> pDrawcalls =
                SortFrameBrowserData( pipeline.m_Drawcalls );

            index.emplace_back( 0 );

            // Enumerate drawcalls in pipeline
            for( const DeviceProfilerDrawcall* pDrawcall : pDrawcalls )
            {
                PrintDrawcall( *pDrawcall, index, pipelineContext );
                index.back()++;
            }

            index.pop_back();
        }

        if( inPipelineSubtree )
        {
            // Finish pipeline subtree
            ImGui::TreePop();
        }
    }

    /***********************************************************************************\

    Function:
        PrintDrawcall

    Description:
        Writes drawcall data to the overlay.

    \***********************************************************************************/
    void ProfilerOverlayOutput::PrintDrawcall( const DeviceProfilerDrawcall& drawcall, FrameBrowserTreeNodeIndex& index, const FrameBrowserContext& context )
    {
        if( drawcall.GetPipelineType() != DeviceProfilerPipelineType::eDebug )
        {
            if( ScrollToSelectedFrameBrowserNode( index ) )
            {
                ImGui::SetScrollHereY();
            }

            // Mark hotspots with color
            DrawSignificanceRect( drawcall, index );

            const bool indirectPayloadPresent =
                (drawcall.HasIndirectPayload()) &&
                (context.pCommandBuffer) &&
                (!context.pCommandBuffer->m_IndirectPayload.empty());

            const char* indexStr = GetFrameBrowserNodeIndexStr( index );
            const bool drawcallTreeOpen = ImGui::TreeNodeEx(
                indexStr,
                indirectPayloadPresent
                    ? ImGuiTreeNodeFlags_None
                    : ImGuiTreeNodeFlags_Leaf,
                "%s",
                m_pStringSerializer->GetName( drawcall ).c_str() );

            PrintDuration( drawcall );

            if( drawcallTreeOpen )
            {
                if( indirectPayloadPresent )
                {
                    PrintDrawcallIndirectPayload( drawcall, context );
                }

                ImGui::TreePop();
            }
        }
        else
        {
            // Draw debug label
            PrintDebugLabel( drawcall.m_Payload.m_DebugLabel.m_pName, drawcall.m_Payload.m_DebugLabel.m_Color );
        }
    }

    /***********************************************************************************\

    Function:
        DrawSignificanceRect

    Description:

    \***********************************************************************************/
    void ProfilerOverlayOutput::PrintDrawcallIndirectPayload( const DeviceProfilerDrawcall& drawcall, const FrameBrowserContext& context )
    {
        switch( drawcall.m_Type )
        {
        case DeviceProfilerDrawcallType::eDrawIndirect:
        {
            const DeviceProfilerDrawcallDrawIndirectPayload& payload = drawcall.m_Payload.m_DrawIndirect;
            const uint8_t* pIndirectData = context.pCommandBuffer->m_IndirectPayload.data() + payload.m_IndirectArgsOffset;

            for( uint32_t drawIndex = 0; drawIndex < payload.m_DrawCount; ++drawIndex )
            {
                const VkDrawIndirectCommand& cmd =
                    *reinterpret_cast<const VkDrawIndirectCommand*>( pIndirectData + drawIndex * payload.m_Stride );

                ImGui::Text( "VkDrawIndirectCommand #%u (%u, %u, %u, %u)",
                    drawIndex,
                    cmd.vertexCount,
                    cmd.instanceCount,
                    cmd.firstVertex,
                    cmd.firstInstance );
            }
            break;
        }

        case DeviceProfilerDrawcallType::eDrawIndexedIndirect:
        {
            const DeviceProfilerDrawcallDrawIndexedIndirectPayload& payload = drawcall.m_Payload.m_DrawIndexedIndirect;
            const uint8_t* pIndirectData = context.pCommandBuffer->m_IndirectPayload.data() + payload.m_IndirectArgsOffset;

            for( uint32_t drawIndex = 0; drawIndex < payload.m_DrawCount; ++drawIndex )
            {
                const VkDrawIndexedIndirectCommand& cmd =
                    *reinterpret_cast<const VkDrawIndexedIndirectCommand*>( pIndirectData + drawIndex * payload.m_Stride );

                ImGui::Text( "VkDrawIndexedIndirectCommand #%u (%u, %u, %u, %d, %u)",
                    drawIndex,
                    cmd.indexCount,
                    cmd.instanceCount,
                    cmd.firstIndex,
                    cmd.vertexOffset,
                    cmd.firstInstance );
            }
            break;
        }

        case DeviceProfilerDrawcallType::eDrawIndirectCount:
        {
            const DeviceProfilerDrawcallDrawIndirectCountPayload& payload = drawcall.m_Payload.m_DrawIndirectCount;
            const uint8_t* pIndirectData = context.pCommandBuffer->m_IndirectPayload.data() + payload.m_IndirectArgsOffset;
            const uint8_t* pIndirectCount = context.pCommandBuffer->m_IndirectPayload.data() + payload.m_IndirectCountOffset;

            const uint32_t drawCount = *reinterpret_cast<const uint32_t*>( pIndirectCount );
            for( uint32_t drawIndex = 0; drawIndex < drawCount; ++drawIndex )
            {
                const VkDrawIndirectCommand& cmd =
                    *reinterpret_cast<const VkDrawIndirectCommand*>( pIndirectData + drawIndex * payload.m_Stride );

                ImGui::Text( "VkDrawIndirectCommand #%u (%u, %u, %u, %u)",
                    drawIndex,
                    cmd.vertexCount,
                    cmd.instanceCount,
                    cmd.firstVertex,
                    cmd.firstInstance );
            }
            break;
        }

        case DeviceProfilerDrawcallType::eDrawIndexedIndirectCount:
        {
            const DeviceProfilerDrawcallDrawIndexedIndirectCountPayload& payload = drawcall.m_Payload.m_DrawIndexedIndirectCount;
            const uint8_t* pIndirectData = context.pCommandBuffer->m_IndirectPayload.data() + payload.m_IndirectArgsOffset;
            const uint8_t* pIndirectCount = context.pCommandBuffer->m_IndirectPayload.data() + payload.m_IndirectCountOffset;

            const uint32_t drawCount = *reinterpret_cast<const uint32_t*>( pIndirectCount );
            for( uint32_t drawIndex = 0; drawIndex < drawCount; ++drawIndex )
            {
                const VkDrawIndexedIndirectCommand& cmd =
                    *reinterpret_cast<const VkDrawIndexedIndirectCommand*>( pIndirectData + drawIndex * payload.m_Stride );

                ImGui::Text( "VkDrawIndexedIndirectCommand #%u (%u, %u, %u, %d, %u)",
                    drawIndex,
                    cmd.indexCount,
                    cmd.instanceCount,
                    cmd.firstIndex,
                    cmd.vertexOffset,
                    cmd.firstInstance );
            }
            break;
        }

        case DeviceProfilerDrawcallType::eDispatchIndirect:
        {
            const DeviceProfilerDrawcallDispatchIndirectPayload& payload = drawcall.m_Payload.m_DispatchIndirect;
            const uint8_t* pIndirectData = context.pCommandBuffer->m_IndirectPayload.data() + payload.m_IndirectArgsOffset;

            const VkDispatchIndirectCommand& cmd =
                *reinterpret_cast<const VkDispatchIndirectCommand*>( pIndirectData );

            ImGui::Text( "VkDispatchIndirectCommand (%u, %u, %u)",
                cmd.x,
                cmd.y,
                cmd.z );
            break;
        }
        }
    }

    /***********************************************************************************\

    Function:
        DrawSignificanceRect

    Description:

    \***********************************************************************************/
    template<typename Data>
    void ProfilerOverlayOutput::DrawSignificanceRect(const Data& data, const FrameBrowserTreeNodeIndex& index)
    {
        DrawSignificanceRect( GetDuration( data ) / m_FrameTime, index );
    }

    /***********************************************************************************\

    Function:
        DrawSignificanceRect

    Description:

    \***********************************************************************************/
    void ProfilerOverlayOutput::DrawSignificanceRect( float significance, const FrameBrowserTreeNodeIndex& index )
    {
        ImVec2 cursorPosition = ImGui::GetCursorScreenPos();
        ImVec2 rectSize;

        cursorPosition.x = ImGui::GetWindowPos().x;

        rectSize.x = cursorPosition.x + ImGui::GetWindowSize().x;
        rectSize.y = cursorPosition.y + ImGui::GetTextLineHeight();

        ImU32 color = ImGui::GetColorU32( { 1, 0, 0, significance } );

        if( index == m_SelectedFrameBrowserNodeIndex )
        {
            using namespace std::chrono;
            using namespace std::chrono_literals;

            // Node is selected
            ImU32 selectionColor = ImGui::GetColorU32( ImGuiCol_TabHovered );

            // Interpolate color
            auto now = std::chrono::high_resolution_clock::now();
            float step = std::max( 0.0f, std::min( 1.0f,
                                             duration_cast<Milliseconds>((now - m_SelectionUpdateTimestamp) - 0.3s).count() / 1000.0f ) );

            // Linear interpolation
            color = ImGuiX::ColorLerp( selectionColor, color, step );
        }

        ImDrawList* pDrawList = ImGui::GetWindowDrawList();
        pDrawList->AddRectFilled( cursorPosition, rectSize, color );
    }

    /***********************************************************************************\

    Function:
        DrawSignificanceRect

    Description:

    \***********************************************************************************/
    void ProfilerOverlayOutput::DrawBadge( uint32_t color, const char* shortName, const char* fmt, ... )
    {
        ImGui::SameLine();
        ImGuiX::BadgeUnformatted( color, 5.f, shortName );

        if( ImGui::IsItemHovered( ImGuiHoveredFlags_ForTooltip ) )
        {
            va_list args;
            va_start( args, fmt );

            ImGui::BeginTooltip();
            ImGui::TextV( fmt, args );
            ImGui::EndTooltip();

            va_end( args );
        }
    }

    /***********************************************************************************\

    Function:
        DrawPipelineCapabilityBadges

    Description:

    \***********************************************************************************/
    void ProfilerOverlayOutput::DrawPipelineCapabilityBadges( const DeviceProfilerPipelineData& pipeline )
    {
        if( !m_ShowShaderCapabilities )
        {
            return;
        }

        if( pipeline.m_UsesShaderObjects )
        {
            ImU32 shaderObjectsColor = IM_COL32( 104, 25, 133, 255 );
            DrawBadge( shaderObjectsColor, "SO", Lang::ShaderObjectsTooltip );
        }

        if( pipeline.m_UsesRayQuery )
        {
            ImU32 rayQueryCapabilityColor = IM_COL32( 133, 82, 25, 255 );
            DrawBadge( rayQueryCapabilityColor, "RQ", Lang::ShaderCapabilityTooltipFmt, "Ray Query" );
        }

        if( pipeline.m_UsesRayTracing )
        {
            ImU32 rayTracingCapabilityColor = ImGuiX::Darker( m_RayTracingPipelineColumnColor, 0.5f );
            DrawBadge( rayTracingCapabilityColor, "RT", Lang::ShaderCapabilityTooltipFmt, "Ray Tracing" );
        }
    }

    /***********************************************************************************\

    Function:
        DrawPipelineStageBadge

    Description:

    \***********************************************************************************/
    void ProfilerOverlayOutput::DrawPipelineStageBadge( const DeviceProfilerPipelineData& pipeline, VkShaderStageFlagBits stage, const char* pStageName )
    {
        const ProfilerShader* pShader = pipeline.m_ShaderTuple.GetFirstShaderAtStage( stage );

        if( !pShader )
        {
            ImGui::PushStyleColor( ImGuiCol_Text, IM_COL32( 255, 255, 255, 48 ) );
        }

        ImGui::TextUnformatted( pStageName );

        if( ImGui::IsItemHovered( ImGuiHoveredFlags_ForTooltip ) )
        {
            if( ImGui::BeginTooltip() )
            {
                ImGui::PushFont( m_Resources.GetBoldFont() );
                ImGui::Text( "%s stage", m_pStringSerializer->GetShaderStageName( stage ).c_str() );
                ImGui::PopFont();

                if( pShader )
                {
                    ImGui::TextUnformatted( m_pStringSerializer->GetShaderName( *pShader ).c_str() );
                }
                else
                {
                    ImGui::TextUnformatted( "Unused" );
                }

                ImGui::EndTooltip();
            }
        }

        if( !pShader )
        {
            ImGui::PopStyleColor();
        }

        ImGui::SameLine();
    }

    /***********************************************************************************\

    Function:
        DrawPipelineContextMenu

    Description:

    \***********************************************************************************/
    void ProfilerOverlayOutput::DrawPipelineContextMenu( const DeviceProfilerPipelineData& pipeline, const char* id )
    {
        if( ImGui::BeginPopupContextItem( id ) )
        {
            if( ImGui::MenuItem( Lang::Inspect, nullptr, nullptr, !pipeline.m_Internal ) )
            {
                Inspect( pipeline );
            }

            if( ImGui::MenuItem( Lang::CopyName, nullptr, nullptr ) )
            {
                ImGui::SetClipboardText( m_pStringSerializer->GetName( pipeline ).c_str() );
            }

            ImGui::EndPopup();
        }
    }

    /***********************************************************************************\

    Function:
        DrawDebugLabel

    Description:

    \***********************************************************************************/
    void ProfilerOverlayOutput::PrintDebugLabel( const char* pName, const float pColor[ 4 ] )
    {
        if( !(m_ShowDebugLabels) ||
            (m_FrameBrowserSortMode != FrameBrowserSortMode::eSubmissionOrder) ||
            !(pName) )
        {
            // Don't print debug labels if frame browser is sorted out of submission order
            return;
        }

        ImVec2 cursorPosition = ImGui::GetCursorScreenPos();
        ImVec2 rectSize;

        rectSize.x = cursorPosition.x + 8;
        rectSize.y = cursorPosition.y + ImGui::GetTextLineHeight();

        // Resolve debug label color
        ImU32 color = ImGui::GetColorU32( *reinterpret_cast<const ImVec4*>(pColor) );

        ImDrawList* pDrawList = ImGui::GetWindowDrawList();
        pDrawList->AddRectFilled( cursorPosition, rectSize, color );
        pDrawList->AddRect( cursorPosition, rectSize, ImGui::GetColorU32( ImGuiCol_Border ) );

        cursorPosition.x += 12;
        ImGui::SetCursorScreenPos( cursorPosition );

        ImGui::TextUnformatted( pName );
    }

    /***********************************************************************************\

    Function:
        PrintDuration

    Description:

    \***********************************************************************************/
    template <typename Data>
    void ProfilerOverlayOutput::PrintDuration( const Data& data )
    {
        PrintDuration( data.m_BeginTimestamp.m_Value, data.m_EndTimestamp.m_Value );
    }

    /***********************************************************************************\

    Function:
        PrintDuration

    Description:

    \***********************************************************************************/
    void ProfilerOverlayOutput::PrintDuration( uint64_t from, uint64_t to )
    {
        if( ( from != UINT64_MAX ) && ( to != UINT64_MAX ) )
        {
            const float time = GetDuration( from, to );

            // Print the duration
            ImGuiX::TextAlignRight( "%.2f %s",
                time,
                m_pTimestampDisplayUnitStr );
        }
        else
        {
            // No data collected in this mode
            ImGuiX::TextAlignRight( "- %s",
                m_pTimestampDisplayUnitStr );
        }
    }

    /***********************************************************************************\

    Function:
        GetDuration

    Description:

    \***********************************************************************************/
    template <typename Data>
    float ProfilerOverlayOutput::GetDuration( const Data& data ) const
    {
        return static_cast<float>(Profiler::GetDuration( data )) * m_TimestampPeriod.count() * m_TimestampDisplayUnit;
    }

    /***********************************************************************************\

    Function:
        GetDuration

    Description:

    \***********************************************************************************/
    float ProfilerOverlayOutput::GetDuration( uint64_t begin, uint64_t end ) const
    {
        return static_cast<float>(end - begin) * m_TimestampPeriod.count() * m_TimestampDisplayUnit;
    }

    /***********************************************************************************\

    Function:
        SetFocus

    Description:
        Set focus to the window on the next frame and make sure the window is open.

    \***********************************************************************************/
    void ProfilerOverlayOutput::WindowState::SetFocus()
    {
        Focus = true;

        if( pOpen != nullptr )
        {
            *pOpen = true;
        }
    }
}

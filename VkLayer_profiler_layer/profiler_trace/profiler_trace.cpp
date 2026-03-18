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

#include "profiler_trace.h"
#include "profiler_trace_event.h"
#include "profiler_json.h"
#include "profiler/profiler_data.h"
#include "profiler/profiler_counters.h"
#include "profiler/profiler_helpers.h"
#include "profiler/profiler_frontend.h"
#include "profiler_layer_objects/VkObject.h"
#include "profiler_helpers/profiler_string_serializer.h"

#include "VkLayer_profiler_layer.generated.h"

#include <chrono>
#include <fstream>
#include <sstream>
#include <iomanip>

#include <nlohmann/json.hpp>

#include "profiler_ext/VkProfilerEXT.h"

// Alias commonly used types
using json = nlohmann::json;

namespace
{
    /*************************************************************************\

    Function:
        GetSamplingModeComponent

    Description:
        Returns a short description of the sampling mode used to generate
        the trace.

    \*************************************************************************/
    static constexpr const char* GetSamplingModeComponent( int samplingMode )
    {
        switch( samplingMode )
        {
        case VK_PROFILER_MODE_PER_DRAWCALL_EXT:
            return "drawcalls";
        case VK_PROFILER_MODE_PER_PIPELINE_EXT:
            return "pipelines";
        case VK_PROFILER_MODE_PER_RENDER_PASS_EXT:
            return "renderpasses";
        case VK_PROFILER_MODE_PER_COMMAND_BUFFER_EXT:
            return "commandbuffers";
        case VK_PROFILER_MODE_PER_SUBMIT_EXT:
            return "submits";
        case VK_PROFILER_MODE_PER_FRAME_EXT:
            return "frame";
        default:
            return "";
        }
    }

    std::ostream& lf( std::ostream& out )
    {
        out.write( "\n", 1 );
        return out;
    }

    static constexpr std::streamoff eof_len = 3;
    std::ostream& eof( std::ostream& out )
    {
        out << "]}" << lf;
        out << std::flush;
        return out;
    }

    static constexpr std::streamoff eol_len = 2;
    std::ostream& eol( std::ostream& out )
    {
        out << ',' << lf;
        return out;
    }
}

namespace Profiler
{
    /*************************************************************************\

    Function:
        DeviceProfilerTraceSerializer

    Description:
        Constructor.

    \*************************************************************************/
    DeviceProfilerTraceSerializer::DeviceProfilerTraceSerializer( DeviceProfilerFrontend& frontend )
        : m_Frontend( frontend )
        , m_pStringSerializer( new DeviceProfilerStringSerializer( m_Frontend ) )
        , m_pJsonSerializer( new DeviceProfilerJsonSerializer( m_pStringSerializer ) )
        , m_OutputFile()
        , m_OutputFileEmpty( true )
        , m_pData( nullptr )
        , m_CommandQueue( VK_NULL_HANDLE )
        , m_Events()
        , m_DebugLabelStackDepth( 0 )
        , m_HostTimeDomain( OSGetDefaultTimeDomain() )
        , m_HostCalibratedTimestamp( 0 )
        , m_DeviceCalibratedTimestamp( 0 )
        , m_HostTimestampFrequency( OSGetTimestampFrequency( m_HostTimeDomain ) )
        , m_GpuTimestampPeriod( Nanoseconds( m_Frontend.GetPhysicalDeviceProperties().limits.timestampPeriod ) )
    {
    }

    /*************************************************************************\

    Function:
        ~DeviceProfilerTraceSerializer

    Description:
        Destructor.

    \*************************************************************************/
    DeviceProfilerTraceSerializer::~DeviceProfilerTraceSerializer()
    {
        CloseOutputFile();

        delete m_pJsonSerializer;
        delete m_pStringSerializer;
    }

    /*************************************************************************\

    Function:
        OpenOutputFile

    Description:
        Open output file for writing the trace.

    \*************************************************************************/
    bool DeviceProfilerTraceSerializer::OpenOutputFile( const std::string& fileName )
    {
        m_OutputFile.open( fileName, std::ios::out | std::ios::trunc | std::ios::binary );

        if( !m_OutputFile.is_open() )
        {
            return false;
        }

        // Write file header.
        m_OutputFile << '{';
        m_OutputFile << "\"displayTimeUnit\":\"ns\"," << lf;
        m_OutputFile << " \"otherData\":" << json::object() << "," << lf;
        m_OutputFile << " \"traceEvents\":[" << eof;

        m_OutputFileEmpty = true;

        return !m_OutputFile.fail();
    }

    /*************************************************************************\

    Function:
        CloseOutputFile

    Description:
        Close the output file.

    \*************************************************************************/
    void DeviceProfilerTraceSerializer::CloseOutputFile()
    {
        if( m_OutputFile.is_open() )
        {
            m_OutputFile.close();
        }
    }

    /*************************************************************************\

    Function:
        AppendEventsToOutputFile

    Description:

    \*************************************************************************/
    void DeviceProfilerTraceSerializer::AppendEventsToOutputFile()
    {
        if( m_Events.empty() )
        {
            return;
        }

        // Remove last 3 characters ("]}" + lf)
        m_OutputFile.seekp( -eof_len, std::ios::end );

        // Continue the array
        if( !m_OutputFileEmpty )
        {
            m_OutputFile << eol;
        }
        else
        {
            m_OutputFile << lf;
        }

        for( const json& event : m_Events )
        {
            m_OutputFile << event << eol;
        }

        // Remove the last comma and insert end of array and end of object
        m_OutputFile.seekp( -eol_len, std::ios::end );
        m_OutputFile << eof;

        m_OutputFileEmpty = false;

        // Cleanup serializer state
        Cleanup();
    }

    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize collected results to the trace events without writing to
        the file.

    \*************************************************************************/
    DeviceProfilerTraceSerializationResult DeviceProfilerTraceSerializer::Serialize( const DeviceProfilerFrameData& data )
    {
        // Setup state for serialization
        m_pData = &data;

        DeviceProfilerTraceSerializationResult result = {};
        result.m_Succeeded = true;
        result.m_Message = "Saved trace to\n";

        SetupTimestampNormalizationConstants();

        const Milliseconds frameGpuBeginTimestamp = GetNormalizedGpuTimestamp( data.m_BeginTimestamp );
        const Milliseconds frameGpuEndTimestamp = GetNormalizedGpuTimestamp( data.m_EndTimestamp );

        std::string frameName = "Frame #" + std::to_string( data.m_CPU.m_FrameIndex );
        m_Events.push_back( TraceEvent(
            TraceEvent::Phase::eDurationBegin,
            frameName,
            "Frames",
            frameGpuBeginTimestamp,
            VK_NULL_HANDLE ) );

        // Serialize the data
        for( const auto& submitBatchData : data.m_Submits )
        {
            m_CommandQueue = submitBatchData.m_Handle;

            // Insert queue submission event
            m_Events.push_back( ApiTraceEvent(
                TraceEvent::Phase::eInstant,
                "vkQueueSubmit",
                submitBatchData.m_ThreadId,
                GetNormalizedCpuTimestamp( submitBatchData.m_Timestamp ) ) );

            for( const auto& submitData : submitBatchData.m_Submits )
            {
                #if ENABLE_FLOW_EVENTS
                for( const auto& waitSemaphore : submitData.m_WaitSemaphores )
                {
                    m_Events.push_back( TraceEvent(
                        TraceEvent::Phase::eFlowEnd,
                        m_pStringSerializer->GetName( waitSemaphore ),
                        "Synchronization",
                        GetNormalizedGpuTimestamp( submitData.m_BeginTimestamp.m_Value ),
                        m_CommandQueue ) );
                }
                #endif

                for( const auto& commandBufferData : submitData.m_CommandBuffers )
                {
                    Serialize( commandBufferData );
                }

                #if ENABLE_FLOW_EVENTS
                for( const auto& signalSemaphpre : submitData.m_SignalSemaphores )
                {
                    m_Events.push_back( TraceEvent(
                        TraceEvent::Phase::eFlowStart,
                        m_pStringSerializer->GetName( signalSemaphpre ),
                        "Synchronization",
                        GetNormalizedGpuTimestamp( submitData.m_EndTimestamp.m_Value ),
                        m_CommandQueue ) );
                }
                #endif
            }
        }

        if( data.m_FrameDelimiter == VK_PROFILER_FRAME_DELIMITER_PRESENT_EXT )
        {
            // Insert present event
            m_Events.push_back( ApiTraceEvent(
                TraceEvent::Phase::eInstant,
                "vkQueuePresentKHR",
                data.m_CPU.m_ThreadId,
                GetNormalizedCpuTimestamp( data.m_CPU.m_EndTimestamp ) ) );
        }

        // Serialize the performance counters straem
        if( !data.m_PerformanceCounters.m_StreamTimestamps.empty() )
        {
            const size_t streamResultCount = data.m_PerformanceCounters.m_StreamTimestamps.size();
            const size_t performanceCountersCount = data.m_PerformanceCounters.m_StreamResults.size();

            std::vector<VkProfilerPerformanceCounterProperties2EXT> performanceCounterProperties( performanceCountersCount );
            m_Frontend.GetPerformanceMetricsSetCounterProperties(
                data.m_PerformanceCounters.m_MetricsSetIndex,
                performanceCountersCount,
                performanceCounterProperties.data() );

            std::vector<VkProfilerPerformanceCounterResultEXT> performanceCounterSamples( performanceCountersCount );

            for( size_t i = 0; i < streamResultCount; ++i )
            {
                for( size_t j = 0; j < performanceCountersCount; ++j )
                {
                    performanceCounterSamples[j] =
                        data.m_PerformanceCounters.m_StreamResults[j].m_Samples[i];
                }

                m_Events.push_back( TraceCounterEvent(
                    GetNormalizedGpuTimestamp( data.m_PerformanceCounters.m_StreamTimestamps[i] ),
                    m_CommandQueue,
                    performanceCountersCount,
                    performanceCounterProperties.data(),
                    performanceCounterSamples.data() ) );
            }

            Fill( performanceCounterSamples, VkProfilerPerformanceCounterResultEXT{} );
            m_Events.push_back( TraceCounterEvent(
                frameGpuEndTimestamp,
                m_CommandQueue,
                performanceCountersCount,
                performanceCounterProperties.data(),
                performanceCounterSamples.data() ) );
        }

        m_Events.push_back( TraceEvent(
            TraceEvent::Phase::eDurationEnd,
            frameName,
            "Frames",
            frameGpuEndTimestamp,
            VK_NULL_HANDLE ) );

        // Insert TIP events
        Serialize( data.m_TIP );

        // Write the serialized events to the output file
        AppendEventsToOutputFile();

        m_pData = nullptr;

        return result;
    }

    /*************************************************************************\

    Function:
        Serialize

    Description:
        Write collected results to the trace file.

    \*************************************************************************/
    DeviceProfilerTraceSerializationResult DeviceProfilerTraceSerializer::Serialize( const std::string& fileName, const DeviceProfilerFrameData& data )
    {
        if( !OpenOutputFile( fileName ) )
        {
            return { false, "Could not open output file.\n" };
        }

        // Write data to the JSON file.
        DeviceProfilerTraceSerializationResult result = Serialize( data );

        CloseOutputFile();

        return result;
    }

    /*************************************************************************\

    Function:
        SetupTimestampNormalizationConstants

    Description:
        Setup constants used by GetNormalizedGpuTimestamp to determine approx
        CPU timestamp relative to the beginning of the frame.

    \*************************************************************************/
    void DeviceProfilerTraceSerializer::SetupTimestampNormalizationConstants()
    {
        // When multiple frames are serialized, the first frame's synchronization timestamp should be used as a reference
        // to avoid overlapping of the regions due to changing the time base frequently.
        if( m_HostCalibratedTimestamp != 0 && m_DeviceCalibratedTimestamp != 0 )
        {
            return;
        }

        // Try to use calibrated timestamps if available.
        m_HostTimeDomain = m_pData->m_SyncTimestamps.m_HostTimeDomain;
        m_HostCalibratedTimestamp = m_pData->m_SyncTimestamps.m_HostCalibratedTimestamp;
        m_DeviceCalibratedTimestamp = m_pData->m_SyncTimestamps.m_DeviceCalibratedTimestamp;
        m_HostTimestampFrequency = m_Frontend.GetHostTimestampFrequency( m_HostTimeDomain );

        // Manually select calibration timestamps from the data.
        if( m_HostCalibratedTimestamp == 0 )
        {
            m_HostCalibratedTimestamp = m_pData->m_CPU.m_BeginTimestamp;
        }

        // Use first submitted packet's begin timestamp as a reference if synchronization timestamps were not sent.
        if( m_DeviceCalibratedTimestamp == 0 )
        {
            uint64_t deviceBeginTimestamp = UINT64_MAX;

            for( const DeviceProfilerSubmitBatchData& submitBatch : m_pData->m_Submits )
            {
                for( const DeviceProfilerSubmitData& submit : submitBatch.m_Submits )
                {
                    uint64_t gpuTimestamp = submit.GetBeginTimestamp().m_Value;
                    if( gpuTimestamp )
                    {
                        deviceBeginTimestamp = std::min( deviceBeginTimestamp, gpuTimestamp );
                        break;
                    }
                }
            }

            if( deviceBeginTimestamp != UINT64_MAX )
            {
                m_DeviceCalibratedTimestamp = deviceBeginTimestamp;
            }
        }
    }

    /*************************************************************************\

    Function:
        GetNormalizedCpuTimestamp

    Description:
        Get CPU timestamp aligned to the frame begin CPU timestamp.

    \*************************************************************************/
    Milliseconds DeviceProfilerTraceSerializer::GetNormalizedCpuTimestamp( uint64_t timestamp ) const
    {
        return std::chrono::duration_cast<Milliseconds>(std::chrono::nanoseconds(
            ((timestamp - m_HostCalibratedTimestamp) * 1'000'000'000) / m_HostTimestampFrequency ));
    }

    /*************************************************************************\

    Function:
        GetNormalizedGpuTimestamp

    Description:
        Get GPU timestamp aligned to the frame begin CPU timestamp.

    \*************************************************************************/
    Milliseconds DeviceProfilerTraceSerializer::GetNormalizedGpuTimestamp( uint64_t gpuTimestamp ) const
    {
        return ((gpuTimestamp - m_DeviceCalibratedTimestamp) * m_GpuTimestampPeriod);
    }

    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize command buffer data to list of TraceEvent structures.

    \*************************************************************************/
    void DeviceProfilerTraceSerializer::Serialize( const DeviceProfilerCommandBufferData& data )
    {
        const std::string eventName = m_pStringSerializer->GetName( data );

        // Begin
        m_Events.push_back( TraceEvent(
            TraceEvent::Phase::eDurationBegin,
            eventName,
            "Command buffers",
            GetNormalizedGpuTimestamp( data.m_BeginTimestamp.m_Value ),
            m_CommandQueue ) );

        // Performance counters
        const uint32_t performanceCounterCount = static_cast<uint32_t>( data.m_PerformanceCounters.m_Results.size() );
        std::vector<VkProfilerPerformanceCounterProperties2EXT> performanceCounterProperties(
            performanceCounterCount );

        if( performanceCounterCount )
        {
            m_Frontend.GetPerformanceMetricsSetCounterProperties(
                data.m_PerformanceCounters.m_MetricsSetIndex,
                performanceCounterCount,
                performanceCounterProperties.data() );

            m_Events.push_back( TraceCounterEvent(
                GetNormalizedGpuTimestamp( data.m_BeginTimestamp.m_Value ),
                m_CommandQueue,
                performanceCounterProperties.size(),
                performanceCounterProperties.data(),
                data.m_PerformanceCounters.m_Results.data() ) );
        }

        for( const auto& renderPassData : data.m_RenderPasses )
        {
            if( renderPassData.m_BeginTimestamp.m_Value != UINT64_MAX )
            {
                // Serialize the render pass
                Serialize( renderPassData );
            }
        }

        // Clear performance counters before the next command buffer
        if( performanceCounterCount )
        {
            m_Events.push_back( TraceCounterEvent(
                GetNormalizedGpuTimestamp( data.m_EndTimestamp.m_Value ),
                m_CommandQueue,
                performanceCounterProperties.size(),
                performanceCounterProperties.data(),
                nullptr ) );
        }

        // End
        m_Events.push_back( TraceEvent(
            TraceEvent::Phase::eDurationEnd,
            eventName,
            "Command buffers",
            GetNormalizedGpuTimestamp( data.m_EndTimestamp.m_Value ),
            m_CommandQueue ) );
    }

    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize render pass data to list of TraceEvent structures.

    \*************************************************************************/
    void DeviceProfilerTraceSerializer::Serialize( const DeviceProfilerRenderPassData& data )
    {
        const bool isValidRenderPass = (data.m_Type != DeviceProfilerRenderPassType::eNone);
        const std::string eventName = m_pStringSerializer->GetName(data);

        if( isValidRenderPass )
        {
            // Begin
            m_Events.push_back( TraceEvent(
                TraceEvent::Phase::eDurationBegin,
                eventName,
                "Render passes",
                GetNormalizedGpuTimestamp( data.m_BeginTimestamp.m_Value ),
                m_CommandQueue ) );

            if( (data.HasBeginCommand()) &&
                (data.m_Begin.m_BeginTimestamp.m_Value != UINT64_MAX) )
            {
                const std::string beginEventName = m_pStringSerializer->GetName( data.m_Begin, data.m_Dynamic );

                // vkCmdBeginRenderPass
                m_Events.push_back( TraceCompleteEvent(
                    beginEventName,
                    "Drawcalls",
                    GetNormalizedGpuTimestamp( data.m_Begin.m_BeginTimestamp.m_Value ),
                    GetDuration( data.m_Begin ),
                    m_CommandQueue ) );
            }
        }

        for( const auto& subpassData : data.m_Subpasses )
        {
            // Serialize the subpass
            Serialize( subpassData, ( data.m_Subpasses.size() == 1 ) );
        }
        
        if( isValidRenderPass )
        {
            if( (data.HasEndCommand()) &&
                (data.m_End.m_BeginTimestamp.m_Value != UINT64_MAX) )
            {
                const std::string endEventName = m_pStringSerializer->GetName( data.m_End, data.m_Dynamic );

                // vkCmdEndRenderPass
                m_Events.push_back( TraceCompleteEvent(
                    endEventName,
                    "Drawcalls",
                    GetNormalizedGpuTimestamp( data.m_End.m_BeginTimestamp.m_Value ),
                    GetDuration( data.m_End ),
                    m_CommandQueue ) );
            }

            // End
            m_Events.push_back( TraceEvent(
                TraceEvent::Phase::eDurationEnd,
                eventName,
                "Render passes",
                GetNormalizedGpuTimestamp( data.m_EndTimestamp.m_Value ),
                m_CommandQueue ) );
        }
    }

    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize subpass data to list of TraceEvent structures.

    \*************************************************************************/
    void DeviceProfilerTraceSerializer::Serialize( const DeviceProfilerSubpassData& data, bool isOnlySubpassInRenderPass )
    {
        const std::string eventName = m_pStringSerializer->GetName( data );

        if( !isOnlySubpassInRenderPass )
        {
            // Begin
            m_Events.push_back( TraceEvent(
                TraceEvent::Phase::eDurationBegin,
                eventName, "",
                GetNormalizedGpuTimestamp( data.m_BeginTimestamp.m_Value ),
                m_CommandQueue ) );
        }

        for( const auto& data : data.m_Data )
        {
            if( data.GetBeginTimestamp().m_Value == UINT64_MAX )
            {
                continue;
            }

            switch( data.GetType() )
            {
            case DeviceProfilerSubpassDataType::ePipeline:
                Serialize( std::get<DeviceProfilerPipelineData>( data ) );
                break;

            case DeviceProfilerSubpassDataType::eCommandBuffer:
                Serialize( std::get<DeviceProfilerCommandBufferData>( data ) );
                break;
            }
        }

        if( !isOnlySubpassInRenderPass )
        {
            // End
            m_Events.push_back( TraceEvent(
                TraceEvent::Phase::eDurationEnd,
                eventName, "",
                GetNormalizedGpuTimestamp( data.m_EndTimestamp.m_Value ),
                m_CommandQueue ) );
        }
    }

    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize pipeline data to list of TraceEvent structures.

    \*************************************************************************/
    void DeviceProfilerTraceSerializer::Serialize( const DeviceProfilerPipelineData& data )
    {
        const std::string eventName = m_pStringSerializer->GetName( data, true /*showEntryPoints*/ );
        const nlohmann::json eventArgs = m_pJsonSerializer->GetPipelineArgs( data );

        const bool isValidPipeline =
            (data.m_Handle ||
                data.m_UsesShaderObjects) &&
            ((data.m_ShaderTuple.m_Hash & 0xFFFF) != 0);

        if( isValidPipeline )
        {
            // Begin
            m_Events.push_back( TraceEvent(
                TraceEvent::Phase::eDurationBegin,
                eventName,
                "Pipelines",
                GetNormalizedGpuTimestamp( data.m_BeginTimestamp.m_Value ),
                m_CommandQueue,
                {} ,
                eventArgs ) );
        }

        for( const auto& drawcall : data.m_Drawcalls )
        {
            if( drawcall.m_BeginTimestamp.m_Value != UINT64_MAX )
            {
                // Serialize the drawcall
                Serialize( drawcall );
            }
        }

        if( isValidPipeline )
        {
            // End
            m_Events.push_back( TraceEvent(
                TraceEvent::Phase::eDurationEnd,
                eventName,
                "Pipelines",
                GetNormalizedGpuTimestamp( data.m_EndTimestamp.m_Value ),
                m_CommandQueue ) );
        }
    }

    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize drawcall to TraceEvent structure.

    \*************************************************************************/
    void DeviceProfilerTraceSerializer::Serialize( const DeviceProfilerDrawcall& data )
    {
        if( data.GetPipelineType() != DeviceProfilerPipelineType::eDebug )
        {
            const std::string eventName = m_pStringSerializer->GetCommandName( data );
            const nlohmann::json eventArgs = m_pJsonSerializer->GetCommandArgs( data );

            // Cannot use complete events due to loss of precision
            m_Events.push_back( TraceEvent(
                TraceEvent::Phase::eDurationBegin,
                eventName,
                "Drawcalls",
                GetNormalizedGpuTimestamp( data.m_BeginTimestamp.m_Value ),
                m_CommandQueue,
                {},
                eventArgs ) );

            m_Events.push_back( TraceEvent(
                TraceEvent::Phase::eDurationEnd,
                eventName,
                "Drawcalls",
                GetNormalizedGpuTimestamp( data.m_EndTimestamp.m_Value ),
                m_CommandQueue ) );
        }

        else
        {
            if( data.m_Type == DeviceProfilerDrawcallType::eInsertDebugLabel )
            {
                const char* pDebugLabel = data.m_Payload.m_DebugLabel.m_pName == nullptr ? "" : data.m_Payload.m_DebugLabel.m_pName;
                // Insert debug labels as instant events
                m_Events.push_back( DebugTraceEvent(
                    TraceEvent::Phase::eInstant,
                    pDebugLabel,
                    GetNormalizedGpuTimestamp( data.m_BeginTimestamp.m_Value ) ) );
            }

            if( data.m_Type == DeviceProfilerDrawcallType::eBeginDebugLabel )
            {
                const char* pDebugLabel = data.m_Payload.m_DebugLabel.m_pName == nullptr ? "" : data.m_Payload.m_DebugLabel.m_pName;
                m_Events.push_back( DebugTraceEvent(
                    TraceEvent::Phase::eDurationBegin,
                    pDebugLabel,
                    GetNormalizedGpuTimestamp( data.m_BeginTimestamp.m_Value ) ) );

                m_DebugLabelStackDepth++;
            }

            if( data.m_Type == DeviceProfilerDrawcallType::eEndDebugLabel )
            {
                // End only events that started in current frame
                if( m_DebugLabelStackDepth > 0 )
                {
                    m_Events.push_back( DebugTraceEvent(
                        TraceEvent::Phase::eDurationEnd,
                        "",
                        GetNormalizedGpuTimestamp( data.m_BeginTimestamp.m_Value ) ) );

                    m_DebugLabelStackDepth--;
                }
            }
        }
    }

    /*************************************************************************\

    Function:
        Serialize

    \*************************************************************************/
    void DeviceProfilerTraceSerializer::Serialize( const std::vector<TipRange>& tipData )
    {
        for( const TipRange& range : tipData )
        {
            m_Events.push_back( ApiTraceEvent(
                TraceEvent::Phase::eDurationBegin,
                range.m_pFunctionName,
                range.m_ThreadId,
                GetNormalizedCpuTimestamp( range.m_BeginTimestamp ) ) );

            m_Events.push_back( ApiTraceEvent(
                TraceEvent::Phase::eDurationEnd,
                range.m_pFunctionName,
                range.m_ThreadId,
                GetNormalizedCpuTimestamp( range.m_EndTimestamp ) ) );
        }
    }

    /*************************************************************************\

    Function:
        ConstructTraceFileName

    \*************************************************************************/
    std::string DeviceProfilerTraceSerializer::GetDefaultTraceFileName( int samplingMode )
    {
        using namespace std::chrono;

        // Get current time and date
        const auto currentTimePoint = system_clock::now();
        const auto currentTimePointInTimeT = system_clock::to_time_t( currentTimePoint );

        tm localTime;
        ProfilerPlatformFunctions::GetLocalTime( &localTime, currentTimePointInTimeT );

        // Get milliseconds
        const auto ms = time_point_cast<milliseconds>(currentTimePoint).time_since_epoch() % 1000;

        // Construct output file name
        std::stringstream stringBuilder;
        stringBuilder << ProfilerPlatformFunctions::GetProcessName() << "_";
        stringBuilder << ProfilerPlatformFunctions::GetCurrentProcessId() << "_";
        stringBuilder << std::put_time( &localTime, "%Y-%m-%d_%H-%M-%S" ) << "_" << ms.count();
        stringBuilder << "_" << GetSamplingModeComponent( samplingMode );
        stringBuilder << ".json";

        return stringBuilder.str();
    }

    /*************************************************************************\

    Function:
        Cleanup

    \*************************************************************************/
    void DeviceProfilerTraceSerializer::Cleanup()
    {
        m_Events.clear();
    }

    /*************************************************************************\

    Function:
        ProfilerTraceOutput

    Description:
        Constructor.

    \*************************************************************************/
    ProfilerTraceOutput::ProfilerTraceOutput( DeviceProfilerFrontend& frontend )
        : DeviceProfilerOutput( frontend )
    {
        ResetMembers();
    }

    /*************************************************************************\

    Function:
        ~ProfilerTraceOutput

    Description:
        Destructor.

    \*************************************************************************/
    ProfilerTraceOutput::~ProfilerTraceOutput()
    {
    }

    /*************************************************************************\

    Function:
        Initialize

    Description:
        Initializes the trace file output for the given profiler.

    \*************************************************************************/
    bool ProfilerTraceOutput::Initialize()
    {
        // Create string serializer.
        m_pStringSerializer = new DeviceProfilerStringSerializer(
            m_Frontend );

        // Create trace serializer.
        m_pTraceSerializer = new DeviceProfilerTraceSerializer(
            m_Frontend );

        // Configure the output.
        const DeviceProfilerConfig& config = m_Frontend.GetProfilerConfig();
        SetMaxFrameCount( config.m_FrameCount );
        SetSkipFrameCount( config.m_FrameSkipCount );

        std::string outputFileName = config.m_OutputTraceFile;
        if( outputFileName.empty() )
        {
            outputFileName = DeviceProfilerTraceSerializer::GetDefaultTraceFileName(
                m_Frontend.GetProfilerSamplingMode() );
        }

        if( !m_pTraceSerializer->OpenOutputFile( outputFileName ) )
        {
            Destroy();
            return false;
        }

        // Start trace serialization thread.
        if( config.m_EnableThreading )
        {
            m_TraceSerializationThread = std::thread( &ProfilerTraceOutput::TraceSerializationThreadProc, this );
            m_TraceSerializationThreadRunning = true;
        }

        return true;
    }

    /*************************************************************************\

    Function:
        Destroy

    Description:
        Flushes and destroys the trace file output.

    \*************************************************************************/
    void ProfilerTraceOutput::Destroy()
    {
        StopTraceSerializationThread();

        delete m_pTraceSerializer;
        delete m_pStringSerializer;

        ResetMembers();
    }

    /*************************************************************************\

    Function:
        IsAvailable

    Description:
        Checks if the trace file output is available.

    \*************************************************************************/
    bool ProfilerTraceOutput::IsAvailable()
    {
        return m_pTraceSerializer != nullptr;
    }

    /*************************************************************************\

    Function:
        Update

    Description:
        Reads data collected by the profiler and appends it to the serializer.

    \*************************************************************************/
    void ProfilerTraceOutput::Update()
    {
        uint64_t totalFrameCount = UINT64_MAX;
        uint64_t skipFrameCount = static_cast<uint64_t>( m_SkipFrameCount ) + 1;

        if( m_MaxFrameCount != UINT32_MAX )
        {
            totalFrameCount =
                static_cast<uint64_t>( m_MaxFrameCount ) +
                static_cast<uint64_t>( skipFrameCount );
        }

        auto pData = m_Frontend.GetData();
        while( pData )
        {
            // Rough check to avoid locking mutex if not needed.
            if( m_ProcessedFrameCount <= totalFrameCount )
            {
                std::scoped_lock lock( m_TraceSerializerMutex );

                if( m_ProcessedFrameCount < skipFrameCount )
                {
                    m_ProcessedFrameCount++;
                }
                else if( m_ProcessedFrameCount <= totalFrameCount )
                {
                    if( m_TraceSerializationThreadRunning )
                    {
                        std::scoped_lock lock( m_FrameDataQueueMutex );
                        m_FrameDataQueue.push( pData );
                        m_TraceSerializationThreadInputAvailable.notify_one();
                    }
                    else
                    {
                        m_pTraceSerializer->Serialize( *pData );
                    }

                    m_ProcessedFrameCount++;
                }
            }

            pData = m_Frontend.GetData();
        }
    }

    /*************************************************************************\

    Function:
        Present

    Description:
        No-op.

    \*************************************************************************/
    void ProfilerTraceOutput::Present()
    {
    }

    /*************************************************************************\

    Function:
        SetMaxFrameCount

    Description:
        Sets max serialized frame count.

    \*************************************************************************/
    void ProfilerTraceOutput::SetMaxFrameCount( uint32_t maxFrameCount )
    {
        m_MaxFrameCount = ( maxFrameCount ? maxFrameCount : UINT32_MAX );

        // Update data buffers.
        m_Frontend.SetDataBufferSize( m_MaxFrameCount );
    }

    /*************************************************************************\

    Function:
        SetSkipFrameCount

    Description:
        Sets number of initial frames to skip before serialization.

    \*************************************************************************/
    void ProfilerTraceOutput::SetSkipFrameCount( uint32_t skipFrameCount )
    {
        m_SkipFrameCount = skipFrameCount;
    }

    /*************************************************************************\

    Function:
        ResetMembers

    Description:
        Sets all members to default values.

    \*************************************************************************/
    void ProfilerTraceOutput::ResetMembers()
    {
        m_pStringSerializer = nullptr;
        m_pTraceSerializer = nullptr;

        m_MaxFrameCount = UINT32_MAX;
        m_SkipFrameCount = 0;
        m_ProcessedFrameCount = 0;

        m_TraceSerializationThread = std::thread();
        m_TraceSerializationThreadRunning = false;
        m_TraceSerializationThreadQuitSignal = false;

        m_FrameDataQueue = {};
    }

    /*************************************************************************\

    Function:
        TraceSerializationThreadProc

    Description:

    \*************************************************************************/
    void ProfilerTraceOutput::TraceSerializationThreadProc()
    {
        while( !m_TraceSerializationThreadQuitSignal )
        {
            std::unique_lock lock( m_FrameDataQueueMutex );

            if( m_FrameDataQueue.empty() )
            {
                m_TraceSerializationThreadInputAvailable.wait( lock, [this]
                    { return !m_FrameDataQueue.empty() || m_TraceSerializationThreadQuitSignal; } );
            }

            while( !m_FrameDataQueue.empty() )
            {
                std::shared_ptr<DeviceProfilerFrameData> pData = m_FrameDataQueue.front();
                m_FrameDataQueue.pop();
                lock.unlock();

                if( pData && !pData->m_Submits.empty() )
                {
                    std::scoped_lock serializerLock( m_TraceSerializerMutex );
                    m_pTraceSerializer->Serialize( *pData );
                }

                lock.lock();
            }
        }
    }

    /*************************************************************************\

    Function:
        StopTraceSerializationThread

    Description:

    \*************************************************************************/
    void ProfilerTraceOutput::StopTraceSerializationThread()
    {
        std::unique_lock lock( m_FrameDataQueueMutex );
        m_TraceSerializationThreadQuitSignal = true;
        m_TraceSerializationThreadInputAvailable.notify_all();
        lock.unlock();

        if( m_TraceSerializationThread.joinable() )
        {
            m_TraceSerializationThread.join();
        }
    }
}

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

#include "profiler_trace.h"
#include "profiler_trace_event.h"
#include "profiler_json.h"
#include "profiler/profiler_data.h"
#include "profiler/profiler_helpers.h"
#include "profiler_layer_objects/VkObject.h"
#include "profiler_helpers/profiler_data_helpers.h"

#include "VkLayer_profiler_layer.generated.h"

#include <chrono>
#include <fstream>
#include <sstream>
#include <iomanip>

#include <nlohmann/json.hpp>

// Alias commonly used types
using json = nlohmann::json;

namespace Profiler
{
    /*************************************************************************\

    Function:
        DeviceProfilerTraceSerializer

    Description:
        Constructor.

    \*************************************************************************/
    DeviceProfilerTraceSerializer::DeviceProfilerTraceSerializer( const DeviceProfilerStringSerializer* pStringSerializer, Milliseconds gpuTimestampPeriod )
        : m_pStringSerializer( pStringSerializer )
        , m_pJsonSerializer( nullptr )
        , m_pData( nullptr )
        , m_CommandQueue( VK_NULL_HANDLE )
        , m_pEvents()
        , m_DebugLabelStackDepth( 0 )
        , m_CpuQueueSubmitTimestampOffset( 0 )
        , m_GpuQueueSubmitTimestampOffset( 0 )
        , m_GpuTimestampPeriod( gpuTimestampPeriod )
    {
        // Initialize JSON serializer
        m_pJsonSerializer = new DeviceProfilerJsonSerializer( m_pStringSerializer );
    }

    /*************************************************************************\

    Function:
        ~DeviceProfilerTraceSerializer

    Description:
        Destructor.

    \*************************************************************************/
    DeviceProfilerTraceSerializer::~DeviceProfilerTraceSerializer()
    {
        delete m_pJsonSerializer;
    }

    /*************************************************************************\

    Function:
        Serialize

    Description:
        Write collected results to the trace file.

    \*************************************************************************/
    DeviceProfilerTraceSerializationResult DeviceProfilerTraceSerializer::Serialize( const DeviceProfilerFrameData& data )
    {
        // Setup state for serialization
        m_pData = &data;

        DeviceProfilerTraceSerializationResult result = {};
        result.m_Succeeded = true;

        // Serialize the data
        for( const auto& submitBatchData : data.m_Submits )
        {
            SetupTimestampNormalizationConstants( submitBatchData.m_Handle );

            // Insert queue submission event
            m_pEvents.push_back( new ApiTraceEvent(
                TraceInstantEvent::Scope::eThread,
                "vkQueueSubmit",
                submitBatchData.m_ThreadId,
                GetNormalizedCpuTimestamp( submitBatchData.m_Timestamp ) ) );

            for( const auto& submitData : submitBatchData.m_Submits )
            {
                #if ENABLE_FLOW_EVENTS
                for( const auto& waitSemaphore : submitData.m_WaitSemaphores )
                {
                    m_pEvents.push_back( new TraceEvent(
                        TraceEvent::Phase::eFlowEnd,
                        m_pStringSerializer->GetName( waitSemaphore ),
                        "Synchronization",
                        GetNormalizedGpuTimestamp( submitData.m_BeginTimestamp ),
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
                    m_pEvents.push_back( new TraceEvent(
                        TraceEvent::Phase::eFlowStart,
                        m_pStringSerializer->GetName( signalSemaphpre ),
                        "Synchronization",
                        GetNormalizedGpuTimestamp( submitData.m_EndTimestamp ),
                        m_CommandQueue ) );
                }
                #endif
            }
        }

        // Insert present event
        m_pEvents.push_back( new ApiTraceEvent(
            TraceInstantEvent::Scope::eThread,
            "vkQueuePresentKHR",
            data.m_CPU.m_ThreadId,
            GetNormalizedCpuTimestamp( data.m_CPU.m_EndTimestamp ) ) );

        // Write JSON file
        SaveEventsToFile( result );

        // Cleanup serializer state
        Cleanup();
        
        return result;
    }

    /*************************************************************************\

    Function:
        SetupTimestampNormalizationConstants

    Description:
        Setup constants used by GetNormalizedGpuTimestamp to determine approx
        CPU timestamp relative to the beginning of the frame.

    \*************************************************************************/
    void DeviceProfilerTraceSerializer::SetupTimestampNormalizationConstants( VkQueue queue )
    {
        m_CommandQueue = queue;

        m_CpuQueueSubmitTimestampOffset = Milliseconds( 0 );

        // Get base command buffer offset.
        // Better CPU-GPU correlation could be achieved from ETLs
        m_GpuQueueSubmitTimestampOffset = m_pData->m_SyncTimestamps.at( queue );

        for( const auto& submitBatchData : m_pData->m_Submits )
        {
            if( submitBatchData.m_Handle == queue )
            {
                m_CpuQueueSubmitTimestampOffset = GetNormalizedCpuTimestamp( submitBatchData.m_Timestamp );

                // Use first submitted packet's begin timestamp as a reference if synchronization timestamps were not sent.
                if( m_GpuQueueSubmitTimestampOffset == 0 )
                {
                    m_GpuQueueSubmitTimestampOffset = !submitBatchData.m_Submits.empty()
                        ? submitBatchData.m_Submits.front().m_BeginTimestamp.m_Value
                        : 0;
                }
                break;
            }
        }

        // If no timestamps were recorded in the command buffer, apply no offset
        if( m_GpuQueueSubmitTimestampOffset == -1 )
        {
            m_GpuQueueSubmitTimestampOffset = 0;
        }
    }

    /*************************************************************************\

    Function:
        GetNormalizedCpuTimestamp

    Description:
        Get CPU timestamp aligned to the frame begin CPU timestamp.

    \*************************************************************************/
    Milliseconds DeviceProfilerTraceSerializer::GetNormalizedCpuTimestamp( std::chrono::high_resolution_clock::time_point timestamp ) const
    {
        assert( timestamp >= m_pData->m_CPU.m_BeginTimestamp );
        assert( timestamp <= m_pData->m_CPU.m_EndTimestamp );
        return timestamp - m_pData->m_CPU.m_BeginTimestamp;
    }

    /*************************************************************************\

    Function:
        GetNormalizedGpuTimestamp

    Description:
        Get GPU timestamp aligned to the frame begin CPU timestamp.

    \*************************************************************************/
    Milliseconds DeviceProfilerTraceSerializer::GetNormalizedGpuTimestamp( uint64_t gpuTimestamp ) const
    {
        return m_CpuQueueSubmitTimestampOffset +
            ((gpuTimestamp - m_GpuQueueSubmitTimestampOffset) * m_GpuTimestampPeriod);
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
        m_pEvents.push_back( new TraceEvent(
            TraceEvent::Phase::eDurationBegin,
            eventName,
            "Command buffers",
            GetNormalizedGpuTimestamp( data.m_BeginTimestamp.m_Value ),
            m_CommandQueue ) );

        for( const auto& renderPassData : data.m_RenderPasses )
        {
            if( renderPassData.m_BeginTimestamp.m_Value != UINT64_MAX )
            {
                // Serialize the render pass
                Serialize( renderPassData );
            }
        }

        // End
        m_pEvents.push_back( new TraceEvent(
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
            m_pEvents.push_back( new TraceEvent(
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
                m_pEvents.push_back( new TraceCompleteEvent(
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
                m_pEvents.push_back( new TraceCompleteEvent(
                    endEventName,
                    "Drawcalls",
                    GetNormalizedGpuTimestamp( data.m_End.m_BeginTimestamp.m_Value ),
                    GetDuration( data.m_End ),
                    m_CommandQueue ) );
            }

            // End
            m_pEvents.push_back( new TraceEvent(
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
            m_pEvents.push_back( new TraceEvent(
                TraceEvent::Phase::eDurationBegin,
                eventName, "",
                GetNormalizedGpuTimestamp( data.m_BeginTimestamp.m_Value ),
                m_CommandQueue ) );
        }

        switch( data.m_Contents )
        {
        case VK_SUBPASS_CONTENTS_INLINE:
        {
            for( const auto& pipelineData : data.m_Pipelines )
            {
                if( pipelineData.m_BeginTimestamp.m_Value != UINT64_MAX )
                {
                    // Serialize the pipelines
                    Serialize( pipelineData );
                }
            }
            break;
        }

        case VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS:
        {
            for( const auto& commandBufferData : data.m_SecondaryCommandBuffers )
            {
                // Serialize the command buffer
                Serialize( commandBufferData );
            }
            break;
        }

        default:
        {
            assert( !"Unsupported VkSubpassContents enum value" );
            break;
        }
        }

        if( !isOnlySubpassInRenderPass )
        {
            // End
            m_pEvents.push_back( new TraceEvent(
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
        const std::string eventName = m_pStringSerializer->GetName( data );

        const bool isValidPipeline =
            (data.m_Handle) &&
            ((data.m_ShaderTuple.m_Hash & 0xFFFF) != 0);

        if( isValidPipeline )
        {
            // Begin
            m_pEvents.push_back( new TraceEvent(
                TraceEvent::Phase::eDurationBegin,
                eventName,
                "Pipelines",
                GetNormalizedGpuTimestamp( data.m_BeginTimestamp.m_Value ),
                m_CommandQueue ) );
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
            m_pEvents.push_back( new TraceEvent(
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
            m_pEvents.push_back( new TraceEvent(
                TraceEvent::Phase::eDurationBegin,
                eventName,
                "Drawcalls",
                GetNormalizedGpuTimestamp( data.m_BeginTimestamp.m_Value ),
                m_CommandQueue,
                {},
                eventArgs ) );

            m_pEvents.push_back( new TraceEvent(
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
                // Insert debug labels as instant events
                m_pEvents.push_back( new DebugTraceEvent(
                    TraceEvent::Phase::eInstant,
                    data.m_Payload.m_DebugLabel.m_pName,
                    GetNormalizedGpuTimestamp( data.m_BeginTimestamp.m_Value ) ) );
            }

            if( data.m_Type == DeviceProfilerDrawcallType::eBeginDebugLabel )
            {
                m_pEvents.push_back( new DebugTraceEvent(
                    TraceEvent::Phase::eDurationBegin,
                    data.m_Payload.m_DebugLabel.m_pName,
                    GetNormalizedGpuTimestamp( data.m_BeginTimestamp.m_Value ) ) );

                m_DebugLabelStackDepth++;
            }

            if( data.m_Type == DeviceProfilerDrawcallType::eEndDebugLabel )
            {
                // End only events that started in current frame
                if( m_DebugLabelStackDepth > 0 )
                {
                    m_pEvents.push_back( new DebugTraceEvent(
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
        ConstructTraceFileName

    \*************************************************************************/
    std::filesystem::path DeviceProfilerTraceSerializer::ConstructTraceFileName() const
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
        stringBuilder << ".json";

        return std::filesystem::absolute( stringBuilder.str() );
    }

    /*************************************************************************\

    Function:
        SaveEventsToFile

    \*************************************************************************/
    void DeviceProfilerTraceSerializer::SaveEventsToFile( DeviceProfilerTraceSerializationResult& result )
    {
        if( result.m_Succeeded )
        {
            json traceJson = {
                { "traceEvents", json::array() },
                { "displayTimeUnit", "ns" },
                { "otherData", json::object() } };

            // Create JSON objects
            json& traceEvents = traceJson[ "traceEvents" ];

            for( const TraceEvent* pEvent : m_pEvents )
            {
                traceEvents.push_back( *pEvent );
            }

            // Open output file
            std::filesystem::path filename = ConstructTraceFileName();
            std::ofstream out( filename );

            if( !out.is_open() )
            {
                // Failed to open file for writing
                result.m_Succeeded = false;
                result.m_Message = "Failed to open file\n" + filename.string();
                return;
            }

            // Write JSON to file
            out << traceJson;
            out.flush();

            if( out.bad() )
            {
                // Failed to write data
                result.m_Succeeded = false;
                result.m_Message = "Failed to write file\n" + filename.string();
                return;
            }

            // Success
            result.m_Succeeded = true;
            result.m_Message = "Saved trace to\n" + filename.string();
        }
    }

    /*************************************************************************\

    Function:
        Cleanup

    \*************************************************************************/
    void DeviceProfilerTraceSerializer::Cleanup()
    {
        // TODO: Check exception safety of allocations, raw pointers may lead to memory leaks
        for( TraceEvent* pEvent : m_pEvents )
        {
            delete pEvent;
        }

        m_pEvents.clear();
        m_pData = nullptr;
    }
}

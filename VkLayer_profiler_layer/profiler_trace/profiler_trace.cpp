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

#include "profiler_trace.h"
#include "profiler_trace_event.h"
#include "profiler/profiler_data.h"
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
        Serialize

    Description:
        Write collected results to the trace file.

    \*************************************************************************/
    void DeviceProfilerTraceSerializer::Serialize( const DeviceProfilerFrameData& data )
    {
        // Setup state for serialization
        m_pData = &data;

        // Serialize the data
        for( const auto& submitBatchData : data.m_Submits )
        {
            SetupTimestampNormalizationConstants( submitBatchData );

            // Setup state for serialization
            m_CommandQueue = submitBatchData.m_Handle;

            // Insert queue submission event
            m_pEvents.push_back( new TraceEvent(
                TraceEventPhase::eInstant,
                "vkQueueSubmit",
                "API",
                GetNormalizedCpuTimestamp( submitBatchData.m_Timestamp ),
                m_CommandQueue ) );

            for( const auto& submitData : submitBatchData.m_Submits )
            {
                for( const auto& commandBufferData : submitData.m_CommandBuffers )
                {
                    Serialize( commandBufferData );
                }
            }
        }

        // Write JSON file
        SaveEventsToFile();

        // Cleanup serializer state
        Cleanup();
        
    }

    /*************************************************************************\

    Function:
        SetupTimestampNormalizationConstants

    Description:
        Setup constants used by GetNormalizedGpuTimestamp to determine approx
        CPU timestamp relative to the beginning of the frame.

    \*************************************************************************/
    void DeviceProfilerTraceSerializer::SetupTimestampNormalizationConstants( const DeviceProfilerSubmitBatchData& submitBatchData )
    {
        // Queue submission timestamp
        m_CpuQueueSubmitTimestampOffset = GetNormalizedCpuTimestamp( submitBatchData.m_Timestamp );

        // Get base command buffer offset.
        // Better CPU-GPU correlation could be achieved from ETLs
        m_GpuQueueSubmitTimestampOffset = -1;

        for( const auto& submitData : submitBatchData.m_Submits )
        {
            for( const auto& commandBufferData : submitData.m_CommandBuffers )
            {
                if( (commandBufferData.m_BeginTimestamp > 0) &&
                    !(commandBufferData.m_RenderPasses.empty()) )
                {
                    m_GpuQueueSubmitTimestampOffset = std::min(
                        m_GpuQueueSubmitTimestampOffset, commandBufferData.m_BeginTimestamp );
                }
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
        if( data.m_BeginTimestamp != 0 )
        {
            // Begin
            m_pEvents.push_back( new TraceEvent(
                TraceEventPhase::eDurationBegin,
                m_pStringSerializer->GetName( data ),
                "Performance",
                GetNormalizedGpuTimestamp( data.m_BeginTimestamp ),
                m_CommandQueue ) );

            for( const auto& renderPassData : data.m_RenderPasses )
            {
                // Serialize the render pass
                Serialize( renderPassData );
            }

            // End
            m_pEvents.push_back( new TraceEvent(
                TraceEventPhase::eDurationEnd,
                m_pStringSerializer->GetName( data ),
                "Performance",
                GetNormalizedGpuTimestamp( data.m_EndTimestamp ),
                m_CommandQueue ) );
        }
    }

    /*************************************************************************\

    Function:
        Serialize

    Description:
        Serialize render pass data to list of TraceEvent structures.

    \*************************************************************************/
    void DeviceProfilerTraceSerializer::Serialize( const DeviceProfilerRenderPassData& data )
    {
        if( data.m_BeginTimestamp != 0 )
        {
            if( data.m_Handle )
            {
                // Begin
                m_pEvents.push_back( new TraceEvent(
                    TraceEventPhase::eDurationBegin,
                    m_pStringSerializer->GetName( data ),
                    "Performance",
                    GetNormalizedGpuTimestamp( data.m_BeginTimestamp ),
                    m_CommandQueue ) );

                // vkCmdBeginRenderPass
                m_pEvents.push_back( new TraceCompleteEvent(
                    "vkCmdBeginRenderPass",
                    "Performance",
                    GetNormalizedGpuTimestamp( data.m_Begin.m_BeginTimestamp ),
                    GetDuration( data.m_Begin ),
                    m_CommandQueue ) );
            }

            for( const auto& subpassData : data.m_Subpasses )
            {
                // Serialize the subpass
                Serialize( subpassData, (data.m_Subpasses.size() == 1) );
            }

            if( data.m_Handle )
            {
                // vkCmdEndRenderPass
                m_pEvents.push_back( new TraceCompleteEvent(
                    "vkCmdEndRenderPass",
                    "Performance",
                    GetNormalizedGpuTimestamp( data.m_End.m_BeginTimestamp ),
                    GetDuration( data.m_End ),
                    m_CommandQueue ) );

                // End
                m_pEvents.push_back( new TraceEvent(
                    TraceEventPhase::eDurationEnd,
                    m_pStringSerializer->GetName( data ),
                    "Performance",
                    GetNormalizedGpuTimestamp( data.m_EndTimestamp ),
                    m_CommandQueue ) );
            }
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
        if( data.m_BeginTimestamp != 0 )
        {
            if( !isOnlySubpassInRenderPass )
            {
                // Begin
                m_pEvents.push_back( new TraceEvent(
                    TraceEventPhase::eDurationBegin,
                    m_pStringSerializer->GetName( data ),
                    "Performance",
                    GetNormalizedGpuTimestamp( data.m_BeginTimestamp ),
                    m_CommandQueue ) );
            }

            switch( data.m_Contents )
            {
            case VK_SUBPASS_CONTENTS_INLINE:
            {
                for( const auto& pipelineData : data.m_Pipelines )
                {
                    // Serialize the pipelines
                    Serialize( pipelineData );
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
                    TraceEventPhase::eDurationEnd,
                    m_pStringSerializer->GetName( data ),
                    "Performance",
                    GetNormalizedGpuTimestamp( data.m_EndTimestamp ),
                    m_CommandQueue ) );
            }
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
        if( data.m_BeginTimestamp != 0 )
        {
            const bool isValidPipeline =
                (data.m_Handle) &&
                ((data.m_ShaderTuple.m_Hash & 0xFFFF) != 0);

            if( isValidPipeline )
            {
                // Begin
                m_pEvents.push_back( new TraceEvent(
                    TraceEventPhase::eDurationBegin,
                    m_pStringSerializer->GetName( data ),
                    "Performance",
                    GetNormalizedGpuTimestamp( data.m_BeginTimestamp ),
                    m_CommandQueue ) );
            }

            for( const auto& drawcall : data.m_Drawcalls )
            {
                // Serialize the drawcall
                Serialize( drawcall );
            }

            if( isValidPipeline )
            {
                // End
                m_pEvents.push_back( new TraceEvent(
                    TraceEventPhase::eDurationEnd,
                    m_pStringSerializer->GetName( data ),
                    "Performance",
                    GetNormalizedGpuTimestamp( data.m_EndTimestamp ),
                    m_CommandQueue ) );
            }
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
        if( data.m_BeginTimestamp != 0 )
        {
            if( data.m_Type != DeviceProfilerDrawcallType::eDebugLabel )
            {
                m_pEvents.push_back( new TraceCompleteEvent(
                    m_pStringSerializer->GetName( data ),
                    "Performance",
                    GetNormalizedGpuTimestamp( data.m_BeginTimestamp ),
                    GetDuration( data ),
                    m_CommandQueue ) );
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

        const auto* tm = std::localtime( &currentTimePointInTimeT );

        // Get milliseconds
        const auto ms = time_point_cast<milliseconds>(currentTimePoint).time_since_epoch() % 1000;

        // Construct output file name
        std::stringstream stringBuilder;
        stringBuilder << "ProcessName_PID_";
        stringBuilder << std::put_time( tm, "%Y-%m-%d_%H-%M-%S" ) << "_" << ms.count();
        stringBuilder << ".json";

        return stringBuilder.str();
    }

    /*************************************************************************\

    Function:
        SaveEventsToFile

    \*************************************************************************/
    void DeviceProfilerTraceSerializer::SaveEventsToFile()
    {
        json traceJson = {
            { "traceEvents", json::array() },
            { "displayTimeUnit", "ms" },
            { "otherData", json::object() } };

        // Create JSON objects
        json& traceEvents = traceJson[ "traceEvents" ];

        for( const TraceEvent* pEvent : m_pEvents )
        {
            traceEvents.push_back( *pEvent );
        }

        // Write JSON to file
        std::ofstream( ConstructTraceFileName() ) << traceJson;
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

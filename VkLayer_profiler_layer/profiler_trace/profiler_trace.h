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
#include "profiler_helpers/profiler_time_helpers.h"
#include "profiler_helpers/profiler_json_builder.h"
#include <vulkan/vulkan.h>
#include <atomic>
#include <list>
#include <vector>
#include <string>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>

namespace Profiler
{
    struct TraceEvent;
    struct TraceHostEvent;
    struct TraceDeviceEvent;

    /*************************************************************************\

    Structure:
        DeviceProfilerTraceSerializationResult

    Description:
        Provides feedback to the callee.

    \*************************************************************************/
    struct DeviceProfilerTraceSerializationResult
    {
        bool m_Succeeded;
        std::string m_Message;
    };

    /*************************************************************************\

    Class:
        DeviceProfilerTraceSerializer

    Description:
        Serializes data collected by the profiler into Chrome-compatible JSON
        format (Trace Event Format).

        Serializer is not thread-safe. For multithreaded serialization, use
        more serializers for the best performance.

    See:
        https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU

    \*************************************************************************/
    class DeviceProfilerTraceSerializer
    {
    public:
        DeviceProfilerTraceSerializer( DeviceProfilerFrontend& frontend );
        ~DeviceProfilerTraceSerializer();

        bool OpenOutputFile( const std::string& fileName );
        bool CloseOutputFile();

        bool Serialize( const struct DeviceProfilerFrameData& data );
        bool Serialize( const std::string& fileName, const struct DeviceProfilerFrameData& data );

        void ClearErrorMessages();
        const std::list<std::string>& GetErrorMessages() const;

        static std::string GetDefaultTraceFileName( int samplingMode );

    private:
        DeviceProfilerFrontend& m_Frontend;

        class DeviceProfilerStringSerializer* m_pStringSerializer;
        class DeviceProfilerJsonSerializer* m_pJsonSerializer;

        std::list<std::string> m_ErrorMessages;

        // Output file
        std::ofstream m_OutputFile;
        bool m_OutputFileEmpty;

        // Currently serialized frame data
        const struct DeviceProfilerFrameData* m_pData;

        // Target command queue for the current batch
        VkQueueHandle  m_CommandQueue;
        std::string    m_CommandQueueName;
        std::vector<uint64_t> m_CommandQueueEventTracks;

        std::unordered_set<uint32_t> m_SortedHostThreads;
        std::unordered_set<uint32_t> m_SortedDeviceThreads;

        // Serialized events
        DeviceProfilerJsonBuilder m_JsonBuilder;

        // Debug labels can cross command buffer and frame boundaries
        // Tracking depth of the stack to detect labels which begin in one frame and end in the next
        uint32_t     m_DebugLabelStackDepth;

        // Timestamp normalization
        VkTimeDomainEXT m_HostTimeDomain;
        uint64_t     m_HostCalibratedTimestamp;
        uint64_t     m_DeviceCalibratedTimestamp;
        uint64_t     m_HostTimestampFrequency;
        Milliseconds m_GpuTimestampPeriod;

        void SetupTimestampNormalizationConstants();
        Milliseconds GetNormalizedCpuTimestamp( uint64_t ) const;
        Milliseconds GetNormalizedGpuTimestamp( uint64_t ) const;

        template<typename DataStructType>
        inline Milliseconds GetDuration( const DataStructType& data ) const
        {
            return (data.m_EndTimestamp.m_Value - data.m_BeginTimestamp.m_Value) * m_GpuTimestampPeriod;
        }

        // Track management
        uint64_t AssignTrackForEvent( uint64_t beginTimestamp, uint64_t endTimestamp );

        template<typename DataStructType>
        inline uint64_t AssignTrackForEvent( const DataStructType& data )
        {
            return AssignTrackForEvent( data.GetBeginTimestamp().m_Value, data.GetEndTimestamp().m_Value );
        }

        // Serialization
        void Serialize( const struct DeviceProfilerCommandBufferData& );
        void Serialize( const struct DeviceProfilerRenderPassData& );
        void Serialize( const struct DeviceProfilerSubpassData&, bool );
        void Serialize( const struct DeviceProfilerPipelineData& );
        void Serialize( const struct DeviceProfilerDrawcall& );
        void Serialize( const std::vector<struct TipRange>& );

        void AppendEvent( const TraceEvent& event );
        void AppendEvent( const TraceHostEvent& event );
        void AppendEvent( const TraceDeviceEvent& event );
        bool AppendEventsToOutputFile();
    };

    /*************************************************************************\

    Class:
        ProfilerTraceOutput

    Description:
        Reads data from the profiler and writes it to a file.

    \*************************************************************************/
    class ProfilerTraceOutput : public DeviceProfilerOutput
    {
    public:
        ProfilerTraceOutput( DeviceProfilerFrontend& frontend );
        ~ProfilerTraceOutput();

        bool Initialize() override;
        void Destroy() override;

        bool IsAvailable() override;

        void Update() override;
        void Present() override;

        void SetMaxFrameCount( uint32_t maxFrameCount );
        void SetSkipFrameCount( uint32_t skipFrameCount );

    private:
        DeviceProfilerStringSerializer* m_pStringSerializer;
        DeviceProfilerTraceSerializer* m_pTraceSerializer;
        std::mutex m_TraceSerializerMutex;

        uint32_t m_MaxFrameCount;
        uint32_t m_SkipFrameCount;
        std::atomic_uint32_t m_ProcessedFrameCount;

        std::thread m_TraceSerializationThread;
        std::condition_variable m_TraceSerializationThreadInputAvailable;
        bool m_TraceSerializationThreadRunning;
        bool m_TraceSerializationThreadQuitSignal;

        std::mutex m_FrameDataQueueMutex;
        std::queue<std::shared_ptr<struct DeviceProfilerFrameData>> m_FrameDataQueue;

        void ResetMembers();

        void TraceSerializationThreadProc();
        void StopTraceSerializationThread();

        void HandleErrorMessages();
    };
}

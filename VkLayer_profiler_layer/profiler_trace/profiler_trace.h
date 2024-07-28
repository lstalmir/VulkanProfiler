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
#include "profiler_helpers/profiler_time_helpers.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <string>

namespace Profiler
{
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
        template<typename GpuDurationType>
        inline DeviceProfilerTraceSerializer( const class DeviceProfilerStringSerializer* pStringSerializer, GpuDurationType gpuTimestampPeriod )
            : DeviceProfilerTraceSerializer(
                pStringSerializer,
                std::chrono::duration_cast<Milliseconds>(gpuTimestampPeriod) )
        {
        }

        DeviceProfilerTraceSerializer( const class DeviceProfilerStringSerializer* pStringSerializer, Milliseconds gpuTimestampPeriod );
        ~DeviceProfilerTraceSerializer();

        DeviceProfilerTraceSerializationResult Serialize( const std::string& fileName, const struct DeviceProfilerFrameData& data );

        static std::string GetDefaultTraceFileName( int samplingMode );

    private:
        const class DeviceProfilerStringSerializer* m_pStringSerializer;
        const class DeviceProfilerJsonSerializer* m_pJsonSerializer;

        // Currently serialized frame data
        const struct DeviceProfilerFrameData* m_pData;

        // Target command queue for the current batch
        VkQueue      m_CommandQueue;

        std::vector<struct TraceEvent*> m_pEvents;

        // Debug labels can cross command buffer and frame boundaries
        // Tracking depth of the stack to detect labels which begin in one frame and end in the next
        uint32_t     m_DebugLabelStackDepth;

        // Timestamp normalization
        Milliseconds m_CpuQueueSubmitTimestampOffset;
        uint64_t     m_GpuQueueSubmitTimestampOffset;
        Milliseconds m_GpuTimestampPeriod;

        void SetupTimestampNormalizationConstants( VkQueue );
        Milliseconds GetNormalizedCpuTimestamp( std::chrono::high_resolution_clock::time_point ) const;
        Milliseconds GetNormalizedGpuTimestamp( uint64_t ) const;

        template<typename DataStructType>
        inline Milliseconds GetDuration( const DataStructType& data ) const
        {
            return (data.m_EndTimestamp.m_Value - data.m_BeginTimestamp.m_Value) * m_GpuTimestampPeriod;
        }

        // Serialization
        void Serialize( const struct DeviceProfilerCommandBufferData& );
        void Serialize( const struct DeviceProfilerRenderPassData& );
        void Serialize( const struct DeviceProfilerSubpassData&, bool );
        void Serialize( const struct DeviceProfilerPipelineData& );
        void Serialize( const struct DeviceProfilerDrawcall& );

        void SaveEventsToFile( const std::string&, DeviceProfilerTraceSerializationResult& );

        void Cleanup();
    };
}

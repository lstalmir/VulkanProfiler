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

#pragma once
#include "profiler_helpers/profiler_time_helpers.h"
#include <vulkan/vulkan.h>
#include <filesystem>

namespace Profiler
{
    class DeviceProfilerStringSerializer;
    struct DeviceProfilerFrameData;
    struct DeviceProfilerSubmitBatchData;
    struct DeviceProfilerCommandBufferData;
    struct DeviceProfilerRenderPassData;
    struct DeviceProfilerSubpassData;
    struct DeviceProfilerPipelineData;
    struct DeviceProfilerDrawcall;

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
        inline DeviceProfilerTraceSerializer(
            const DeviceProfilerStringSerializer* pStringSerializer,
            GpuDurationType gpuTimestampPeriod )
            : m_pStringSerializer( pStringSerializer )
            , m_GpuTimestampPeriod( gpuTimestampPeriod )
        {
        }

        void Serialize( const DeviceProfilerFrameData& data );

    private:
        const DeviceProfilerStringSerializer* m_pStringSerializer;

        // Currently serialized frame data
        const DeviceProfilerFrameData* m_pData;

        // Target command queue for the current batch
        VkQueue m_CommandQueue;

        std::vector<struct TraceEvent*> m_pEvents;

        // Timestamp normalization
        Milliseconds m_CpuQueueSubmitTimestampOffset;
        uint64_t     m_GpuQueueSubmitTimestampOffset;
        Milliseconds m_GpuTimestampPeriod;

        void SetupTimestampNormalizationConstants( const DeviceProfilerSubmitBatchData& );
        Milliseconds GetNormalizedCpuTimestamp( std::chrono::high_resolution_clock::time_point ) const;
        Milliseconds GetNormalizedGpuTimestamp( uint64_t ) const;

        template<typename DataStructType>
        inline Milliseconds GetDuration( const DataStructType& data ) const
        {
            return (data.m_EndTimestamp - data.m_BeginTimestamp) * m_GpuTimestampPeriod;
        }

        // Serialization
        void Serialize( const DeviceProfilerCommandBufferData& );
        void Serialize( const DeviceProfilerRenderPassData& );
        void Serialize( const DeviceProfilerSubpassData&, bool );
        void Serialize( const DeviceProfilerPipelineData& );
        void Serialize( const DeviceProfilerDrawcall& );

        std::filesystem::path ConstructTraceFileName() const;
        void SaveEventsToFile();

        void Cleanup();
    };
}

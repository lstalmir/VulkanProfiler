// Copyright (c) 2022-2024 Lukasz Stalmirski
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
#include "profiler_memory_manager.h"
#include "profiler_helpers.h"

#include <vulkan/vk_layer.h>

#include <map>

namespace Profiler
{
    class DeviceProfiler;

    struct DeviceProfilerQueryDataContext
    {
        uint32_t m_TimestampDataOffset = 0;
        uint32_t m_TimestampDataSize = 0;
        uint32_t m_PerformanceDataSize = 0;
        uint32_t m_PerformanceDataMetricsSetIndex = 0;
        VkQueryPool m_PerformanceQueryPool = VK_NULL_HANDLE;
    };

    /***********************************************************************************\

    Class:
        DeviceProfilerQueryDataBuffer

    Description:
        An allocation that stores results of timestamp and performance queries.

    \***********************************************************************************/
    class DeviceProfilerQueryDataBuffer
    {
    public:
        DeviceProfilerQueryDataBuffer( DeviceProfiler& profiler, uint64_t size );
        ~DeviceProfilerQueryDataBuffer();

        DeviceProfilerQueryDataBuffer( const DeviceProfilerQueryDataBuffer& ) = delete;
        DeviceProfilerQueryDataBuffer& operator=( const DeviceProfilerQueryDataBuffer& ) = delete;

        DeviceProfilerQueryDataBuffer( DeviceProfilerQueryDataBuffer&& ) = delete;
        DeviceProfilerQueryDataBuffer& operator=( DeviceProfilerQueryDataBuffer&& ) = delete;

        void FallbackToCpuAllocation();

        bool UsesGpuAllocation() const;
        VkBuffer GetGpuBuffer() const;
        uint8_t* GetCpuBuffer() const;
        const uint8_t* GetMappedData() const;

        DeviceProfilerQueryDataContext* CreateContext( const void* handle );
        const DeviceProfilerQueryDataContext* GetContext( const void* handle ) const;

    private:
        DeviceProfiler&   m_Profiler;
        VkBuffer          m_Buffer;
        VmaAllocation     m_Allocation;
        VmaAllocationInfo m_AllocationInfo;
        void*             m_pCpuAllocation;
        std::map<const void*, DeviceProfilerQueryDataContext> m_Contexts;
    };

    /***********************************************************************************\

    Class:
        DeviceProfilerQueryDataBufferWriter

    Description:
        Helper class that can be used to store data into the buffer.

    \***********************************************************************************/
    class DeviceProfilerQueryDataBufferWriter
    {
    public:
        explicit DeviceProfilerQueryDataBufferWriter(
            DeviceProfiler& profiler,
            DeviceProfilerQueryDataBuffer& dataBuffer,
            VkCommandBuffer copyCommandBuffer = VK_NULL_HANDLE );

        void SetContext( const void* handle );
        void WriteTimestampQueryResults( VkQueryPool queryPool, uint32_t queryCount );
        void WritePerformanceQueryResults( VkQueryPool queryPool, uint32_t metricsSetIndex, uint32_t queueFamilyIndex );

    private:
        DeviceProfiler*                 m_pProfiler;
        DeviceProfilerQueryDataBuffer*  m_pData;
        DeviceProfilerQueryDataContext* m_pContext;
        VkCommandBuffer                 m_CommandBuffer;
        union {
            VkBuffer                    m_GpuBuffer;
            uint8_t*                    m_CpuBuffer;
        };
        uint32_t                        m_DataOffset;
    };

    /***********************************************************************************\

    Class:
        DeviceProfilerQueryDataBufferWriter

    Description:
        Helper class that can be used to read the data from the buffer.

    \***********************************************************************************/
    class DeviceProfilerQueryDataBufferReader
    {
    public:
        explicit DeviceProfilerQueryDataBufferReader(
            const DeviceProfiler& profiler,
            const DeviceProfilerQueryDataBuffer& dataBuffer );

        void SetContext( const void* handle );
        uint64_t ReadTimestampQueryResult( uint64_t index ) const;
        uint32_t GetPerformanceQueryMetricsSetIndex() const;
        uint32_t GetPerformanceQueryResultSize() const;
        const uint8_t* ReadPerformanceQueryResult();
        bool HasPerformanceQueryResult() const;

    private:
        const DeviceProfiler*                 m_pProfiler;
        const DeviceProfilerQueryDataBuffer*  m_pData;
        const DeviceProfilerQueryDataContext* m_pContext;
        const uint8_t*                        m_pMappedData;
        const uint64_t*                       m_pMappedTimestampQueryData;
        std::vector<uint8_t>                  m_PerformanceQueryData;
    };
}

// Copyright (c) 2025 Lukasz Stalmirski
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
#include "profiler_data.h"
#include <vulkan/vulkan.h>
#include <vector>
// Import extension structures
#include "profiler_ext/VkProfilerEXT.h"

namespace Profiler
{
    struct VkDevice_Object;
    struct DeviceProfilerConfig;

    /***********************************************************************************\

    Class:
        DeviceProfilerPerformanceCountersSamplingMode

    Description:
        Defines the sampling modes available for performance counters.

    \***********************************************************************************/
    enum class DeviceProfilerPerformanceCountersSamplingMode
    {
        eQuery,
        eStream,
    };

    /***********************************************************************************\

    Class:
        DeviceProfilerPerformanceCounters

    Description:
        Common interface for all supported performance counters providers.
        Default implementation provides no functionality.

    \***********************************************************************************/
    class DeviceProfilerPerformanceCounters
    {
    public:
        virtual ~DeviceProfilerPerformanceCounters() {}

        virtual VkResult Initialize( VkDevice_Object* pDevice, const DeviceProfilerConfig& config ) { return VK_ERROR_FEATURE_NOT_PRESENT; }
        virtual void Destroy() {}

        virtual VkResult SetQueuePerformanceConfiguration( VkQueue queue ) { return VK_SUCCESS; }

        virtual DeviceProfilerPerformanceCountersSamplingMode GetSamplingMode() const { return DeviceProfilerPerformanceCountersSamplingMode::eQuery; }

        virtual uint32_t GetReportSize( uint32_t metricsSetIndex, uint32_t queueFamilyIndex ) const { return 0; }
        virtual uint32_t GetMetricsCount( uint32_t metricsSetIndex ) const { return 0; }
        virtual uint32_t GetMetricsSetCount() const { return 0; }
        virtual VkResult SetActiveMetricsSet( uint32_t metricsSetIndex ) { return VK_ERROR_FEATURE_NOT_PRESENT; }
        virtual uint32_t GetActiveMetricsSetIndex() const { return 0; }
        virtual bool AreMetricsSetsCompatible( uint32_t metricsSet1, uint32_t metricsSet2 ) const { return false; }
        virtual uint32_t GetRequiredPasses( uint32_t counterCount, const uint32_t* pCounterIndices ) const { return 0; }
        virtual uint32_t GetMetricsSets( uint32_t count, VkProfilerPerformanceMetricsSetProperties2EXT* pProperties ) const { return 0; }
        virtual void GetMetricsSetProperties( uint32_t metricsSetIndex, VkProfilerPerformanceMetricsSetProperties2EXT* pProperties ) const {}
        virtual uint32_t GetMetricsSetMetricsProperties( uint32_t metricsSetIndex, uint32_t count, VkProfilerPerformanceCounterProperties2EXT* pProperties ) const { return 0; }
        virtual uint32_t GetMetricsProperties( uint32_t count, VkProfilerPerformanceCounterProperties2EXT* pProperties ) const { return 0; }
        virtual void GetAvailableMetrics( uint32_t selectedCountersCount, const uint32_t* pSelectedCounters, uint32_t& availableCountersCount, uint32_t* pAvailableCounters ) const { availableCountersCount = 0; }

        virtual bool SupportsQueryPoolReuse() const { return false; }
        virtual VkResult CreateQueryPool( uint32_t queueFamilyIndex, uint32_t size, VkQueryPool* pQueryPool ) { return VK_ERROR_FEATURE_NOT_PRESENT; }

        virtual bool SupportsCustomMetricsSets() const { return false; }
        virtual uint32_t CreateCustomMetricsSet( const VkProfilerCustomPerformanceMetricsSetCreateInfoEXT* pCreateInfo ) { return UINT32_MAX; }
        virtual void DestroyCustomMetricsSet( uint32_t ) {}
        virtual void UpdateCustomMetricsSets( uint32_t updateCount, const VkProfilerCustomPerformanceMetricsSetUpdateInfoEXT* pUpdateInfos ) {}

        virtual uint32_t InsertCommandBufferStreamMarker( VkCommandBuffer commandBuffer ) { return 0; }
        virtual void ReadStreamData( uint64_t beginTimestamp, uint64_t endTimestamp, std::vector<DeviceProfilerPerformanceCountersStreamResult>& results ) { results.clear(); }
        virtual void ReadStreamSynchronizationTimestamps( uint64_t* pGpuTimestamp, uint64_t* pCpuTimestamp ) {}

        virtual void ParseReport( uint32_t metricsSetIndex, uint32_t queueFamilyIndex, uint32_t reportSize, const uint8_t* pReport, std::vector<VkProfilerPerformanceCounterResultEXT>& results ) {}
    };
}

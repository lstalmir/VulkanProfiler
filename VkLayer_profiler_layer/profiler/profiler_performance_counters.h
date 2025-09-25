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
#include <vulkan/vulkan.h>
#include <vector>
#include <shared_mutex>
// Import extension structures
#include "profiler_ext/VkProfilerEXT.h"

namespace Profiler
{
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

        virtual VkResult Initialize( struct VkDevice_Object* ) { return VK_ERROR_FEATURE_NOT_PRESENT; }
        virtual void Destroy() {}

        virtual uint32_t GetReportSize( uint32_t ) const { return 0; }
        virtual uint32_t GetMetricsCount( uint32_t ) const { return 0; }
        virtual uint32_t GetMetricsSetCount() const { return 0; }
        virtual VkResult SetActiveMetricsSet( uint32_t ) { return VK_ERROR_FEATURE_NOT_PRESENT; }
        virtual uint32_t GetActiveMetricsSetIndex() const { return 0; }
        virtual bool AreMetricsSetsCompatible( uint32_t, uint32_t ) const { return false; }

        virtual void GetMetricsSets( std::vector<VkProfilerPerformanceMetricsSetPropertiesEXT>& ) const {}
        virtual void GetMetricsProperties( uint32_t, std::vector<VkProfilerPerformanceCounterPropertiesEXT>& ) const {}
        virtual void GetQueueFamilyMetricsProperties( uint32_t, std::vector<VkProfilerPerformanceCounterPropertiesEXT>& ) const {}
        virtual void GetAvailableMetrics( uint32_t, const std::vector<uint32_t>&, std::vector<uint32_t>& ) const {}

        virtual bool SupportsQueryPoolReuse() const { return false; }
        virtual VkResult CreateQueryPool( uint32_t, uint32_t, VkQueryPool* ) { return VK_ERROR_FEATURE_NOT_PRESENT; }

        virtual bool SupportsCustomMetricsSets() const { return false; }
        virtual uint32_t CreateCustomMetricsSet( uint32_t, const std::string&, const std::string&, const std::vector<uint32_t>& ) { return UINT32_MAX; }
        virtual void DestroyCustomMetricsSet( uint32_t ) {}

        virtual void ParseReport( uint32_t, uint32_t, const uint8_t*, std::vector<VkProfilerPerformanceCounterResultEXT>& ) {}
    };

    /***********************************************************************************\

    Class:
        DeviceProfilerPerformanceCountersKHR

    Description:
        Implementation of performance counters using VK_KHR_performance_query extension.

    \***********************************************************************************/
    class DeviceProfilerPerformanceCountersKHR
        : public DeviceProfilerPerformanceCounters
    {
    public:
        DeviceProfilerPerformanceCountersKHR();

        VkResult Initialize( struct VkDevice_Object* pDevice ) final;
        void Destroy() final;

        uint32_t GetReportSize( uint32_t metricsSetIndex ) const final;
        uint32_t GetMetricsCount( uint32_t metricsSetIndex ) const final;
        uint32_t GetMetricsSetCount() const final;
        VkResult SetActiveMetricsSet( uint32_t metricsSetIndex ) final;
        uint32_t GetActiveMetricsSetIndex() const final;

        void GetMetricsSets( std::vector<VkProfilerPerformanceMetricsSetPropertiesEXT>& metricsSets ) const final;
        void GetMetricsProperties( uint32_t metricsSetIndex, std::vector<VkProfilerPerformanceCounterPropertiesEXT>& metrics ) const final;
        void GetQueueFamilyMetricsProperties( uint32_t queueFamilyIndex, std::vector<VkProfilerPerformanceCounterPropertiesEXT>& metrics ) const final;
        void GetAvailableMetrics( uint32_t queueFamilyIndex, const std::vector<uint32_t>& allocatedCounters, std::vector<uint32_t>& availableCounters ) const final;
        bool AreMetricsSetsCompatible( uint32_t firstMetricsSetIndex, uint32_t secondMetricsSetIndex ) const final;

        VkResult CreateQueryPool( uint32_t queueFamilyIndex, uint32_t size, VkQueryPool* pQueryPool ) final;

        bool SupportsCustomMetricsSets() const final { return true; }
        uint32_t CreateCustomMetricsSet( uint32_t queueFamilyIndex, const std::string& name, const std::string& description, const std::vector<uint32_t>& counterIndices ) final;
        void DestroyCustomMetricsSet( uint32_t metricsSetIndex ) final;

        void ParseReport(
            uint32_t metricsSetIndex,
            uint32_t reportSize,
            const uint8_t* pReport,
            std::vector<VkProfilerPerformanceCounterResultEXT>& results ) final;

    private:
        struct VkDevice_Object* m_pDevice;

        bool m_DeviceProfilingLockAcquired;

        std::shared_mutex mutable m_ActiveMetricsSetMutex;
        uint32_t m_ActiveMetricsSetIndex;

        struct MetricsSet
        {
            std::string m_Name;
            std::string m_Description;
            std::vector<uint32_t> m_Counters;
            uint32_t m_QueueFamilyIndex;
            uint32_t m_CompatibleHash; // m_QueueFamilyIndex & m_Counters
            uint32_t m_FullHash;       // All fields
        };

        struct QueueFamilyCounters
        {
            std::vector<VkPerformanceCounterKHR> m_Counters;
            std::vector<VkPerformanceCounterDescriptionKHR> m_CounterDescriptions;
        };

        std::vector<QueueFamilyCounters> m_QueueFamilyCounters;

        std::shared_mutex mutable m_MetricsSetsMutex;
        std::vector<MetricsSet> m_MetricsSets;

        VkQueryPoolPerformanceCreateInfoKHR GetQueryPoolPerformanceCreateInfo( uint32_t queueFamilyIndex ) const;

        uint32_t RegisterMetricsSet( MetricsSet&& metricsSet );
    };
}

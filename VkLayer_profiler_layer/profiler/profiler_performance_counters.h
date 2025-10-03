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
#include <set>
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

        virtual uint32_t GetReportSize( uint32_t, uint32_t ) const { return 0; }
        virtual uint32_t GetMetricsCount( uint32_t ) const { return 0; }
        virtual uint32_t GetMetricsSetCount() const { return 0; }
        virtual VkResult SetActiveMetricsSet( uint32_t ) { return VK_ERROR_FEATURE_NOT_PRESENT; }
        virtual uint32_t GetActiveMetricsSetIndex() const { return 0; }
        virtual bool AreMetricsSetsCompatible( uint32_t, uint32_t ) const { return false; }

        virtual void GetMetricsSets( std::vector<VkProfilerPerformanceMetricsSetProperties2EXT>& ) const {}
        virtual void GetMetricsSetProperties( uint32_t, VkProfilerPerformanceMetricsSetProperties2EXT& ) const {}
        virtual void GetMetricsSetMetricsProperties( uint32_t, std::vector<VkProfilerPerformanceCounterProperties2EXT>& ) const {}
        virtual void GetMetricsProperties( std::vector<VkProfilerPerformanceCounterProperties2EXT>& ) const {}
        virtual void GetAvailableMetrics( const std::vector<uint32_t>&, std::vector<uint32_t>& ) const {}

        virtual bool SupportsQueryPoolReuse() const { return false; }
        virtual VkResult CreateQueryPool( uint32_t, uint32_t, VkQueryPool* ) { return VK_ERROR_FEATURE_NOT_PRESENT; }

        virtual bool SupportsCustomMetricsSets() const { return false; }
        virtual uint32_t CreateCustomMetricsSet( const std::string&, const std::string&, const std::vector<uint32_t>& ) { return UINT32_MAX; }
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

        uint32_t GetReportSize( uint32_t metricsSetIndex, uint32_t queueFamilyIndex ) const final;
        uint32_t GetMetricsCount( uint32_t metricsSetIndex ) const final;
        uint32_t GetMetricsSetCount() const final;
        VkResult SetActiveMetricsSet( uint32_t metricsSetIndex ) final;
        uint32_t GetActiveMetricsSetIndex() const final;

        void GetMetricsSets( std::vector<VkProfilerPerformanceMetricsSetProperties2EXT>& metricsSets ) const final;
        void GetMetricsSetProperties( uint32_t metricsSetIndex, VkProfilerPerformanceMetricsSetProperties2EXT& properties ) const final;
        void GetMetricsSetMetricsProperties( uint32_t metricsSetIndex, std::vector<VkProfilerPerformanceCounterProperties2EXT>& metrics ) const final;
        void GetMetricsProperties( std::vector<VkProfilerPerformanceCounterProperties2EXT>& metrics ) const final;
        void GetAvailableMetrics( const std::vector<uint32_t>& allocatedCounters, std::vector<uint32_t>& availableCounters ) const final;
        bool AreMetricsSetsCompatible( uint32_t firstMetricsSetIndex, uint32_t secondMetricsSetIndex ) const final;

        VkResult CreateQueryPool( uint32_t queueFamilyIndex, uint32_t size, VkQueryPool* pQueryPool ) final;

        bool SupportsCustomMetricsSets() const final { return true; }
        uint32_t CreateCustomMetricsSet( const std::string& name, const std::string& description, const std::vector<uint32_t>& counterIndices ) final;
        void DestroyCustomMetricsSet( uint32_t metricsSetIndex ) final;

        void ParseReport(
            uint32_t metricsSetIndex,
            uint32_t queueFamilyIndex,
            const uint8_t* pReport,
            std::vector<VkProfilerPerformanceCounterResultEXT>& results ) final;

    private:
        struct VkDevice_Object* m_pDevice;

        bool m_DeviceProfilingLockAcquired;
        uint32_t m_QueueFamilyCount;
        std::set<uint32_t> m_UsedQueueFamilies;

        std::shared_mutex mutable m_ActiveMetricsSetMutex;
        uint32_t m_ActiveMetricsSetIndex;

        struct MetricsSetQueueFamilyCounters
        {
            std::vector<uint32_t> m_CounterIndices;
            std::vector<uint32_t> m_ReverseMapping;
        };

        struct MetricsSet
        {
            std::string m_Name;
            std::string m_Description;
            std::vector<uint32_t> m_CounterIndices;
            std::vector<MetricsSetQueueFamilyCounters> m_QueueFamilyCounters;
            uint32_t m_CompatibleHash; // Counters
            uint32_t m_FullHash;       // All fields
        };

        struct Counter
        {
            std::string m_Name;
            std::string m_Category;
            std::string m_Description;
            std::vector<uint32_t> m_QueueFamilyCounterIndices;
            VkProfilerPerformanceCounterFlagsEXT m_Flags;
            VkProfilerPerformanceCounterUnitEXT m_Unit;
            VkProfilerPerformanceCounterStorageEXT m_Storage;
            uint8_t m_UUID[VK_UUID_SIZE];
        };

        std::vector<Counter> m_Counters;

        std::shared_mutex mutable m_MetricsSetsMutex;
        std::vector<MetricsSet> m_MetricsSets;

        static void FillPerformanceMetricsSetProperties( const MetricsSet& set, VkProfilerPerformanceMetricsSetProperties2EXT& properties );
        static void FillPerformanceCounterProperties( const Counter& counter, VkProfilerPerformanceCounterProperties2EXT& properties );

        uint32_t FindMetricsSetByHash( uint32_t fullHash ) const;
        uint32_t RegisterMetricsSet( MetricsSet&& metricsSet );
        void RegisterCounter( uint32_t queueFamilyIndex, uint32_t counterIndexInFamily, const VkPerformanceCounterKHR& counter, const VkPerformanceCounterDescriptionKHR& description );
    };
}

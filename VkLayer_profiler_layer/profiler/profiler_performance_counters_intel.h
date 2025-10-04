// Copyright (c) 2019-2025 Lukasz Stalmirski
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
#include "profiler_performance_counters.h"
#include <metrics_discovery_api.h>
#ifdef WIN32
#include <filesystem>
#endif
#include <vector>
#include <string>
#include <shared_mutex>

namespace Profiler
{
    /***********************************************************************************\

    Class:
        DeviceProfilerPerformanceCountersINTEL

    Description:
        Wrapper for metrics exposed by INTEL GPUs.

    \***********************************************************************************/
    class DeviceProfilerPerformanceCountersINTEL
        : public DeviceProfilerPerformanceCounters
    {
    public:
        DeviceProfilerPerformanceCountersINTEL();

        VkResult Initialize( struct VkDevice_Object* pDevice ) final;
        void Destroy() final;

        uint32_t GetReportSize( uint32_t metricsSetIndex, uint32_t queueFamilyIndex ) const final;
        uint32_t GetMetricsCount( uint32_t metricsSetIndex ) const final;
        uint32_t GetMetricsSetCount() const final;
        VkResult SetActiveMetricsSet( uint32_t metricsSetIndex ) final;
        uint32_t GetActiveMetricsSetIndex() const final;

        uint32_t GetMetricsSets( uint32_t count, VkProfilerPerformanceMetricsSetProperties2EXT* pProperties ) const final;
        void GetMetricsSetProperties( uint32_t metricsSetIndex, VkProfilerPerformanceMetricsSetProperties2EXT* pProperties ) const final;
        uint32_t GetMetricsSetMetricsProperties( uint32_t metricsSetIndex, uint32_t count, VkProfilerPerformanceCounterProperties2EXT* pProperties ) const final;

        bool SupportsQueryPoolReuse() const final { return true; }
        VkResult CreateQueryPool( uint32_t queueFamilyIndex, uint32_t size, VkQueryPool* pQueryPool ) final;

        void ParseReport(
            uint32_t metricsSetIndex,
            uint32_t queueFamilyIndex,
            uint32_t reportSize,
            const uint8_t* pReport,
            std::vector<VkProfilerPerformanceCounterResultEXT>& results ) final;

    private:
        // Require at least version 1.1.
        static const uint32_t m_RequiredVersionMajor = 1;
        static const uint32_t m_MinRequiredVersionMinor = 1;

        struct Counter
        {
            uint32_t m_MetricIndex;

            MetricsDiscovery::IMetric_1_0* m_pMetric;
            MetricsDiscovery::TMetricParams_1_0* m_pMetricParams;

            VkProfilerPerformanceCounterUnitEXT m_Unit;
            VkProfilerPerformanceCounterStorageEXT m_Storage;

            // Some metrics are reported in premultiplied units, e.g., MHz.
            // This vector contains factors applied to each metric in output reports.
            double m_ResultFactor;

            // Metrics discovery API does not provide UUIDs for metrics.
            uint8_t m_UUID[VK_UUID_SIZE];
        };

        struct MetricsSet
        {
            MetricsDiscovery::IMetricSet_1_1* m_pMetricSet;
            MetricsDiscovery::TMetricSetParams_1_0* m_pMetricSetParams;

            std::vector<Counter> m_Counters;
        };

        #ifdef WIN32
        HMODULE m_hMDDll;
        // Since there is no official support for Windows, we have to open the library manually
        std::filesystem::path FindMetricsDiscoveryLibrary();
        #endif

        struct VkDevice_Object*               m_pVulkanDevice;

        MetricsDiscovery::IMetricsDevice_1_1* m_pDevice;
        MetricsDiscovery::TMetricsDeviceParams_1_0* m_pDeviceParams;

        MetricsDiscovery::IConcurrentGroup_1_1* m_pConcurrentGroup;
        MetricsDiscovery::TConcurrentGroupParams_1_0* m_pConcurrentGroupParams;

        std::vector<MetricsSet> m_MetricsSets;

        std::shared_mutex mutable             m_ActiveMetricSetMutex;
        uint32_t                              m_ActiveMetricsSetIndex;

        bool                                  m_PerformanceApiInitialized;
        VkPerformanceConfigurationINTEL       m_PerformanceApiConfiguration;

        bool LoadMetricsDiscoveryLibrary();
        void UnloadMetricsDiscoveryLibrary();

        bool OpenMetricsDevice();
        void CloseMetricsDevice();

        void ResetMembers();

        static void FillPerformanceMetricsSetProperties( const MetricsSet& metricsSet, VkProfilerPerformanceMetricsSetProperties2EXT& properties );
        static void FillPerformanceCounterProperties( const Counter& counter, VkProfilerPerformanceCounterProperties2EXT& properties );

        static bool TranslateStorage( MetricsDiscovery::EMetricResultType resultType, VkProfilerPerformanceCounterStorageEXT& storage );
        static bool TranslateUnit( const char* pUnit, double& factor, VkProfilerPerformanceCounterUnitEXT& unit );
    };
}

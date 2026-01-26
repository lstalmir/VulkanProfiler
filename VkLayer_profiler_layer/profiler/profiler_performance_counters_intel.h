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
#include "profiler_performance_counters.h"
#include "profiler_counters.h"
#include <metrics_discovery_api.h>
#include <filesystem>
#include <vector>
#include <string>
#include <shared_mutex>
#include <thread>

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

        VkResult Initialize( VkDevice_Object* pDevice, const DeviceProfilerConfig& config ) final;
        void Destroy() final;

        VkResult SetQueuePerformanceConfiguration( VkQueue queue ) final;

        VkProfilerPerformanceCountersSamplingModeEXT GetSamplingMode() const final;

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

        bool ReadStreamData( uint64_t beginTimestamp, uint64_t endTimestamp, std::vector<DeviceProfilerPerformanceCountersStreamResult>& results ) final;

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
        static const uint32_t m_MinRequiredAdapterGroupVersionMinor = 6;

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

        struct ReportInformations
        {
            uint32_t                          m_Reason;
            uint32_t                          m_Value;
            uint64_t                          m_Timestamp;
        };

        struct MetricsSet
        {
            MetricsDiscovery::IMetricSet_1_1* m_pMetricSet;
            MetricsDiscovery::TMetricSetParams_1_0* m_pMetricSetParams;

            uint32_t                          m_ReportReasonInformationIndex;
            uint32_t                          m_ValueInformationIndex;
            uint32_t                          m_TimestampInformationIndex;

            std::vector<Counter>              m_Counters;
        };

        void*                                 m_MDLibraryHandle;

        struct VkDevice_Object*               m_pVulkanDevice;

        MetricsDiscovery::IAdapterGroup_1_6*  m_pAdapterGroup;
        MetricsDiscovery::IAdapter_1_6*       m_pAdapter;
        MetricsDiscovery::IMetricsDevice_1_1* m_pDevice;
        MetricsDiscovery::TMetricsDeviceParams_1_0* m_pDeviceParams;

        MetricsDiscovery::IConcurrentGroup_1_1* m_pConcurrentGroup;
        MetricsDiscovery::TConcurrentGroupParams_1_0* m_pConcurrentGroupParams;

        VkProfilerPerformanceCountersSamplingModeEXT m_SamplingMode;
        uint32_t                              m_SamplingPeriodInNanoseconds;

        CpuTimestampCounter                   m_CpuTimestampCounter;

        double                                m_GpuTimestampQueryPeriod;
        double                                m_GpuTimestampPeriod;
        uint64_t                              m_GpuTimestampMax;
        bool                                  m_GpuTimestampIs32Bit;

        std::vector<MetricsSet>               m_MetricsSets;

        std::shared_mutex mutable             m_ActiveMetricSetMutex;
        uint32_t                              m_ActiveMetricsSetIndex;

        bool                                  m_PerformanceApiInitialized;
        VkPerformanceConfigurationINTEL       m_PerformanceApiConfiguration;

        std::thread                           m_MetricsStreamCollectionThread;
        std::mutex                            m_MetricsStreamCollectionMutex;
        bool                                  m_MetricsStreamCollectionThreadExit;

        uint32_t                              m_MetricsStreamMaxReportCount;
        uint64_t                              m_MetricsStreamMaxBufferLengthInNanoseconds;
        std::vector<char>                     m_MetricsStreamDataBuffer;

        std::mutex mutable                    m_MetricsStreamResultsMutex;
        std::vector<DeviceProfilerPerformanceCountersStreamResult> m_MetricsStreamResults;
        uint64_t                              m_MetricsStreamLastResultTimestamp;

        std::filesystem::path FindMetricsDiscoveryLibrary();

        bool LoadMetricsDiscoveryLibrary();
        void UnloadMetricsDiscoveryLibrary();

        bool OpenMetricsDevice();
        void CloseMetricsDevice();

        void MetricsStreamCollectionThreadProc();
        size_t CollectMetricsStreamSamples();
        void FreeUnusedMetricsStreamSamples();

        void ParseReport(
            uint32_t metricsSetIndex,
            uint32_t queueFamilyIndex,
            uint32_t reportSize,
            const uint8_t* pReport,
            std::vector<VkProfilerPerformanceCounterResultEXT>& results,
            ReportInformations* pReportInformations );

        uint64_t ConvertGpuTimestampToNanoseconds( uint64_t gpuTimestamp );
        uint64_t ConvertNanosecondsToGpuTimestamp( uint64_t nanoseconds );

        void ResetMembers();

        static void FillPerformanceMetricsSetProperties( const MetricsSet& metricsSet, VkProfilerPerformanceMetricsSetProperties2EXT& properties );
        static void FillPerformanceCounterProperties( const Counter& counter, VkProfilerPerformanceCounterProperties2EXT& properties );

        static bool TranslateStorage( MetricsDiscovery::EMetricResultType resultType, VkProfilerPerformanceCounterStorageEXT& storage );
        static bool TranslateUnit( const char* pUnit, double& factor, VkProfilerPerformanceCounterUnitEXT& unit );
    };
}

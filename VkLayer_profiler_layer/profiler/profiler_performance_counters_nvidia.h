// Copyright (c) 2026 Lukasz Stalmirski
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
#include "utils/lockable_unordered_map.h"

#define VK_NO_PROTOTYPES
#include <NvPerfVulkan.h>
#include <NvPerfMetricsEvaluator.h>
#include <NvPerfCounterConfiguration.h>
#include <NvPerfPeriodicSamplerGpu.h>
#include <NvPerfCounterData.h>

#include <vector>
#include <mutex>
#include <shared_mutex>
#include <thread>

namespace Profiler
{
    /***********************************************************************************\

    Class:
        DeviceProfilerPerformanceCountersNVIDIA

    Description:
        Wrapper for metrics exposed by NVIDIA NvPerf SDK.

    \***********************************************************************************/
    class DeviceProfilerPerformanceCountersNVIDIA
        : public DeviceProfilerPerformanceCounters
    {
    public:
        DeviceProfilerPerformanceCountersNVIDIA();

        VkResult Initialize( VkDevice_Object* pDevice, const DeviceProfilerConfig& config ) final;
        void Destroy() final;

        VkProfilerPerformanceCountersSamplingModeEXT GetSamplingMode() const final;

        uint32_t GetMetricsCount( uint32_t metricsSetIndex ) const final;
        uint32_t GetMetricsSetCount() const final;
        VkResult SetActiveMetricsSet( uint32_t metricsSetIndex ) final;
        uint32_t GetActiveMetricsSetIndex() const final;
        uint32_t GetRequiredPasses( uint32_t counterCount, const uint32_t* pCounterIndices ) const final;
        uint32_t GetMetricsSets( uint32_t count, VkProfilerPerformanceMetricsSetProperties2EXT* pProperties ) const final;
        void GetMetricsSetProperties( uint32_t metricsSetIndex, VkProfilerPerformanceMetricsSetProperties2EXT* pProperties ) const final;
        uint32_t GetMetricsSetMetricsProperties( uint32_t metricsSetIndex, uint32_t count, VkProfilerPerformanceCounterProperties2EXT* pProperties ) const final;
        uint32_t GetMetricsProperties( uint32_t count, VkProfilerPerformanceCounterProperties2EXT* pProperties ) const final;
        void GetAvailableMetrics( uint32_t selectedCountersCount, const uint32_t* pSelectedCounters, uint32_t& availableCountersCount, uint32_t* pAvailableCounters ) const final;

        bool SupportsCustomMetricsSets() const final;
        uint32_t CreateCustomMetricsSet( const VkProfilerCustomPerformanceMetricsSetCreateInfoEXT* pCreateInfo ) final;

        bool ReadStreamData( uint64_t beginTimestamp, uint64_t endTimestamp, std::vector<DeviceProfilerPerformanceCountersStreamResult>& results ) final;

    private:
        struct Counter
        {
            std::string             m_Name;
            std::string             m_Description;

            VkProfilerPerformanceCounterUnitEXT m_Unit;
            VkProfilerPerformanceCounterStorageEXT m_Storage;

            uint8_t                 m_UUID[VK_UUID_SIZE];

            NVPW_MetricType         m_Type;
            size_t                  m_Index;
        };

        struct MetricsSet
        {
            std::string             m_Name;
            std::string             m_Description;
            std::vector<uint32_t>   m_CounterIndices;
            uint32_t                m_CompatibleHash; // Counters
            uint32_t                m_FullHash;       // All fields
        };

        struct VkDevice_Object*     m_pVulkanDevice;

        VkProfilerPerformanceCountersSamplingModeEXT m_SamplingMode;
        uint32_t                    m_SamplingPeriodInNanoseconds;

        std::string                 m_ChipName;

        nv::perf::MetricsEvaluator  m_MetricsEvaluator;
        nv::perf::sampler::GpuPeriodicSampler m_PeriodicSampler;
        nv::perf::sampler::RingBufferCounterData m_CounterData;
        ConcurrentMap<uint32_t, nv::perf::CounterConfiguration> m_CounterConfigurations;

        std::shared_mutex mutable   m_ActiveMetricsSetMutex;
        uint32_t                    m_ActiveMetricsSetIndex;

        DeviceProfilerCustomMetricsSetManager<MetricsSet, Counter> m_MetricsSetManager;

        void ResetMembers();

        VkResult SetActiveMetricsSet_NoLock( uint32_t metricsSetIndex );
        bool PrepareCounterConfigurationForMetricsSet( const MetricsSet& metricsSet );

        static void FillPerformanceMetricsSetProperties( const MetricsSet& set, VkProfilerPerformanceMetricsSetProperties2EXT& properties );
        static void FillPerformanceCounterProperties( const Counter& counter, VkProfilerPerformanceCounterProperties2EXT& properties );
    };
}

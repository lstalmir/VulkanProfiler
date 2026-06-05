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

#define VK_NO_PROTOTYPES
#include <NvPerfVulkan.h>
#include <NvPerfMetricsEvaluator.h>
#include <NvPerfPeriodicSamplerGpu.h>
#include <NvPerfCounterData.h>
#include <NvPerfCounterConfiguration.h>

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

        VkResult SetActiveMetricsSet( uint32_t metricsSetIndex ) final;
        uint32_t GetActiveMetricsSetIndex() const final;

    private:
        struct Counter
        {
            uint32_t m_MetricIndex;

            VkProfilerPerformanceCounterUnitEXT m_Unit;
            VkProfilerPerformanceCounterStorageEXT m_Storage;

            // NvPerf SDK does not provide UUIDs for metrics.
            uint8_t m_UUID[VK_UUID_SIZE];
        };

        struct MetricsSet
        {
            nv::perf::CounterConfiguration m_CounterConfiguration;
            std::vector<Counter>    m_Counters;
        };

        struct VkDevice_Object*     m_pVulkanDevice;

        VkProfilerPerformanceCountersSamplingModeEXT m_SamplingMode;
        uint32_t                    m_SamplingPeriodInNanoseconds;

        nv::perf::MetricsEvaluator  m_MetricsEvaluator;
        nv::perf::sampler::GpuPeriodicSampler m_PeriodicSampler;
        nv::perf::sampler::RingBufferCounterData m_CounterData;

        std::vector<MetricsSet>     m_MetricsSets;

        std::shared_mutex mutable   m_ActiveMetricsSetMutex;
        uint32_t                    m_ActiveMetricsSetIndex;

        void ResetMembers();
    };
}

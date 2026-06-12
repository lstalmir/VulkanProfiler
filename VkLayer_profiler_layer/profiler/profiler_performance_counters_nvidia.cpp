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

#include "profiler_performance_counters_nvidia.h"
#include "profiler_helpers.h"
#include "profiler_config.h"

#include <profiler_layer_objects/VkDevice_object.h>

#include <NvPerfMetricsConfigBuilder.h>
#include <NvPerfMetricConfigurationsHAL.h>

#ifdef WIN32
#include <windows-desktop-x64/nvperf_host_impl.h>
#else
#include <linux-desktop-x64/nvperf_host_impl.h>
#endif

namespace Profiler
{
    /***********************************************************************************\

    Function:
        DeviceProfilerPerformanceCountersNVIDIA

    Description:
        Constructor.

    \***********************************************************************************/
    DeviceProfilerPerformanceCountersNVIDIA::DeviceProfilerPerformanceCountersNVIDIA()
    {
        ResetMembers();
    }

    /***********************************************************************************\

    Function:
        Initialize

    Description:
        Initializes NVAPI performance counters.

    \***********************************************************************************/
    VkResult DeviceProfilerPerformanceCountersNVIDIA::Initialize(
        VkDevice_Object* pDevice,
        const DeviceProfilerConfig& config )
    {
        VkResult result = VK_SUCCESS;

        m_pVulkanDevice = pDevice;

        // Configure the performance counters.
        {
            switch( config.m_PerformanceQueryMode.value )
            {
            case performance_query_mode_t::query:
                m_SamplingMode = VK_PROFILER_PERFORMANCE_COUNTERS_SAMPLING_MODE_QUERY_EXT;
                break;

            case performance_query_mode_t::stream:
                m_SamplingMode = VK_PROFILER_PERFORMANCE_COUNTERS_SAMPLING_MODE_STREAM_EXT;
                m_SamplingPeriodInNanoseconds = config.m_PerformanceStreamTimerPeriod;
                break;

            default:
                // Unsupported mode.
                result = VK_ERROR_FEATURE_NOT_PRESENT;
                break;
            }
        }

        // Get commonly used Vulkan objects and function pointers.
        const VkInstance instanceHandle = m_pVulkanDevice->pInstance->Handle;
        const VkPhysicalDevice physicalDeviceHandle = m_pVulkanDevice->pPhysicalDevice->Handle;
        const VkDevice deviceHandle = m_pVulkanDevice->Handle;
        const PFN_vkGetInstanceProcAddr pfnGetInstanceProcAddr = m_pVulkanDevice->pInstance->Callbacks.GetInstanceProcAddr;
        const PFN_vkGetDeviceProcAddr pfnGetDeviceProcAddr = m_pVulkanDevice->Callbacks.GetDeviceProcAddr;

        // Initialize NvPerf SDK.
        if( result == VK_SUCCESS )
        {
            if( !nv::perf::InitializeNvPerf() )
            {
                result = VK_ERROR_INITIALIZATION_FAILED;
            }
        }

        // Initialize Vulkan driver.
        if( result == VK_SUCCESS )
        {
            if( !nv::perf::VulkanLoadDriver(
                    instanceHandle ) )
            {
                result = VK_ERROR_INITIALIZATION_FAILED;
            }
        }

        if( result == VK_SUCCESS )
        {
            if( !nv::perf::profiler::VulkanIsGpuSupported(
                    instanceHandle,
                    physicalDeviceHandle,
                    deviceHandle,
                    pfnGetInstanceProcAddr,
                    pfnGetDeviceProcAddr ) )
            {
                result = VK_ERROR_FEATURE_NOT_PRESENT;
            }
        }

        // Get device chip name for further configuration.
        nv::perf::DeviceIdentifiers deviceIdentifiers = {};

        if( result == VK_SUCCESS )
        {
            deviceIdentifiers = nv::perf::VulkanGetDeviceIdentifiers(
                instanceHandle,
                physicalDeviceHandle,
                deviceHandle,
                pfnGetInstanceProcAddr,
                pfnGetDeviceProcAddr );

            if( !deviceIdentifiers.pChipName )
            {
                result = VK_ERROR_INITIALIZATION_FAILED;
            }
        }

        // Create GPU periodic sampler.
        if( result == VK_SUCCESS )
        {
            if( m_SamplingMode == VK_PROFILER_PERFORMANCE_COUNTERS_SAMPLING_MODE_STREAM_EXT )
            {
                uint32_t deviceIndex = nv::perf::VulkanGetNvperfDeviceIndex(
                    instanceHandle,
                    physicalDeviceHandle,
                    deviceHandle,
                    pfnGetInstanceProcAddr,
                    pfnGetDeviceProcAddr );

                if( !m_PeriodicSampler.Initialize( deviceIndex ) )
                {
                    result = VK_ERROR_INITIALIZATION_FAILED;
                }
            }
        }

        // Create metrics evaluator.
        if( result == VK_SUCCESS )
        {
            NVPW_MetricsEvaluator* pMetricsEvaluator = nullptr;
            std::vector<uint8_t> metricsEvaluatorScratchBuffer( 0 );

            switch( m_SamplingMode )
            {
            case VK_PROFILER_PERFORMANCE_COUNTERS_SAMPLING_MODE_QUERY_EXT:
            {
                metricsEvaluatorScratchBuffer.resize(
                    nv::perf::VulkanCalculateMetricsEvaluatorScratchBufferSize( deviceIdentifiers.pChipName ) );

                pMetricsEvaluator = nv::perf::VulkanCreateMetricsEvaluator(
                    metricsEvaluatorScratchBuffer.data(),
                    metricsEvaluatorScratchBuffer.size(),
                    deviceIdentifiers.pChipName );
                break;
            }

            case VK_PROFILER_PERFORMANCE_COUNTERS_SAMPLING_MODE_STREAM_EXT:
            {
                metricsEvaluatorScratchBuffer.resize(
                    nv::perf::sampler::DeviceCalculateMetricsEvaluatorScratchBufferSize( deviceIdentifiers.pChipName ) );

                pMetricsEvaluator = nv::perf::sampler::DeviceCreateMetricsEvaluator(
                    metricsEvaluatorScratchBuffer.data(),
                    metricsEvaluatorScratchBuffer.size(),
                    deviceIdentifiers.pChipName );
                break;
            }

            default:
                assert( false );
                break;
            }

            if( pMetricsEvaluator )
            {
                m_MetricsEvaluator = nv::perf::MetricsEvaluator(
                    pMetricsEvaluator,
                    std::move( metricsEvaluatorScratchBuffer ) );
            }
            else
            {
                result = VK_ERROR_INITIALIZATION_FAILED;
            }
        }

        // Load metric configuration object.
        nv::perf::MetricConfigObject metricConfigObject = {};

        if( result == VK_SUCCESS )
        {
            if( !nv::perf::MetricConfigurations::LoadMetricConfigObject(
                    metricConfigObject,
                    deviceIdentifiers.pChipName,
                    "Top_Level_Triage" ) )
            {
                result = VK_ERROR_INITIALIZATION_FAILED;
            }
        }

        // Configure the metrics evaluator.
        if( result == VK_SUCCESS )
        {
            if( result == VK_SUCCESS )
            {
                if( !m_MetricsEvaluator.UserDefinedMetrics_Initialize() )
                {
                    result = VK_ERROR_INITIALIZATION_FAILED;
                }
            }

            if( result == VK_SUCCESS )
            {
                if( !m_MetricsEvaluator.UserDefinedMetrics_Execute(
                        metricConfigObject.GenerateScriptForAllNamespacedUserMetrics() ) )
                {
                    result = VK_ERROR_INITIALIZATION_FAILED;
                }
            }

            if( result == VK_SUCCESS )
            {
                if( !m_MetricsEvaluator.UserDefinedMetrics_Commit() )
                {
                    result = VK_ERROR_INITIALIZATION_FAILED;
                }
            }
        }

        // Enumerate available counters.
        if( result == VK_SUCCESS )
        {
            for( const char* pMetricName : nv::perf::EnumerateCounters( m_MetricsEvaluator.Get() ) )
            {
                Counter counter = {};
                counter.m_Name = pMetricName;

                // Read metric type and index.
                if( !m_MetricsEvaluator.GetMetricTypeAndIndex(
                        pMetricName,
                        counter.m_Type,
                        counter.m_Index ) )
                {
                    continue;
                }

                // Read metric properties.
                if( const char* pMetricDescription = m_MetricsEvaluator.GetMetricDescription(
                        counter.m_Type,
                        counter.m_Index ) )
                {
                    counter.m_Description = pMetricDescription;
                }

                // API does not provide UUIDs for metrics
                uint32_t uuid[VK_UUID_SIZE / 4] = {};
                uuid[0] = counter.m_Type;
                uuid[1] = counter.m_Index;
                memcpy( counter.m_UUID, uuid, VK_UUID_SIZE );

                m_Counters.push_back( std::move( counter ) );
            }
        }

        if( result != VK_SUCCESS )
        {
            // Cleanup partially initialized state.
            Destroy();
        }

        return result;
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:
        Destroys NVAPI performance counters and releases any allocated resources.

    \***********************************************************************************/
    void DeviceProfilerPerformanceCountersNVIDIA::Destroy()
    {
        ResetMembers();
    }

    /***********************************************************************************\

    Function:
        GetSamplingMode

    Description:
        Returns sampling mode used by NVIDIA performance counters.

    \***********************************************************************************/
    VkProfilerPerformanceCountersSamplingModeEXT DeviceProfilerPerformanceCountersNVIDIA::GetSamplingMode() const
    {
        return m_SamplingMode;
    }

    /***********************************************************************************\

    Function:
        GetMetricsCount

    Description:
        Returns the number of metrics in the specified metrics set.

    \***********************************************************************************/
    uint32_t DeviceProfilerPerformanceCountersNVIDIA::GetMetricsCount( uint32_t metricsSetIndex ) const
    {
        if( metricsSetIndex >= m_MetricsSets.size() )
        {
            return 0;
        }

        return static_cast<uint32_t>( m_MetricsSets[metricsSetIndex].m_CounterIndices.size() );
    }

    /***********************************************************************************\

    Function:
        GetMetricsSetCount

    Description:
        Returns the number of metrics sets exposed by this extension.

    \***********************************************************************************/
    uint32_t DeviceProfilerPerformanceCountersNVIDIA::GetMetricsSetCount() const
    {
        return static_cast<uint32_t>( m_MetricsSets.size() );
    }

    /***********************************************************************************\

    Function:
        SetActiveMetricsSet

    Description:
        Sets the active metrics set.

    \***********************************************************************************/
    VkResult DeviceProfilerPerformanceCountersNVIDIA::SetActiveMetricsSet(
        uint32_t metricsSetIndex )
    {
        std::unique_lock lk( m_ActiveMetricsSetMutex );

        // Early-out if the set is already set.
        if( m_ActiveMetricsSetIndex == metricsSetIndex )
        {
            return VK_SUCCESS;
        }

        // Implementation for setting the active metrics set.

        if( m_SamplingMode == VK_PROFILER_PERFORMANCE_COUNTERS_SAMPLING_MODE_STREAM_EXT )
        {
            // Close the current stream if any.
            if( m_ActiveMetricsSetIndex != UINT32_MAX )
            {
                if( !m_PeriodicSampler.StopSampling() )
                {
                    assert( false );
                    return VK_ERROR_INITIALIZATION_FAILED;
                }

                if( !m_PeriodicSampler.EndSession() )
                {
                    assert( false );
                    return VK_ERROR_INITIALIZATION_FAILED;
                }

                m_ActiveMetricsSetIndex = UINT32_MAX;
            }

            // Configure the new session.
            size_t recordBufferSize = 0;
            const size_t maxNumUndecodedSamples = 0x20000;
            const size_t maxNumUndecodedSamplingRanges = 1;
            const auto samplingInterval = m_PeriodicSampler.GetGpuPulseSamplingInterval(
                m_SamplingPeriodInNanoseconds );

            if( !nv::perf::sampler::GpuPeriodicSamplerCalculateRecordBufferSize(
                    m_PeriodicSampler.GetDeviceIndex(), {},
                    maxNumUndecodedSamples,
                    recordBufferSize ) )
            {
                assert( false );
                return VK_ERROR_INITIALIZATION_FAILED;
            }

            // Begin the new stream.
            if( !m_PeriodicSampler.BeginSession(
                    recordBufferSize,
                    maxNumUndecodedSamplingRanges,
                    { samplingInterval.triggerSource },
                    samplingInterval.samplingInterval ) )
            {
                assert( false );
                return VK_ERROR_INITIALIZATION_FAILED;
            }

            if( !m_PeriodicSampler.StartSampling() )
            {
                assert( false );
                return VK_ERROR_INITIALIZATION_FAILED;
            }
        }

        m_ActiveMetricsSetIndex = metricsSetIndex;

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        GetActiveMetricsSetIndex

    Description:
        Returns index of the active metrics set.

    \***********************************************************************************/
    uint32_t DeviceProfilerPerformanceCountersNVIDIA::GetActiveMetricsSetIndex() const
    {
        std::shared_lock lk( m_ActiveMetricsSetMutex );
        return m_ActiveMetricsSetIndex;
    }

    /***********************************************************************************\

    Function:
        GetMetricsProperties

    Description:
        Returns properties of all available metrics.

    \***********************************************************************************/
    uint32_t DeviceProfilerPerformanceCountersNVIDIA::GetMetricsProperties(
        uint32_t count,
        VkProfilerPerformanceCounterProperties2EXT* pProperties ) const
    {
        const size_t writeCount = std::min<size_t>( count, m_Counters.size() );
        for( size_t i = 0; i < writeCount; ++i )
        {
            FillPerformanceCounterProperties( m_Counters[i], pProperties[i] );
        }

        return static_cast<uint32_t>( m_Counters.size() );
    }

    /***********************************************************************************\

    Function:
        SupportsCustomMetricsSets

    Description:
        NVIDIA implementation supports custom metrics sets.

    \***********************************************************************************/
    bool DeviceProfilerPerformanceCountersNVIDIA::SupportsCustomMetricsSets() const
    {
        return true;
    }

    /***********************************************************************************\

    Function:
        ResetMembers

    Description:
        Set default values for all members of the class.

    \***********************************************************************************/
    void DeviceProfilerPerformanceCountersNVIDIA::ResetMembers()
    {
        m_pVulkanDevice = nullptr;

        m_SamplingMode = VK_PROFILER_PERFORMANCE_COUNTERS_SAMPLING_MODE_QUERY_EXT;
        m_SamplingPeriodInNanoseconds = 0;

        m_MetricsEvaluator.Reset();
        m_PeriodicSampler.Reset();
        m_CounterData.Reset();

        m_Counters.clear();

        m_MetricsSets.clear();

        m_ActiveMetricsSetIndex = UINT32_MAX;
    }

    /***********************************************************************************\

    Function:
        FillPerformanceCounterProperties

    Description:
        Copy counter properties from internal representation to Vulkan structure.

    \***********************************************************************************/
    void DeviceProfilerPerformanceCountersNVIDIA::FillPerformanceCounterProperties(
        const Counter& counter,
        VkProfilerPerformanceCounterProperties2EXT& properties )
    {
        assert( properties.sType == VK_STRUCTURE_TYPE_PROFILER_PERFORMANCE_COUNTER_PROPERTIES_2_EXT );
        assert( properties.pNext == nullptr );

        ProfilerStringFunctions::CopyString(
            properties.shortName,
            counter.m_Name.c_str(),
            counter.m_Name.length() );

        ProfilerStringFunctions::CopyString(
            properties.description,
            counter.m_Description.c_str(),
            counter.m_Description.length() );

        properties.flags = 0;
        properties.unit = counter.m_Unit;
        properties.storage = counter.m_Storage;

        memcpy( properties.uuid, counter.m_UUID, VK_UUID_SIZE );
    }
}

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
        if( result == VK_SUCCESS )
        {
            m_SamplingMode = VK_PROFILER_PERFORMANCE_COUNTERS_SAMPLING_MODE_STREAM_EXT;
            m_SamplingPeriodInNanoseconds = config.m_NvidiaPerformanceQuery.m_NvidiaPerformanceStreamTimerPeriod;
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
        if( result == VK_SUCCESS )
        {
            nv::perf::DeviceIdentifiers deviceIdentifiers = nv::perf::VulkanGetDeviceIdentifiers(
                instanceHandle,
                physicalDeviceHandle,
                deviceHandle,
                pfnGetInstanceProcAddr,
                pfnGetDeviceProcAddr );

            if( deviceIdentifiers.pChipName )
            {
                m_ChipName = deviceIdentifiers.pChipName;
            }
            else
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
                    nv::perf::VulkanCalculateMetricsEvaluatorScratchBufferSize( m_ChipName.c_str() ) );

                pMetricsEvaluator = nv::perf::VulkanCreateMetricsEvaluator(
                    metricsEvaluatorScratchBuffer.data(),
                    metricsEvaluatorScratchBuffer.size(),
                    m_ChipName.c_str() );
                break;
            }

            case VK_PROFILER_PERFORMANCE_COUNTERS_SAMPLING_MODE_STREAM_EXT:
            {
                metricsEvaluatorScratchBuffer.resize(
                    nv::perf::sampler::DeviceCalculateMetricsEvaluatorScratchBufferSize( m_ChipName.c_str() ) );

                pMetricsEvaluator = nv::perf::sampler::DeviceCreateMetricsEvaluator(
                    metricsEvaluatorScratchBuffer.data(),
                    metricsEvaluatorScratchBuffer.size(),
                    m_ChipName.c_str() );
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
                    m_ChipName.c_str(),
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
                counter.m_Name.append( ".avg" );

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

                counter.m_Unit = VK_PROFILER_PERFORMANCE_COUNTER_UNIT_GENERIC_EXT;
                counter.m_Storage = VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_FLOAT32_EXT;

                // API does not provide UUIDs for metrics
                uint32_t uuid[VK_UUID_SIZE / 4] = {};
                uuid[0] = counter.m_Type;
                uuid[1] = counter.m_Index;
                memcpy( counter.m_UUID, uuid, VK_UUID_SIZE );

                m_Counters.push_back( std::move( counter ) );
            }
        }

        // Start metrics stream collection thread
        if( result == VK_SUCCESS )
        {
            if( m_SamplingMode == VK_PROFILER_PERFORMANCE_COUNTERS_SAMPLING_MODE_STREAM_EXT )
            {
                m_MetricsStreamCollectionThreadExit = false;
                m_MetricsStreamCollectionThread = std::thread(
                    &DeviceProfilerPerformanceCountersNVIDIA::MetricsStreamCollectionThreadProc,
                    this );
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
        if( m_MetricsStreamCollectionThread.joinable() )
        {
            m_MetricsStreamCollectionThreadExit = true;
            m_MetricsStreamCollectionThread.join();
        }

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
    uint32_t DeviceProfilerPerformanceCountersNVIDIA::GetMetricsCount(
        uint32_t metricsSetIndex ) const
    {
        std::shared_lock metricsSetsLock( m_MetricsSetsMutex );

        if( metricsSetIndex < m_MetricsSets.size() )
        {
            return static_cast<uint32_t>( m_MetricsSets[metricsSetIndex].m_CounterIndices.size() );
        }

        return 0;
    }

    /***********************************************************************************\

    Function:
        GetMetricsSetCount

    Description:
        Returns the number of metrics sets exposed by this extension.

    \***********************************************************************************/
    uint32_t DeviceProfilerPerformanceCountersNVIDIA::GetMetricsSetCount() const
    {
        std::shared_lock metricsSetsLock( m_MetricsSetsMutex );
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
        std::scoped_lock lk( m_ActiveMetricsSetMutex );
        return SetActiveMetricsSet_NoLock( metricsSetIndex );
    }

    /***********************************************************************************\

    Function:
        SetActiveMetricsSet_NoLock

    Description:
        Sets the active metrics set without acquiring the mutex.
        Caller is responsible for synchronizing access to the active metrics set.

    \***********************************************************************************/
    VkResult DeviceProfilerPerformanceCountersNVIDIA::SetActiveMetricsSet_NoLock(
        uint32_t metricsSetIndex )
    {
        // Early-out if the set is already set.
        if( m_ActiveMetricsSetIndex == metricsSetIndex )
        {
            return VK_SUCCESS;
        }

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

                m_CounterData.Reset();
                m_ActiveMetricsSetIndex = UINT32_MAX;
            }

            if( metricsSetIndex == UINT32_MAX )
            {
                // No new stream to start.
                return VK_SUCCESS;
            }

            // Configure the new session.
            size_t recordBufferSize = 0;
            const bool validateCounterData = true;
            const size_t maxNumUndecodedSamples = 1'000'000'000 / m_SamplingPeriodInNanoseconds;
            const size_t maxNumUndecodedSamplingRanges = 1;
            const auto samplingInterval = m_PeriodicSampler.GetGpuPulseSamplingInterval(
                m_SamplingPeriodInNanoseconds );

            const MetricsSet& metricsSet = m_MetricsSets.at( metricsSetIndex );
            const auto& counterConfiguration = m_CounterConfigurations.at( metricsSet.m_CompatibleHash );

            auto CreateCounterData = [&]( uint32_t maxSamples, NVPW_PeriodicSampler_CounterData_AppendMode appendMode, std::vector<uint8_t>& counterData )
            {
                return nv::perf::sampler::GpuPeriodicSamplerCreateCounterData(
                    m_PeriodicSampler.GetDeviceIndex(),
                    counterConfiguration.counterDataPrefix.data(),
                    counterConfiguration.counterDataPrefix.size(),
                    maxSamples,
                    appendMode,
                    counterData );
            };

            if( !m_CounterData.Initialize(
                    maxNumUndecodedSamples,
                    validateCounterData,
                    CreateCounterData ) )
            {
                assert( false );
                return VK_ERROR_INITIALIZATION_FAILED;
            }

            if( !nv::perf::sampler::GpuPeriodicSamplerCalculateRecordBufferSize(
                    m_PeriodicSampler.GetDeviceIndex(),
                    counterConfiguration.configImage,
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

            if( !m_PeriodicSampler.SetConfig(
                    counterConfiguration.configImage, 0 ) )
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
        GetRequiredPasses

    Description:
        Returns the number of required passes for the specified counters.

    \***********************************************************************************/
    uint32_t DeviceProfilerPerformanceCountersNVIDIA::GetRequiredPasses(
        uint32_t counterCount,
        const uint32_t* pCounterIndices ) const
    {
        nv::perf::MetricsConfigBuilder configBuilder;
        if( !configBuilder.Initialize(
                m_MetricsEvaluator.Get(),
                nv::perf::sampler::DeviceCreateRawCounterConfig( m_ChipName.c_str() ),
                m_ChipName.c_str() ) )
        {
            return 0;
        }

        for( size_t i = 0; i < counterCount; ++i )
        {
            const uint32_t counterIndex = pCounterIndices[i];
            const Counter& counter = m_Counters.at( counterIndex );

            if( !configBuilder.AddMetric( counter.m_Name.c_str() ) )
            {
                return 0;
            }
        }

        return static_cast<uint32_t>( configBuilder.GetNumPasses() );
    }

    /***********************************************************************************\

    Function:
        GetMetricsSets

    Description:
        Returns the list of available metrics sets.

    \***********************************************************************************/
    uint32_t DeviceProfilerPerformanceCountersNVIDIA::GetMetricsSets(
        uint32_t count,
        VkProfilerPerformanceMetricsSetProperties2EXT* pProperties ) const
    {
        std::shared_lock metricsSetsLock( m_MetricsSetsMutex );

        // Fill in the properties.
        const size_t writeCount = std::min<size_t>( count, m_MetricsSets.size() );
        for( size_t i = 0; i < writeCount; ++i )
        {
            FillPerformanceMetricsSetProperties( m_MetricsSets[i], pProperties[i] );
        }

        return static_cast<uint32_t>( m_MetricsSets.size() );
    }

    /***********************************************************************************\

    Function:
        GetMetricsSetProperties

    Description:
        Returns properties of the specified performance counter set.

    \***********************************************************************************/
    void DeviceProfilerPerformanceCountersNVIDIA::GetMetricsSetProperties(
        uint32_t metricsSetIndex,
        VkProfilerPerformanceMetricsSetProperties2EXT* pProperties ) const
    {
        std::shared_lock metricsSetsLock( m_MetricsSetsMutex );

        if( metricsSetIndex >= m_MetricsSets.size() )
        {
            memset( pProperties, 0, sizeof( VkProfilerPerformanceMetricsSetProperties2EXT ) );
            return;
        }

        FillPerformanceMetricsSetProperties( m_MetricsSets[metricsSetIndex], *pProperties );
    }

    /***********************************************************************************\

    Function:
        GetMetricsSetMetricsProperties

    Description:
        Returns list of performance counters in the specified counter set.

    \***********************************************************************************/
    uint32_t DeviceProfilerPerformanceCountersNVIDIA::GetMetricsSetMetricsProperties(
        uint32_t metricsSetIndex,
        uint32_t counterCount,
        VkProfilerPerformanceCounterProperties2EXT* pCounters ) const
    {
        std::shared_lock metricsSetsLock( m_MetricsSetsMutex );

        if( metricsSetIndex >= m_MetricsSets.size() )
        {
            return 0;
        }

        // Fill in the properties.
        const auto& metricsSet = m_MetricsSets.at( metricsSetIndex );
        const size_t writeCount = std::min<size_t>( counterCount, metricsSet.m_CounterIndices.size() );
        for( size_t i = 0; i < writeCount; ++i )
        {
            const uint32_t counterIndex = metricsSet.m_CounterIndices[i];
            const Counter& counter = m_Counters.at( counterIndex );

            FillPerformanceCounterProperties( counter, pCounters[i] );
        }

        return static_cast<uint32_t>( metricsSet.m_CounterIndices.size() );
    }

    /***********************************************************************************\

    Function:
        GetMetricsProperties

    Description:
        Returns properties of all available metrics.

    \***********************************************************************************/
    uint32_t DeviceProfilerPerformanceCountersNVIDIA::GetMetricsProperties(
        uint32_t counterCount,
        VkProfilerPerformanceCounterProperties2EXT* pCounters ) const
    {
        // Fill in the properties.
        const size_t writeCount = std::min<size_t>( counterCount, m_Counters.size() );
        for( size_t i = 0; i < writeCount; ++i )
        {
            FillPerformanceCounterProperties( m_Counters[i], pCounters[i] );
        }

        return static_cast<uint32_t>( m_Counters.size() );
    }

    /***********************************************************************************\

    Function:
        GetAvailableMetrics

    Description:
        Updates the list of available performance counters for a given queue family.

    \***********************************************************************************/
    void DeviceProfilerPerformanceCountersNVIDIA::GetAvailableMetrics(
        uint32_t selectedCountersCount,
        const uint32_t* pSelectedCounters,
        uint32_t& availableCountersCount,
        uint32_t* pAvailableCounters ) const
    {
        // Prepare a createInfo structure to test the number of passes.
        std::vector<uint32_t> testedCounters(
            pSelectedCounters,
            pSelectedCounters + selectedCountersCount );

        // Reserve a placeholder for the tested counter.
        testedCounters.push_back( UINT32_MAX );

        // Check which counters are not increasing the number of required runs.
        uint32_t* counterIt = pAvailableCounters;
        uint32_t* counterEnd = pAvailableCounters + availableCountersCount;

        while( counterIt != counterEnd )
        {
            testedCounters.back() = *counterIt;

            uint32_t numPasses = GetRequiredPasses(
                selectedCountersCount + 1,
                testedCounters.data() );

            if( numPasses > 1 )
            {
                counterEnd = std::remove( counterIt, counterEnd, *counterIt );
                continue;
            }

            counterIt++;
        }

        availableCountersCount = static_cast<uint32_t>( counterEnd - pAvailableCounters );
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
        CreateCustomMetricsSet

    Description:
        Creates a custom metrics set from the provided configuration.

    \***********************************************************************************/
    uint32_t DeviceProfilerPerformanceCountersNVIDIA::CreateCustomMetricsSet(
        const VkProfilerCustomPerformanceMetricsSetCreateInfoEXT* pCreateInfo )
    {
        assert( pCreateInfo->sType == VK_STRUCTURE_TYPE_PROFILER_CUSTOM_PERFORMANCE_METRICS_SET_CREATE_INFO_EXT );
        assert( pCreateInfo->pNext == nullptr );

        // Validate parameters.
        if( !pCreateInfo->metricsCount || !pCreateInfo->pMetricsIndices )
        {
            return UINT32_MAX;
        }

        // Sort counter indices.
        std::vector<uint32_t> sortedCounterIndices(
            pCreateInfo->pMetricsIndices,
            pCreateInfo->pMetricsIndices + pCreateInfo->metricsCount );
        std::sort( sortedCounterIndices.begin(), sortedCounterIndices.end() );

        // Calculate a hash of the counter set to identify compatible sets.
        HashInput hashInput;

        for( uint32_t counterIndex : sortedCounterIndices )
        {
            const Counter& counter = m_Counters.at( counterIndex );
            hashInput.Add( counter.m_UUID, sizeof( counter.m_UUID ) );
        }

        uint32_t compatibleHash = Farmhash::Fingerprint32(
            hashInput.GetData(),
            hashInput.GetSize() );

        // Calculate a full hash to identify identical sets.
        hashInput.Reset();
        hashInput.Add( compatibleHash );
        hashInput.Add( pCreateInfo->pName );
        hashInput.Add( pCreateInfo->pDescription );

        uint32_t fullHash = Farmhash::Fingerprint32(
            hashInput.GetData(),
            hashInput.GetSize() );

        // Check if an identical counter set already exists.
        uint32_t metricsSetIndex = FindMetricsSetByHash( fullHash );

        // Create and register the counter set if it does not exist yet.
        if( metricsSetIndex == UINT32_MAX )
        {
            MetricsSet metricsSet = {};
            metricsSet.m_Name = pCreateInfo->pName ? pCreateInfo->pName : std::string();
            metricsSet.m_Description = pCreateInfo->pDescription ? pCreateInfo->pDescription : std::string();
            metricsSet.m_CounterIndices = std::move( sortedCounterIndices );
            metricsSet.m_CompatibleHash = compatibleHash;
            metricsSet.m_FullHash = fullHash;

            if( !PrepareCounterConfigurationForMetricsSet( metricsSet ) )
            {
                return UINT32_MAX;
            }

            metricsSetIndex = RegisterMetricsSet( std::move( metricsSet ) );
        }

        return metricsSetIndex;
    }

    /***********************************************************************************\

    Function:
        ReadStreamData

    Description:
        Reads the stream data for the specified time range and parses it into counter
        results.

    \***********************************************************************************/
    bool DeviceProfilerPerformanceCountersNVIDIA::ReadStreamData(
        uint64_t beginTimestamp,
        uint64_t endTimestamp,
        std::vector<DeviceProfilerPerformanceCountersStreamResult>& samples )
    {
        std::scoped_lock lk( m_MetricsStreamResultsMutex );

        if( m_SamplingMode != VK_PROFILER_PERFORMANCE_COUNTERS_SAMPLING_MODE_STREAM_EXT )
        {
            // No stream data available in query mode.
            return true;
        }

        if( beginTimestamp == endTimestamp )
        {
            // No data to read.
            return true;
        }

        auto begin = m_MetricsStreamResults.begin();
        auto end = m_MetricsStreamResults.end();

        if( endTimestamp < beginTimestamp )
        {
            return true;
        }
        else
        {
            while( ( begin != end ) && ( begin->m_GpuTimestamp < beginTimestamp ) )
                begin++;
            while( ( end != begin ) && ( end - 1 )->m_GpuTimestamp > endTimestamp )
                end--;
        }

        bool dataComplete = ( end != m_MetricsStreamResults.end() );

        if( begin != end )
        {
            // Copy the data to the output buffer.
            samples.insert( samples.begin(), begin, end );

            m_MetricsStreamResults.erase( begin, end );
        }
        else
        {
            // No more data will arrive if the thread has exited.
            if( !m_MetricsStreamCollectionThread.joinable() )
            {
                dataComplete = true;
            }
        }

        return dataComplete;
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

        m_ChipName.clear();

        m_MetricsEvaluator.Reset();

        m_PeriodicSampler.Reset();
        m_CounterData.Reset();

        m_CounterConfigurations.clear();

        m_ActiveMetricsSetIndex = UINT32_MAX;

        m_Counters.clear();
        m_MetricsSets.clear();

        m_MetricsStreamCollectionThread = std::thread();
        m_MetricsStreamCollectionThreadExit = false;

        m_MetricsStreamResults.clear();
        m_MetricsStreamLastResultTimestamp = 0;
        m_MetricsStreamMaxBufferLengthInNanoseconds = 1'000'000'000ull;
        m_MetricsStreamMaxReportCount = 16'384;
    }

    /***********************************************************************************\

    Function:
        PrepareCounterConfigurationForMetricsSet

    Description:
        Creates a counter configuration for the specified set of counters if it does
        not exist yet.

    \***********************************************************************************/
    bool DeviceProfilerPerformanceCountersNVIDIA::PrepareCounterConfigurationForMetricsSet(
        const MetricsSet& metricsSet )
    {
        std::scoped_lock counterConfigurationsLock( m_CounterConfigurations );

        auto it = m_CounterConfigurations.unsafe_find( metricsSet.m_CompatibleHash );
        if( it == m_CounterConfigurations.end() )
        {
            nv::perf::MetricsConfigBuilder configBuilder;
            if( !configBuilder.Initialize(
                    m_MetricsEvaluator.Get(),
                    nv::perf::sampler::DeviceCreateRawCounterConfig( m_ChipName.c_str() ),
                    m_ChipName.c_str() ) )
            {
                return false;
            }

            const size_t counterCount = metricsSet.m_CounterIndices.size();
            for( size_t i = 0; i < counterCount; ++i )
            {
                const uint32_t counterIndex = metricsSet.m_CounterIndices[i];
                const Counter& counter = m_Counters.at( counterIndex );

                if( !configBuilder.AddMetric( counter.m_Name.c_str() ) )
                {
                    return false;
                }
            }

            nv::perf::CounterConfiguration counterConfiguration;
            if( !nv::perf::CreateConfiguration(
                    configBuilder,
                    counterConfiguration ) )
            {
                return false;
            }

            m_CounterConfigurations.unsafe_insert(
                metricsSet.m_CompatibleHash,
                counterConfiguration );
        }

        return true;
    }

    /***********************************************************************************\

    Function:
        GetMetricEvalRequests

    Description:

    \***********************************************************************************/
    std::vector<NVPW_MetricEvalRequest> DeviceProfilerPerformanceCountersNVIDIA::GetMetricEvalRequests(
        uint32_t metricsSetIndex )
    {
        std::shared_lock metricsSetsLock( m_MetricsSetsMutex );
        const MetricsSet& metricsSet = m_MetricsSets.at( metricsSetIndex );

        std::vector<NVPW_MetricEvalRequest> metricEvalRequests;
        metricEvalRequests.reserve( metricsSet.m_CounterIndices.size() );

        for( uint32_t counterIndex : metricsSet.m_CounterIndices )
        {
            const Counter& counter = m_Counters.at( counterIndex );

            NVPW_MetricEvalRequest metricEvalRequest = {};
            if( m_MetricsEvaluator.ToMetricEvalRequest(
                    counter.m_Name.c_str(),
                    metricEvalRequest ) )
            {
                metricEvalRequests.push_back( metricEvalRequest );
            }
        }

        return metricEvalRequests;
    }

    /***********************************************************************************\

    Function:
        MetricsStreamCollectionThreadProc

    Description:
        Thread procedure for collecting stream data in the background and parsing it into
        counter results.

    \***********************************************************************************/
    void DeviceProfilerPerformanceCountersNVIDIA::MetricsStreamCollectionThreadProc()
    {
        while( !m_MetricsStreamCollectionThreadExit )
        {
            try
            {
                // Limit size of the buffered data.
                FreeUnusedMetricsStreamSamples();

                // Collect pending samples from the stream.
                const size_t reportCount = CollectMetricsStreamSamples();

                // Wait for the next batch of reports to be available.
                // Avoid sleeping if the reportCount is high to avoid dropping samples.
                if( reportCount < ( m_MetricsStreamMaxReportCount / 2 ) )
                {
                    std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
                }
            }
            catch( ... )
            {
                // Prevent the thread from exiting on exceptions.
                assert( false );
            }
        }
    }

    /***********************************************************************************\

    Function:
        CollectMetricsStreamSamples

    Description:

    \***********************************************************************************/
    size_t DeviceProfilerPerformanceCountersNVIDIA::CollectMetricsStreamSamples()
    {
        thread_local std::vector<double> parsedValues;
        thread_local std::vector<VkProfilerPerformanceCounterResultEXT> parsedResults;

        // Don't switch the active metrics set while reading the stream.
        std::shared_lock lk( m_ActiveMetricsSetMutex );

        const uint64_t cpuTimestamp = m_CpuTimestampCounter.GetCurrentValue();

        const uint32_t activeMetricsSetIndex = m_ActiveMetricsSetIndex;
        if( activeMetricsSetIndex == UINT32_MAX )
        {
            // No active metrics set, nothing to read.
            return 0;
        }

        // Get the number of bytes available in the record buffer and check for overflow.
        nv::perf::sampler::GpuPeriodicSampler::GetRecordBufferStatusParams bufferStatus = {};
        bufferStatus.queryNumUnreadBytes = true;

        if( !m_PeriodicSampler.GetRecordBufferStatus( bufferStatus ) )
        {
            return 0;
        }

        if( bufferStatus.numUnreadBytes == 0 )
        {
            // No data to read.
            return 0;
        }

        // Read the data and acknowledge the read bytes.
        NVPW_GPU_PeriodicSampler_DecodeStopReason decodeStopReason = NVPW_GPU_PERIODIC_SAMPLER_DECODE_STOP_REASON_OTHER;
        size_t numSamplesMerged = 0;
        size_t numBytesConsumed = 0;

        if( !m_PeriodicSampler.DecodeCounters(
                m_CounterData.GetCounterData(),
                bufferStatus.numUnreadBytes,
                decodeStopReason,
                numSamplesMerged,
                numBytesConsumed ) )
        {
            return 0;
        }

        if( !m_PeriodicSampler.AcknowledgeRecordBuffer(
                numBytesConsumed ) )
        {
            return 0;
        }

        if( !m_CounterData.UpdatePut() )
        {
            return 0;
        }

        // Prepare the metrics evaluator and the metrics evaluation requests.
        const auto& counterData = m_CounterData.GetCounterData();
        if( !m_MetricsEvaluator.MetricsEvaluatorSetDeviceAttributes(
                counterData.data(),
                counterData.size() ) )
        {
            return 0;
        }

        const std::vector<NVPW_MetricEvalRequest> metricEvalRequests =
            GetMetricEvalRequests( activeMetricsSetIndex );

        parsedValues.resize( metricEvalRequests.size() );
        parsedResults.resize( metricEvalRequests.size() );

        uint32_t reportCount = 0;

        auto ConsumeProc = [&]( const uint8_t* pData, size_t dataSize, uint32_t rangeIndex, bool& stop )
        {
            stop = false;

            // Evaluate the metrics and convert them to VkProfilerPerformanceCounterResultEXT values.
            if( !m_MetricsEvaluator.EvaluateToGpuValues(
                    pData,
                    dataSize,
                    rangeIndex,
                    metricEvalRequests.size(),
                    metricEvalRequests.data(),
                    parsedValues.data() ) )
            {
                return false;
            }

            for( size_t i = 0; i < parsedValues.size(); ++i )
            {
                parsedResults[i].float32 = static_cast<float>( parsedValues[i] );
            }

            // Read the sample collection timestamp.
            nv::perf::sampler::SampleTimestamp sampleTimestamp = {};

            if( !nv::perf::sampler::CounterDataGetSampleTime(
                    pData,
                    rangeIndex,
                    sampleTimestamp ) )
            {
                return false;
            }

            if( sampleTimestamp.end != m_MetricsStreamLastResultTimestamp )
            {
                // Store the parsed results in the stream results buffer.
                std::scoped_lock resultsLock( m_MetricsStreamResultsMutex );
                m_MetricsStreamResults.push_back( {
                    sampleTimestamp.end,
                    cpuTimestamp,
                    activeMetricsSetIndex,
                    parsedResults } );

                m_MetricsStreamLastResultTimestamp = sampleTimestamp.end;
            }

            reportCount++;
            return true;
        };

        if( !m_CounterData.ConsumeData( ConsumeProc ) )
        {
            return 0;
        }

        if( !m_CounterData.UpdateGet( reportCount ) )
        {
            return 0;
        }

        return reportCount;
    }

    /***********************************************************************************\

    Function:
        FreeUnusedMetricsStreamSamples

    Description:

    \***********************************************************************************/
    void DeviceProfilerPerformanceCountersNVIDIA::FreeUnusedMetricsStreamSamples()
    {
        std::scoped_lock resultsLock( m_MetricsStreamResultsMutex );

        if( !m_MetricsStreamResults.empty() )
        {
            const uint64_t currentTimestamp = m_CpuTimestampCounter.GetCurrentValue();

            const auto begin = m_MetricsStreamResults.begin();
            const auto end = m_MetricsStreamResults.end();

            // Find the first sample that is within the max buffer length.
            auto it = begin;
            while( ( it != end ) && ( m_CpuTimestampCounter.Convert( currentTimestamp - it->m_CpuTimestamp ).count() > m_MetricsStreamMaxBufferLengthInNanoseconds ) )
                it++;

            if( it != begin )
            {
                m_MetricsStreamResults.erase( begin, it );
            }
        }
    }

    /***********************************************************************************\

    Function:
        FreeUnusedMetricsStreamSamples

    Description:
        Try to find an existing counter set by its full hash (all properties must match).

    \***********************************************************************************/
    uint32_t DeviceProfilerPerformanceCountersNVIDIA::FindMetricsSetByHash(
        uint32_t fullHash ) const
    {
        std::shared_lock metricsSetsLock( m_MetricsSetsMutex );

        const uint32_t setCount = static_cast<uint32_t>( m_MetricsSets.size() );
        for( uint32_t setIndex = 0; setIndex < setCount; ++setIndex )
        {
            if( m_MetricsSets[setIndex].m_FullHash == fullHash )
            {
                return setIndex;
            }
        }

        return UINT32_MAX;
    }

    /***********************************************************************************\

    Function:
        RegisterMetricsSet

    Description:
        Append the counter set to the list of available sets.

    \***********************************************************************************/
    uint32_t DeviceProfilerPerformanceCountersNVIDIA::RegisterMetricsSet(
        MetricsSet&& metricsSet )
    {
        std::unique_lock metricsSetsLock( m_MetricsSetsMutex );

        // Append the counter set to the list.
        m_MetricsSets.push_back( std::move( metricsSet ) );

        // Return the index of the newly created counter set.
        return static_cast<uint32_t>( m_MetricsSets.size() - 1 );
    }

    /***********************************************************************************\

    Function:
        FillPerformanceMetricsSetProperties

    Description:
        Copy metrics set properties from internal representation to Vulkan structure.

    \***********************************************************************************/
    void DeviceProfilerPerformanceCountersNVIDIA::FillPerformanceMetricsSetProperties(
        const MetricsSet& metricsSet,
        VkProfilerPerformanceMetricsSetProperties2EXT& properties )
    {
        assert( properties.sType == VK_STRUCTURE_TYPE_PROFILER_PERFORMANCE_METRICS_SET_PROPERTIES_2_EXT );
        assert( properties.pNext == nullptr );

        ProfilerStringFunctions::CopyString(
            properties.name,
            metricsSet.m_Name.c_str(),
            metricsSet.m_Name.length() );

        ProfilerStringFunctions::CopyString(
            properties.description,
            metricsSet.m_Description.c_str(),
            metricsSet.m_Description.length() );

        properties.metricsCount = static_cast<uint32_t>( metricsSet.m_CounterIndices.size() );
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

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

#include "profiler_performance_counters_intel.h"
#include "profiler/profiler_config.h"
#include "profiler/profiler_helpers.h"
#include "profiler_layer_objects/VkDevice_object.h"

#ifndef NDEBUG
#ifndef _DEBUG
#define NDEBUG
#endif
#endif
#include <assert.h>

#ifdef WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

#ifdef WIN32
#ifdef _WIN64
#define PROFILER_METRICS_DLL_INTEL "igdmd64.dll"
#else
#define PROFILER_METRICS_DLL_INTEL "igdmd32.dll"
#endif
#else
#define PROFILER_METRICS_DLL_INTEL "libigdmd.so"
#endif

#include <nlohmann/json.hpp>
#include <fstream>

namespace MD = MetricsDiscovery;

namespace Profiler
{
    /***********************************************************************************\

    Function:
        DeviceProfilerPerformanceCountersINTEL

    Description:
        Constructor.

    \***********************************************************************************/
    DeviceProfilerPerformanceCountersINTEL::DeviceProfilerPerformanceCountersINTEL()
    {
        ResetMembers();
    }

    /***********************************************************************************\

    Function:
        Initialize

    Description:

    \***********************************************************************************/
    VkResult DeviceProfilerPerformanceCountersINTEL::Initialize(
        VkDevice_Object* pDevice,
        const DeviceProfilerConfig& config )
    {
        m_pVulkanDevice = pDevice;

        // Returning errors from this function is fine - it is optional feature and will be 
        // disabled when initialization fails. If these errors were moved later (to other functions)
        // whole layer could crash.
        VkResult result = VK_SUCCESS;

        // Load metrics discovery DLL.
        if( !LoadMetricsDiscoveryLibrary() )
        {
            result = VK_ERROR_INCOMPATIBLE_DRIVER;
        }

        // Open metrics discovery device.
        if( result == VK_SUCCESS )
        {
            if( !OpenMetricsDevice() )
            {
                result = VK_ERROR_INITIALIZATION_FAILED;
            }
        }

        // Read device properties
        if( result == VK_SUCCESS )
        {
            const MD::TTypedValue_1_0* pGpuTimestampFrequency = m_pDevice->GetGlobalSymbolValueByName( "GpuTimestampFrequency" );
            const MD::TTypedValue_1_0* pGpuTimestampMax = m_pDevice->GetGlobalSymbolValueByName( "MaxTimestamp" );

            if( pGpuTimestampFrequency && pGpuTimestampMax )
            {
                m_GpuTimestampPeriod = 1e9 / pGpuTimestampFrequency->ValueUInt64;
                m_GpuTimestampMax = pGpuTimestampMax->ValueUInt64;
                m_GpuTimestampIs32Bit = ( m_GpuTimestampMax <= static_cast<uint64_t>( UINT32_MAX * m_GpuTimestampPeriod ) );
            }
            else
            {
                result = VK_ERROR_INITIALIZATION_FAILED;
            }
        }

        // Setup sampling mode
        if( result == VK_SUCCESS )
        {
            switch( config.m_PerformanceQueryMode )
            {
            case performance_query_mode_t::query:
                m_SamplingMode = VK_PROFILER_PERFORMANCE_COUNTERS_SAMPLING_MODE_QUERY_EXT;
                break;
            case performance_query_mode_t::stream:
                m_SamplingMode = VK_PROFILER_PERFORMANCE_COUNTERS_SAMPLING_MODE_STREAM_EXT;
                break;
            default:
                // Unsupported mode
                result = VK_ERROR_FEATURE_NOT_PRESENT;
                break;
            }
        }

        // Import extension functions
        if( result == VK_SUCCESS )
        {
            do
            {
                #define LoadVulkanExtensionFunction( PROC )                         \
                    m_pVulkanDevice->Callbacks.PROC =                               \
                        (PFN_vk##PROC)m_pVulkanDevice->Callbacks.GetDeviceProcAddr( \
                            m_pVulkanDevice->Handle,                                \
                            "vk" #PROC );                                           \
                    if( !m_pVulkanDevice->Callbacks.PROC )                          \
                    {                                                               \
                        assert( !"vk" #PROC " not found" );                         \
                        result = VK_ERROR_INCOMPATIBLE_DRIVER;                      \
                        break;                                                      \
                    }

                LoadVulkanExtensionFunction( AcquirePerformanceConfigurationINTEL );
                LoadVulkanExtensionFunction( CmdSetPerformanceMarkerINTEL );
                LoadVulkanExtensionFunction( CmdSetPerformanceOverrideINTEL );
                LoadVulkanExtensionFunction( CmdSetPerformanceStreamMarkerINTEL );
                LoadVulkanExtensionFunction( GetPerformanceParameterINTEL );
                LoadVulkanExtensionFunction( InitializePerformanceApiINTEL );
                LoadVulkanExtensionFunction( QueueSetPerformanceConfigurationINTEL );
                LoadVulkanExtensionFunction( ReleasePerformanceConfigurationINTEL );
                LoadVulkanExtensionFunction( UninitializePerformanceApiINTEL );

                #undef LoadVulkanExtensionFunction
            } while( false );
        }

        // Initialize performance API
        if( result == VK_SUCCESS )
        {
            VkInitializePerformanceApiInfoINTEL initInfo = {};
            initInfo.sType = VK_STRUCTURE_TYPE_INITIALIZE_PERFORMANCE_API_INFO_INTEL;

            result = m_pVulkanDevice->Callbacks.InitializePerformanceApiINTEL(
                m_pVulkanDevice->Handle,
                &initInfo );

            m_PerformanceApiInitialized = ( result == VK_SUCCESS );
        }

        // Iterate over all concurrent groups to find OA
        if( result == VK_SUCCESS )
        {
            const uint32_t concurrentGroupCount = m_pDeviceParams->ConcurrentGroupsCount;

            for( uint32_t i = 0; i < concurrentGroupCount; ++i )
            {
                MD::IConcurrentGroup_1_1* pConcurrentGroup = m_pDevice->GetConcurrentGroup( i );
                assert( pConcurrentGroup );

                MD::TConcurrentGroupParams_1_0* pConcurrentGroupParams = pConcurrentGroup->GetParams();
                assert( pConcurrentGroupParams );

                if( (std::strcmp( pConcurrentGroupParams->SymbolName, "OA" ) == 0) &&
                    (pConcurrentGroupParams->MetricSetsCount > 0) )
                {
                    m_pConcurrentGroup = pConcurrentGroup;
                    m_pConcurrentGroupParams = pConcurrentGroupParams;
                    break;
                }
            }

            // Check if OA metric group is available
            if( !m_pConcurrentGroup )
            {
                result = VK_ERROR_INCOMPATIBLE_DRIVER;
            }
        }

        // Start metrics stream collection thread
        if( result == VK_SUCCESS )
        {
            if( m_SamplingMode == VK_PROFILER_PERFORMANCE_COUNTERS_SAMPLING_MODE_STREAM_EXT )
            {
                m_MetricsStreamCollectionThreadExit = false;
                m_MetricsStreamCollectionThread = std::thread(
                    &DeviceProfilerPerformanceCountersINTEL::MetricsStreamCollectionThreadProc,
                    this );
            }
        }

        // Enumerate available metric sets
        if( result == VK_SUCCESS )
        {
            const uint32_t oaMetricSetCount = m_pConcurrentGroupParams->MetricSetsCount;
            assert( oaMetricSetCount > 0 );

            uint32_t defaultMetricsSetIndex = UINT32_MAX;
            const char* pDefaultMetricsSetName = config.m_DefaultMetricsSet.c_str();

            uint32_t apiMask = ( m_SamplingMode == VK_PROFILER_PERFORMANCE_COUNTERS_SAMPLING_MODE_STREAM_EXT ) ?
                MD::API_TYPE_IOSTREAM :
                MD::API_TYPE_VULKAN;

            for( uint32_t setIndex = 0; setIndex < oaMetricSetCount; ++setIndex )
            {
                MetricsSet set = {};
                set.m_pMetricSet = m_pConcurrentGroup->GetMetricSet( setIndex );
                set.m_pMetricSet->SetApiFiltering( apiMask );

                // Read params - must be done after API filtering.
                set.m_pMetricSetParams = set.m_pMetricSet->GetParams();

                // Read counters in the set.
                const uint32_t counterCount = set.m_pMetricSetParams->MetricsCount;
                set.m_Counters.reserve( counterCount );

                for( uint32_t metricIndex = 0; metricIndex < counterCount; ++metricIndex )
                {
                    Counter counter = {};
                    counter.m_MetricIndex = metricIndex;
                    counter.m_pMetric = set.m_pMetricSet->GetMetric( metricIndex );
                    counter.m_pMetricParams = counter.m_pMetric->GetParams();

                    if( !TranslateStorage( counter.m_pMetricParams->ResultType, counter.m_Storage ) )
                    {
                        // Unsupported metric type
                        continue;
                    }

                    if( !TranslateUnit( counter.m_pMetricParams->MetricResultUnits, counter.m_ResultFactor, counter.m_Unit ) )
                    {
                        // Unsupported metric unit
                        continue;
                    }

                    // API does not provide UUIDs for metrics
                    uint32_t uuid[VK_UUID_SIZE / 4] = {};
                    uuid[0] = setIndex;
                    uuid[1] = metricIndex;
                    memcpy( counter.m_UUID, uuid, VK_UUID_SIZE );

                    set.m_Counters.push_back( std::move( counter ) );
                }

                if( set.m_Counters.empty() )
                {
                    // No supported counters in this set.
                    continue;
                }

                if( m_SamplingMode == VK_PROFILER_PERFORMANCE_COUNTERS_SAMPLING_MODE_STREAM_EXT )
                {
                    // Read informations in the set.
                    set.m_ReportReasonInformationIndex = UINT32_MAX;
                    set.m_ValueInformationIndex = UINT32_MAX;
                    set.m_TimestampInformationIndex = UINT32_MAX;

                    const uint32_t informationCount = set.m_pMetricSetParams->InformationCount;
                    for( uint32_t infoIndex = 0; infoIndex < informationCount; ++infoIndex )
                    {
                        MD::IInformation_1_0* pInformation = set.m_pMetricSet->GetInformation( infoIndex );
                        MD::TInformationParams_1_0* pInformationParams = pInformation->GetParams();

                        switch( pInformationParams->InfoType )
                        {
                        case MD::INFORMATION_TYPE_REPORT_REASON:
                            set.m_ReportReasonInformationIndex = infoIndex + set.m_pMetricSetParams->MetricsCount;
                            break;
                        case MD::INFORMATION_TYPE_VALUE:
                            set.m_ValueInformationIndex = infoIndex + set.m_pMetricSetParams->MetricsCount;
                            break;
                        case MD::INFORMATION_TYPE_TIMESTAMP:
                            set.m_TimestampInformationIndex = infoIndex + set.m_pMetricSetParams->MetricsCount;
                            break;
                        }
                    }

                    if( ( set.m_ReportReasonInformationIndex == UINT32_MAX ) ||
                        ( set.m_ValueInformationIndex == UINT32_MAX ) ||
                        ( set.m_TimestampInformationIndex == UINT32_MAX ) )
                    {
                        // Required informations not found.
                        continue;
                    }
                }

                // Find default metrics set index.
                if( (defaultMetricsSetIndex == UINT32_MAX) &&
                    (strcmp( set.m_pMetricSetParams->SymbolName, pDefaultMetricsSetName ) == 0) )
                {
                    defaultMetricsSetIndex = setIndex;
                }

                m_MetricsSets.push_back( std::move( set ) );
            }

            // Use the first available set if RenderBasic was not found.
            if( defaultMetricsSetIndex == UINT32_MAX )
            {
                defaultMetricsSetIndex = 0;
            }

            result = SetActiveMetricsSet( defaultMetricsSetIndex );
        }

        // Cleanup if any error occurred
        if( result != VK_SUCCESS )
        {
            Destroy();
        }

        return result;
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:

    \***********************************************************************************/
    void DeviceProfilerPerformanceCountersINTEL::Destroy()
    {
        if( m_MetricsStreamCollectionThread.joinable() )
        {
            m_MetricsStreamCollectionThreadExit = true;
            m_MetricsStreamCollectionThread.join();
        }

        if( m_PerformanceApiConfiguration )
        {
            assert( m_pVulkanDevice && m_pVulkanDevice->Callbacks.ReleasePerformanceConfigurationINTEL );
            m_pVulkanDevice->Callbacks.ReleasePerformanceConfigurationINTEL(
                m_pVulkanDevice->Handle,
                m_PerformanceApiConfiguration );
        }

        if( m_PerformanceApiInitialized )
        {
            assert( m_pVulkanDevice && m_pVulkanDevice->Callbacks.UninitializePerformanceApiINTEL );
            m_pVulkanDevice->Callbacks.UninitializePerformanceApiINTEL(
                m_pVulkanDevice->Handle );
        }

        CloseMetricsDevice();
        UnloadMetricsDiscoveryLibrary();

        ResetMembers();
    }

    /***********************************************************************************\

    Function:
        ResetMembers

    Description:

    \***********************************************************************************/
    void DeviceProfilerPerformanceCountersINTEL::ResetMembers()
    {
        m_MDLibraryHandle = nullptr;

        m_pVulkanDevice = nullptr;

        m_pAdapterGroup = nullptr;
        m_pAdapter = nullptr;
        m_pDevice = nullptr;
        m_pDeviceParams = nullptr;

        m_pConcurrentGroup = nullptr;
        m_pConcurrentGroupParams = nullptr;

        m_SamplingMode = VK_PROFILER_PERFORMANCE_COUNTERS_SAMPLING_MODE_QUERY_EXT;

        m_GpuTimestampPeriod = 1.0;
        m_GpuTimestampMax = UINT64_MAX;
        m_GpuTimestampIs32Bit = false;

        m_MetricsSets.clear();

        m_ActiveMetricsSetIndex = UINT32_MAX;

        m_PerformanceApiInitialized = false;
        m_PerformanceApiConfiguration = VK_NULL_HANDLE;

        m_MetricsStreamCollectionThread = std::thread();
        m_MetricsStreamCollectionThreadExit = false;

        m_MetricsStreamMaxReportCount = 16'384;
        m_MetricsStreamMaxBufferLengthInNanoseconds = 1'000'000'000ull;
        m_MetricsStreamDataBuffer.clear();

        m_MetricsStreamResults.clear();
        m_MetricsStreamLastResultTimestamp = 0;
    }

    /***********************************************************************************\

    Function:
        SetQueuePerformanceConfiguration

    Description:
        Configure queue for collection of Intel performance counters.

    \***********************************************************************************/
    VkResult DeviceProfilerPerformanceCountersINTEL::SetQueuePerformanceConfiguration( VkQueue queue )
    {
        std::shared_lock lk( m_ActiveMetricSetMutex );

        VkResult result = VK_SUCCESS;

        // Configure the queue only if query mode is used and a metrics set is active.
        if( ( m_SamplingMode == VK_PROFILER_PERFORMANCE_COUNTERS_SAMPLING_MODE_QUERY_EXT ) &&
            ( m_ActiveMetricsSetIndex != UINT32_MAX ) )
        {
            assert( m_PerformanceApiConfiguration != VK_NULL_HANDLE );
            assert( m_pVulkanDevice->Callbacks.QueueSetPerformanceConfigurationINTEL );

            result = m_pVulkanDevice->Callbacks.QueueSetPerformanceConfigurationINTEL(
                queue,
                m_PerformanceApiConfiguration );
        }

        return result;
    }

    /***********************************************************************************\

    Function:
        GetSamplingMode

    Description:

    \***********************************************************************************/
    VkProfilerPerformanceCountersSamplingModeEXT DeviceProfilerPerformanceCountersINTEL::GetSamplingMode() const
    {
        return m_SamplingMode;
    }

    /***********************************************************************************\

    Function:
        GetReportSize

    Description:

    \***********************************************************************************/
    uint32_t DeviceProfilerPerformanceCountersINTEL::GetReportSize( uint32_t metricsSetIndex, uint32_t queueFamilyIndex ) const
    {
        return m_MetricsSets[ metricsSetIndex ].m_pMetricSetParams->QueryReportSize;
    }

    /***********************************************************************************\

    Function:
        GetMetricCount

    Description:
        Get number of HW metrics exposed by this extension.

    \***********************************************************************************/
    uint32_t DeviceProfilerPerformanceCountersINTEL::GetMetricsCount( uint32_t metricsSetIndex ) const
    {
        return m_MetricsSets[ metricsSetIndex ].m_pMetricSetParams->MetricsCount;
        // Skip InformationCount - no valuable data here
    }

    /***********************************************************************************\

    Function:
        GetMetricsSetCount

    Description:
        Get number of metrics sets exposed by this extensions.

    \***********************************************************************************/
    uint32_t DeviceProfilerPerformanceCountersINTEL::GetMetricsSetCount() const
    {
        return static_cast<uint32_t>( m_MetricsSets.size() );
    }

    /***********************************************************************************\

    Function:
        SetActiveMetricsSet

    Description:

    \***********************************************************************************/
    VkResult DeviceProfilerPerformanceCountersINTEL::SetActiveMetricsSet( uint32_t metricsSetIndex )
    {
        std::unique_lock lk( m_ActiveMetricSetMutex );

        // Early-out if the set is already set.
        if( m_ActiveMetricsSetIndex == metricsSetIndex )
        {
            return VK_SUCCESS;
        }

        // Check if the metric set is available
        if( metricsSetIndex >= m_MetricsSets.size() )
        {
            assert( false );
            return VK_ERROR_VALIDATION_FAILED_EXT;
        }

        // Get the new metrics set object.
        auto& metricsSet = m_MetricsSets[metricsSetIndex];

        if( m_SamplingMode == VK_PROFILER_PERFORMANCE_COUNTERS_SAMPLING_MODE_QUERY_EXT )
        {
            // Release the current performance configuration.
            if( m_PerformanceApiConfiguration )
            {
                m_pVulkanDevice->Callbacks.ReleasePerformanceConfigurationINTEL(
                    m_pVulkanDevice->Handle,
                    m_PerformanceApiConfiguration );

                m_PerformanceApiConfiguration = VK_NULL_HANDLE;
                m_ActiveMetricsSetIndex = UINT32_MAX;
            }

            // Activate the metrics set.
            if( metricsSet.m_pMetricSet->Activate() != MD::CC_OK )
            {
                assert( false );
                return VK_ERROR_INITIALIZATION_FAILED;
            }

            // Acquire new performance configuration for the activated metrics set.
            VkPerformanceConfigurationAcquireInfoINTEL acquireInfo = {};
            acquireInfo.sType = VK_STRUCTURE_TYPE_PERFORMANCE_CONFIGURATION_ACQUIRE_INFO_INTEL;
            acquireInfo.type = VK_PERFORMANCE_CONFIGURATION_TYPE_COMMAND_QUEUE_METRICS_DISCOVERY_ACTIVATED_INTEL;

            VkResult result = m_pVulkanDevice->Callbacks.AcquirePerformanceConfigurationINTEL(
                m_pVulkanDevice->Handle,
                &acquireInfo,
                &m_PerformanceApiConfiguration );

            // Set can be deactivated once the performance configuration is acquired.
            metricsSet.m_pMetricSet->Deactivate();

            if( result != VK_SUCCESS )
            {
                m_PerformanceApiConfiguration = VK_NULL_HANDLE;
                assert( false );
                return result;
            }
        }

        if( m_SamplingMode == VK_PROFILER_PERFORMANCE_COUNTERS_SAMPLING_MODE_STREAM_EXT )
        {
            // Close the current stream if any.
            if( m_ActiveMetricsSetIndex != UINT32_MAX )
            {
                if( m_pConcurrentGroup->CloseIoStream() != MD::CC_OK )
                {
                    assert( false );
                    return VK_ERROR_INITIALIZATION_FAILED;
                }

                m_ActiveMetricsSetIndex = UINT32_MAX;
            }

            // Begin the new stream.
            uint32_t timerPeriodNs = 25'000;
            uint32_t bufferSize = metricsSet.m_pMetricSetParams->RawReportSize * m_MetricsStreamMaxReportCount;

            if( m_pConcurrentGroup->OpenIoStream( metricsSet.m_pMetricSet, 0, &timerPeriodNs, &bufferSize ) != MD::CC_OK )
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

    \***********************************************************************************/
    uint32_t DeviceProfilerPerformanceCountersINTEL::GetActiveMetricsSetIndex() const
    {
        std::shared_lock lk( m_ActiveMetricSetMutex );
        return m_ActiveMetricsSetIndex;
    }

    /***********************************************************************************\

    Function:
        GetMetricsSets

    Description:
        Retrieve properties of all available metrics sets.

    \***********************************************************************************/
    uint32_t DeviceProfilerPerformanceCountersINTEL::GetMetricsSets(
        uint32_t count,
        VkProfilerPerformanceMetricsSetProperties2EXT* pProperties ) const
    {
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
        Retrieve properties of the selected metrics set.

    \***********************************************************************************/
    void DeviceProfilerPerformanceCountersINTEL::GetMetricsSetProperties(
        uint32_t metricsSetIndex,
        VkProfilerPerformanceMetricsSetProperties2EXT* pProperties ) const
    {
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
        Retrieve properties of all metrics in the selected metrics set.

    \***********************************************************************************/
    uint32_t DeviceProfilerPerformanceCountersINTEL::GetMetricsSetMetricsProperties(
        uint32_t metricsSetIndex,
        uint32_t count,
        VkProfilerPerformanceCounterProperties2EXT* pProperties ) const
    {
        if( metricsSetIndex >= m_MetricsSets.size() )
        {
            return 0;
        }

        const auto& metricsSet = m_MetricsSets[metricsSetIndex];
        const size_t writeCount = std::min<size_t>( count, metricsSet.m_Counters.size() );
        for( size_t i = 0; i < writeCount; ++i )
        {
            FillPerformanceCounterProperties( metricsSet.m_Counters[i], pProperties[i] );
        }

        return static_cast<uint32_t>( metricsSet.m_Counters.size() );
    }

    /***********************************************************************************\

    Function:
        CreateQueryPool

    Description:
        Create query pool for Intel performance query.

    \***********************************************************************************/
    VkResult DeviceProfilerPerformanceCountersINTEL::CreateQueryPool(
        uint32_t queueFamilyIndex,
        uint32_t size,
        VkQueryPool* pQueryPool )
    {
        VkQueryPoolCreateInfoINTEL intelCreateInfo = {};
        intelCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO_INTEL;
        intelCreateInfo.performanceCountersSampling = VK_QUERY_POOL_SAMPLING_MODE_MANUAL_INTEL;

        VkQueryPoolCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        createInfo.pNext = &intelCreateInfo;
        createInfo.queryType = VK_QUERY_TYPE_PERFORMANCE_QUERY_INTEL;
        createInfo.queryCount = size;

        return m_pVulkanDevice->Callbacks.CreateQueryPool(
            m_pVulkanDevice->Handle,
            &createInfo,
            nullptr,
            pQueryPool );
    }

    /***********************************************************************************\

    Function:
        ReadStreamData

    Description:
        Get the stream data for a specific marker.

    \***********************************************************************************/
    bool DeviceProfilerPerformanceCountersINTEL::ReadStreamData(
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

        const uint64_t beginTimestampNs = ConvertGpuTimestampToNanoseconds( beginTimestamp );
        const uint64_t endTimestampNs = ConvertGpuTimestampToNanoseconds( endTimestamp );

        if( endTimestampNs < beginTimestampNs )
        {
            return true;
        }
        else
        {
            while( ( begin != end ) && ( begin->m_GpuTimestamp < beginTimestampNs ) )
                begin++;
            while( ( end != begin ) && ( end - 1 )->m_GpuTimestamp > endTimestampNs )
                end--;
        }

        bool dataComplete = ( end != m_MetricsStreamResults.end() );

        if( begin != end )
        {
            // Adjust timestamps to be relative to the begin timestamp.
            for( auto it = begin; it != end; ++it )
            {
                it->m_GpuTimestamp -= beginTimestampNs;
            }

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
        ParseReport

    Description:
        Convert query data to human-readable form.

    \***********************************************************************************/
    void DeviceProfilerPerformanceCountersINTEL::ParseReport(
        uint32_t metricsSetIndex,
        uint32_t queueFamilyIndex,
        uint32_t reportSize,
        const uint8_t* pReport,
        std::vector<VkProfilerPerformanceCounterResultEXT>& results )
    {
        ParseReport( metricsSetIndex, queueFamilyIndex, reportSize, pReport, results, nullptr );
    }

    /***********************************************************************************\

    Function:
        ParseReport

    Description:
        Convert query data to human-readable form.

    \***********************************************************************************/
    void DeviceProfilerPerformanceCountersINTEL::ParseReport(
        uint32_t metricsSetIndex,
        uint32_t queueFamilyIndex,
        uint32_t reportSize,
        const uint8_t* pReport,
        std::vector<VkProfilerPerformanceCounterResultEXT>& results,
        ReportInformations* pReportInformations )
    {
        const auto& metricsSet = m_MetricsSets[ metricsSetIndex ];

        // Intermediate values used for computations
        thread_local std::vector<MD::TTypedValue_1_0> intermediateValues;
        const uint32_t intermediateValueCount =
            metricsSet.m_pMetricSetParams->MetricsCount +
            metricsSet.m_pMetricSetParams->InformationCount;
        intermediateValues.resize( intermediateValueCount );

        // Convert MDAPI-specific TTypedValue_1_0 to custom VkProfilerMetricEXT
        uint32_t reportCount = 0;

        // Check if there is data, otherwise we'll get integer zero-division
        if( !metricsSet.m_Counters.empty() )
        {
            // Calculate normalized metrics from raw query data
            MD::ECompletionCode cc = metricsSet.m_pMetricSet->CalculateMetrics(
                pReport,
                reportSize,
                intermediateValues.data(),
                intermediateValueCount * sizeof( MD::TTypedValue_1_0 ),
                &reportCount,
                false );

            if( cc != MD::CC_OK || reportCount == 0 )
            {
                // Calculation failed
                results.clear();
                return;
            }
        }

        const size_t resultCount = metricsSet.m_Counters.size();
        results.resize( resultCount );

        for( size_t i = 0; i < resultCount; ++i )
        {
            const Counter& counter = metricsSet.m_Counters[ i ];

            // Get intermediate value for this metric
            assert( counter.m_MetricIndex < intermediateValueCount );
            const MD::TTypedValue_1_0 intermediateValue = intermediateValues[ counter.m_MetricIndex ];

            switch( intermediateValue.ValueType )
            {
            case MD::VALUE_TYPE_FLOAT:
                results[i].float32 = static_cast<float>( intermediateValue.ValueFloat * counter.m_ResultFactor );
                break;

            case MD::VALUE_TYPE_UINT32:
                results[i].uint32 = static_cast<uint32_t>( intermediateValue.ValueUInt32 * counter.m_ResultFactor );
                break;

            case MD::VALUE_TYPE_UINT64:
                results[i].uint64 = static_cast<uint64_t>( intermediateValue.ValueUInt64 * counter.m_ResultFactor );
                break;

            case MD::VALUE_TYPE_BOOL:
                results[i].uint32 = intermediateValue.ValueBool;
                break;

            default:
            case MD::VALUE_TYPE_CSTRING:
                assert( !"PROFILER: Intel MDAPI string metrics not supported!" );
            }
        }

        if( pReportInformations != nullptr )
        {
            // Retrieve report informations
            pReportInformations->m_Reason = intermediateValues[metricsSet.m_ReportReasonInformationIndex].ValueUInt32;
            pReportInformations->m_Value = intermediateValues[metricsSet.m_ValueInformationIndex].ValueUInt32;
            pReportInformations->m_Timestamp = intermediateValues[metricsSet.m_TimestampInformationIndex].ValueUInt64;
        }
    }

    /***********************************************************************************\

    Function:
        ConvertGpuTimestampToNanoseconds

    Description:

    \***********************************************************************************/
    uint64_t DeviceProfilerPerformanceCountersINTEL::ConvertGpuTimestampToNanoseconds( uint64_t gpuTimestamp )
    {
        constexpr uint64_t scGpuTimestampMask32Bits = ( 1ull << 32 ) - 1;
        constexpr uint64_t scGpuTimestampMask56Bits = ( 1ull << 56 ) - 1;

        if( m_GpuTimestampPeriod == 0 )
        {
            return 0;
        }

        if( m_GpuTimestampIs32Bit )
        {
            // Ticks masked to 32bit to get sync with report timestamps.
            return static_cast<uint64_t>( ( gpuTimestamp & scGpuTimestampMask32Bits ) * m_GpuTimestampPeriod );
        }

        // Ticks masked to 56bit to get sync with report timestamps.
        const double gpuTimestampNsHigh = ( ( gpuTimestamp & scGpuTimestampMask56Bits ) >> 32 ) * m_GpuTimestampPeriod;
        const double gpuTimestampNsHighFractionalPart = ( gpuTimestampNsHigh - static_cast<uint64_t>( gpuTimestampNsHigh ) ) * ( scGpuTimestampMask32Bits + 1 );
        const double gpuTimestampNsLow = ( gpuTimestamp & scGpuTimestampMask32Bits ) * m_GpuTimestampPeriod;

        return ( static_cast<uint64_t>( gpuTimestampNsHigh ) << 32 ) + static_cast<uint64_t>( gpuTimestampNsLow + gpuTimestampNsHighFractionalPart );
    }

    /***********************************************************************************\

    Function:
        FindMetricsDiscoveryLibrary

    Description:
        Locate igdmdX.dll in the directory.

    Input:
        searchDirectory

    \***********************************************************************************/
    std::filesystem::path DeviceProfilerPerformanceCountersINTEL::FindMetricsDiscoveryLibrary()
    {
        std::filesystem::path igdmdPath;

    #ifdef WIN32
        // Open registry key with the display adapters.
        HKEY hRegistryKey = NULL;
        if( RegOpenKeyA( HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}", &hRegistryKey ) != ERROR_SUCCESS )
        {
            return igdmdPath;
        }

        char vulkanDriverName[ MAX_PATH ];
        char vulkanDeviceId[ 64 ];

        // Enumerate subkeys.
        for( DWORD dwKeyIndex = 0;
             RegEnumKeyA( hRegistryKey, dwKeyIndex, vulkanDriverName, MAX_PATH ) == ERROR_SUCCESS;
             ++dwKeyIndex )
        {
            // Open device's registry key.
            HKEY hDeviceRegistryKey = NULL;
            if( RegOpenKeyA( hRegistryKey, vulkanDriverName, &hDeviceRegistryKey ) != ERROR_SUCCESS )
            {
                continue;
            }

            // Read vendor and device ID from the registry.
            DWORD vulkanDeviceIdLength = sizeof( vulkanDeviceId );
            if( RegGetValueA( hDeviceRegistryKey, NULL, "MatchingDeviceId", RRF_RT_REG_SZ, NULL, vulkanDeviceId, &vulkanDeviceIdLength ) != ERROR_SUCCESS )
            {
                RegCloseKey( hDeviceRegistryKey );
                continue;
            }

            uint32_t vendorId, deviceId;
            sscanf_s( vulkanDeviceId, "PCI\\VEN_%04x&DEV_%04x", &vendorId, &deviceId );
            if( (vendorId != m_pVulkanDevice->pPhysicalDevice->Properties.vendorID) ||
                (deviceId != m_pVulkanDevice->pPhysicalDevice->Properties.deviceID) )
            {
                RegCloseKey( hDeviceRegistryKey );
                continue;
            }

            // Get VulkanDriverName value.
            DWORD vulkanDriverNameLength = sizeof( vulkanDriverName );
            if( RegGetValueA( hDeviceRegistryKey, NULL, "VulkanDriverName", RRF_RT_REG_SZ, NULL, vulkanDriverName, &vulkanDriverNameLength ) != ERROR_SUCCESS )
            {
                RegCloseKey( hDeviceRegistryKey );
                continue;
            }

            // Make sure the string is null-terminated.
            vulkanDriverNameLength = std::min<DWORD>( vulkanDriverNameLength, MAX_PATH - 1 );
            vulkanDriverName[ vulkanDriverNameLength ] = 0;

            // Parse JSON file.
            nlohmann::json icd = nlohmann::json::parse( std::ifstream( vulkanDriverName ) );

            if( icd[ "file_format_version" ] == "1.0.0" )
            {
                // Get path to the DLL.
                std::filesystem::path vulkanModulePath = icd[ "ICD" ][ "library_path" ];

                if( !vulkanModulePath.is_absolute() )
                {
                    // library_path may be relative to the JSON.
                    vulkanModulePath = std::filesystem::path( vulkanDriverName ).parent_path() / vulkanModulePath;
                    vulkanModulePath = vulkanModulePath.lexically_normal();
                }

                // Check if the DLL is loaded.
                if( GetModuleHandleA( vulkanModulePath.string().c_str() ) != NULL )
                {
                    igdmdPath = ProfilerPlatformFunctions::FindFile( vulkanModulePath.parent_path(), PROFILER_METRICS_DLL_INTEL );
                }
            }
            
            RegCloseKey( hDeviceRegistryKey );

            // Exit enumeration if the DLL has been found.
            if( !igdmdPath.empty() )
            {
                break;
            }
        }

        RegCloseKey( hRegistryKey );

    #else
        // On Linux the library is distributed with the profiler.
        igdmdPath = ProfilerPlatformFunctions::GetLayerDir() / PROFILER_METRICS_DLL_INTEL;
    #endif

        // Normalize the path.
        igdmdPath = igdmdPath.lexically_normal();

        return igdmdPath;
    }

    /***********************************************************************************\

    Function:
        LoadMetricsDiscoveryLibrary

    Description:

    \***********************************************************************************/
    bool DeviceProfilerPerformanceCountersINTEL::LoadMetricsDiscoveryLibrary()
    {
        // Find location of metrics discovery library.
        const std::filesystem::path mdDllPath = FindMetricsDiscoveryLibrary();

        if( !mdDllPath.empty() && std::filesystem::exists( mdDllPath ) )
        {
            m_MDLibraryHandle = ProfilerPlatformFunctions::OpenLibrary( mdDllPath.string().c_str() );
        }

        return m_MDLibraryHandle != nullptr;
    }

    /***********************************************************************************\

    Function:
        UnloadMetricsDiscoveryLibrary

    Description:

    \***********************************************************************************/
    void DeviceProfilerPerformanceCountersINTEL::UnloadMetricsDiscoveryLibrary()
    {
        if( m_MDLibraryHandle != nullptr )
        {
            ProfilerPlatformFunctions::CloseLibrary( m_MDLibraryHandle );
            m_MDLibraryHandle = nullptr;
        }
    }

    /***********************************************************************************\

    Function:
        OpenMetricsDevice

    Description:

    \***********************************************************************************/
    bool DeviceProfilerPerformanceCountersINTEL::OpenMetricsDevice()
    {
        assert( m_pDevice == nullptr );

        auto pfnOpenAdapterGroup = ProfilerPlatformFunctions::GetProcAddress<MD::OpenAdapterGroup_fn>( m_MDLibraryHandle, "OpenAdapterGroup" );
        if( pfnOpenAdapterGroup )
        {
            // Create adapter group.
            MD::IAdapterGroupLatest* pAdapterGroup = nullptr;
            MD::ECompletionCode result = pfnOpenAdapterGroup( &pAdapterGroup );

            if( result != MD::CC_OK )
            {
                return false;
            }

            m_pAdapterGroup = pAdapterGroup;

            // Verify that the adapter group supports at least version 1.6 to use IAdapter_1_6.
            auto* pAdapterGroupParams = m_pAdapterGroup->GetParams();
            if( ( pAdapterGroupParams->Version.MajorNumber != m_RequiredVersionMajor ) ||
                ( pAdapterGroupParams->Version.MinorNumber < m_MinRequiredAdapterGroupVersionMinor ) )
            {
                return false;
            }

            // Find adapter matching the current device.
            const uint32_t adapterCount = pAdapterGroupParams->AdapterCount;
            for( uint32_t i = 0; i < adapterCount; ++i )
            {
                auto* pAdapter = m_pAdapterGroup->GetAdapter( i );
                auto* pAdapterParams = pAdapter->GetParams();

                // TODO: Consider using PCI BDF address.
                if( ( pAdapterParams->VendorId == m_pVulkanDevice->pPhysicalDevice->Properties.vendorID ) &&
                    ( pAdapterParams->DeviceId == m_pVulkanDevice->pPhysicalDevice->Properties.deviceID ) )
                {
                    m_pAdapter = pAdapter;
                    break;
                }
            }

            if( !m_pAdapter )
            {
                return false;
            }

            // Reset the adapter to clear any previous state.
            m_pAdapter->Reset();

            // Open device for the selected adapter.
            MD::IMetricsDevice_1_5* pDevice = nullptr;
            result = m_pAdapter->OpenMetricsDevice( &pDevice );

            if( result != MD::CC_OK )
            {
                return false;
            }

            m_pDevice = pDevice;
            m_pDeviceParams = m_pDevice->GetParams();

            // Check if the required version is supported by the current driver.
            if( ( m_pDeviceParams->Version.MajorNumber != m_RequiredVersionMajor ) ||
                ( m_pDeviceParams->Version.MinorNumber < m_MinRequiredVersionMinor ) )
            {
                return false;
            }

            return true;
        }

        auto pfnOpenMetricsDevice = ProfilerPlatformFunctions::GetProcAddress<MD::OpenMetricsDevice_fn>( m_MDLibraryHandle, "OpenMetricsDevice" );
        if( pfnOpenMetricsDevice )
        {
            // Create metrics device.
            MD::IMetricsDeviceLatest* pDevice = nullptr;
            MD::ECompletionCode result = pfnOpenMetricsDevice( &pDevice );

            if( result != MD::CC_OK )
            {
                return false;
            }

            m_pDevice = pDevice;
            m_pDeviceParams = m_pDevice->GetParams();

            // Check if the required version is supported by the current driver.
            if( ( m_pDeviceParams->Version.MajorNumber != m_RequiredVersionMajor) ||
                ( m_pDeviceParams->Version.MinorNumber < m_MinRequiredVersionMinor ) )
            {
                return false;
            }

            return true;
        }

        // Required entry points not found.
        return false;
    }

    /***********************************************************************************\

    Function:
        CloseMetricsDevice

    Description:

    \***********************************************************************************/
    void DeviceProfilerPerformanceCountersINTEL::CloseMetricsDevice()
    {
        if( m_pAdapterGroup )
        {
            if( m_pDevice )
            {
                // Adapter must not be null if device has been successfully opened.
                assert( m_pAdapter );

                m_pAdapter->CloseMetricsDevice( static_cast<MD::IMetricsDevice_1_5*>( m_pDevice ) );

                m_pDevice = nullptr;
                m_pDeviceParams = nullptr;
            }

            m_pAdapter = nullptr;

            m_pAdapterGroup->Close();
            m_pAdapterGroup = nullptr;
        }

        if( m_pDevice )
        {
            auto pfnCloseMetricsDevice = ProfilerPlatformFunctions::GetProcAddress<MD::CloseMetricsDevice_fn>( m_MDLibraryHandle, "CloseMetricsDevice" );

            // Close function should be available since we have successfully created device
            // using other function from the same library
            assert( pfnCloseMetricsDevice );

            // Destroy metrics device
            pfnCloseMetricsDevice( static_cast<MD::IMetricsDeviceLatest*>( m_pDevice ) );

            m_pDevice = nullptr;
            m_pDeviceParams = nullptr;
        }
    }

    /***********************************************************************************\

    Function:
        MetricsStreamCollectionThreadProc

    Description:

    \***********************************************************************************/
    void DeviceProfilerPerformanceCountersINTEL::MetricsStreamCollectionThreadProc()
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
    size_t DeviceProfilerPerformanceCountersINTEL::CollectMetricsStreamSamples()
    {
        thread_local std::vector<VkProfilerPerformanceCounterResultEXT> parsedResults;

        // Don't switch the active metrics set while reading the stream.
        std::shared_lock lk( m_ActiveMetricSetMutex );

        const uint32_t activeMetricsSetIndex = m_ActiveMetricsSetIndex;
        if( activeMetricsSetIndex == UINT32_MAX )
        {
            // No active metrics set, nothing to read.
            return 0;
        }

        const MetricsSet& metricsSet = m_MetricsSets[activeMetricsSetIndex];
        const size_t reportSize = metricsSet.m_pMetricSetParams->RawReportSize;
        uint32_t reportCount = m_MetricsStreamMaxReportCount;

        // Make sure the buffer is large enough.
        const size_t requiredBufferSize = reportSize * reportCount;
        if( m_MetricsStreamDataBuffer.size() < requiredBufferSize )
        {
            m_MetricsStreamDataBuffer.resize( requiredBufferSize );
        }

        MD::TCompletionCode cc = m_pConcurrentGroup->ReadIoStream(
            &reportCount,
            m_MetricsStreamDataBuffer.data(),
            MD::IO_READ_FLAG_DROP_OLD_REPORTS );

        const uint64_t cpuTimestamp = m_CpuTimestampCounter.GetCurrentValue();

        // Unlock the active metrics set mutex while parsing the reports.
        // The function is thread-safe and keeping it would block SetActiveMetricsSet calls.
        lk.unlock();

        if( cc == MD::CC_OK || cc == MD::CC_READ_PENDING )
        {
            const uint8_t* pReport = reinterpret_cast<const uint8_t*>( m_MetricsStreamDataBuffer.data() );

            // Parse each report
            for( uint32_t i = 0; i < reportCount; ++i )
            {
                ReportInformations informations;
                ParseReport(
                    activeMetricsSetIndex,
                    VK_QUEUE_FAMILY_IGNORED,
                    reportSize,
                    pReport,
                    parsedResults,
                    &informations );

                // Save the parsed results.
                if( !parsedResults.empty() &&
                    ( informations.m_Timestamp != m_MetricsStreamLastResultTimestamp ) )
                {
                    std::scoped_lock resultsLock( m_MetricsStreamResultsMutex );
                    m_MetricsStreamResults.push_back( {
                        informations.m_Timestamp,
                        cpuTimestamp,
                        activeMetricsSetIndex,
                        parsedResults } );

                    m_MetricsStreamLastResultTimestamp = informations.m_Timestamp;
                }

                pReport += reportSize;
            }
        }

        return reportCount;
    }

    /***********************************************************************************\

    Function:
        FreeUnusedMetricsStreamSamples

    Description:

    \***********************************************************************************/
    void DeviceProfilerPerformanceCountersINTEL::FreeUnusedMetricsStreamSamples()
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
        FillPerformanceMetricsSetProperties

    Description:
        Fill performance metrics set properties structure from MetricsDiscovery structures.

    \***********************************************************************************/
    void DeviceProfilerPerformanceCountersINTEL::FillPerformanceMetricsSetProperties(
        const MetricsSet& set,
        VkProfilerPerformanceMetricsSetProperties2EXT& properties )
    {
        assert( properties.sType == VK_STRUCTURE_TYPE_PROFILER_PERFORMANCE_METRICS_SET_PROPERTIES_2_EXT );
        assert( properties.pNext == nullptr );

        ProfilerStringFunctions::CopyString(
            properties.name,
            set.m_pMetricSetParams->ShortName, -1 );

        properties.metricsCount = static_cast<uint32_t>( set.m_Counters.size() );
    }

    /***********************************************************************************\

    Function:
        FillPerformanceCounterProperties

    Description:
        Fill performance metric properties structure from MetricsDiscovery structures.

    \***********************************************************************************/
    void DeviceProfilerPerformanceCountersINTEL::FillPerformanceCounterProperties(
        const Counter& counter,
        VkProfilerPerformanceCounterProperties2EXT& properties )
    {
        assert( properties.sType == VK_STRUCTURE_TYPE_PROFILER_PERFORMANCE_COUNTER_PROPERTIES_2_EXT );
        assert( properties.pNext == nullptr );

        ProfilerStringFunctions::CopyString(
            properties.shortName,
            counter.m_pMetricParams->ShortName, -1 );

        ProfilerStringFunctions::CopyString(
            properties.category,
            counter.m_pMetricParams->GroupName, -1 );

        ProfilerStringFunctions::CopyString(
            properties.description,
            counter.m_pMetricParams->LongName, -1 );

        properties.flags = 0;
        properties.unit = counter.m_Unit;
        properties.storage = counter.m_Storage;

        memcpy( properties.uuid, counter.m_UUID, VK_UUID_SIZE );
    }

    /***********************************************************************************\

    Function:
        TranslateUnit

    Description:
        Get unit enum value from unit string.

    \***********************************************************************************/
    bool DeviceProfilerPerformanceCountersINTEL::TranslateStorage(
        MetricsDiscovery::EMetricResultType resultType,
        VkProfilerPerformanceCounterStorageEXT& storage )
    {
        switch( resultType )
        {
        case MetricsDiscovery::RESULT_UINT32:
            storage = VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_UINT32_EXT;
            return true;

        case MetricsDiscovery::RESULT_UINT64:
            storage = VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_UINT64_EXT;
            return true;

        case MetricsDiscovery::RESULT_BOOL:
            storage = VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_UINT32_EXT;
            return true;

        case MetricsDiscovery::RESULT_FLOAT:
            storage = VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_FLOAT32_EXT;
            return true;
        }

        return false;
    }

    /***********************************************************************************\

    Function:
        TranslateUnit

    Description:
        Get unit enum value from unit string.

    \***********************************************************************************/
    bool DeviceProfilerPerformanceCountersINTEL::TranslateUnit(
        const char* pUnit,
        double& factor,
        VkProfilerPerformanceCounterUnitEXT& unit )
    {
        // Time
        if( std::strcmp( pUnit, "ns" ) == 0 )
        {
            unit = VK_PROFILER_PERFORMANCE_COUNTER_UNIT_NANOSECONDS_EXT;
            factor = 1.0;
            return true;
        }

        // Cycles
        if( std::strcmp( pUnit, "cycles" ) == 0 )
        {
            unit = VK_PROFILER_PERFORMANCE_COUNTER_UNIT_CYCLES_EXT;
            factor = 1.0;
            return true;
        }

        // Frequency
        if( std::strcmp( pUnit, "MHz" ) == 0 )
        {
            unit = VK_PROFILER_PERFORMANCE_COUNTER_UNIT_HERTZ_EXT;
            factor = 1'000'000.0;
            return true;
        }

        if( std::strcmp( pUnit, "kHz" ) == 0 )
        {
            unit = VK_PROFILER_PERFORMANCE_COUNTER_UNIT_HERTZ_EXT;
            factor = 1'000.0;
            return true;
        }

        if( std::strcmp( pUnit, "Hz" ) == 0 )
        {
            unit = VK_PROFILER_PERFORMANCE_COUNTER_UNIT_HERTZ_EXT;
            factor = 1.0;
            return true;
        }

        // Percents
        if( std::strcmp( pUnit, "percent" ) == 0 )
        {
            unit = VK_PROFILER_PERFORMANCE_COUNTER_UNIT_PERCENTAGE_EXT;
            factor = 1.0;
            return true;
        }

        // Default
        unit = VK_PROFILER_PERFORMANCE_COUNTER_UNIT_GENERIC_EXT;
        factor = 1.0;
        return true;
    }
}

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

        // Setup sampling mode
        if( result == VK_SUCCESS )
        {
            switch( config.m_PerformanceQueryMode )
            {
            case performance_query_mode_t::query:
                m_SamplingMode = VK_PROFILER_PERFORMANCE_COUNTERS_SAMPLING_MODE_QUERY_EXT;
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

        // Enumerate available metric sets
        if( result == VK_SUCCESS )
        {
            const uint32_t oaMetricSetCount = m_pConcurrentGroupParams->MetricSetsCount;
            assert( oaMetricSetCount > 0 );

            uint32_t defaultMetricsSetIndex = UINT32_MAX;
            const char* pDefaultMetricsSetName = config.m_DefaultMetricsSet.c_str();

            for( uint32_t setIndex = 0; setIndex < oaMetricSetCount; ++setIndex )
            {
                MetricsSet set = {};
                set.m_pMetricSet = m_pConcurrentGroup->GetMetricSet( setIndex );
                set.m_pMetricSet->SetApiFiltering( MD::API_TYPE_VULKAN );

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

        m_MetricsSets.clear();

        m_ActiveMetricsSetIndex = UINT32_MAX;

        m_PerformanceApiInitialized = false;
        m_PerformanceApiConfiguration = VK_NULL_HANDLE;
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

        if( m_ActiveMetricsSetIndex == UINT32_MAX )
        {
            // No configuration to set (not an error as the performance counters are optional).
            return VK_SUCCESS;
        }

        return m_pVulkanDevice->Callbacks.QueueSetPerformanceConfigurationINTEL(
            queue,
            m_PerformanceApiConfiguration );
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

        // Release the current performance configuration.
        if( m_PerformanceApiConfiguration )
        {
            m_pVulkanDevice->Callbacks.ReleasePerformanceConfigurationINTEL(
                m_pVulkanDevice->Handle,
                m_PerformanceApiConfiguration );

            m_PerformanceApiConfiguration = VK_NULL_HANDLE;
            m_ActiveMetricsSetIndex = UINT32_MAX;
        }

        // Get the new metrics set object.
        auto& metricsSet = m_MetricsSets[ metricsSetIndex ];

        // Activate only metrics supported by Vulkan driver.
        if( metricsSet.m_pMetricSet->SetApiFiltering( MD::API_TYPE_VULKAN ) != MD::CC_OK )
        {
            assert( false );
            return VK_ERROR_INITIALIZATION_FAILED;
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

            if( cc != MD::CC_OK )
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

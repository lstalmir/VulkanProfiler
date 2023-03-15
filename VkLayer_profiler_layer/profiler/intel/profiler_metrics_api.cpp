// Copyright (c) 2019-2023 Lukasz Stalmirski
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

#include "profiler_metrics_api.h"
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
#endif

#ifdef WIN32
#ifdef _WIN64
#define PROFILER_METRICS_DLL_INTEL "igdmd64.dll"
#else
#define PROFILER_METRICS_DLL_INTEL "igdmd32.dll"
#endif
#else // LINUX
#define PROFILER_METRICS_DLL_INTEL "libmd.so"
#endif

#include <nlohmann/json.hpp>
#include <fstream>

namespace MD = MetricsDiscovery;

namespace Profiler
{
    /***********************************************************************************\

    Function:
        ProfilerMetrics_INTEL

    Description:
        Constructor.

    \***********************************************************************************/
    ProfilerMetricsApi_INTEL::ProfilerMetricsApi_INTEL()
        : m_pDevice( nullptr )
        , m_pDeviceParams( nullptr )
        , m_pConcurrentGroup( nullptr )
        , m_pConcurrentGroupParams( nullptr )
        , m_MetricsSets()
        , m_ActiveMetricSetMutex()
        , m_ActiveMetricsSetIndex( UINT32_MAX )
        #ifdef WIN32
        , m_hMDDll( nullptr )
        #endif
    {
    }

    /***********************************************************************************\

    Function:
        Initialize

    Description:

    \***********************************************************************************/
    VkResult ProfilerMetricsApi_INTEL::Initialize(
        struct VkDevice_Object* pDevice )
    {
        // Returning errors from this function is fine - it is optional feature and will be 
        // disabled when initialization fails. If these errors were moved later (to other functions)
        // whole layer could crash.

        if( !LoadMetricsDiscoveryLibrary( pDevice ) )
            return VK_ERROR_INCOMPATIBLE_DRIVER;

        if( !OpenMetricsDevice() )
            return VK_ERROR_INITIALIZATION_FAILED;

        assert( m_pDevice );
        assert( m_pDeviceParams );

        // Iterate over all concurrent groups to find OA
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
                return VK_ERROR_INCOMPATIBLE_DRIVER;
            }
        }

        // Enumerate available metric sets
        const uint32_t oaMetricSetCount = m_pConcurrentGroupParams->MetricSetsCount;

        uint32_t defaultMetricsSetIndex = UINT32_MAX;

        for( uint32_t setIndex = 0; setIndex < oaMetricSetCount; ++setIndex )
        {
            MD::IMetricSet_1_1* pMetricSet = m_pConcurrentGroup->GetMetricSet( setIndex );

            // Temporarily activate the set.
            pMetricSet->SetApiFiltering( MD::API_TYPE_VULKAN );

            if( pMetricSet->Activate() != MD::CC_OK )
            {
                // Activation failed, skip the set.
                continue;
            }

            auto& metricsSet = m_MetricsSets.emplace_back();
            metricsSet.m_pMetricSet = pMetricSet;
            metricsSet.m_pMetricSetParams = pMetricSet->GetParams();

            // Construct metric properties
            for( uint32_t metricIndex = 0; metricIndex < metricsSet.m_pMetricSetParams->MetricsCount; ++metricIndex )
            {
                MD::IMetric_1_0* pMetric = metricsSet.m_pMetricSet->GetMetric( metricIndex );
                MD::TMetricParams_1_0* pMetricParams = pMetric->GetParams();

                VkProfilerPerformanceCounterPropertiesEXT counterProperties = {};
                ProfilerStringFunctions::CopyString( counterProperties.shortName, pMetricParams->ShortName, -1 );
                ProfilerStringFunctions::CopyString( counterProperties.description, pMetricParams->LongName, -1 );

                switch( pMetricParams->ResultType )
                {
                case MD::RESULT_FLOAT:
                    counterProperties.storage = VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_FLOAT32_EXT;
                    break;

                case MD::RESULT_UINT32:
                    counterProperties.storage = VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_UINT32_EXT;
                    break;

                case MD::RESULT_UINT64:
                    counterProperties.storage = VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_UINT64_EXT;
                    break;

                case MD::RESULT_BOOL:
                    counterProperties.storage = VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_UINT32_EXT;
                    break;

                default:
                    assert( !"PROFILER: Intel MDAPI metric result type not supported" );
                }

                // Factor applied to the output
                double metricFactor = 1.0;
                counterProperties.unit = TranslateUnit( pMetricParams->MetricResultUnits, metricFactor );

                metricsSet.m_MetricsProperties.push_back( counterProperties );
                metricsSet.m_MetricFactors.push_back( metricFactor );
            }

            // Deactivate the set.
            pMetricSet->Deactivate();

            // Find default metrics set index.
            if( (defaultMetricsSetIndex == UINT32_MAX) &&
                (strcmp( metricsSet.m_pMetricSetParams->SymbolName, "RenderBasic" ) == 0) )
            {
                defaultMetricsSetIndex = setIndex;
            }
        }

        if( defaultMetricsSetIndex != UINT32_MAX )
        {
            return SetActiveMetricsSet( defaultMetricsSetIndex );
        }

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:

    \***********************************************************************************/
    void ProfilerMetricsApi_INTEL::Destroy()
    {
        CloseMetricsDevice();
        UnloadMetricsDiscoveryLibrary();
    }

    /***********************************************************************************\

    Function:
        IsAvailable

    Description:

    \***********************************************************************************/
    bool ProfilerMetricsApi_INTEL::IsAvailable() const
    {
        std::shared_lock lk( m_ActiveMetricSetMutex );
        return m_pDevice != nullptr &&
            m_ActiveMetricsSetIndex != UINT32_MAX;
    }

    /***********************************************************************************\

    Function:
        GetReportSize

    Description:

    \***********************************************************************************/
    size_t ProfilerMetricsApi_INTEL::GetReportSize( uint32_t metricsSetIndex ) const
    {
        return m_MetricsSets[ metricsSetIndex ].m_pMetricSetParams->QueryReportSize;
    }

    /***********************************************************************************\

    Function:
        GetMetricCount

    Description:
        Get number of HW metrics exposed by this extension.

    \***********************************************************************************/
    size_t ProfilerMetricsApi_INTEL::GetMetricsCount( uint32_t metricsSetIndex ) const
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
    size_t ProfilerMetricsApi_INTEL::GetMetricsSetCount() const
    {
        return m_MetricsSets.size();
    }

    /***********************************************************************************\

    Function:
        GetMetricsSets

    Description:

    \***********************************************************************************/
    std::vector<VkProfilerPerformanceMetricsSetPropertiesEXT> ProfilerMetricsApi_INTEL::GetMetricsSets() const
    {
        std::vector<VkProfilerPerformanceMetricsSetPropertiesEXT> metricsSetsProperties;
        metricsSetsProperties.reserve( m_MetricsSets.size() );

        for( const auto& metricsSet : m_MetricsSets )
        {
            auto& metricsSetProperties = metricsSetsProperties.emplace_back();
            metricsSetProperties.metricsCount = metricsSet.m_pMetricSetParams->MetricsCount;
            
            ProfilerStringFunctions::CopyString( metricsSetProperties.name, metricsSet.m_pMetricSetParams->ShortName, -1 );
        }

        return metricsSetsProperties;
    }

    /***********************************************************************************\

    Function:
        SetActiveMetricsSet

    Description:

    \***********************************************************************************/
    VkResult ProfilerMetricsApi_INTEL::SetActiveMetricsSet( uint32_t metricsSetIndex )
    {
        std::unique_lock lk( m_ActiveMetricSetMutex );

        // Disable the current active metrics set.
        if( m_ActiveMetricsSetIndex != UINT32_MAX )
        {
            if( m_MetricsSets[ m_ActiveMetricsSetIndex ].m_pMetricSet->Deactivate() != MD::CC_OK )
            {
                assert( false );
                return VK_ERROR_NOT_PERMITTED_EXT;
            }

            m_ActiveMetricsSetIndex = UINT32_MAX;
        }

        // Check if the metric set is available
        if( metricsSetIndex >= m_MetricsSets.size() )
        {
            assert( false );
            return VK_ERROR_VALIDATION_FAILED_EXT;
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

        m_ActiveMetricsSetIndex = metricsSetIndex;

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        GetActiveMetricsSetIndex

    Description:

    \***********************************************************************************/
    uint32_t ProfilerMetricsApi_INTEL::GetActiveMetricsSetIndex() const
    {
        return m_ActiveMetricsSetIndex;
    }

    /***********************************************************************************\

    Function:
        GetMetricsProperties

    Description:
        Get detailed description of each reported metric.
        Metrics must appear in the same order as in returned reports.

    \***********************************************************************************/
    std::vector<VkProfilerPerformanceCounterPropertiesEXT> ProfilerMetricsApi_INTEL::GetMetricsProperties(
        uint32_t metricsSetIndex ) const
    {
        // Check if the metrics set is available.
        if (metricsSetIndex >= m_MetricsSets.size())
        {
            return {};
        }

        return m_MetricsSets[ metricsSetIndex ].m_MetricsProperties;
    }

    /***********************************************************************************\

    Function:
        ParseReport

    Description:
        Convert query data to human-readable form.

    \***********************************************************************************/
    void ProfilerMetricsApi_INTEL::ParseReport(
        uint32_t                                            metricsSetIndex,
        ProfilerMetricsReport_INTEL&                        report,
        std::vector<VkProfilerPerformanceCounterResultEXT>& results )
    {
        const auto& metricsSet = m_MetricsSets[ metricsSetIndex ];

        // Convert MDAPI-specific TTypedValue_1_0 to custom VkProfilerMetricEXT
        results.clear();
        report.m_IntermediateValues.resize(
            metricsSet.m_pMetricSetParams->MetricsCount +
            metricsSet.m_pMetricSetParams->InformationCount );

        uint32_t reportCount = 0;

        // Check if there is data, otherwise we'll get integer zero-division
        if( metricsSet.m_pMetricSetParams->MetricsCount > 0 )
        {
            // Calculate normalized metrics from raw query data
            MD::ECompletionCode cc = metricsSet.m_pMetricSet->CalculateMetrics(
                reinterpret_cast<const unsigned char*>( report.m_QueryResult.data() ),
                static_cast<uint32_t>( report.m_QueryResult.size() ),
                report.m_IntermediateValues.data(),
                static_cast<uint32_t>( report.m_IntermediateValues.size() * sizeof( MD::TTypedValue_1_0 ) ),
                &reportCount,
                false );

            assert( cc == MD::CC_OK );
        }

        for( uint32_t i = 0; i < metricsSet.m_pMetricSetParams->MetricsCount; ++i )
        {
            // Metric type information is stored in metric properties to reduce memory transaction overhead
            VkProfilerPerformanceCounterResultEXT parsedMetric = {};

            // Const factor applied to the metric
            const double factor = metricsSet.m_MetricFactors[ i ];

            switch( report.m_IntermediateValues[ i ].ValueType )
            {
            default:
            case MD::VALUE_TYPE_FLOAT:
                parsedMetric.float32 = static_cast<float>(report.m_IntermediateValues[ i ].ValueFloat * factor);
                break;

            case MD::VALUE_TYPE_UINT32:
                parsedMetric.uint32 = static_cast<uint32_t>(report.m_IntermediateValues[ i ].ValueUInt32 * factor);
                break;

            case MD::VALUE_TYPE_UINT64:
                parsedMetric.uint64 = static_cast<uint64_t>(report.m_IntermediateValues[ i ].ValueUInt64 * factor);
                break;

            case MD::VALUE_TYPE_BOOL:
                parsedMetric.uint32 = report.m_IntermediateValues[ i ].ValueBool;
                break;

            case MD::VALUE_TYPE_CSTRING:
                assert( !"PROFILER: Intel MDAPI string metrics not supported!" );
            }

            results.push_back( parsedMetric );
        }

        // This must match every time
        assert( results.size() == metricsSet.m_MetricsProperties.size() );
    }

    #ifdef WIN32
    /***********************************************************************************\

    Function:
        FindMetricsDiscoveryLibrary

    Description:
        Locate igdmdX.dll in the directory.

    Input:
        searchDirectory

    \***********************************************************************************/
    std::filesystem::path ProfilerMetricsApi_INTEL::FindMetricsDiscoveryLibrary(
        struct VkDevice_Object* pDevice )
    {
        std::filesystem::path igdmdPath;

        // Open registry key with the display adapters.
        HKEY hRegistryKey = NULL;
        if( RegOpenKeyA( HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}", &hRegistryKey ) != ERROR_SUCCESS )
        {
            return igdmdPath;
        }

        char vulkanDriverName[ MAX_PATH ];
        char vulkanDeviceId[ 64 ];
        char displayAdapterIndex[ 8 ];

        for( int i = 0; igdmdPath.empty(); ++i )
        {
            sprintf_s( displayAdapterIndex, "%04d", i );

            // Open device's registry key.
            HKEY hDeviceRegistryKey = NULL;
            if( RegOpenKeyA( hRegistryKey, displayAdapterIndex, &hDeviceRegistryKey ) != ERROR_SUCCESS )
            {
                break;
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
            if( (vendorId != pDevice->pPhysicalDevice->Properties.vendorID) ||
                (deviceId != pDevice->pPhysicalDevice->Properties.deviceID) )
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
        }

        RegCloseKey( hRegistryKey );

        return igdmdPath;
    }
    #endif

    /***********************************************************************************\

    Function:
        LoadMetricsDiscoveryLibrary

    Description:

    \***********************************************************************************/
    bool ProfilerMetricsApi_INTEL::LoadMetricsDiscoveryLibrary(
        struct VkDevice_Object* pDevice )
    {
        #ifdef WIN32
        // Load library from driver store
        char pSystemDirectory[ MAX_PATH ];
        GetSystemDirectoryA( pSystemDirectory, MAX_PATH );

        // Find location of igdmdX.dll
        const std::filesystem::path mdDllPath = FindMetricsDiscoveryLibrary( pDevice );

        if( !mdDllPath.empty() )
        {
            // Load metrics discovery library
            m_hMDDll = LoadLibraryA( mdDllPath.string().c_str() );

            return m_hMDDll != nullptr;
        }
        return false;
        #else
        return true;
        #endif
    }

    /***********************************************************************************\

    Function:
        UnloadMetricsDiscoveryLibrary

    Description:

    \***********************************************************************************/
    void ProfilerMetricsApi_INTEL::UnloadMetricsDiscoveryLibrary()
    {
        #ifdef WIN32
        if( m_hMDDll )
        {
            FreeLibrary( m_hMDDll );
            m_hMDDll = nullptr;
        }
        #endif
    }

    /***********************************************************************************\

    Function:
        OpenMetricsDevice

    Description:

    \***********************************************************************************/
    bool ProfilerMetricsApi_INTEL::OpenMetricsDevice()
    {
        assert( m_pDevice == nullptr );

        MD::OpenMetricsDevice_fn pfnOpenMetricsDevice = nullptr;

        #ifdef WIN32
        pfnOpenMetricsDevice = reinterpret_cast<MD::OpenMetricsDevice_fn>(
            GetProcAddress( m_hMDDll, "OpenMetricsDevice" ));
        #endif

        if( pfnOpenMetricsDevice )
        {
            // Create metrics device
            MD::IMetricsDeviceLatest* pDevice = nullptr;
            MD::ECompletionCode result = pfnOpenMetricsDevice( &pDevice );

            if( result == MD::CC_OK )
            {
                // Get device parameters
                m_pDevice = pDevice;
                m_pDeviceParams = m_pDevice->GetParams();

                // Check if the required version is supported by the current driver.
                if( ( m_pDeviceParams->Version.MajorNumber != m_RequiredVersionMajor) ||
                    ( m_pDeviceParams->Version.MinorNumber < m_MinRequiredVersionMinor ) )
                {
                    CloseMetricsDevice();

                    result = MD::CC_ERROR_NOT_SUPPORTED;
                }
            }

            return result == MD::CC_OK;
        }

        return false;
    }

    /***********************************************************************************\

    Function:
        CloseMetricsDevice

    Description:

    \***********************************************************************************/
    void ProfilerMetricsApi_INTEL::CloseMetricsDevice()
    {
        if( m_pDevice )
        {
            MD::CloseMetricsDevice_fn pfnCloseMetricsDevice = nullptr;

            #ifdef WIN32
            pfnCloseMetricsDevice = reinterpret_cast<MD::CloseMetricsDevice_fn>(
                GetProcAddress( m_hMDDll, "OpenMetricsDevice" ));
            #endif

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
        TranslateUnit

    Description:
        Get unit enum value from unit string.

    \***********************************************************************************/
    VkProfilerPerformanceCounterUnitEXT ProfilerMetricsApi_INTEL::TranslateUnit( const char* pUnit, double& factor )
    {
        // Time
        if( std::strcmp( pUnit, "ns" ) == 0 )
        {
            return VK_PROFILER_PERFORMANCE_COUNTER_UNIT_NANOSECONDS_EXT;
        }

        // Cycles
        if( std::strcmp( pUnit, "cycles" ) == 0 )
        {
            return VK_PROFILER_PERFORMANCE_COUNTER_UNIT_CYCLES_EXT;
        }

        // Frequency
        if( std::strcmp( pUnit, "MHz" ) == 0 ||
            std::strcmp( pUnit, "kHz" ) == 0 ||
            std::strcmp( pUnit, "Hz" ) == 0 )
        {
            if( std::strcmp( pUnit, "MHz" ) == 0 )
                factor = 1'000'000.0;

            if( std::strcmp( pUnit, "kHz" ) == 0 )
                factor = 1'000.0;

            return VK_PROFILER_PERFORMANCE_COUNTER_UNIT_HERTZ_EXT;
        }

        // Percents
        if( std::strcmp( pUnit, "percent" ) == 0 )
        {
            return VK_PROFILER_PERFORMANCE_COUNTER_UNIT_PERCENTAGE_EXT;
        }

        // Default
        return VK_PROFILER_PERFORMANCE_COUNTER_UNIT_GENERIC_EXT;
    }
}

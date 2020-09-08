#include "profiler_metrics_api.h"
#include "profiler/profiler_helpers.h"

#ifndef _DEBUG
#define NDEBUG
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
        , m_pActiveMetricSet( nullptr )
        , m_pActiveMetricSetParams( nullptr )
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
    VkResult ProfilerMetricsApi_INTEL::Initialize()
    {
        // Returning errors from this function is fine - it is optional feature and will be 
        // disabled when initialization fails. If these errors were moved later (to other functions)
        // whole layer could crash.

        if( !LoadMetricsDiscoveryLibrary() )
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
                MD::IConcurrentGroup_1_5* pConcurrentGroup = m_pDevice->GetConcurrentGroup( i );
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

        // Find RenderBasic metric set
        {
            const uint32_t oaMetricSetCount = m_pConcurrentGroupParams->MetricSetsCount;

            for( uint32_t i = 0; i < oaMetricSetCount; ++i )
            {
                MD::IMetricSet_1_5* pMetricSet = m_pConcurrentGroup->GetMetricSet( i );
                assert( pMetricSet );

                MD::TMetricSetParams_1_4* pMetricSetParams = pMetricSet->GetParams();
                assert( pMetricSetParams );

                if( (std::strcmp( pMetricSetParams->SymbolName, "RenderBasic" ) == 0) &&
                    (pMetricSetParams->MetricsCount > 0) /*&&
                    (pMetricSetParams->ApiMask & MD::API_TYPE_VULKAN)*/ )
                {
                    m_pActiveMetricSet = pMetricSet;
                    m_pActiveMetricSetParams = pMetricSetParams;
                    break;
                }
            }

            // Check if RenderBasic metric set is available
            if( !m_pActiveMetricSet )
            {
                return VK_ERROR_INCOMPATIBLE_DRIVER;
            }
        }

        // Activate metric set
        {
            // Activate only metrics supported by Vulkan driver
            // TMP: Little hack to enable metrics
            m_pActiveMetricSet->SetApiFiltering( MD::API_TYPE_DX11 );

            if( m_pActiveMetricSet->Activate() != MD::CC_OK )
            {
                return VK_ERROR_INITIALIZATION_FAILED;
            }

            // Refresh metric set params
            m_pActiveMetricSetParams = m_pActiveMetricSet->GetParams();
        }

        // Construct metric properties
        for( uint32_t i = 0; i < m_pActiveMetricSetParams->MetricsCount; ++i )
        {
            MD::IMetric_1_0* pMetric = m_pActiveMetricSet->GetMetric( i );
            MD::TMetricParams_1_0* pMetricParams = pMetric->GetParams();

            VkProfilerPerformanceCounterPropertiesEXT counterProperties = {};
            std::strcpy( counterProperties.shortName, pMetricParams->ShortName );
            std::strcpy( counterProperties.description, pMetricParams->LongName );

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

            m_ActiveMetricsProperties.push_back( counterProperties );
            m_MetricFactors.push_back( metricFactor );
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
        return m_pDevice != nullptr &&
            m_pActiveMetricSet != nullptr &&
            m_pActiveMetricSetParams->MetricsCount > 0;
    }

    /***********************************************************************************\

    Function:
        GetReportSize

    Description:

    \***********************************************************************************/
    size_t ProfilerMetricsApi_INTEL::GetReportSize() const
    {
        return m_pActiveMetricSetParams->QueryReportSize;
    }

    /***********************************************************************************\

    Function:
        GetMetricCount

    Description:
        Get number of HW metrics exposed by this extension.

    \***********************************************************************************/
    size_t ProfilerMetricsApi_INTEL::GetMetricsCount() const
    {
        return m_pActiveMetricSetParams->MetricsCount;
        // Skip InformationCount - no valuable data here
    }

    /***********************************************************************************\

    Function:
        GetMetricsProperties

    Description:
        Get detailed description of each reported metric.
        Metrics must appear in the same order as in returned reports.

    \***********************************************************************************/
    std::vector<VkProfilerPerformanceCounterPropertiesEXT> ProfilerMetricsApi_INTEL::GetMetricsProperties() const
    {
        return m_ActiveMetricsProperties;
    }

    /***********************************************************************************\

    Function:
        ParseReport

    Description:
        Convert query data to human-readable form.

    \***********************************************************************************/
    std::vector<VkProfilerPerformanceCounterResultEXT> ProfilerMetricsApi_INTEL::ParseReport(
        const char* pQueryReportData,
        size_t queryReportSize )
    {
        // Convert MDAPI-specific TTypedValue_1_0 to custom VkProfilerMetricEXT
        std::vector<VkProfilerPerformanceCounterResultEXT> parsedMetrics;

        std::vector<MD::TTypedValue_1_0> metrics(
            m_pActiveMetricSetParams->MetricsCount +
            m_pActiveMetricSetParams->InformationCount );

        uint32_t reportCount = 0;

        // Check if there is data, otherwise we'll get integer zero-division
        if( m_pActiveMetricSetParams->MetricsCount > 0 )
        {
            // Calculate normalized metrics from raw query data
            MD::ECompletionCode cc = m_pActiveMetricSet->CalculateMetrics(
                (const unsigned char*)pQueryReportData,
                queryReportSize,
                metrics.data(),
                metrics.size() * sizeof( MD::TTypedValue_1_0 ),
                &reportCount,
                false );

            assert( cc == MD::CC_OK );
        }

        for( int i = 0; i < m_pActiveMetricSetParams->MetricsCount; ++i )
        {
            // Metric type information is stored in metric properties to reduce memory transaction overhead
            VkProfilerPerformanceCounterResultEXT parsedMetric = {};

            // Const factor applied to the metric
            const double factor = m_MetricFactors[ i ];

            switch( metrics[ i ].ValueType )
            {
            default:
            case MD::VALUE_TYPE_FLOAT:
                parsedMetric.float32 = static_cast<float>(metrics[ i ].ValueFloat * factor);
                break;

            case MD::VALUE_TYPE_UINT32:
                parsedMetric.uint32 = static_cast<uint32_t>(metrics[ i ].ValueUInt32 * factor);
                break;

            case MD::VALUE_TYPE_UINT64:
                parsedMetric.uint64 = static_cast<uint64_t>(metrics[ i ].ValueUInt64 * factor);
                break;

            case MD::VALUE_TYPE_BOOL:
                parsedMetric.uint32 = metrics[ i ].ValueBool;
                break;

            case MD::VALUE_TYPE_CSTRING:
                assert( !"PROFILER: Intel MDAPI string metrics not supported!" );
            }

            parsedMetrics.push_back( parsedMetric );
        }

        // This must match every time
        assert( parsedMetrics.size() == m_ActiveMetricsProperties.size() );

        return parsedMetrics;
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
        const std::filesystem::path& searchDirectory )
    {
        return ProfilerPlatformFunctions::FindFile( searchDirectory, PROFILER_METRICS_DLL_INTEL );
    }
    #endif

    /***********************************************************************************\

    Function:
        LoadMetricsDiscoveryLibrary

    Description:

    \***********************************************************************************/
    bool ProfilerMetricsApi_INTEL::LoadMetricsDiscoveryLibrary()
    {
        #ifdef WIN32
        // Load library from driver store
        char pSystemDirectory[ MAX_PATH ];
        GetSystemDirectoryA( pSystemDirectory, MAX_PATH );

        strcat( pSystemDirectory, "\\DriverStore\\FileRepository" );

        // Find location of igdmdX.dll
        const std::filesystem::path mdDllPath = FindMetricsDiscoveryLibrary( pSystemDirectory );

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
            const MD::ECompletionCode result = pfnOpenMetricsDevice( &m_pDevice );

            if( result == MD::CC_OK )
            {
                // Get device parameters
                m_pDeviceParams = m_pDevice->GetParams();
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
            pfnCloseMetricsDevice( m_pDevice );

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

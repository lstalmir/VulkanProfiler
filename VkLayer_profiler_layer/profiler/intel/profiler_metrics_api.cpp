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
        if( !LoadMetricsDiscoveryLibrary() )
            return VK_ERROR_INCOMPATIBLE_DRIVER;

        if( !OpenMetricsDevice() )
            return VK_ERROR_INITIALIZATION_FAILED;

        assert( m_pDevice );
        assert( m_pDeviceParams );

        // Enumerate metric groups
        for( int groupIdx = 0; groupIdx < m_pDeviceParams->ConcurrentGroupsCount; ++groupIdx )
        {
            MD::IConcurrentGroup_1_5* pGroup = m_pDevice->GetConcurrentGroup( groupIdx );
            MD::TConcurrentGroupParams_1_0* pGroupParams = pGroup->GetParams();

            // Enumerate metric sets in the group
            for( int setIdx = 0; setIdx < pGroupParams->MetricSetsCount; ++setIdx )
            {
                MD::IMetricSet_1_5* pSet = pGroup->GetMetricSet( setIdx );

                // Display only metrics supported by Vulkan driver
                // TMP: Little hack to enable metrics
                pSet->SetApiFiltering( MD::API_TYPE_DX11 );

                MD::TMetricSetParams_1_4* pSetParams = pSet->GetParams();

                char msg[ 256 ];
                sprintf( msg, "PROFILER: %s - %u metrics\n", pSetParams->ShortName, pSetParams->MetricsCount );
                #ifdef WIN32
                OutputDebugStringA( msg );
                #endif
            }
        }

        // Activate render metrics from the OA group
        m_pActiveMetricSet = m_pDevice->GetConcurrentGroup( 5 )->GetMetricSet( 1 );

        MD::ECompletionCode cc = m_pActiveMetricSet->Activate();
        assert( cc == MD::CC_OK );

        m_pActiveMetricSetParams = m_pActiveMetricSet->GetParams();

        // Construct metric properties
        for( uint32_t i = 0; i < m_pActiveMetricSetParams->MetricsCount; ++i )
        {
            MD::IMetric_1_0* pMetric = m_pActiveMetricSet->GetMetric( i );
            MD::TMetricParams_1_0* pMetricParams = pMetric->GetParams();

            VkProfilerMetricPropertiesEXT metricProperties = {};
            std::strcpy( metricProperties.shortName, pMetricParams->ShortName );
            std::strcpy( metricProperties.description, pMetricParams->LongName );
            std::strcpy( metricProperties.unit, pMetricParams->MetricResultUnits );

            switch( pMetricParams->ResultType )
            {
            case MD::RESULT_FLOAT:
                metricProperties.type = VK_PROFILER_METRIC_TYPE_FLOAT_EXT;
                break;

            case MD::RESULT_UINT32:
                metricProperties.type = VK_PROFILER_METRIC_TYPE_UINT32_EXT;
                break;

            case MD::RESULT_UINT64:
                metricProperties.type = VK_PROFILER_METRIC_TYPE_UINT64_EXT;
                break;

            case MD::RESULT_BOOL:
                metricProperties.type = VK_PROFILER_METRIC_TYPE_BOOL_EXT;
                break;

            default:
                assert( !"PROFILER: Intel MDAPI metric result type not supported" );
            }

            m_ActiveMetricsProperties.push_back( metricProperties );
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
    std::vector<VkProfilerMetricPropertiesEXT> ProfilerMetricsApi_INTEL::GetMetricsProperties() const
    {
        return m_ActiveMetricsProperties;
    }

    /***********************************************************************************\

    Function:
        ParseReport

    Description:
        Convert query data to human-readable form.

    \***********************************************************************************/
    std::vector<VkProfilerMetricEXT> ProfilerMetricsApi_INTEL::ParseReport(
        const char* pQueryReportData,
        size_t queryReportSize )
    {
        // Convert MDAPI-specific TTypedValue_1_0 to custom VkProfilerMetricEXT
        std::vector<VkProfilerMetricEXT> parsedMetrics;

        std::vector<MD::TTypedValue_1_0> metrics(
            m_pActiveMetricSetParams->MetricsCount + m_pActiveMetricSetParams->InformationCount );

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
            VkProfilerMetricEXT parsedMetric = {};

            switch( metrics[ i ].ValueType )
            {
            default:// TODO: Normalize metrics, keep units information
            case MD::VALUE_TYPE_FLOAT:
                parsedMetric.floatValue = metrics[ i ].ValueFloat;
                break;

            case MD::VALUE_TYPE_UINT32:
                parsedMetric.uint32Value = metrics[ i ].ValueUInt32;
                break;

            case MD::VALUE_TYPE_UINT64:
                parsedMetric.uint64Value = metrics[ i ].ValueUInt64;
                break;

            case MD::VALUE_TYPE_BOOL:
                parsedMetric.boolValue = metrics[ i ].ValueBool;
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
        // Enumerate all files in the directory
        const std::string query = searchDirectory.string() + "\\*";
        std::filesystem::path path;

        WIN32_FIND_DATAA file;
        HANDLE searchHandle = FindFirstFileA( query.c_str(), &file );

        bool hasNext = (searchHandle != INVALID_HANDLE_VALUE);

        while( hasNext )
        {
            // Skip . and .. directories to avoid infinite recursion
            if( file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
            {
                if( strcmp( file.cFileName, "." ) == 0 ||
                    strcmp( file.cFileName, ".." ) == 0 )
                {
                    hasNext = FindNextFileA( searchHandle, &file );
                    continue;
                }
            }

            // Check if this is the file we're looking for
            if( strcmp( file.cFileName, PROFILER_METRICS_DLL_INTEL ) == 0 )
            {
                path = searchDirectory / PROFILER_METRICS_DLL_INTEL;
                break;
            }

            // Search recursively in all subdirectories
            if( file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
            {
                path = FindMetricsDiscoveryLibrary( searchDirectory / file.cFileName );
                if( !path.empty() )
                    break;
            }

            hasNext = FindNextFileA( searchHandle, &file );
        }

        if( searchHandle != INVALID_HANDLE_VALUE )
        {
            FindClose( searchHandle );
        }

        return path;
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
}

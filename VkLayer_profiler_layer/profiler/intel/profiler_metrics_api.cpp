#include "profiler_metrics_api.h"

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
                pSet->SetApiFiltering( MD::API_TYPE_VULKAN );

                MD::TMetricSetParams_1_4* pSetParams = pSet->GetParams();

                char msg[ 256 ];
                sprintf_s( msg, "PROFILER: %s - %u metrics\n", pSetParams->ShortName, pSetParams->MetricsCount );
                OutputDebugStringA( msg );
            }
        }

        // Activate render metrics from the OA group
        m_pActiveMetricSet = m_pDevice->GetConcurrentGroup( 4 )->GetMetricSet( 1 );

        MD::ECompletionCode cc = m_pActiveMetricSet->Activate();
        assert( cc == MD::CC_OK );

        m_pActiveMetricSetParams = m_pActiveMetricSet->GetParams();

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
        ParseReport

    Description:
        Convert query data to human-readable form.

    \***********************************************************************************/
    std::list<std::pair<std::string, float>> ProfilerMetricsApi_INTEL::ParseReport(
        const char* pQueryReportData,
        size_t queryReportSize )
    {
        // Collect in list of NAME:VALUE pairs
        std::list<std::pair<std::string, float>> parsedMetrics;

        std::vector<MD::TTypedValue_1_0> metrics( m_pActiveMetricSetParams->MetricsCount );
        uint32_t reportCount = 0;

        // Check if there is data, otherwise we'll get integer zero-division
        if( m_pActiveMetricSetParams->MetricsCount > 0 )
        {
            // Calculate normalized metrics from raw query data
            MD::ECompletionCode cc = m_pActiveMetricSet->CalculateMetrics(
                (const unsigned char*)pQueryReportData,
                queryReportSize,
                metrics.data(),
                metrics.size(),
                &reportCount,
                false );

            assert( cc == MD::CC_OK );
        }

        assert( reportCount == m_pActiveMetricSetParams->MetricsCount );

        for( int i = 0; i < reportCount; ++i )
        {
            MD::IMetric_1_0* pMetric = m_pActiveMetricSet->GetMetric( i );
            MD::TMetricParams_1_0* pMetricParams = pMetric->GetParams();

            switch( metrics[ i ].ValueType )
            {
            default:// TODO: Normalize metrics, keep units information
            //case MD::VALUE_TYPE_FLOAT:
                parsedMetrics.push_back( { pMetricParams->ShortName, metrics[ i ].ValueFloat } );
                break;
            }
        }

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

        strcat_s( pSystemDirectory, "\\DriverStore\\FileRepository" );

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

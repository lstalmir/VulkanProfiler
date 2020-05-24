#pragma once
#include "metrics-discovery/inc/common/instrumentation/api/metrics_discovery_api.h"
#include <filesystem>
#include <vulkan/vulkan.h>

namespace Profiler
{
    /***********************************************************************************\

    Class:
        ProfilerMetricsApi_INTEL

    Description:
        Wrapper for metrics exposed by INTEL GPUs.

    \***********************************************************************************/
    class ProfilerMetricsApi_INTEL
    {
    public:
        ProfilerMetricsApi_INTEL();

        VkResult Initialize();
        void Destroy();

        bool IsAvailable() const;

        size_t GetReportSize() const;

        std::list<std::pair<std::string, float>> ParseReport(
            const char* pQueryReportData,
            size_t queryReportSize );

    private:
        #ifdef WIN32
        HMODULE m_hMDDll;

        // Since we have no official support for Windows, we have to open the library manually
        static std::filesystem::path FindMetricsDiscoveryLibrary( const std::filesystem::path& );
        #endif

        MetricsDiscovery::IMetricsDevice_1_5* m_pDevice;
        MetricsDiscovery::TMetricsDeviceParams_1_2* m_pDeviceParams;

        MetricsDiscovery::IMetricSet_1_5* m_pActiveMetricSet;
        MetricsDiscovery::TMetricSetParams_1_4* m_pActiveMetricSetParams;

        bool LoadMetricsDiscoveryLibrary();
        void UnloadMetricsDiscoveryLibrary();

        bool OpenMetricsDevice();
        void CloseMetricsDevice();
    };
}

#pragma once
#include "metrics-discovery/inc/common/instrumentation/api/metrics_discovery_api.h"
#include <filesystem>
#include <list>
#include <string>
#include <vulkan/vulkan.h>
// Import extension structures
#include "profiler_ext/VkProfilerEXT.h"

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

        size_t GetMetricsCount() const;

        std::vector<VkProfilerPerformanceCounterPropertiesEXT> GetMetricsProperties() const;

        std::vector<VkProfilerPerformanceCounterResultEXT> ParseReport(
            const char* pQueryReportData,
            size_t queryReportSize );

    private:
        #ifdef WIN32
        HMODULE m_hMDDll;
        // Since there is no official support for Windows, we have to open the library manually
        static std::filesystem::path FindMetricsDiscoveryLibrary( const std::filesystem::path& );
        #endif

        MetricsDiscovery::IMetricsDevice_1_5* m_pDevice;
        MetricsDiscovery::TMetricsDeviceParams_1_2* m_pDeviceParams;

        MetricsDiscovery::IConcurrentGroup_1_5* m_pConcurrentGroup;
        MetricsDiscovery::TConcurrentGroupParams_1_0* m_pConcurrentGroupParams;

        MetricsDiscovery::IMetricSet_1_5* m_pActiveMetricSet;
        MetricsDiscovery::TMetricSetParams_1_4* m_pActiveMetricSetParams;

        std::vector<VkProfilerPerformanceCounterPropertiesEXT> m_ActiveMetricsProperties;

        // Some metrics are reported in premultiplied units, e.g., MHz
        // This vector contains factors applied to each metric in output reports
        std::vector<double> m_MetricFactors;

        bool LoadMetricsDiscoveryLibrary();
        void UnloadMetricsDiscoveryLibrary();

        bool OpenMetricsDevice();
        void CloseMetricsDevice();

        static VkProfilerPerformanceCounterUnitEXT TranslateUnit( const char* pUnit, double& factor );
    };
}

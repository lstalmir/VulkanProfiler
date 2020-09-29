// Copyright (c) 2020 Lukasz Stalmirski
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

#pragma once
#include "metrics-discovery/inc/common/instrumentation/api/metrics_discovery_api.h"
#ifdef WIN32
#include <filesystem>
#endif
#include <vector>
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

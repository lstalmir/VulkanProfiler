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

#pragma once
#include <metrics_discovery_api.h>
#ifdef WIN32
#include <filesystem>
#endif
#include <vector>
#include <string>
#include <shared_mutex>
#include <vulkan/vulkan.h>
// Import extension structures
#include "profiler_ext/VkProfilerEXT.h"

namespace Profiler
{
    class DeviceProfiler;

    struct ProfilerMetricsSet_INTEL
    {
        MetricsDiscovery::IMetricSet_1_1* m_pMetricSet;
        MetricsDiscovery::TMetricSetParams_1_0* m_pMetricSetParams;

        std::vector<VkProfilerPerformanceCounterPropertiesEXT> m_MetricsProperties;

        // Some metrics are reported in premultiplied units, e.g., MHz
        // This vector contains factors applied to each metric in output reports
        std::vector<double> m_MetricFactors;
    };

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

        VkResult Initialize( struct VkDevice_Object* pDevice );
        void Destroy();

        bool IsAvailable() const;

        uint32_t GetReportSize( uint32_t metricsSetIndex ) const;

        uint32_t GetMetricsCount( uint32_t metricsSetIndex ) const;

        uint32_t GetMetricsSetCount() const;

        VkResult GetMetricsSets(
            uint32_t*                                     pPropertyCount,
            VkProfilerPerformanceMetricsSetPropertiesEXT* pProperties ) const;

        VkResult SetActiveMetricsSet( uint32_t metricsSetIndex );

        uint32_t GetActiveMetricsSetIndex() const;

        VkResult GetMetricsProperties(
            uint32_t                                   metricsSetIndex,
            uint32_t*                                  pPropertyCount,
            VkProfilerPerformanceCounterPropertiesEXT* pProperties ) const;

        void ParseReport(
            uint32_t                                            metricsSetIndex,
            uint32_t                                            reportSize,
            const uint8_t*                                      pReport,
            std::vector<VkProfilerPerformanceCounterResultEXT>& results );

    private:
        // Require at least version 1.1.
        static const uint32_t m_RequiredVersionMajor = 1;
        static const uint32_t m_MinRequiredVersionMinor = 1;

        #ifdef WIN32
        HMODULE m_hMDDll;
        // Since there is no official support for Windows, we have to open the library manually
        static std::filesystem::path FindMetricsDiscoveryLibrary( struct VkDevice_Object* pDevice );
        #endif

        MetricsDiscovery::IMetricsDevice_1_1* m_pDevice;
        MetricsDiscovery::TMetricsDeviceParams_1_0* m_pDeviceParams;

        MetricsDiscovery::IConcurrentGroup_1_1* m_pConcurrentGroup;
        MetricsDiscovery::TConcurrentGroupParams_1_0* m_pConcurrentGroupParams;

        std::vector<ProfilerMetricsSet_INTEL> m_MetricsSets;

        std::shared_mutex mutable             m_ActiveMetricSetMutex;
        uint32_t                              m_ActiveMetricsSetIndex;


        bool LoadMetricsDiscoveryLibrary( struct VkDevice_Object* pDevice );
        void UnloadMetricsDiscoveryLibrary();

        bool OpenMetricsDevice();
        void CloseMetricsDevice();

        static VkProfilerPerformanceCounterUnitEXT TranslateUnit( const char* pUnit, double& factor );
    };
}

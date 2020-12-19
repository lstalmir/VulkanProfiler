#pragma once
#include <list>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace Profiler
{
    class VkDevice_Object;

    /***********************************************************************************\

    Class:
        ProfilerMetricsApi_KHR

    Description:
        Wrapper for metrics exposed by GPUs.

    \***********************************************************************************/
    class ProfilerMetricsApi_KHR
    {
    public:
        ProfilerMetricsApi_KHR();

        VkResult Initialize( VkDevice_Object* pDevice );
        void Destroy();

        bool IsAvailable() const;

        size_t GetReportSize() const;

        std::list<std::pair<std::string, float>> ParseReport(
            const char* pQueryReportData,
            size_t queryReportSize );

    private:
        VkDevice_Object* m_pDevice;

        std::vector<VkPerformanceCounterKHR> m_Counters;
        std::vector<VkPerformanceCounterDescriptionKHR> m_CounterDescriptions;
    };
}

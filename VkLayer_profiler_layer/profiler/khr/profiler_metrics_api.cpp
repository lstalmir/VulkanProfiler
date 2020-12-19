#include "profiler_metrics_api.h"
#include "profiler_layer_objects/VkDevice_object.h"

#ifndef _DEBUG
#define NDEBUG
#endif
#include <assert.h>

namespace Profiler
{
    /***********************************************************************************\

    Function:
        ProfilerMetricsApi_KHR

    Description:
        Constructor.

    \***********************************************************************************/
    ProfilerMetricsApi_KHR::ProfilerMetricsApi_KHR()
        : m_pDevice( nullptr )
        , m_Counters()
        , m_CounterDescriptions()
    {
    }

    /***********************************************************************************\

    Function:
        Initialize

    Description:

    \***********************************************************************************/
    VkResult ProfilerMetricsApi_KHR::Initialize( VkDevice_Object* pDevice )
    {
        assert( m_pDevice == nullptr );

        VkResult result = VK_ERROR_FEATURE_NOT_PRESENT;

        if( pDevice->PerformanceCounterQueryPoolsAvailable )
        {
            m_pDevice = pDevice;

            uint32_t counterCount = 0;
            m_pDevice->pInstance->Callbacks.EnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR(
                m_pDevice->PhysicalDevice,
                0,
                &counterCount,
                nullptr,
                nullptr );

            if( counterCount > 0 )
            {
                m_Counters.resize( counterCount );
                m_CounterDescriptions.resize( counterCount );

                m_pDevice->pInstance->Callbacks.EnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR(
                    m_pDevice->PhysicalDevice,
                    0,
                    &counterCount,
                    m_Counters.data(),
                    m_CounterDescriptions.data() );
            }

            result = VK_SUCCESS;
        }

        return result;
    }

    /***********************************************************************************\

    Function:
        IsAvailable

    Description:

    \***********************************************************************************/
    bool ProfilerMetricsApi_KHR::IsAvailable() const
    {
        return m_pDevice != nullptr &&
            m_pDevice->PerformanceCounterQueryPoolsAvailable &&
            !m_Counters.empty();
    }
}

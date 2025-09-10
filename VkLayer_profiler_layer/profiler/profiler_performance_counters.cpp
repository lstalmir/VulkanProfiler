// Copyright (c) 2025 Lukasz Stalmirski
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

#include "profiler_performance_counters.h"
#include "profiler_helpers.h"
#include "profiler_layer_objects/VkDevice_object.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        DeviceProfilerPerformanceCountersKHR

    Description:
        Constructor.

    \***********************************************************************************/
    DeviceProfilerPerformanceCountersKHR::DeviceProfilerPerformanceCountersKHR()
        : m_pDevice( nullptr )
        , m_DeviceProfilingLockAcquired( false )
        , m_ActiveMetricsSetMutex()
        , m_ActiveMetricsSetIndex( UINT32_MAX )
        , m_QueueFamilyCounters( 0 )
        , m_MetricsSets( 0 )
        , m_MetricsSetsMutex()
    {
    }

    /***********************************************************************************\

    Function:
        Initialize

    Description:
        Initializes performance counters using VK_KHR_performance_query extension.

    \***********************************************************************************/
    VkResult DeviceProfilerPerformanceCountersKHR::Initialize( VkDevice_Object* pDevice )
    {
        assert( pDevice->EnabledExtensions.count( VK_KHR_PERFORMANCE_QUERY_EXTENSION_NAME ) );

        VkResult result = VK_SUCCESS;

        m_pDevice = pDevice;

        if( result == VK_SUCCESS )
        {
            // Acquire profiling lock.
            VkAcquireProfilingLockInfoKHR acquireInfo = {};
            acquireInfo.sType = VK_STRUCTURE_TYPE_ACQUIRE_PROFILING_LOCK_INFO_KHR;
            acquireInfo.timeout = UINT64_MAX;

            result = m_pDevice->Callbacks.AcquireProfilingLockKHR(
                m_pDevice->Handle,
                &acquireInfo );

            m_DeviceProfilingLockAcquired = ( result == VK_SUCCESS );
        }

        if( result == VK_SUCCESS )
        {
            // Enumerate available performance counters for each queue family.
            const size_t queueFamilyCount = m_pDevice->pPhysicalDevice->QueueFamilyProperties.size();
            m_QueueFamilyCounters.resize( queueFamilyCount );

            for( uint32_t queueFamilyIndex = 0; queueFamilyIndex < queueFamilyCount; ++queueFamilyIndex )
            {
                uint32_t performanceCountersCount = 0;
                result = m_pDevice->pInstance->Callbacks.EnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR(
                    m_pDevice->pPhysicalDevice->Handle,
                    queueFamilyIndex,
                    &performanceCountersCount,
                    nullptr,
                    nullptr );

                if( result == VK_SUCCESS && performanceCountersCount > 0 )
                {
                    auto& queueFamilyPerformanceCounters = m_QueueFamilyCounters.at( queueFamilyIndex );
                    queueFamilyPerformanceCounters.m_Counters.resize( performanceCountersCount );
                    queueFamilyPerformanceCounters.m_CounterDescriptions.resize( performanceCountersCount );

                    m_pDevice->pInstance->Callbacks.EnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR(
                        m_pDevice->pPhysicalDevice->Handle,
                        queueFamilyIndex,
                        &performanceCountersCount,
                        queueFamilyPerformanceCounters.m_Counters.data(),
                        queueFamilyPerformanceCounters.m_CounterDescriptions.data() );
                }
            }
        }

        if( result != VK_SUCCESS )
        {
            // Cleanup partially initialized state.
            Destroy();
        }

        return result;
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:
        Destroys performance counters.

    \***********************************************************************************/
    void DeviceProfilerPerformanceCountersKHR::Destroy()
    {
        if( m_DeviceProfilingLockAcquired )
        {
            assert( m_pDevice != nullptr );
            m_pDevice->Callbacks.ReleaseProfilingLockKHR( m_pDevice->Handle );
            m_DeviceProfilingLockAcquired = false;
        }

        m_QueueFamilyCounters.clear();
        m_MetricsSets.clear();

        m_pDevice = nullptr;
    }

    /***********************************************************************************\

    Function:
        GetReportSize

    Description:
        Returns size of the result of a performance query.

    \***********************************************************************************/
    uint32_t DeviceProfilerPerformanceCountersKHR::GetReportSize( uint32_t metricsSetIndex ) const
    {
        return GetMetricsCount( metricsSetIndex ) * sizeof( VkPerformanceCounterResultKHR );
    }

    /***********************************************************************************\

    Function:
        GetMetricsCount

    Description:
        Returns number of counters in the specified counter set.

    \***********************************************************************************/
    uint32_t DeviceProfilerPerformanceCountersKHR::GetMetricsCount( uint32_t metricsSetIndex ) const
    {
        std::shared_lock metricsSetsLock( m_MetricsSetsMutex );

        if( metricsSetIndex < m_MetricsSets.size() )
        {
            return static_cast<uint32_t>( m_MetricsSets[metricsSetIndex].m_Counters.size() );
        }

        return 0;
    }

    /***********************************************************************************\

    Function:
        GetMetricsSetCount

    Description:
        Returns number of available counter sets.

    \***********************************************************************************/
    uint32_t DeviceProfilerPerformanceCountersKHR::GetMetricsSetCount() const
    {
        std::shared_lock metricsSetsLock( m_MetricsSetsMutex );
        return static_cast<uint32_t>( m_MetricsSets.size() );
    }

    /***********************************************************************************\

    Function:
        SetActiveMetricsSet

    Description:
        Sets the active counter set index. All subsequently created query pools will
        use this counter set.

    \***********************************************************************************/
    VkResult DeviceProfilerPerformanceCountersKHR::SetActiveMetricsSet( uint32_t metricsSetIndex )
    {
        std::shared_lock metricsSetsLock( m_MetricsSetsMutex );
        if( ( metricsSetIndex >= m_MetricsSets.size() ) &&
            ( metricsSetIndex != UINT32_MAX ) )
        {
            return VK_ERROR_VALIDATION_FAILED_EXT;
        }

        std::unique_lock activeCounterLock( m_ActiveMetricsSetMutex );
        m_ActiveMetricsSetIndex = metricsSetIndex;

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        GetActiveMetricsSetIndex

    Description:
        Returns the currently active counter set index.

    \***********************************************************************************/
    uint32_t DeviceProfilerPerformanceCountersKHR::GetActiveMetricsSetIndex() const
    {
        std::shared_lock activeCounterLock( m_ActiveMetricsSetMutex );
        return m_ActiveMetricsSetIndex;
    }

    /***********************************************************************************\

    Function:
        GetActiveMetricsSetIndex

    Description:
        Returns the currently active counter set index.

    \***********************************************************************************/
    void DeviceProfilerPerformanceCountersKHR::GetMetricsSets( std::vector<VkProfilerPerformanceMetricsSetPropertiesEXT>& metricsSets ) const
    {
        std::shared_lock metricsSetsLock( m_MetricsSetsMutex );

        // Allocate space for the metrics sets.
        metricsSets.resize( m_MetricsSets.size() );

        // Fill in the properties.
        for( size_t i = 0; i < m_MetricsSets.size(); ++i )
        {
            const MetricsSet& set = m_MetricsSets.at( i );

            metricsSets[i].metricsCount = static_cast<uint32_t>( set.m_Counters.size() );
            ProfilerStringFunctions::CopyString( metricsSets[i].name, set.m_Name.c_str(), set.m_Name.length() );
        }
    }

    /***********************************************************************************\

    Function:
        GetActiveMetricsSetIndex

    Description:
        Returns the currently active counter set index.

    \***********************************************************************************/
    void DeviceProfilerPerformanceCountersKHR::GetMetricsProperties( uint32_t metricsSetIndex, std::vector<VkProfilerPerformanceCounterPropertiesEXT>& metrics ) const
    {
        std::shared_lock metricsSetsLock( m_MetricsSetsMutex );

        // Allocate space for the metrics.
        metrics.clear();

        if( metricsSetIndex >= m_MetricsSets.size() )
        {
            return;
        }

        const MetricsSet& set = m_MetricsSets.at( metricsSetIndex );
        metrics.resize( set.m_Counters.size() );

        const QueueFamilyCounters& queueFamilyCounters = m_QueueFamilyCounters[set.m_QueueFamilyIndex];

        // Fill in the properties.
        for( size_t i = 0; i < set.m_Counters.size(); ++i )
        {
            const uint32_t counterIndex = set.m_Counters[i];
            const VkPerformanceCounterKHR& counter = queueFamilyCounters.m_Counters[counterIndex];
            const VkPerformanceCounterDescriptionKHR& description = queueFamilyCounters.m_CounterDescriptions[counterIndex];

            metrics[i].unit = static_cast<VkProfilerPerformanceCounterUnitEXT>( counter.unit );
            metrics[i].storage = static_cast<VkProfilerPerformanceCounterStorageEXT>( counter.storage );
            ProfilerStringFunctions::CopyString( metrics[i].shortName, description.name, std::size( metrics[i].shortName ) );
            ProfilerStringFunctions::CopyString( metrics[i].description, description.description, std::size( metrics[i].description ) );
        }
    }

    /***********************************************************************************\

    Function:
        CreateQueryPool

    Description:
        Creates a query pool for performance queries.

    \***********************************************************************************/
    VkResult DeviceProfilerPerformanceCountersKHR::CreateQueryPool( uint32_t queueFamilyIndex, uint32_t size, VkQueryPool* pQueryPool )
    {
        std::shared_lock metricsSetsLock( m_MetricsSetsMutex );

        VkQueryPoolCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        createInfo.queryType = VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR;
        createInfo.queryCount = size;

        VkQueryPoolPerformanceCreateInfoKHR performanceCreateInfo = GetQueryPoolPerformanceCreateInfo( queueFamilyIndex );
        createInfo.pNext = &performanceCreateInfo;

        return m_pDevice->Callbacks.CreateQueryPool(
            m_pDevice->Handle,
            &createInfo,
            nullptr,
            pQueryPool );
    }

    /***********************************************************************************\

    Function:
        CreateCustomMetricsSet

    Description:
        Create a custom counter set.

    \***********************************************************************************/
    VkResult DeviceProfilerPerformanceCountersKHR::CreateCustomMetricsSet( uint32_t queueFamilyIndex, const char* pName, const char* pDescription, uint32_t counterCount, const uint32_t* pCounters, uint32_t* pMetricsSetIndex )
    {
        // Validate parameters.
        if( ( queueFamilyIndex >= m_QueueFamilyCounters.size() ) ||
            ( pMetricsSetIndex == nullptr ) ||
            ( pCounters == nullptr ) ||
            ( counterCount == 0 ) )
        {
            return VK_ERROR_VALIDATION_FAILED_EXT;
        }

        // Validate that all requested counters are available in the specified queue family.
        const QueueFamilyCounters& availableCounters = m_QueueFamilyCounters[queueFamilyIndex];
        for( uint32_t i = 0; i < counterCount; ++i )
        {
            if( pCounters[i] >= availableCounters.m_Counters.size() )
            {
                return VK_ERROR_VALIDATION_FAILED_EXT;
            }
        }

        // Create and register the counter set.
        MetricsSet metricsSet = {};
        metricsSet.m_Name = ( pName != nullptr ) ? pName : "";
        metricsSet.m_Description = ( pDescription != nullptr ) ? pDescription : "";
        metricsSet.m_Counters.assign( pCounters, pCounters + counterCount );
        metricsSet.m_QueueFamilyIndex = queueFamilyIndex;

        *pMetricsSetIndex = RegisterMetricsSet( std::move( metricsSet ) );

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        DestroyCustomMetricsSet

    Description:
        Destroy the custom counter set.

    \***********************************************************************************/
    void DeviceProfilerPerformanceCountersKHR::DestroyCustomMetricsSet( uint32_t metricsSetIndex )
    {
        std::scoped_lock lock( m_MetricsSetsMutex, m_ActiveMetricsSetMutex );

        // Disable active counter set if it is being removed.
        if( metricsSetIndex == m_ActiveMetricsSetIndex )
        {
            m_ActiveMetricsSetIndex = UINT32_MAX;
        }

        // Remove the counter set from the vector.
        if( metricsSetIndex < m_MetricsSets.size() )
        {
            m_MetricsSets.erase( m_MetricsSets.begin() + metricsSetIndex );
        }
    }

    /***********************************************************************************\

    Function:
        ParseReport

    Description:
        Convert raw performance query report into human readable results.

    \***********************************************************************************/
    void DeviceProfilerPerformanceCountersKHR::ParseReport(
        uint32_t /*metricsSetIndex*/,
        uint32_t reportSize,
        const uint8_t* pReport,
        std::vector<VkProfilerPerformanceCounterResultEXT>& results )
    {
        static_assert( sizeof( VkPerformanceCounterResultKHR ) == sizeof( VkProfilerPerformanceCounterResultEXT ) );
        assert( reportSize % sizeof( VkProfilerPerformanceCounterResultEXT ) == 0 );

        const uint32_t counterCount = reportSize / sizeof( VkProfilerPerformanceCounterResultEXT );
        const VkProfilerPerformanceCounterResultEXT* pCounterResults = reinterpret_cast<const VkProfilerPerformanceCounterResultEXT*>( pReport );
        results.assign( pCounterResults, pCounterResults + counterCount );
    }

    /***********************************************************************************\

    Function:
        GetQueryPoolPerformanceCreateInfo

    Description:
        Returns structure used to create performance query pools.
        m_MetricsSetsMutex must be locked when calling this function.

    \***********************************************************************************/
    VkQueryPoolPerformanceCreateInfoKHR DeviceProfilerPerformanceCountersKHR::GetQueryPoolPerformanceCreateInfo( uint32_t queueFamilyIndex ) const
    {
        VkQueryPoolPerformanceCreateInfoKHR performanceCreateInfo = {};
        performanceCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_PERFORMANCE_CREATE_INFO_KHR;
        performanceCreateInfo.queueFamilyIndex = queueFamilyIndex;

        const uint32_t activeMetricsSetIndex = GetActiveMetricsSetIndex();
        if( activeMetricsSetIndex < m_MetricsSets.size() )
        {
            const MetricsSet& activeCounters = m_MetricsSets[activeMetricsSetIndex];
            performanceCreateInfo.counterIndexCount = static_cast<uint32_t>( activeCounters.m_Counters.size() );
            performanceCreateInfo.pCounterIndices = activeCounters.m_Counters.data();
        }

        return performanceCreateInfo;
    }

    /***********************************************************************************\

    Function:
        RegisterMetricsSet

    Description:
        Append the counter set to the list of available sets.

    \***********************************************************************************/
    uint32_t DeviceProfilerPerformanceCountersKHR::RegisterMetricsSet( MetricsSet&& metricsSet )
    {
        std::unique_lock metricsSetsLock( m_MetricsSetsMutex );

        // Append the counter set to the list.
        m_MetricsSets.push_back( std::move( metricsSet ) );

        // Return the index of the newly created counter set.
        return static_cast<uint32_t>( m_MetricsSets.size() - 1 );
    }
}

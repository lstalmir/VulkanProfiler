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
        AreMetricsSetsCompatible

    Description:
        Checks if two counter sets are compatible, i.e. VkQueryPools created for one
        can be used with the other one.

    \***********************************************************************************/
    bool DeviceProfilerPerformanceCountersKHR::AreMetricsSetsCompatible( uint32_t firstMetricsSetIndex, uint32_t secondMetricsSetIndex ) const
    {
        std::shared_lock metricsSetsLock( m_MetricsSetsMutex );

        if( ( firstMetricsSetIndex >= m_MetricsSets.size() ) ||
            ( secondMetricsSetIndex >= m_MetricsSets.size() ) )
        {
            return false;
        }

        return ( m_MetricsSets[firstMetricsSetIndex].m_CompatibleHash == m_MetricsSets[secondMetricsSetIndex].m_CompatibleHash );
    }

    /***********************************************************************************\

    Function:
        GetMetricsSets

    Description:
        Returns list of available performance counter sets.

    \***********************************************************************************/
    void DeviceProfilerPerformanceCountersKHR::GetMetricsSets( std::vector<VkProfilerPerformanceMetricsSetPropertiesEXT>& metricsSets ) const
    {
        std::shared_lock metricsSetsLock( m_MetricsSetsMutex );

        // Allocate space for the metrics sets.
        metricsSets.resize( m_MetricsSets.size() );

        // Fill in the properties.
        for( size_t i = 0; i < m_MetricsSets.size(); ++i )
        {
            FillPerformanceMetricsSetProperties( m_MetricsSets[i], metricsSets[i] );
        }
    }

    /***********************************************************************************\

    Function:
        GetMetricsSetProperties

    Description:
        Returns properties of the specified performance counter set.

    \***********************************************************************************/
    void DeviceProfilerPerformanceCountersKHR::GetMetricsSetProperties( uint32_t metricsSetIndex, VkProfilerPerformanceMetricsSetPropertiesEXT& properties ) const
    {
        std::shared_lock metricsSetsLock( m_MetricsSetsMutex );

        if( metricsSetIndex < m_MetricsSets.size() )
        {
            FillPerformanceMetricsSetProperties( m_MetricsSets[metricsSetIndex], properties );
        }
        else
        {
            memset( &properties, 0, sizeof( properties ) );
        }
    }

    /***********************************************************************************\

    Function:
        GetMetricsSetMetricsProperties

    Description:
        Returns list of performance counters in the specified counter set.

    \***********************************************************************************/
    void DeviceProfilerPerformanceCountersKHR::GetMetricsSetMetricsProperties( uint32_t metricsSetIndex, std::vector<VkProfilerPerformanceCounterPropertiesEXT>& metrics ) const
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

            FillPerformanceCounterProperties( counter, description, metrics[i] );
        }
    }

    /***********************************************************************************\

    Function:
        GetQueueFamilyMetricsProperties

    Description:
        Returns list of available performance counters for a given queue family.

    \***********************************************************************************/
    void DeviceProfilerPerformanceCountersKHR::GetQueueFamilyMetricsProperties( uint32_t queueFamilyIndex, std::vector<VkProfilerPerformanceCounterPropertiesEXT>& metrics ) const
    {
        metrics.clear();

        if( queueFamilyIndex >= m_QueueFamilyCounters.size() )
        {
            return;
        }

        const QueueFamilyCounters& queueFamilyCounters = m_QueueFamilyCounters[queueFamilyIndex];
        metrics.resize( queueFamilyCounters.m_Counters.size() );

        // Fill in the properties.
        for( size_t i = 0; i < queueFamilyCounters.m_Counters.size(); ++i )
        {
            const VkPerformanceCounterKHR& counter = queueFamilyCounters.m_Counters[i];
            const VkPerformanceCounterDescriptionKHR& description = queueFamilyCounters.m_CounterDescriptions[i];

            FillPerformanceCounterProperties( counter, description, metrics[i] );
        }
    }

    /***********************************************************************************\

    Function:
        GetAvailableMetrics

    Description:
        Updates the list of available performance counters for a given queue family.

    \***********************************************************************************/
    void DeviceProfilerPerformanceCountersKHR::GetAvailableMetrics( uint32_t queueFamilyIndex, const std::vector<uint32_t>& allocatedCounters, std::vector<uint32_t>& availableCounters ) const
    {
        if( queueFamilyIndex >= m_QueueFamilyCounters.size() )
        {
            availableCounters.clear();
            return;
        }

        // Prepare a createInfo structure to test the number of passes.
        std::vector<uint32_t> testedCounters( allocatedCounters );
        testedCounters.push_back( UINT32_MAX );

        VkQueryPoolPerformanceCreateInfoKHR performanceCreateInfo = {};
        performanceCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_PERFORMANCE_CREATE_INFO_KHR;
        performanceCreateInfo.queueFamilyIndex = queueFamilyIndex;
        performanceCreateInfo.counterIndexCount = static_cast<uint32_t>( testedCounters.size() );
        performanceCreateInfo.pCounterIndices = testedCounters.data();

        PFN_vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR pfnGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR =
            m_pDevice->pInstance->Callbacks.GetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR;
        assert( pfnGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR != nullptr );

        // Check which counters are not increasing the number of required runs.
        for( auto counterIt = availableCounters.begin(); counterIt != availableCounters.end(); )
        {
            testedCounters.back() = *counterIt;

            uint32_t numPasses = 0;
            pfnGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR(
                m_pDevice->pPhysicalDevice->Handle,
                &performanceCreateInfo,
                &numPasses );

            if( numPasses > 1 )
            {
                counterIt = availableCounters.erase( counterIt );
                continue;
            }

            counterIt++;
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
    uint32_t DeviceProfilerPerformanceCountersKHR::CreateCustomMetricsSet( uint32_t queueFamilyIndex, const std::string& name, const std::string& description, const std::vector<uint32_t>& counterIndices )
    {
        // Validate parameters.
        if( ( queueFamilyIndex >= m_QueueFamilyCounters.size() ) ||
            ( counterIndices.empty() ) )
        {
            return VK_ERROR_VALIDATION_FAILED_EXT;
        }

        // Validate that all requested counters are available in the specified queue family.
        const QueueFamilyCounters& availableCounters = m_QueueFamilyCounters[queueFamilyIndex];
        for( uint32_t counter : counterIndices )
        {
            if( counter >= availableCounters.m_Counters.size() )
            {
                return VK_ERROR_VALIDATION_FAILED_EXT;
            }
        }

        // Calculate a hash of the counter set to identify compatible sets.
        HashInput hashInput;
        hashInput.Add( queueFamilyIndex );
        hashInput.Add( counterIndices, true /*sort*/ );

        uint32_t compatibleHash = Farmhash::Fingerprint32(
            hashInput.GetData(),
            hashInput.GetSize() );

        // Calculate a full hash to identify identical sets.
        hashInput.Reset();
        hashInput.Add( compatibleHash );
        hashInput.Add( name );
        hashInput.Add( description );

        uint32_t fullHash = Farmhash::Fingerprint32(
            hashInput.GetData(),
            hashInput.GetSize() );

        // Check if an identical counter set already exists.
        uint32_t metricsSetIndex = FindMetricsSetByHash( fullHash );

        // Create and register the counter set if it does not exist yet.
        if( metricsSetIndex == UINT32_MAX )
        {
            MetricsSet metricsSet = {};
            metricsSet.m_Name = name;
            metricsSet.m_Description = description;
            metricsSet.m_Counters = counterIndices;
            metricsSet.m_QueueFamilyIndex = queueFamilyIndex;
            metricsSet.m_CompatibleHash = compatibleHash;
            metricsSet.m_FullHash = fullHash;

            metricsSetIndex = RegisterMetricsSet( std::move( metricsSet ) );
        }

        return metricsSetIndex;
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
        FindMetricsSetByHash

    Description:
        Try to find an existing counter set by its full hash (all properties must match).

    \***********************************************************************************/
    uint32_t DeviceProfilerPerformanceCountersKHR::FindMetricsSetByHash( uint32_t fullHash ) const
    {
        std::shared_lock metricsSetsLock( m_MetricsSetsMutex );

        const uint32_t setCount = static_cast<uint32_t>( m_MetricsSets.size() );
        for( uint32_t setIndex = 0; setIndex < setCount; ++setIndex )
        {
            if( m_MetricsSets[setIndex].m_FullHash == fullHash )
            {
                return setIndex;
            }
        }

        return UINT32_MAX;
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

    /***********************************************************************************\

    Function:
        FillPerformanceMetricsSetProperties

    Description:
        Fill in the properties structure using internal counter set representation.

    \***********************************************************************************/
    void DeviceProfilerPerformanceCountersKHR::FillPerformanceMetricsSetProperties(
        const MetricsSet& metricsSet,
        VkProfilerPerformanceMetricsSetPropertiesEXT& properties )
    {
        ProfilerStringFunctions::CopyString(
            properties.name,
            std::size( properties.name ),
            metricsSet.m_Name.c_str(),
            metricsSet.m_Name.length() );

        properties.metricsCount = static_cast<uint32_t>( metricsSet.m_Counters.size() );
    }

    /***********************************************************************************\

    Function:
        FillPerformanceCounterProperties

    Description:
        Fill in the properties structure using VK_KHR_performance_query structures.

    \***********************************************************************************/
    void DeviceProfilerPerformanceCountersKHR::FillPerformanceCounterProperties(
        const VkPerformanceCounterKHR& counter,
        const VkPerformanceCounterDescriptionKHR& description,
        VkProfilerPerformanceCounterPropertiesEXT& properties )
    {
        ProfilerStringFunctions::CopyString(
            properties.shortName,
            std::size( properties.shortName ),
            description.name,
            std::size( description.name ) );

        ProfilerStringFunctions::CopyString(
            properties.description,
            std::size( properties.description ),
            description.description,
            std::size( description.description ) );

        properties.unit = static_cast<VkProfilerPerformanceCounterUnitEXT>( counter.unit );
        properties.storage = static_cast<VkProfilerPerformanceCounterStorageEXT>( counter.storage );
    }
}

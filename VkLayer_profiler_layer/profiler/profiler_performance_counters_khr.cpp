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

#include "profiler_performance_counters_khr.h"
#include "profiler_helpers.h"
#include "profiler_layer_objects/VkDevice_object.h"

#include <algorithm>

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
        , m_QueueFamilyCount( 0 )
        , m_UsedQueueFamilies()
        , m_ActiveMetricsSetMutex()
        , m_ActiveMetricsSetIndex( UINT32_MAX )
        , m_Counters( 0 )
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
            m_QueueFamilyCount = m_pDevice->pPhysicalDevice->QueueFamilyProperties.size();

            for( const auto& [_, queue] : m_pDevice->Queues )
            {
                m_UsedQueueFamilies.insert( queue.Family );
            }

            std::vector<VkPerformanceCounterKHR> counters;
            std::vector<VkPerformanceCounterDescriptionKHR> counterDescriptions;

            for( uint32_t queueFamilyIndex : m_UsedQueueFamilies )
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
                    counters.resize( performanceCountersCount );
                    counterDescriptions.resize( performanceCountersCount );

                    m_pDevice->pInstance->Callbacks.EnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR(
                        m_pDevice->pPhysicalDevice->Handle,
                        queueFamilyIndex,
                        &performanceCountersCount,
                        counters.data(),
                        counterDescriptions.data() );

                    for( uint32_t i = 0; i < performanceCountersCount; ++i )
                    {
                        RegisterCounter( queueFamilyIndex, i, counters[i], counterDescriptions[i] );
                    }
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

        m_QueueFamilyCount = 0;

        m_Counters.clear();
        m_MetricsSets.clear();

        m_pDevice = nullptr;
    }

    /***********************************************************************************\

    Function:
        GetReportSize

    Description:
        Returns size of the result of a performance query.

    \***********************************************************************************/
    uint32_t DeviceProfilerPerformanceCountersKHR::GetReportSize( uint32_t metricsSetIndex, uint32_t queueFamilyIndex ) const
    {
        std::shared_lock metricsSetsLock( m_MetricsSetsMutex );

        if( metricsSetIndex < m_MetricsSets.size() )
        {
            const MetricsSet& metricsSet = m_MetricsSets[metricsSetIndex];
            const MetricsSetQueueFamilyCounters& queueFamilyCounters = metricsSet.m_QueueFamilyCounters[queueFamilyIndex];
            return static_cast<uint32_t>( queueFamilyCounters.m_CounterIndices.size() * sizeof( VkPerformanceCounterResultKHR ) );
        }

        return 0;
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
            return static_cast<uint32_t>( m_MetricsSets[metricsSetIndex].m_CounterIndices.size() );
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
        GetRequiredPasses

    Description:
        Returns number of passes required to capture the specified counters.

    \***********************************************************************************/
    uint32_t DeviceProfilerPerformanceCountersKHR::GetRequiredPasses( uint32_t counterCount, const uint32_t* pCounterIndices ) const
    {
        uint32_t requiredPasses = 0;

        PFN_vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR pfnGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR =
            m_pDevice->pInstance->Callbacks.GetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR;
        assert( pfnGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR != nullptr );

        // Prepare a createInfo structure to test the number of passes.
        std::vector<uint32_t> testedCounters( 0 );
        testedCounters.reserve( counterCount );

        for( uint32_t queueFamilyIndex : m_UsedQueueFamilies )
        {
            testedCounters.clear();

            // Construct a list of allocated counters for the given queue family.
            for( uint32_t i = 0; i < counterCount; ++i )
            {
                const Counter& counter = m_Counters.at( pCounterIndices[i] );
                const uint32_t counterIndexInFamily = counter.m_QueueFamilyCounterIndices[queueFamilyIndex];

                if( counterIndexInFamily != UINT32_MAX )
                {
                    testedCounters.push_back( counterIndexInFamily );
                }
            }

            VkQueryPoolPerformanceCreateInfoKHR performanceCreateInfo = {};
            performanceCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_PERFORMANCE_CREATE_INFO_KHR;
            performanceCreateInfo.queueFamilyIndex = queueFamilyIndex;
            performanceCreateInfo.counterIndexCount = static_cast<uint32_t>( testedCounters.size() );
            performanceCreateInfo.pCounterIndices = testedCounters.data();

            uint32_t numPasses = 0;
            pfnGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR(
                m_pDevice->pPhysicalDevice->Handle,
                &performanceCreateInfo,
                &numPasses );

            requiredPasses = std::max( requiredPasses, numPasses );
        }

        return requiredPasses;
    }

    /***********************************************************************************\

    Function:
        GetMetricsSets

    Description:
        Returns list of available performance counter sets.

    \***********************************************************************************/
    uint32_t DeviceProfilerPerformanceCountersKHR::GetMetricsSets(
        uint32_t count,
        VkProfilerPerformanceMetricsSetProperties2EXT* pProperties ) const
    {
        std::shared_lock metricsSetsLock( m_MetricsSetsMutex );

        // Fill in the properties.
        const size_t writeCount = std::min<size_t>( count, m_MetricsSets.size() );
        for( size_t i = 0; i < writeCount; ++i )
        {
            FillPerformanceMetricsSetProperties( m_MetricsSets[i], pProperties[i] );
        }

        return static_cast<uint32_t>( m_MetricsSets.size() );
    }

    /***********************************************************************************\

    Function:
        GetMetricsSetProperties

    Description:
        Returns properties of the specified performance counter set.

    \***********************************************************************************/
    void DeviceProfilerPerformanceCountersKHR::GetMetricsSetProperties(
        uint32_t metricsSetIndex,
        VkProfilerPerformanceMetricsSetProperties2EXT* pProperties ) const
    {
        std::shared_lock metricsSetsLock( m_MetricsSetsMutex );

        if( metricsSetIndex >= m_MetricsSets.size() )
        {
            memset( pProperties, 0, sizeof( VkProfilerPerformanceMetricsSetProperties2EXT ) );
            return;
        }

        FillPerformanceMetricsSetProperties( m_MetricsSets[metricsSetIndex], *pProperties );
    }

    /***********************************************************************************\

    Function:
        GetMetricsSetMetricsProperties

    Description:
        Returns list of performance counters in the specified counter set.

    \***********************************************************************************/
    uint32_t DeviceProfilerPerformanceCountersKHR::GetMetricsSetMetricsProperties(
        uint32_t metricsSetIndex,
        uint32_t counterCount,
        VkProfilerPerformanceCounterProperties2EXT* pCounters ) const
    {
        std::shared_lock metricsSetsLock( m_MetricsSetsMutex );

        if( metricsSetIndex >= m_MetricsSets.size() )
        {
            return 0;
        }

        // Fill in the properties.
        const auto& metricsSet = m_MetricsSets[metricsSetIndex];
        const size_t writeCount = std::min<size_t>( counterCount, metricsSet.m_CounterIndices.size() );
        for( size_t i = 0; i < writeCount; ++i )
        {
            const uint32_t counterIndex = metricsSet.m_CounterIndices[i];
            const Counter& counter = m_Counters.at( counterIndex );

            FillPerformanceCounterProperties( counter, pCounters[i] );
        }

        return static_cast<uint32_t>( metricsSet.m_CounterIndices.size() );
    }

    /***********************************************************************************\

    Function:
        GetMetricsProperties

    Description:
        Returns list of all performance counters.

    \***********************************************************************************/
    uint32_t DeviceProfilerPerformanceCountersKHR::GetMetricsProperties(
        uint32_t counterCount,
        VkProfilerPerformanceCounterProperties2EXT* pCounters ) const
    {
        // Fill in the properties.
        const size_t writeCount = std::min<size_t>( counterCount, m_Counters.size() );
        for( size_t i = 0; i < counterCount; ++i )
        {
            FillPerformanceCounterProperties( m_Counters[i], pCounters[i] );
        }

        return static_cast<uint32_t>( m_Counters.size() );
    }

    /***********************************************************************************\

    Function:
        GetAvailableMetrics

    Description:
        Updates the list of available performance counters for a given queue family.

    \***********************************************************************************/
    void DeviceProfilerPerformanceCountersKHR::GetAvailableMetrics(
        uint32_t selectedCountersCount,
        const uint32_t* pSelectedCounters,
        uint32_t& availableCountersCount,
        uint32_t* pAvailableCounters ) const
    {
        PFN_vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR pfnGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR =
            m_pDevice->pInstance->Callbacks.GetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR;
        assert( pfnGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR != nullptr );

        // Prepare a createInfo structure to test the number of passes.
        std::vector<uint32_t> testedCounters( 0 );
        testedCounters.reserve( selectedCountersCount + 1 );

        for( uint32_t queueFamilyIndex : m_UsedQueueFamilies )
        {
            testedCounters.clear();

            // Construct a list of allocated counters for the given queue family.
            for( uint32_t i = 0; i < selectedCountersCount; ++i )
            {
                const Counter& counter = m_Counters.at( pSelectedCounters[i] );
                const uint32_t counterIndexInFamily = counter.m_QueueFamilyCounterIndices[queueFamilyIndex];

                if( counterIndexInFamily != UINT32_MAX )
                {
                    testedCounters.push_back( counterIndexInFamily );
                }
            }

            // Reserve placeholder for the tested counter.
            testedCounters.push_back( UINT32_MAX );

            VkQueryPoolPerformanceCreateInfoKHR performanceCreateInfo = {};
            performanceCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_PERFORMANCE_CREATE_INFO_KHR;
            performanceCreateInfo.queueFamilyIndex = queueFamilyIndex;
            performanceCreateInfo.counterIndexCount = static_cast<uint32_t>( testedCounters.size() );
            performanceCreateInfo.pCounterIndices = testedCounters.data();

            // Check which counters are not increasing the number of required runs.
            uint32_t* counterIt = pAvailableCounters;
            uint32_t* counterEnd = pAvailableCounters + availableCountersCount;

            while( counterIt != counterEnd )
            {
                testedCounters.back() = *counterIt;

                uint32_t numPasses = 0;
                pfnGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR(
                    m_pDevice->pPhysicalDevice->Handle,
                    &performanceCreateInfo,
                    &numPasses );

                if( numPasses > 1 )
                {
                    counterEnd = std::remove( counterIt, counterEnd, *counterIt );
                    continue;
                }

                counterIt++;
            }

            availableCountersCount = static_cast<uint32_t>( counterEnd - pAvailableCounters );
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

        VkQueryPoolPerformanceCreateInfoKHR performanceCreateInfo = {};
        performanceCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_PERFORMANCE_CREATE_INFO_KHR;
        performanceCreateInfo.queueFamilyIndex = queueFamilyIndex;

        const uint32_t activeMetricsSetIndex = GetActiveMetricsSetIndex();
        if( activeMetricsSetIndex < m_MetricsSets.size() )
        {
            const MetricsSet& activeCounters = m_MetricsSets[activeMetricsSetIndex];
            const MetricsSetQueueFamilyCounters& queueFamilyCounters = activeCounters.m_QueueFamilyCounters[queueFamilyIndex];
            performanceCreateInfo.counterIndexCount = static_cast<uint32_t>( queueFamilyCounters.m_CounterIndices.size() );
            performanceCreateInfo.pCounterIndices = queueFamilyCounters.m_CounterIndices.data();
        }

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
    uint32_t DeviceProfilerPerformanceCountersKHR::CreateCustomMetricsSet( const VkProfilerCustomPerformanceMetricsSetCreateInfoEXT* pCreateInfo )
    {
        assert( pCreateInfo->sType == VK_STRUCTURE_TYPE_PROFILER_CUSTOM_PERFORMANCE_METRICS_SET_CREATE_INFO_EXT );
        assert( pCreateInfo->pNext == nullptr );

        // Validate parameters.
        if( !pCreateInfo->metricsCount || !pCreateInfo->pMetricsIndices )
        {
            return UINT32_MAX;
        }

        // Sort counter indices.
        std::vector<uint32_t> sortedCounterIndices(
            pCreateInfo->pMetricsIndices,
            pCreateInfo->pMetricsIndices + pCreateInfo->metricsCount );
        std::sort( sortedCounterIndices.begin(), sortedCounterIndices.end() );

        // Calculate a hash of the counter set to identify compatible sets.
        HashInput hashInput;

        for( uint32_t counterIndex : sortedCounterIndices )
        {
            const Counter& counter = m_Counters.at( counterIndex );
            hashInput.Add( counter.m_UUID, sizeof( counter.m_UUID ) );
        }

        uint32_t compatibleHash = Farmhash::Fingerprint32(
            hashInput.GetData(),
            hashInput.GetSize() );

        // Calculate a full hash to identify identical sets.
        hashInput.Reset();
        hashInput.Add( compatibleHash );
        hashInput.Add( pCreateInfo->pName );
        hashInput.Add( pCreateInfo->pDescription );

        uint32_t fullHash = Farmhash::Fingerprint32(
            hashInput.GetData(),
            hashInput.GetSize() );

        // Check if an identical counter set already exists.
        uint32_t metricsSetIndex = FindMetricsSetByHash( fullHash );

        // Create and register the counter set if it does not exist yet.
        if( metricsSetIndex == UINT32_MAX )
        {
            MetricsSet metricsSet = {};
            metricsSet.m_Name = pCreateInfo->pName ? pCreateInfo->pName : std::string();
            metricsSet.m_Description = pCreateInfo->pDescription ? pCreateInfo->pDescription : std::string();
            metricsSet.m_CounterIndices = std::move( sortedCounterIndices );
            metricsSet.m_CompatibleHash = compatibleHash;
            metricsSet.m_FullHash = fullHash;

            for( uint32_t queueFamilyIndex : m_UsedQueueFamilies )
            {
                MetricsSetQueueFamilyCounters queueFamilyCounters = {};

                const size_t counterCount = metricsSet.m_CounterIndices.size();
                for( size_t i = 0; i < counterCount; ++i )
                {
                    const uint32_t counterIndex = metricsSet.m_CounterIndices[i];
                    const Counter& counter = m_Counters.at( counterIndex );
                    const uint32_t counterIndexInFamily = counter.m_QueueFamilyCounterIndices[queueFamilyIndex];

                    if( counterIndexInFamily != UINT32_MAX )
                    {
                        queueFamilyCounters.m_CounterIndices.push_back( counterIndexInFamily );
                        queueFamilyCounters.m_ReverseMapping.push_back( static_cast<uint32_t>( i ) );
                    }
                }

                metricsSet.m_QueueFamilyCounters.push_back( std::move( queueFamilyCounters ) );
            }

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
        UpdateCustomMetricsSets

    Description:
        Update properties of existing custom counter sets.

    \***********************************************************************************/
    void DeviceProfilerPerformanceCountersKHR::UpdateCustomMetricsSets( uint32_t updateCount, const VkProfilerCustomPerformanceMetricsSetUpdateInfoEXT* pUpdateInfos )
    {
        if( !updateCount || !pUpdateInfos )
        {
            return;
        }

        // Avoid vector reallocation during updates.
        std::scoped_lock lock( m_MetricsSetsMutex );

        for( uint32_t i = 0; i < updateCount; ++i )
        {
            const VkProfilerCustomPerformanceMetricsSetUpdateInfoEXT& updateInfo = pUpdateInfos[i];
            assert( updateInfo.sType == VK_STRUCTURE_TYPE_PROFILER_CUSTOM_PERFORMANCE_METRICS_SET_UPDATE_INFO_EXT );
            assert( updateInfo.pNext == nullptr );

            if( updateInfo.metricsSetIndex >= m_MetricsSets.size() )
            {
                continue;
            }

            // Update the metrics set properties.
            MetricsSet& metricsSet = m_MetricsSets[updateInfo.metricsSetIndex];

            if( updateInfo.pName != nullptr )
            {
                metricsSet.m_Name = updateInfo.pName;
            }

            if( updateInfo.pDescription != nullptr )
            {
                metricsSet.m_Description = updateInfo.pDescription;
            }

            // Update the full hash.
            HashInput hashInput;
            hashInput.Add( metricsSet.m_CompatibleHash );
            hashInput.Add( metricsSet.m_Name );
            hashInput.Add( metricsSet.m_Description );

            metricsSet.m_FullHash = Farmhash::Fingerprint32(
                hashInput.GetData(),
                hashInput.GetSize() );
        }
    }

    /***********************************************************************************\

    Function:
        ParseReport

    Description:
        Convert raw performance query report into human readable results.

    \***********************************************************************************/
    void DeviceProfilerPerformanceCountersKHR::ParseReport(
        uint32_t metricsSetIndex,
        uint32_t queueFamilyIndex,
        uint32_t reportSize,
        const uint8_t* pReport,
        std::vector<VkProfilerPerformanceCounterResultEXT>& results )
    {
        static_assert( sizeof( VkPerformanceCounterResultKHR ) == sizeof( VkProfilerPerformanceCounterResultEXT ) );
        assert( reportSize == GetReportSize( metricsSetIndex, queueFamilyIndex ) );

        std::shared_lock metricsSetsLock( m_MetricsSetsMutex );

        const MetricsSet& metricsSet = m_MetricsSets.at( metricsSetIndex );
        const MetricsSetQueueFamilyCounters& queueFamilyCounters = metricsSet.m_QueueFamilyCounters.at( queueFamilyIndex );

        // Allocate space for the results.
        results.resize( metricsSet.m_CounterIndices.size() );
        memset( results.data(), 0, results.size() * sizeof( VkProfilerPerformanceCounterResultEXT ) );

        const size_t counterCount = queueFamilyCounters.m_CounterIndices.size();
        for( size_t i = 0; i < counterCount; ++i )
        {
            const uint32_t counterIndex = queueFamilyCounters.m_ReverseMapping[i];
            results[counterIndex] = reinterpret_cast<const VkProfilerPerformanceCounterResultEXT*>( pReport )[i];
        }
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
        RegisterCounter

    Description:
        Append the counter to the list of available counters.

    \***********************************************************************************/
    void DeviceProfilerPerformanceCountersKHR::RegisterCounter(
        uint32_t queueFamilyIndex,
        uint32_t counterIndexInFamily,
        const VkPerformanceCounterKHR& counter,
        const VkPerformanceCounterDescriptionKHR& description )
    {
        // Check if the counter is already registered.
        for( Counter& existingCounter : m_Counters )
        {
            if( memcmp( existingCounter.m_UUID, counter.uuid, sizeof( counter.uuid ) ) == 0 )
            {
                // Counter already registered, just update the queue family index.
                existingCounter.m_QueueFamilyCounterIndices[queueFamilyIndex] = counterIndexInFamily;
                return;
            }
        }

        // Create a new counter.
        Counter newCounter = {};
        newCounter.m_Name = description.name;
        newCounter.m_Category = description.category;
        newCounter.m_Description = description.description;
        newCounter.m_QueueFamilyCounterIndices.resize( m_QueueFamilyCount, UINT32_MAX );
        newCounter.m_QueueFamilyCounterIndices[queueFamilyIndex] = counterIndexInFamily;
        newCounter.m_Flags = static_cast<VkProfilerPerformanceCounterFlagsEXT>( description.flags );
        newCounter.m_Unit = static_cast<VkProfilerPerformanceCounterUnitEXT>( counter.unit );
        newCounter.m_Storage = static_cast<VkProfilerPerformanceCounterStorageEXT>( counter.storage );
        memcpy( newCounter.m_UUID, counter.uuid, sizeof( counter.uuid ) );

        m_Counters.push_back( std::move( newCounter ) );
    }

    /***********************************************************************************\

    Function:
        FillPerformanceMetricsSetProperties

    Description:
        Fill in the properties structure using internal counter set representation.

    \***********************************************************************************/
    void DeviceProfilerPerformanceCountersKHR::FillPerformanceMetricsSetProperties(
        const MetricsSet& metricsSet,
        VkProfilerPerformanceMetricsSetProperties2EXT& properties )
    {
        ProfilerStringFunctions::CopyString(
            properties.name,
            std::size( properties.name ),
            metricsSet.m_Name.c_str(),
            metricsSet.m_Name.length() );

        ProfilerStringFunctions::CopyString(
            properties.description,
            std::size( properties.description ),
            metricsSet.m_Description.c_str(),
            metricsSet.m_Description.length() );

        properties.metricsCount = static_cast<uint32_t>( metricsSet.m_CounterIndices.size() );
    }

    /***********************************************************************************\

    Function:
        FillPerformanceCounterProperties

    Description:
        Fill in the properties structure using VK_KHR_performance_query structures.

    \***********************************************************************************/
    void DeviceProfilerPerformanceCountersKHR::FillPerformanceCounterProperties(
        const Counter& counter,
        VkProfilerPerformanceCounterProperties2EXT& properties )
    {
        ProfilerStringFunctions::CopyString(
            properties.shortName,
            std::size( properties.shortName ),
            counter.m_Name.c_str(),
            counter.m_Name.length() );

        ProfilerStringFunctions::CopyString(
            properties.category,
            std::size( properties.category ),
            counter.m_Category.c_str(),
            counter.m_Category.length() );

        ProfilerStringFunctions::CopyString(
            properties.description,
            std::size( properties.description ),
            counter.m_Description.c_str(),
            counter.m_Description.length() );

        properties.flags = counter.m_Flags;
        properties.unit = counter.m_Unit;
        properties.storage = counter.m_Storage;

        memcpy( properties.uuid, counter.m_UUID, sizeof( properties.uuid ) );
    }
}

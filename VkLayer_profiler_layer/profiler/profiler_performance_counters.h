// Copyright (c) 2025-2026 Lukasz Stalmirski
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
#include "profiler_helpers.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <shared_mutex>
// Import extension structures
#include "profiler_ext/VkProfilerEXT.h"

namespace Profiler
{
    struct VkDevice_Object;
    struct DeviceProfilerConfig;

    /***********************************************************************************\

    Class:
        DeviceProfilerPerformanceCountersStreamResult

    Description:
        An intermediate structure holding performance counters stream data with all
        counters at a specific timestamp.

    \***********************************************************************************/
    struct DeviceProfilerPerformanceCountersStreamResult
    {
        uint64_t m_GpuTimestamp;
        uint64_t m_CpuTimestamp;
        uint32_t m_MetricsSetIndex;
        std::vector<VkProfilerPerformanceCounterResultEXT> m_Data;
    };

    /***********************************************************************************\

    Class:
        DeviceProfilerPerformanceCounters

    Description:
        Common interface for all supported performance counters providers.
        Default implementation provides no functionality.

    \***********************************************************************************/
    class DeviceProfilerPerformanceCounters
    {
    public:
        virtual ~DeviceProfilerPerformanceCounters() {}

        virtual VkResult Initialize( VkDevice_Object* pDevice, const DeviceProfilerConfig& config ) { return VK_ERROR_FEATURE_NOT_PRESENT; }
        virtual void Destroy() {}

        virtual VkResult SetQueuePerformanceConfiguration( VkQueue queue ) { return VK_SUCCESS; }

        virtual VkProfilerPerformanceCountersSamplingModeEXT GetSamplingMode() const { return VK_PROFILER_PERFORMANCE_COUNTERS_SAMPLING_MODE_QUERY_EXT; }

        virtual uint32_t GetReportSize( uint32_t metricsSetIndex, uint32_t queueFamilyIndex ) const { return 0; }
        virtual uint32_t GetMetricsCount( uint32_t metricsSetIndex ) const { return 0; }
        virtual uint32_t GetMetricsSetCount() const { return 0; }
        virtual VkResult SetActiveMetricsSet( uint32_t metricsSetIndex ) { return VK_ERROR_FEATURE_NOT_PRESENT; }
        virtual uint32_t GetActiveMetricsSetIndex() const { return 0; }
        virtual bool AreMetricsSetsCompatible( uint32_t metricsSet1, uint32_t metricsSet2 ) const { return false; }
        virtual uint32_t GetRequiredPasses( uint32_t counterCount, const uint32_t* pCounterIndices ) const { return 0; }
        virtual uint32_t GetMetricsSets( uint32_t count, VkProfilerPerformanceMetricsSetProperties2EXT* pProperties ) const { return 0; }
        virtual void GetMetricsSetProperties( uint32_t metricsSetIndex, VkProfilerPerformanceMetricsSetProperties2EXT* pProperties ) const {}
        virtual uint32_t GetMetricsSetMetricsProperties( uint32_t metricsSetIndex, uint32_t count, VkProfilerPerformanceCounterProperties2EXT* pProperties ) const { return 0; }
        virtual uint32_t GetMetricsProperties( uint32_t count, VkProfilerPerformanceCounterProperties2EXT* pProperties ) const { return 0; }
        virtual void GetAvailableMetrics( uint32_t selectedCountersCount, const uint32_t* pSelectedCounters, uint32_t& availableCountersCount, uint32_t* pAvailableCounters ) const { availableCountersCount = 0; }

        virtual bool SupportsQueryPoolReuse() const { return false; }
        virtual VkResult CreateQueryPool( uint32_t queueFamilyIndex, uint32_t size, VkQueryPool* pQueryPool ) { return VK_ERROR_FEATURE_NOT_PRESENT; }

        virtual bool SupportsCustomMetricsSets() const { return false; }
        virtual uint32_t CreateCustomMetricsSet( const VkProfilerCustomPerformanceMetricsSetCreateInfoEXT* pCreateInfo ) { return UINT32_MAX; }
        virtual void DestroyCustomMetricsSet( uint32_t ) {}
        virtual void UpdateCustomMetricsSets( uint32_t updateCount, const VkProfilerCustomPerformanceMetricsSetUpdateInfoEXT* pUpdateInfos ) {}

        virtual bool ReadStreamData( uint64_t beginTimestamp, uint64_t endTimestamp, std::vector<DeviceProfilerPerformanceCountersStreamResult>& results ) { return true; }

        virtual void ParseReport( uint32_t metricsSetIndex, uint32_t queueFamilyIndex, uint32_t reportSize, const uint8_t* pReport, std::vector<VkProfilerPerformanceCounterResultEXT>& results ) {}
    };

    /***********************************************************************************\

    Class:
        DeviceProfilerCustomMetricsSetBuilder

    Description:
        Helper class for building custom metrics sets.

    \***********************************************************************************/
    template<typename CustomMetricsSetManager>
    class DeviceProfilerCustomMetricsSetBuilder
    {
    public:
        DeviceProfilerCustomMetricsSetBuilder(
            const VkProfilerCustomPerformanceMetricsSetCreateInfoEXT* pCreateInfo,
            const CustomMetricsSetManager& metricsSetManager );

        bool IsInputValid() const { return m_InputValid; }
        auto& GetSortedCounterIndices() const { return m_SortedCounterIndices; }
        uint32_t GetCompatibleMetricsSetHash() const { return m_CompatibleHash; }
        uint32_t GetFullMetricsSetHash() const { return m_FullHash; }
        uint32_t GetExistingMetricsSetIndex() const { return m_ExistingMetricsSetIndex; }

    private:
        const CustomMetricsSetManager& m_MetricsSetManager;

        std::vector<uint32_t> m_SortedCounterIndices;
        uint32_t m_CompatibleHash;
        uint32_t m_FullHash;
        uint32_t m_ExistingMetricsSetIndex;

        bool m_InputValid;
        bool ValidateMetricsSetCreateInfo(
            const VkProfilerCustomPerformanceMetricsSetCreateInfoEXT* pCreateInfo ) const;
    };

    /***********************************************************************************\

    Class:
        DeviceProfilerCustomMetricsSetManager

    Description:
        Helper class for managing custom metrics sets.

    \***********************************************************************************/
    template<typename MetricsSet, typename Counter>
    class DeviceProfilerCustomMetricsSetManager
    {
    public:
        void Destroy();

        uint32_t FindMetricsSetByHash( uint32_t fullHash ) const;
        uint32_t RegisterMetricsSet( MetricsSet&& metricsSet );
        void UnregisterMetricsSet( uint32_t metricsSetIndex );
        auto& GetMetricsSetsMutex() const { return m_MetricsSetsMutex; }
        auto& GetMetricsSet( uint32_t metricsSetIndex ) const { return m_MetricsSets.at( metricsSetIndex ); }
        auto& GetMetricsSets() { return m_MetricsSets; }
        auto& GetMetricsSets() const { return m_MetricsSets; }

        void RegisterCounter( Counter&& counter );
        auto& GetCounter( uint32_t counterIndex ) const { return m_Counters.at( counterIndex ); }
        auto& GetCounters() { return m_Counters; }
        auto& GetCounters() const { return m_Counters; }

    private:
        std::shared_mutex mutable m_MetricsSetsMutex;
        std::vector<MetricsSet> m_MetricsSets;
        std::vector<Counter> m_Counters;
    };

    /***********************************************************************************\

    Function:
        DeviceProfilerCustomMetricsSetBuilder

    Description:
        Constructor.

    \***********************************************************************************/
    template<typename CustomMetricsSetManager>
    inline DeviceProfilerCustomMetricsSetBuilder<CustomMetricsSetManager>::DeviceProfilerCustomMetricsSetBuilder(
        const VkProfilerCustomPerformanceMetricsSetCreateInfoEXT* pCreateInfo,
        const CustomMetricsSetManager& metricsSetManager )
        : m_MetricsSetManager( metricsSetManager )
        , m_SortedCounterIndices( 0 )
        , m_CompatibleHash( 0 )
        , m_FullHash( 0 )
        , m_ExistingMetricsSetIndex( UINT32_MAX )
        , m_InputValid( ValidateMetricsSetCreateInfo( pCreateInfo ) )
    {
        if( m_InputValid )
        {
            // Sort counter indices.
            m_SortedCounterIndices = std::vector<uint32_t>(
                pCreateInfo->pMetricsIndices,
                pCreateInfo->pMetricsIndices + pCreateInfo->metricsCount );
            std::sort( m_SortedCounterIndices.begin(), m_SortedCounterIndices.end() );

            // Calculate a hash of the counter set to identify compatible sets.
            HashInput hashInput;

            for( uint32_t counterIndex : m_SortedCounterIndices )
            {
                const auto& counter = m_MetricsSetManager.GetCounter( counterIndex );
                hashInput.Add( counter.m_UUID, sizeof( counter.m_UUID ) );
            }

            m_CompatibleHash = Farmhash::Fingerprint32(
                hashInput.GetData(),
                hashInput.GetSize() );

            // Calculate a full hash to identify identical sets.
            hashInput.Reset();
            hashInput.Add( m_CompatibleHash );
            hashInput.Add( pCreateInfo->pName );
            hashInput.Add( pCreateInfo->pDescription );

            m_FullHash = Farmhash::Fingerprint32(
                hashInput.GetData(),
                hashInput.GetSize() );

            // Check if an identical counter set already exists.
            m_ExistingMetricsSetIndex = m_MetricsSetManager.FindMetricsSetByHash( m_FullHash );
        }
    }

    /***********************************************************************************\

    Function:
        ValidateMetricsSetCreateInfo

    Description:
        Checks if the provided create info structure is valid.

    \***********************************************************************************/
    template<typename Counter>
    inline bool DeviceProfilerCustomMetricsSetBuilder<Counter>::ValidateMetricsSetCreateInfo(
        const VkProfilerCustomPerformanceMetricsSetCreateInfoEXT* pCreateInfo ) const
    {
        if( ( pCreateInfo == nullptr ) ||
            ( pCreateInfo->sType != VK_STRUCTURE_TYPE_PROFILER_CUSTOM_PERFORMANCE_METRICS_SET_CREATE_INFO_EXT ) ||
            ( pCreateInfo->metricsCount == 0 ) ||
            ( pCreateInfo->pMetricsIndices == nullptr ) )
        {
            return false;
        }

        const size_t countersCount = m_MetricsSetManager.GetCounters().size();
        for( uint32_t i = 0; i < pCreateInfo->metricsCount; ++i )
        {
            const uint32_t counterIndex = pCreateInfo->pMetricsIndices[i];
            if( counterIndex >= countersCount )
            {
                return false;
            }
        }

        return true;
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:
        Removes all registered counter sets.

    \***********************************************************************************/
    template<typename MetricsSet, typename Counter>
    inline void DeviceProfilerCustomMetricsSetManager<MetricsSet, Counter>::Destroy()
    {
        std::unique_lock metricsSetsLock( m_MetricsSetsMutex );

        m_MetricsSets.clear();
    }

    /***********************************************************************************\

    Function:
        FindMetricsSetByHash

    Description:
        Returns the index of the counter set with the specified full hash or
        UINT32_MAX if not found.

    \***********************************************************************************/
    template<typename MetricsSet, typename Counter>
    inline uint32_t DeviceProfilerCustomMetricsSetManager<MetricsSet, Counter>::FindMetricsSetByHash(
        uint32_t fullHash ) const
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
        Appends the counter set to the list of available sets.

    \***********************************************************************************/
    template<typename MetricsSet, typename Counter>
    inline uint32_t DeviceProfilerCustomMetricsSetManager<MetricsSet, Counter>::RegisterMetricsSet(
        MetricsSet&& metricsSet )
    {
        std::unique_lock metricsSetsLock( m_MetricsSetsMutex );

        // Append the counter set to the list.
        m_MetricsSets.push_back( std::move( metricsSet ) );

        // Return the index of the newly created counter set.
        return static_cast<uint32_t>( m_MetricsSets.size() - 1 );
    }

    /***********************************************************************************\

    Function:
        UnregisterMetricsSet

    Description:
        Removes the counter set from the list of available sets.

    \***********************************************************************************/
    template<typename MetricsSet, typename Counter>
    inline void DeviceProfilerCustomMetricsSetManager<MetricsSet, Counter>::UnregisterMetricsSet(
        uint32_t metricsSetIndex )
    {
        std::unique_lock metricsSetsLock( m_MetricsSetsMutex );

        // Remove the counter set from the vector.
        if( metricsSetIndex < m_MetricsSets.size() )
        {
            m_MetricsSets.erase( m_MetricsSets.begin() + metricsSetIndex );
        }
    }

    /***********************************************************************************\

    Function:
        RegisterCounter

    Description:
        Appends the counter to the list of available counters.

    \***********************************************************************************/
    template<typename MetricsSet, typename Counter>
    inline void DeviceProfilerCustomMetricsSetManager<MetricsSet, Counter>::RegisterCounter(
        Counter&& counter )
    {
        // Append the counter to the list.
        m_Counters.push_back( std::move( counter ) );
    }
}

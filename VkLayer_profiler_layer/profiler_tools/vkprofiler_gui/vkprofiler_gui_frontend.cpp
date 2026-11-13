// Copyright (c) 2026 Lukasz Stalmirski
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

#include "vkprofiler_gui_frontend.h"

namespace Profiler::GUI
{
    /***********************************************************************************\

    Function:
        ProfilerProxyFrontend

    Description:
        Constructor.

    \***********************************************************************************/
    ProfilerProxyFrontend::ProfilerProxyFrontend()
        : m_pSource( nullptr )
    {
    }

    /***********************************************************************************\

    Function:
        SetSource

    Description:
        Sets the source frontend.

    \***********************************************************************************/
    void ProfilerProxyFrontend::SetSource( DeviceProfilerFrontend* pSource )
    {
        m_pSource = pSource;
    }

    /***********************************************************************************\

    Function:
        IsAvailable

    Description:
        Checks if the source frontend is available.

    \***********************************************************************************/
    bool ProfilerProxyFrontend::IsAvailable()
    {
        return m_pSource && m_pSource->IsAvailable();
    }

    /***********************************************************************************\

    Function:
        GetApplicationInfo

    Description:
        Returns the application info.

    \***********************************************************************************/
    const VkApplicationInfo& ProfilerProxyFrontend::GetApplicationInfo()
    {
        static const VkApplicationInfo scNullApplicationInfo = {};
        return m_pSource ? m_pSource->GetApplicationInfo() : scNullApplicationInfo;
    }

    /***********************************************************************************\

    Function:
        GetPhysicalDeviceProperties

    Description:
        Returns the physical device properties.

    \***********************************************************************************/
    const VkPhysicalDeviceProperties& ProfilerProxyFrontend::GetPhysicalDeviceProperties()
    {
        static const VkPhysicalDeviceProperties scNullPhysicalDeviceProperties = {};
        return m_pSource ? m_pSource->GetPhysicalDeviceProperties() : scNullPhysicalDeviceProperties;
    }

    /***********************************************************************************\

    Function:
        GetPhysicalDeviceMemoryProperties

    Description:
        Returns the physical device memory properties.

    \***********************************************************************************/
    const VkPhysicalDeviceMemoryProperties& ProfilerProxyFrontend::GetPhysicalDeviceMemoryProperties()
    {
        static const VkPhysicalDeviceMemoryProperties scNullPhysicalDeviceMemoryProperties = {};
        return m_pSource ? m_pSource->GetPhysicalDeviceMemoryProperties() : scNullPhysicalDeviceMemoryProperties;
    }

    /***********************************************************************************\

    Function:
        GetQueueFamilyProperties

    Description:
        Returns the queue family properties.

    \***********************************************************************************/
    const std::vector<VkQueueFamilyProperties>& ProfilerProxyFrontend::GetQueueFamilyProperties()
    {
        static const std::vector<VkQueueFamilyProperties> scNullQueueFamilyProperties = {};
        return m_pSource ? m_pSource->GetQueueFamilyProperties() : scNullQueueFamilyProperties;
    }

    /***********************************************************************************\

    Function:
        GetEnabledInstanceExtensions

    Description:
        Returns the enabled instance extensions.

    \***********************************************************************************/
    const std::unordered_set<std::string>& ProfilerProxyFrontend::GetEnabledInstanceExtensions()
    {
        static const std::unordered_set<std::string> scNullInstanceExtensions = {};
        return m_pSource ? m_pSource->GetEnabledInstanceExtensions() : scNullInstanceExtensions;
    }

    /***********************************************************************************\

    Function:
        GetEnabledDeviceExtensions

    Description:
        Returns the enabled device extensions.

    \***********************************************************************************/
    const std::unordered_set<std::string>& ProfilerProxyFrontend::GetEnabledDeviceExtensions()
    {
        static const std::unordered_set<std::string> scNullDeviceExtensions = {};
        return m_pSource ? m_pSource->GetEnabledDeviceExtensions() : scNullDeviceExtensions;
    }

    /***********************************************************************************\

    Function:
        GetDeviceQueues

    Description:
        Returns the available device queues.

    \***********************************************************************************/
    const std::unordered_map<VkQueue, VkQueue_Object>& ProfilerProxyFrontend::GetDeviceQueues()
    {
        static const std::unordered_map<VkQueue, VkQueue_Object> scNullDeviceQueues = {};
        return m_pSource ? m_pSource->GetDeviceQueues() : scNullDeviceQueues;
    }

    /***********************************************************************************\

    Function:
        SupportsCustomPerformanceMetricsSets

    Description:
        Checks if the source frontend supports custom performance metrics sets.

    \***********************************************************************************/
    bool ProfilerProxyFrontend::SupportsCustomPerformanceMetricsSets()
    {
        return m_pSource ? m_pSource->SupportsCustomPerformanceMetricsSets() : false;
    }

    /***********************************************************************************\

    Function:
        CreateCustomPerformanceMetricsSet

    Description:
        Creates a custom performance metrics set.

    \***********************************************************************************/
    uint32_t ProfilerProxyFrontend::CreateCustomPerformanceMetricsSet( const VkProfilerCustomPerformanceMetricsSetCreateInfoEXT* pCreateInfo )
    {
        return m_pSource ? m_pSource->CreateCustomPerformanceMetricsSet( pCreateInfo ) : UINT32_MAX;
    }

    /***********************************************************************************\

    Function:
        DestroyCustomPerformanceMetricsSet

    Description:
        Destroys the custom performance metrics set.

    \***********************************************************************************/
    void ProfilerProxyFrontend::DestroyCustomPerformanceMetricsSet( uint32_t setIndex )
    {
        if( m_pSource )
        {
            m_pSource->DestroyCustomPerformanceMetricsSet( setIndex );
        }
    }

    /***********************************************************************************\

    Function:
        UpdateCustomPerformanceMetricsSets

    Description:
        Updates custom performance metrics sets.

    \***********************************************************************************/
    void ProfilerProxyFrontend::UpdateCustomPerformanceMetricsSets( uint32_t updateCount, const VkProfilerCustomPerformanceMetricsSetUpdateInfoEXT* pUpdateInfos )
    {
        if( m_pSource )
        {
            m_pSource->UpdateCustomPerformanceMetricsSets( updateCount, pUpdateInfos );
        }
    }

    /***********************************************************************************\

    Function:
        GetPerformanceCounterProperties

    Description:
        Returns performance counter properties.

    \***********************************************************************************/
    uint32_t ProfilerProxyFrontend::GetPerformanceCounterProperties( uint32_t counterCount, VkProfilerPerformanceCounterProperties2EXT* pCounters )
    {
        return m_pSource ? m_pSource->GetPerformanceCounterProperties( counterCount, pCounters ) : 0;
    }

    /***********************************************************************************\

    Function:
        GetPerformanceMetricsSets

    Description:
        Returns performance metrics sets.

    \***********************************************************************************/
    uint32_t ProfilerProxyFrontend::GetPerformanceMetricsSets( uint32_t setCount, VkProfilerPerformanceMetricsSetProperties2EXT* pSets )
    {
        return m_pSource ? m_pSource->GetPerformanceMetricsSets( setCount, pSets ) : 0;
    }

    /***********************************************************************************\

    Function:
        GetPerformanceMetricsSetProperties

    Description:
        Returns performance metrics set properties.

    \***********************************************************************************/
    void ProfilerProxyFrontend::GetPerformanceMetricsSetProperties( uint32_t setIndex, VkProfilerPerformanceMetricsSetProperties2EXT* pProperties )
    {
        if( m_pSource )
        {
            m_pSource->GetPerformanceMetricsSetProperties( setIndex, pProperties );
        }
    }

    /***********************************************************************************\

    Function:
        GetPerformanceMetricsSetCounterProperties

    Description:
        Returns performance metrics set counter properties.

    \***********************************************************************************/
    uint32_t ProfilerProxyFrontend::GetPerformanceMetricsSetCounterProperties( uint32_t setIndex, uint32_t counterCount, VkProfilerPerformanceCounterProperties2EXT* pCounters )
    {
        return m_pSource ? m_pSource->GetPerformanceMetricsSetCounterProperties( setIndex, counterCount, pCounters ) : 0;
    }

    /***********************************************************************************\

    Function:
        GetPerformanceCounterRequiredPasses

    Description:
        Returns the number of required passes for selected performance counters.

    \***********************************************************************************/
    uint32_t ProfilerProxyFrontend::GetPerformanceCounterRequiredPasses( uint32_t counterCount, const uint32_t* pCounters )
    {
        return m_pSource ? m_pSource->GetPerformanceCounterRequiredPasses( counterCount, pCounters ) : 0;
    }

    /***********************************************************************************\

    Function:
        GetAvailablePerformanceCounters

    Description:
        Updates the list of available performance counters based on the selected counters.

    \***********************************************************************************/
    void ProfilerProxyFrontend::GetAvailablePerformanceCounters( uint32_t selectedCounterCount, const uint32_t* pSelectedCounters, uint32_t& availableCounterCount, uint32_t* pAvailableCounters )
    {
        if( m_pSource )
        {
            m_pSource->GetAvailablePerformanceCounters( selectedCounterCount, pSelectedCounters, availableCounterCount, pAvailableCounters );
        }
        else
        {
            availableCounterCount = 0;
        }
    }

    /***********************************************************************************\

    Function:
        SetPreformanceMetricsSetIndex

    Description:
        Sets the active performance metrics set index.

    \***********************************************************************************/
    VkResult ProfilerProxyFrontend::SetPreformanceMetricsSetIndex( uint32_t setIndex )
    {
        return m_pSource ? m_pSource->SetPreformanceMetricsSetIndex( setIndex ) : VK_ERROR_FEATURE_NOT_PRESENT;
    }

    /***********************************************************************************\

    Function:
        GetPerformanceMetricsSetIndex

    Description:
        Returns the active performance metrics set index.

    \***********************************************************************************/
    uint32_t ProfilerProxyFrontend::GetPerformanceMetricsSetIndex()
    {
        return m_pSource ? m_pSource->GetPerformanceMetricsSetIndex() : UINT32_MAX;
    }

    /***********************************************************************************\

    Function:
        GetPerformanceCountersSamplingMode

    Description:
        Returns the current performance counters sampling mode.

    \***********************************************************************************/
    VkProfilerPerformanceCountersSamplingModeEXT ProfilerProxyFrontend::GetPerformanceCountersSamplingMode()
    {
        return m_pSource ? m_pSource->GetPerformanceCountersSamplingMode() : VK_PROFILER_PERFORMANCE_COUNTERS_SAMPLING_MODE_QUERY_EXT;
    }

    /***********************************************************************************\

    Function:
        GetDeviceCreateTimestamp

    Description:
        Returns the device creation timestamp in the specified time domain.

    \***********************************************************************************/
    uint64_t ProfilerProxyFrontend::GetDeviceCreateTimestamp( VkTimeDomainEXT timeDomain )
    {
        return m_pSource ? m_pSource->GetDeviceCreateTimestamp( timeDomain ) : 0;
    }

    /***********************************************************************************\

    Function:
        GetHostTimestampFrequency

    Description:
        Returns host timestamp frequency in the specified time domain.

    \***********************************************************************************/
    uint64_t ProfilerProxyFrontend::GetHostTimestampFrequency( VkTimeDomainEXT timeDomain )
    {
        return m_pSource ? m_pSource->GetHostTimestampFrequency( timeDomain ) : 1'000'000'000;
    }

    /***********************************************************************************\

    Function:
        GetProfilerConfig

    Description:
        Returns profiler configuration.

    \***********************************************************************************/
    const DeviceProfilerConfig& ProfilerProxyFrontend::GetProfilerConfig()
    {
        static const DeviceProfilerConfig scNullConfig = {};
        return m_pSource ? m_pSource->GetProfilerConfig() : scNullConfig;
    }

    /***********************************************************************************\

    Function:
        GetProfilerFrameDelimiter

    Description:
        Returns the current frame delimiter used by the profiler.

    \***********************************************************************************/
    VkProfilerFrameDelimiterEXT ProfilerProxyFrontend::GetProfilerFrameDelimiter()
    {
        return m_pSource ? m_pSource->GetProfilerFrameDelimiter() : VK_PROFILER_FRAME_DELIMITER_PRESENT_EXT;
    }

    /***********************************************************************************\

    Function:
        SetProfilerFrameDelimiter

    Description:
        Sets the frame delimiter for the profiler.

    \***********************************************************************************/
    VkResult ProfilerProxyFrontend::SetProfilerFrameDelimiter( VkProfilerFrameDelimiterEXT frameDelimiter )
    {
        return m_pSource ? m_pSource->SetProfilerFrameDelimiter( frameDelimiter ) : VK_ERROR_FEATURE_NOT_PRESENT;
    }

    /***********************************************************************************\

    Function:
        GetProfilerSamplingMode

    Description:
        Returns the current sampling mode used by the profiler.

    \***********************************************************************************/
    VkProfilerModeEXT ProfilerProxyFrontend::GetProfilerSamplingMode()
    {
        return m_pSource ? m_pSource->GetProfilerSamplingMode() : VK_PROFILER_MODE_PER_DRAWCALL_EXT;
    }

    /***********************************************************************************\

    Function:
        SetProfilerSamplingMode

    Description:
        Sets the sampling mode for the profiler.

    \***********************************************************************************/
    VkResult ProfilerProxyFrontend::SetProfilerSamplingMode( VkProfilerModeEXT mode )
    {
        return m_pSource ? m_pSource->SetProfilerSamplingMode( mode ) : VK_ERROR_FEATURE_NOT_PRESENT;
    }

    /***********************************************************************************\

    Function:
        GetObjectName

    Description:
        Returns the debug name of the specified Vulkan object.

    \***********************************************************************************/
    std::string ProfilerProxyFrontend::GetObjectName( const VkObject& object )
    {
        return m_pSource ? m_pSource->GetObjectName( object ) : std::string();
    }

    /***********************************************************************************\

    Function:
        SetObjectName

    Description:
        Sets the debug name of the specified Vulkan object.

    \***********************************************************************************/
    void ProfilerProxyFrontend::SetObjectName( const VkObject& object, const std::string& name )
    {
        if( m_pSource )
        {
            m_pSource->SetObjectName( object, name );
        }
    }

    /***********************************************************************************\

    Function:
        GetData

    Description:
        Returns the latest profiler frame data.

    \***********************************************************************************/
    std::shared_ptr<DeviceProfilerFrameData> ProfilerProxyFrontend::GetData()
    {
        return m_pSource ? m_pSource->GetData() : std::make_shared<DeviceProfilerFrameData>();
    }

    /***********************************************************************************\

    Function:
        SetDataBufferSize

    Description:
        Sets the maximum number of frames to store in the profiler data buffer.

    \***********************************************************************************/
    void ProfilerProxyFrontend::SetDataBufferSize( uint32_t maxFrames )
    {
        if( m_pSource )
        {
            m_pSource->SetDataBufferSize( maxFrames );
        }
    }
}

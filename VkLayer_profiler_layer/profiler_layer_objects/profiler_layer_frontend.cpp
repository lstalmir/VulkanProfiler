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

#include "profiler_layer_frontend.h"
#include "profiler/profiler.h"
#include "VkDevice_object.h"
#include "VkQueue_object.h"
#include "VkInstance_object.h"
#include "VkPhysicalDevice_object.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        Initialize

    Description:
        Initializes the frontend.

    \***********************************************************************************/
    void DeviceProfilerLayerFrontend::Initialize( VkDevice_Object& device, DeviceProfiler& profiler )
    {
        m_pDevice = &device;
        m_pProfiler = &profiler;
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:
        Destroys the frontend.

    \***********************************************************************************/
    void DeviceProfilerLayerFrontend::Destroy()
    {
        m_pDevice = nullptr;
        m_pProfiler = nullptr;
    }

    /***********************************************************************************\

    Function:
        IsAvailable

    Description:
        Checks if the frontend has been initialized and can provide data from the
        profiler.

    \***********************************************************************************/
    bool DeviceProfilerLayerFrontend::IsAvailable()
    {
        return (m_pDevice != nullptr) && (m_pProfiler != nullptr);
    }

    /***********************************************************************************\

    Function:
        GetApplicationInfo

    Description:
        Returns VkApplicationInfo provided by the profiled application.

    \***********************************************************************************/
    const VkApplicationInfo& DeviceProfilerLayerFrontend::GetApplicationInfo()
    {
        return m_pDevice->pInstance->ApplicationInfo;
    }

    /***********************************************************************************\

    Function:
        GetPhysicalDeviceProperties

    Description:
        Returns properties of the profiled device.

    \***********************************************************************************/
    const VkPhysicalDeviceProperties& DeviceProfilerLayerFrontend::GetPhysicalDeviceProperties()
    {
        return m_pDevice->pPhysicalDevice->Properties;
    }

    /***********************************************************************************\

    Function:
        GetPhysicalDeviceMemoryProperties

    Description:
        Returns memory properties of the profiled device.

    \***********************************************************************************/
    const VkPhysicalDeviceMemoryProperties& DeviceProfilerLayerFrontend::GetPhysicalDeviceMemoryProperties()
    {
        return m_pDevice->pPhysicalDevice->MemoryProperties;
    }

    /***********************************************************************************\

    Function:
        GetQueueFamilyProperties

    Description:
        Returns queue family properties of the profiled device.

    \***********************************************************************************/
    const std::vector<VkQueueFamilyProperties>& DeviceProfilerLayerFrontend::GetQueueFamilyProperties()
    {
        return m_pDevice->pPhysicalDevice->QueueFamilyProperties;
    }

    /***********************************************************************************\

    Function:
        GetEnabledInstanceExtensions

    Description:
        Returns list of enabled instance extensions.

    \***********************************************************************************/
    const std::unordered_set<std::string>& DeviceProfilerLayerFrontend::GetEnabledInstanceExtensions()
    {
        return m_pDevice->pInstance->EnabledExtensions;
    }

    /***********************************************************************************\

    Function:
        GetEnabledDeviceExtensions

    Description:
        Returns list of enabled device extensions.

    \***********************************************************************************/
    const std::unordered_set<std::string>& DeviceProfilerLayerFrontend::GetEnabledDeviceExtensions()
    {
        return m_pDevice->EnabledExtensions;
    }

    /***********************************************************************************\

    Function:
        GetDeviceQueues

    Description:
        Returns list of queues created with the profiled device.

    \***********************************************************************************/
    const std::unordered_map<VkQueue, VkQueue_Object>& DeviceProfilerLayerFrontend::GetDeviceQueues()
    {
        return m_pDevice->Queues;
    }

    /***********************************************************************************\

    Function:
        SupportsCustomPerformanceMetricsSets

    Description:
        Checks if the profiler supports custom performance metrics sets.

    \***********************************************************************************/
    bool DeviceProfilerLayerFrontend::SupportsCustomPerformanceMetricsSets()
    {
        if( m_pProfiler->m_pPerformanceCounters != nullptr )
        {
            return m_pProfiler->m_pPerformanceCounters->SupportsCustomMetricsSets();
        }

        return false;
    }

    /***********************************************************************************\

    Function:
        CreateCustomPerformanceMetricsSet

    Description:
        Creates a custom performance metrics set.

    \***********************************************************************************/
    uint32_t DeviceProfilerLayerFrontend::CreateCustomPerformanceMetricsSet( uint32_t queueFamilyIndex, const std::string& name, const std::string& description, const std::vector<uint32_t>& counters )
    {
        if( m_pProfiler->m_pPerformanceCounters != nullptr )
        {
            return m_pProfiler->m_pPerformanceCounters->CreateCustomMetricsSet( queueFamilyIndex, name, description, counters );
        }

        return UINT32_MAX;
    }

    /***********************************************************************************\

    Function:
        DestroyCustomPerformanceMetricsSet

    Description:
        Destroys the custom performance metrics set.

    \***********************************************************************************/
    void DeviceProfilerLayerFrontend::DestroyCustomPerformanceMetricsSet( uint32_t setIndex )
    {
        if( m_pProfiler->m_pPerformanceCounters != nullptr )
        {
            m_pProfiler->m_pPerformanceCounters->DestroyCustomMetricsSet( setIndex );
        }
    }

    /***********************************************************************************\

    Function:
        DestroyCustomPerformanceMetricsSet

    Description:
        Destroys the custom performance metrics set.

    \***********************************************************************************/
    void DeviceProfilerLayerFrontend::GetQueueFamilyPerformanceCounterProperties( uint32_t queueFamilyIndex, std::vector<VkProfilerPerformanceCounterPropertiesEXT>& metrics )
    {
        metrics.clear();

        if( m_pProfiler->m_pPerformanceCounters != nullptr )
        {
            m_pProfiler->m_pPerformanceCounters->GetQueueFamilyMetricsProperties( queueFamilyIndex, metrics );
        }
    }

    /***********************************************************************************\

    Function:
        GetAvailablePerformanceCounters

    Description:
        Updates the list of available performance counters for a given queue family.

    \***********************************************************************************/
    void DeviceProfilerLayerFrontend::GetAvailablePerformanceCounters( uint32_t queueFamilyIndex, const std::vector<uint32_t>& allocatedCounters, std::vector<uint32_t>& availableCounters )
    {
        if( m_pProfiler->m_pPerformanceCounters != nullptr )
        {
            m_pProfiler->m_pPerformanceCounters->GetAvailableMetrics( queueFamilyIndex, allocatedCounters, availableCounters );
        }
    }

    /***********************************************************************************\

    Function:
        GetPerformanceMetricsSets

    Description:
        Returns list of available performance metrics sets.

    \***********************************************************************************/
    void DeviceProfilerLayerFrontend::GetPerformanceMetricsSets( std::vector<VkProfilerPerformanceMetricsSetPropertiesEXT>& sets )
    {
        sets.clear();

        if( m_pProfiler->m_pPerformanceCounters != nullptr )
        {
            m_pProfiler->m_pPerformanceCounters->GetMetricsSets( sets );
        }
    }

    /***********************************************************************************\

    Function:
        GetPerformanceMetricsSetCounterProperties

    Description:
        Returns list of performance counter properties for a given metrics set.

    \***********************************************************************************/
    void DeviceProfilerLayerFrontend::GetPerformanceMetricsSetProperties( uint32_t setIndex, VkProfilerPerformanceMetricsSetPropertiesEXT& properties )
    {
        memset( &properties, 0, sizeof( properties ) );

        if( m_pProfiler->m_pPerformanceCounters != nullptr )
        {
            m_pProfiler->m_pPerformanceCounters->GetMetricsSetProperties( setIndex, properties );
        }
    }

    /***********************************************************************************\

    Function:
        GetPerformanceMetricsSetCounterProperties

    Description:
        Returns list of performance counter properties for a given metrics set.

    \***********************************************************************************/
    void DeviceProfilerLayerFrontend::GetPerformanceMetricsSetCounterProperties( uint32_t setIndex, std::vector<VkProfilerPerformanceCounterPropertiesEXT>& metrics )
    {
        metrics.clear();

        if( m_pProfiler->m_pPerformanceCounters != nullptr )
        {
            m_pProfiler->m_pPerformanceCounters->GetMetricsSetMetricsProperties( setIndex, metrics );
        }
    }

    /***********************************************************************************\

    Function:
        SetPreformanceMetricsSetIndex

    Description:
        Sets the active performance metrics set.

    \***********************************************************************************/
    VkResult DeviceProfilerLayerFrontend::SetPreformanceMetricsSetIndex( uint32_t setIndex )
    {
        if( m_pProfiler->m_pPerformanceCounters != nullptr )
        {
            return m_pProfiler->m_pPerformanceCounters->SetActiveMetricsSet( setIndex );
        }

        return VK_ERROR_FEATURE_NOT_PRESENT;
    }

    /***********************************************************************************\

    Function:
        GetPreformanceMetricsSetIndex

    Description:
        Returns the active performance metrics set.

    \***********************************************************************************/
    uint32_t DeviceProfilerLayerFrontend::GetPerformanceMetricsSetIndex()
    {
        if( m_pProfiler->m_pPerformanceCounters != nullptr )
        {
            return m_pProfiler->m_pPerformanceCounters->GetActiveMetricsSetIndex();
        }

        return UINT32_MAX;
    }

    /***********************************************************************************\

    Function:
        GetHostTimestampFrequency

    Description:
        Returns the timestamp query frequency in the selected time domain.

    \***********************************************************************************/
    uint64_t DeviceProfilerLayerFrontend::GetDeviceCreateTimestamp( VkTimeDomainEXT timeDomain )
    {
        return m_pProfiler->m_Synchronization.GetCreateTimestamp( timeDomain );
    }

    /***********************************************************************************\

    Function:
        GetHostTimestampFrequency

    Description:
        Returns the timestamp query frequency in the selected time domain.

    \***********************************************************************************/
    uint64_t DeviceProfilerLayerFrontend::GetHostTimestampFrequency( VkTimeDomainEXT timeDomain )
    {
        return OSGetTimestampFrequency( timeDomain );
    }

    /***********************************************************************************\

    Function:
        GetProfilerConfig

    Description:
        Returns the configuration of the profiler.

    \***********************************************************************************/
    const DeviceProfilerConfig& DeviceProfilerLayerFrontend::GetProfilerConfig()
    {
        return m_pProfiler->m_Config;
    }

    /***********************************************************************************\

    Function:
        GetProfilerFrameDelimiter

    Description:
        Returns the frame delimiter currently used by the profiler.

    \***********************************************************************************/
    VkProfilerFrameDelimiterEXT DeviceProfilerLayerFrontend::GetProfilerFrameDelimiter()
    {
        return static_cast<VkProfilerFrameDelimiterEXT>( m_pProfiler->m_Config.m_FrameDelimiter.value );
    }

    /***********************************************************************************\

    Function:
        SetProfilerFrameDelimiter

    Description:
        Sets the frame delimiter used by the profiler.

    \***********************************************************************************/
    VkResult DeviceProfilerLayerFrontend::SetProfilerFrameDelimiter( VkProfilerFrameDelimiterEXT frameDelimiter )
    {
        return m_pProfiler->SetFrameDelimiter( frameDelimiter );
    }

    /***********************************************************************************\

    Function:
        GetProfilerSamplingMode

    Description:
        Returns the data sampling mode currently used by the profiler.

    \***********************************************************************************/
    VkProfilerModeEXT DeviceProfilerLayerFrontend::GetProfilerSamplingMode()
    {
        return static_cast<VkProfilerModeEXT>( m_pProfiler->m_Config.m_SamplingMode.value );
    }

    /***********************************************************************************\

    Function:
        SetProfilerSamplingMode

    Description:
        Sets the data sampling mode used by the profiler.

    \***********************************************************************************/
    VkResult DeviceProfilerLayerFrontend::SetProfilerSamplingMode( VkProfilerModeEXT mode )
    {
        return m_pProfiler->SetSamplingMode( mode );
    }

    /***********************************************************************************\

    Function:
        GetObjectName

    Description:
        Returns the name of the object set by the profiled application.

    \***********************************************************************************/
    std::string DeviceProfilerLayerFrontend::GetObjectName( const VkObject& object )
    {
        const char* pName = m_pProfiler->GetObjectName( object );
        if( pName )
        {
            return pName;
        }

        return std::string();
    }

    /***********************************************************************************\

    Function:
        SetObjectName

    Description:
        Sets the name of the object.

    \***********************************************************************************/
    void DeviceProfilerLayerFrontend::SetObjectName( const VkObject& object, const std::string& name )
    {
        m_pProfiler->SetObjectName( object, name.c_str() );
    }

    /***********************************************************************************\

    Function:
        GetData

    Description:
        Returns the current frame data.

    \***********************************************************************************/
    std::shared_ptr<DeviceProfilerFrameData> DeviceProfilerLayerFrontend::GetData()
    {
        return m_pProfiler->GetData();
    }

    /***********************************************************************************\

    Function:
        SetDataBufferSize

    Description:
        Sets the maximum number of buffered frames.

    \***********************************************************************************/
    void DeviceProfilerLayerFrontend::SetDataBufferSize( uint32_t maxFrames )
    {
        m_pProfiler->SetDataBufferSize( maxFrames );
    }
}

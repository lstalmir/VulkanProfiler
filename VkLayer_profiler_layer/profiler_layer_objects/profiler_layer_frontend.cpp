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
        GetRayTracingPipelineProperties

    Description:
        Returns ray tracing pipeline properties of the profiled device.

    \***********************************************************************************/
    const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& DeviceProfilerLayerFrontend::GetRayTracingPipelineProperties()
    {
        return m_pDevice->pPhysicalDevice->RayTracingPipelineProperties;
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
        GetPerformanceMetricsSets

    Description:
        Returns list of available performance metrics sets.

    \***********************************************************************************/
    const std::vector<VkProfilerPerformanceMetricsSetPropertiesEXT>& DeviceProfilerLayerFrontend::GetPerformanceMetricsSets()
    {
        return m_pProfiler->m_MetricsApiINTEL.GetMetricsSets();
    }

    /***********************************************************************************\

    Function:
        GetPerformanceCounterProperties

    Description:
        Returns list of performance counter properties for a given metrics set.

    \***********************************************************************************/
    const std::vector<VkProfilerPerformanceCounterPropertiesEXT>& DeviceProfilerLayerFrontend::GetPerformanceCounterProperties( uint32_t setIndex )
    {
        return m_pProfiler->m_MetricsApiINTEL.GetMetricsProperties( setIndex );
    }

    /***********************************************************************************\

    Function:
        SetPreformanceMetricsSetIndex

    Description:
        Sets the active performance metrics set.

    \***********************************************************************************/
    VkResult DeviceProfilerLayerFrontend::SetPreformanceMetricsSetIndex( uint32_t setIndex )
    {
        return m_pProfiler->m_MetricsApiINTEL.SetActiveMetricsSet( setIndex );
    }

    /***********************************************************************************\

    Function:
        GetPreformanceMetricsSetIndex

    Description:
        Returns the active performance metrics set.

    \***********************************************************************************/
    uint32_t DeviceProfilerLayerFrontend::GetPerformanceMetricsSetIndex()
    {
        return m_pProfiler->m_MetricsApiINTEL.GetActiveMetricsSetIndex();
    }

    /***********************************************************************************\

    Function:
        GetProfilerSyncMode

    Description:
        Returns the data synchronization mode currently used by the profiler.

    \***********************************************************************************/
    VkProfilerSyncModeEXT DeviceProfilerLayerFrontend::GetProfilerSyncMode()
    {
        return static_cast<VkProfilerSyncModeEXT>( m_pProfiler->m_Config.m_SyncMode.value );
    }

    /***********************************************************************************\

    Function:
        SetProfilerSyncMode

    Description:
        Sets the data synchronization mode used by the profiler.

    \***********************************************************************************/
    VkResult DeviceProfilerLayerFrontend::SetProfilerSyncMode( VkProfilerSyncModeEXT mode )
    {
        return m_pProfiler->SetSyncMode( mode );
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
        return m_pProfiler->SetMode( mode );
    }

    /***********************************************************************************\

    Function:
        GetObjectName

    Description:
        Returns the name of the object set by the profiled application.

    \***********************************************************************************/
    std::string DeviceProfilerLayerFrontend::GetObjectName( const VkObject& object )
    {
        std::shared_lock lk( m_pDevice->Debug.ObjectNames );

        auto it = m_pDevice->Debug.ObjectNames.unsafe_find( object );
        if( it != m_pDevice->Debug.ObjectNames.end() )
        {
            return it->second;
        }

        return std::string();
    }

    /***********************************************************************************\

    Function:
        SetObjectName

    Description:
        Returns the name of the object set by the profiled application.

    \***********************************************************************************/
    void DeviceProfilerLayerFrontend::SetObjectName( const VkObject& object, const std::string& name )
    {
        m_pDevice->Debug.ObjectNames.insert( object, name );
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
}

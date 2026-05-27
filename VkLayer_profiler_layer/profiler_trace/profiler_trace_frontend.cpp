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

#include "profiler_trace_frontend.h"

namespace Profiler
{
    /*************************************************************************\

    Function:
        DeviceProfilerTraceFrontend

    Description:
        Constructor.

    \*************************************************************************/
    DeviceProfilerTraceFrontend::DeviceProfilerTraceFrontend()
    {
        ResetMembers();
    }

    /*************************************************************************\

    Function:
        OpenTraceFile

    Description:
        Opens a trace file for reading.

    \*************************************************************************/
    bool DeviceProfilerTraceFrontend::OpenTraceFile( const std::string& filePath )
    {
        // Close the previously opened file.
        CloseTraceFile();

        // Open the file for reading.
        m_TraceFile.exceptions( std::ios::iostate( 0 ) );
        m_TraceFile.open( filePath, std::ios::in );

        if( !m_TraceFile.is_open() )
        {
            ResetMembers();
            return false;
        }

        // TODO: Read the file header and initialize members.

        return true;
    }

    /*************************************************************************\

    Function:
        CloseTraceFile

    Description:
        Closes the trace file.

    \*************************************************************************/
    void DeviceProfilerTraceFrontend::CloseTraceFile()
    {
        if( m_TraceFile.is_open() )
        {
            m_TraceFile.close();
        }

        ResetMembers();
    }

    /*************************************************************************\

    Function:
        IsAvailable

    Description:
        Checks if the frontend is available (i.e. if a trace file is open).

    \*************************************************************************/
    bool DeviceProfilerTraceFrontend::IsAvailable()
    {
        return m_TraceFile.is_open();
    }

    /*************************************************************************\

    Function:
        GetApplicationInfo

    Description:
        Returns the application info saved in the trace file.

    \*************************************************************************/
    const VkApplicationInfo& DeviceProfilerTraceFrontend::GetApplicationInfo()
    {
        return m_ApplicationInfo;
    }

    /*************************************************************************\

    Function:
        GetPhysicalDeviceProperties

    Description:
        Returns the physical device properties saved in the trace file.

    \*************************************************************************/
    const VkPhysicalDeviceProperties& DeviceProfilerTraceFrontend::GetPhysicalDeviceProperties()
    {
        return m_PhysicalDeviceProperties;
    }

    /*************************************************************************\

    Function:
        GetPhysicalDeviceMemoryProperties

    Description:
        Returns the physical device memory properties saved in the trace file.

    \*************************************************************************/
    const VkPhysicalDeviceMemoryProperties& DeviceProfilerTraceFrontend::GetPhysicalDeviceMemoryProperties()
    {
        return m_PhysicalDeviceMemoryProperties;
    }

    /*************************************************************************\

    Function:
        GetQueueFamilyProperties

    Description:
        Returns the queue family properties saved in the trace file.

    \*************************************************************************/
    const std::vector<VkQueueFamilyProperties>& DeviceProfilerTraceFrontend::GetQueueFamilyProperties()
    {
        return m_QueueFamilyProperties;
    }

    /*************************************************************************\

    Function:
        GetEnabledInstanceExtensions

    Description:
        Returns the enabled instance extensions saved in the trace file.

    \*************************************************************************/
    const std::unordered_set<std::string>& DeviceProfilerTraceFrontend::GetEnabledInstanceExtensions()
    {
        return m_EnabledInstanceExtensions;
    }

    /*************************************************************************\

    Function:
        GetEnabledDeviceExtensions

    Description:
        Returns the enabled device extensions saved in the trace file.

    \*************************************************************************/
    const std::unordered_set<std::string>& DeviceProfilerTraceFrontend::GetEnabledDeviceExtensions()
    {
        return m_EnabledDeviceExtensions;
    }

    /*************************************************************************\

    Function:
        GetDeviceQueues

    Description:
        Returns the device queues created by the application.

    \*************************************************************************/
    const std::unordered_map<VkQueue, VkQueue_Object>& DeviceProfilerTraceFrontend::GetDeviceQueues()
    {
        return m_DeviceQueues;
    }

    /*************************************************************************\

    Function:
        SupportsCustomPerformanceMetricsSets

    Description:
        Not supported.

    \*************************************************************************/
    bool DeviceProfilerTraceFrontend::SupportsCustomPerformanceMetricsSets()
    {
        return false;
    }

    /*************************************************************************\

    Function:
        CreateCustomPerformanceMetricsSet

    Description:
        Not supported.

    \*************************************************************************/
    uint32_t DeviceProfilerTraceFrontend::CreateCustomPerformanceMetricsSet(
        const VkProfilerCustomPerformanceMetricsSetCreateInfoEXT* pCreateInfo )
    {
        return UINT32_MAX;
    }

    /*************************************************************************\

    Function:
        DestroyCustomPerformanceMetricsSet

    Description:
        Not supported.

    \*************************************************************************/
    void DeviceProfilerTraceFrontend::DestroyCustomPerformanceMetricsSet(
        uint32_t setIndex )
    {
    }

    /*************************************************************************\

    Function:
        UpdateCustomPerformanceMetricsSets

    Description:
        Not supported.

    \*************************************************************************/
    void DeviceProfilerTraceFrontend::UpdateCustomPerformanceMetricsSets(
        uint32_t updateCount,
        const VkProfilerCustomPerformanceMetricsSetUpdateInfoEXT* pUpdateInfos )
    {
    }

    /*************************************************************************\

    Function:
        GetPerformanceCounterProperties

    Description:
        Returns the properties of performance counters saved in the trace file.

    \*************************************************************************/
    uint32_t DeviceProfilerTraceFrontend::GetPerformanceCounterProperties(
        uint32_t counterCount,
        VkProfilerPerformanceCounterProperties2EXT* pCounters )
    {
        return 0; // todo
    }

    /*************************************************************************\

    Function:
        GetPerformanceMetricsSets

    Description:
        Returns the performance metrics sets saved in the trace file.

    \*************************************************************************/
    uint32_t DeviceProfilerTraceFrontend::GetPerformanceMetricsSets(
        uint32_t setCount,
        VkProfilerPerformanceMetricsSetProperties2EXT* pSets )
    {
        return 0; // todo
    }

    /*************************************************************************\

    Function:
        GetPerformanceMetricsSetProperties

    Description:
        Returns the properties of a performance metrics set saved in the trace file.

    \*************************************************************************/
    void DeviceProfilerTraceFrontend::GetPerformanceMetricsSetProperties(
        uint32_t setIndex,
        VkProfilerPerformanceMetricsSetProperties2EXT* pProperties )
    {
        if( setIndex < m_PerformanceMetricsSets.size() )
        {
            *pProperties = m_PerformanceMetricsSets[setIndex];
        }
    }

    /*************************************************************************\

    Function:
        GetPerformanceMetricsSetCounterProperties

    Description:
        Returns the properties of performance counters in a performance metrics set
        saved in the trace file.

    \*************************************************************************/
    uint32_t DeviceProfilerTraceFrontend::GetPerformanceMetricsSetCounterProperties(
        uint32_t setIndex,
        uint32_t counterCount,
        VkProfilerPerformanceCounterProperties2EXT* pCounters )
    {
        return 0; // todo
    }

    /*************************************************************************\

    Function:
        GetPerformanceCounterRequiredPasses

    Description:
        Not supported.

    \*************************************************************************/
    uint32_t DeviceProfilerTraceFrontend::GetPerformanceCounterRequiredPasses(
        uint32_t counterCount,
        const uint32_t* pCounters )
    {
        return 0;
    }

    /*************************************************************************\

    Function:
        GetAvailablePerformanceCounters

    Description:
        Not supported.

    \*************************************************************************/
    void DeviceProfilerTraceFrontend::GetAvailablePerformanceCounters(
        uint32_t selectedCounterCount,
        const uint32_t* pSelectedCounters,
        uint32_t& availableCounterCount,
        uint32_t* pAvailableCounters )
    {
        availableCounterCount = 0;
    }

    /*************************************************************************\

    Function:
        SetPreformanceMetricsSetIndex

    Description:
        Not supported.

    \*************************************************************************/
    VkResult DeviceProfilerTraceFrontend::SetPreformanceMetricsSetIndex(
        uint32_t setIndex )
    {
        return VK_ERROR_FEATURE_NOT_PRESENT;
    }

    /*************************************************************************\

    Function:
        GetPerformanceMetricsSetIndex

    Description:
        Returns the index of the performance metrics set selected for the
        current frame.

    \*************************************************************************/
    uint32_t DeviceProfilerTraceFrontend::GetPerformanceMetricsSetIndex()
    {
        if( m_pFrameData )
        {
            return m_pFrameData->m_PerformanceCounters.m_MetricsSetIndex;
        }

        return UINT32_MAX;
    }

    /*************************************************************************\

    Function:
        GetPerformanceCountersSamplingMode

    Description:
        Returns the sampling mode of performance counters saved in the trace file.

    \*************************************************************************/
    VkProfilerPerformanceCountersSamplingModeEXT DeviceProfilerTraceFrontend::GetPerformanceCountersSamplingMode()
    {
        return m_PerformanceCountersSamplingMode;
    }

    /*************************************************************************\

    Function:
        GetDeviceCreateTimestamp

    Description:
        Returns the timestamp when the device was created, in the specified time
        domain.

    \*************************************************************************/
    uint64_t DeviceProfilerTraceFrontend::GetDeviceCreateTimestamp(
        VkTimeDomainEXT timeDomain )
    {
        return 0; // todo
    }

    /*************************************************************************\

    Function:
        GetHostTimestampFrequency

    Description:
        Returns the frequency of the host timestamp, in the specified time
        domain.

    \*************************************************************************/
    uint64_t DeviceProfilerTraceFrontend::GetHostTimestampFrequency(
        VkTimeDomainEXT timeDomain )
    {
        return 0; // todo
    }

    /*************************************************************************\

    Function:
        GetProfilerConfig

    Description:
        Returns the profiler configuration saved in the trace file.

    \*************************************************************************/
    const DeviceProfilerConfig& DeviceProfilerTraceFrontend::GetProfilerConfig()
    {
        return m_ProfilerConfig;
    }

    /*************************************************************************\

    Function:
        GetProfilerFrameDelimiter

    Description:
        Returns the frame delimiter used in the trace file.

    \*************************************************************************/
    VkProfilerFrameDelimiterEXT DeviceProfilerTraceFrontend::GetProfilerFrameDelimiter()
    {
        return static_cast<VkProfilerFrameDelimiterEXT>( m_ProfilerConfig.m_FrameDelimiter.value );
    }

    /*************************************************************************\

    Function:
        SetProfilerFrameDelimiter

    Description:
        Not supported.

    \*************************************************************************/
    VkResult DeviceProfilerTraceFrontend::SetProfilerFrameDelimiter(
        VkProfilerFrameDelimiterEXT frameDelimiter )
    {
        return VK_ERROR_FEATURE_NOT_PRESENT;
    }

    /*************************************************************************\

    Function:
        GetProfilerSamplingMode

    Description:
        Returns the sampling mode used in the trace file.

    \*************************************************************************/
    VkProfilerModeEXT DeviceProfilerTraceFrontend::GetProfilerSamplingMode()
    {
        return static_cast<VkProfilerModeEXT>( m_ProfilerConfig.m_SamplingMode.value );
    }

    /*************************************************************************\

    Function:
        SetProfilerSamplingMode

    Description:
        Not supported.

    \*************************************************************************/
    VkResult DeviceProfilerTraceFrontend::SetProfilerSamplingMode(
        VkProfilerModeEXT mode )
    {
        return VK_ERROR_FEATURE_NOT_PRESENT;
    }

    /*************************************************************************\

    Function:
        GetObjectName

    Description:
        Returns the name of an object saved in the trace file.

    \*************************************************************************/
    std::string DeviceProfilerTraceFrontend::GetObjectName(
        const VkObject& object )
    {
        auto it = m_ObjectNames.find( object );
        if( it != m_ObjectNames.end() )
        {
            return it->second;
        }

        return std::string();
    }

    /*************************************************************************\

    Function:
        SetObjectName

    Description:
        Sets the name of an object.

    \*************************************************************************/
    void DeviceProfilerTraceFrontend::SetObjectName(
        const VkObject& object,
        const std::string& name )
    {
        m_ObjectNames[object] = name;
    }

    /*************************************************************************\

    Function:
        GetData

    Description:
        Reads the next frame from the trace file and returns it.
        Returns nullptr if the end of the file is reached or if an error occurs.

    \*************************************************************************/
    std::shared_ptr<DeviceProfilerFrameData> DeviceProfilerTraceFrontend::GetData()
    {
        // Check if the file is in readable state.
        if( !m_TraceFile.is_open() || m_TraceFile.fail() || m_TraceFile.eof() )
        {
            return nullptr;
        }

        // TODO: Read the data.

        return m_pFrameData;
    }

    /*************************************************************************\

    Function:
        SetDataBufferSize

    Description:
        Not supported.

    \*************************************************************************/
    void DeviceProfilerTraceFrontend::SetDataBufferSize(
        uint32_t maxFrames )
    {
    }

    /*************************************************************************\

    Function:
        ResetMembers

    Description:
        Clears all members to their default values.

    \*************************************************************************/
    void DeviceProfilerTraceFrontend::ResetMembers()
    {
        m_TraceFile = std::ifstream();

        m_ApplicationInfo = {};
        m_PhysicalDeviceProperties = {};
        m_PhysicalDeviceMemoryProperties = {};
        m_QueueFamilyProperties.clear();

        m_EnabledInstanceExtensions.clear();
        m_EnabledDeviceExtensions.clear();

        m_DeviceQueues.clear();

        m_ObjectNames.clear();

        m_PerformanceMetricsSets.clear();
        m_PerformanceMetricsSetCounters.clear();
        m_PerformanceCountersSamplingMode = {};

        m_pFrameData.reset();

        m_ProfilerConfig = {};
    }
}

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

#pragma once
#include <profiler/profiler_frontend.h>
#include <profiler_layer_objects/VkQueue_object.h>

#include <fstream>

namespace Profiler
{
    /*************************************************************************\

    Class:
        DeviceProfilerTraceFrontend

    Description:
        Provides data from the serialized trace file.

    \*************************************************************************/
    class DeviceProfilerTraceFrontend : public DeviceProfilerFrontend
    {
    public:
        DeviceProfilerTraceFrontend();

        bool OpenTraceFile( const std::string& filePath );
        void CloseTraceFile();

        bool IsAvailable() override;

        const VkApplicationInfo& GetApplicationInfo() override;
        const VkPhysicalDeviceProperties& GetPhysicalDeviceProperties() override;
        const VkPhysicalDeviceMemoryProperties& GetPhysicalDeviceMemoryProperties() override;
        const std::vector<VkQueueFamilyProperties>& GetQueueFamilyProperties() override;

        const std::unordered_set<std::string>& GetEnabledInstanceExtensions() override;
        const std::unordered_set<std::string>& GetEnabledDeviceExtensions() override;

        const std::unordered_map<VkQueue, VkQueue_Object>& GetDeviceQueues() override;

        bool SupportsCustomPerformanceMetricsSets() override;
        uint32_t CreateCustomPerformanceMetricsSet( const VkProfilerCustomPerformanceMetricsSetCreateInfoEXT* pCreateInfo ) override;
        void DestroyCustomPerformanceMetricsSet( uint32_t setIndex ) override;
        void UpdateCustomPerformanceMetricsSets( uint32_t updateCount, const VkProfilerCustomPerformanceMetricsSetUpdateInfoEXT* pUpdateInfos ) override;
        uint32_t GetPerformanceCounterProperties( uint32_t counterCount, VkProfilerPerformanceCounterProperties2EXT* pCounters ) override;
        uint32_t GetPerformanceMetricsSets( uint32_t setCount, VkProfilerPerformanceMetricsSetProperties2EXT* pSets ) override;
        void GetPerformanceMetricsSetProperties( uint32_t setIndex, VkProfilerPerformanceMetricsSetProperties2EXT* pProperties ) override;
        uint32_t GetPerformanceMetricsSetCounterProperties( uint32_t setIndex, uint32_t counterCount, VkProfilerPerformanceCounterProperties2EXT* pCounters ) override;
        uint32_t GetPerformanceCounterRequiredPasses( uint32_t counterCount, const uint32_t* pCounters ) override;
        void GetAvailablePerformanceCounters( uint32_t selectedCounterCount, const uint32_t* pSelectedCounters, uint32_t& availableCounterCount, uint32_t* pAvailableCounters ) override;
        VkResult SetPreformanceMetricsSetIndex( uint32_t setIndex ) override;
        uint32_t GetPerformanceMetricsSetIndex() override;
        VkProfilerPerformanceCountersSamplingModeEXT GetPerformanceCountersSamplingMode() override;

        uint64_t GetDeviceCreateTimestamp( VkTimeDomainEXT timeDomain ) override;
        uint64_t GetHostTimestampFrequency( VkTimeDomainEXT timeDomain ) override;

        const DeviceProfilerConfig& GetProfilerConfig() override;

        VkProfilerFrameDelimiterEXT GetProfilerFrameDelimiter() override;
        VkResult SetProfilerFrameDelimiter( VkProfilerFrameDelimiterEXT frameDelimiter ) override;

        VkProfilerModeEXT GetProfilerSamplingMode() override;
        VkResult SetProfilerSamplingMode( VkProfilerModeEXT mode ) override;

        std::string GetObjectName( const VkObject& object ) override;
        void SetObjectName( const VkObject& object, const std::string& name );

        std::shared_ptr<DeviceProfilerFrameData> GetData() override;
        void SetDataBufferSize( uint32_t maxFrames ) override;

    private:
        std::ifstream m_TraceFile;

        VkApplicationInfo m_ApplicationInfo;
        VkPhysicalDeviceProperties m_PhysicalDeviceProperties;
        VkPhysicalDeviceMemoryProperties m_PhysicalDeviceMemoryProperties;
        std::vector<VkQueueFamilyProperties> m_QueueFamilyProperties;

        std::unordered_set<std::string> m_EnabledInstanceExtensions;
        std::unordered_set<std::string> m_EnabledDeviceExtensions;

        std::unordered_map<VkQueue, VkQueue_Object> m_DeviceQueues;

        std::unordered_map<VkObject, std::string> m_ObjectNames;

        std::vector<VkProfilerPerformanceMetricsSetProperties2EXT> m_PerformanceMetricsSets;
        std::vector<std::vector<VkProfilerPerformanceCounterProperties2EXT>> m_PerformanceMetricsSetCounters;
        VkProfilerPerformanceCountersSamplingModeEXT m_PerformanceCountersSamplingMode;

        std::shared_ptr<DeviceProfilerFrameData> m_pFrameData;

        DeviceProfilerConfig m_ProfilerConfig;

        void ResetMembers();
    };
}

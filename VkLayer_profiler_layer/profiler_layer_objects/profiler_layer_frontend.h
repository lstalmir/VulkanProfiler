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

#pragma once
#include "profiler/profiler_frontend.h"

namespace Profiler
{
    struct VkDevice_Object;
    class DeviceProfiler;

    /***********************************************************************************\

    Class:
        DeviceProfilerLayerFrontend

    Description:
        Implementation of the DeviceProfilerFrontend interface for displaying data from
        the profiling layer using the built-in overlay.

        It forwards all calls to the current VkDevice and the profiler associated with it.

    \***********************************************************************************/
    class DeviceProfilerLayerFrontend final
        : public DeviceProfilerFrontend
    {
    public:
        void Initialize( VkDevice_Object& device, DeviceProfiler& profiler );
        void Destroy();

        bool IsAvailable() final;

        const VkApplicationInfo& GetApplicationInfo() final;
        const VkPhysicalDeviceProperties& GetPhysicalDeviceProperties() final;
        const VkPhysicalDeviceMemoryProperties& GetPhysicalDeviceMemoryProperties() final;
        const std::vector<VkQueueFamilyProperties>& GetQueueFamilyProperties() final;

        const std::unordered_set<std::string>& GetEnabledInstanceExtensions() final;
        const std::unordered_set<std::string>& GetEnabledDeviceExtensions() final;

        const std::unordered_map<VkQueue, VkQueue_Object>& GetDeviceQueues() final;

        bool SupportsCustomPerformanceMetricsSets() final;
        uint32_t CreateCustomPerformanceMetricsSet( const std::string& name, const std::string& description, const std::vector<uint32_t>& counters ) final;
        void DestroyCustomPerformanceMetricsSet( uint32_t setIndex ) final;
        void GetPerformanceCounterProperties( std::vector<VkProfilerPerformanceCounterProperties2EXT>& metrics ) final;
        void GetAvailablePerformanceCounters( const std::vector<uint32_t>& allocatedCounters, std::vector<uint32_t>& availableCounters ) final;
        void GetPerformanceMetricsSets( std::vector<VkProfilerPerformanceMetricsSetProperties2EXT>& sets ) final;
        void GetPerformanceMetricsSetProperties( uint32_t setIndex, VkProfilerPerformanceMetricsSetProperties2EXT& properties ) final;
        void GetPerformanceMetricsSetCounterProperties( uint32_t setIndex, std::vector<VkProfilerPerformanceCounterProperties2EXT>& metrics ) final;
        VkResult SetPreformanceMetricsSetIndex( uint32_t setIndex ) final;
        uint32_t GetPerformanceMetricsSetIndex() final;

        uint64_t GetDeviceCreateTimestamp( VkTimeDomainEXT timeDomain ) final;
        uint64_t GetHostTimestampFrequency( VkTimeDomainEXT timeDomain ) final;

        const DeviceProfilerConfig& GetProfilerConfig() final;

        VkProfilerFrameDelimiterEXT GetProfilerFrameDelimiter() final;
        VkResult SetProfilerFrameDelimiter( VkProfilerFrameDelimiterEXT frameDelimiter ) final;

        VkProfilerModeEXT GetProfilerSamplingMode() final;
        VkResult SetProfilerSamplingMode( VkProfilerModeEXT mode ) final;

        std::string GetObjectName( const VkObject& object ) final;
        void SetObjectName( const VkObject& object, const std::string& name ) final;

        std::shared_ptr<DeviceProfilerFrameData> GetData() final;
        void SetDataBufferSize( uint32_t maxFrames ) final;

    private:
        VkDevice_Object* m_pDevice = nullptr;
        DeviceProfiler* m_pProfiler = nullptr;
    };
}

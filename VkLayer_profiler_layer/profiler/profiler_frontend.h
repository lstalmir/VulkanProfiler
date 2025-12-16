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
#include "profiler_config.h"
#include "profiler_data.h"
#include "profiler_ext/VkProfilerEXT.h"

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace Profiler
{
    /***********************************************************************************\

    Class:
        DeviceProfilerFrontend

    Description:
        An interface between the profiling layer and the display layer (e.g. overlay).

    \***********************************************************************************/
    class DeviceProfilerFrontend
    {
    public:
        virtual ~DeviceProfilerFrontend() = default;

        virtual bool IsAvailable() = 0;

        virtual const VkApplicationInfo& GetApplicationInfo() = 0;
        virtual const VkPhysicalDeviceProperties& GetPhysicalDeviceProperties() = 0;
        virtual const VkPhysicalDeviceMemoryProperties& GetPhysicalDeviceMemoryProperties() = 0;
        virtual const std::vector<VkQueueFamilyProperties>& GetQueueFamilyProperties() = 0;
        virtual const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& GetRayTracingPipelineProperties() = 0;

        virtual const std::unordered_set<std::string>& GetEnabledInstanceExtensions() = 0;
        virtual const std::unordered_set<std::string>& GetEnabledDeviceExtensions() = 0;

        virtual const std::unordered_map<VkQueue, struct VkQueue_Object>& GetDeviceQueues() = 0;

        virtual bool SupportsCustomPerformanceMetricsSets() = 0;
        virtual uint32_t CreateCustomPerformanceMetricsSet( const VkProfilerCustomPerformanceMetricsSetCreateInfoEXT* pCreateInfo ) = 0;
        virtual void DestroyCustomPerformanceMetricsSet( uint32_t setIndex ) = 0;
        virtual void UpdateCustomPerformanceMetricsSets( uint32_t updateCount, const VkProfilerCustomPerformanceMetricsSetUpdateInfoEXT* pUpdateInfos ) = 0;
        virtual uint32_t GetPerformanceCounterProperties( uint32_t counterCount, VkProfilerPerformanceCounterProperties2EXT* pCounters ) = 0;
        virtual uint32_t GetPerformanceMetricsSets( uint32_t setCount, VkProfilerPerformanceMetricsSetProperties2EXT* pSets ) = 0;
        virtual void GetPerformanceMetricsSetProperties( uint32_t setIndex, VkProfilerPerformanceMetricsSetProperties2EXT* pProperties ) = 0;
        virtual uint32_t GetPerformanceMetricsSetCounterProperties( uint32_t setIndex, uint32_t counterCount, VkProfilerPerformanceCounterProperties2EXT* pCounters ) = 0;
        virtual uint32_t GetPerformanceCounterRequiredPasses( uint32_t counterCount, const uint32_t* pCounters ) = 0;
        virtual void GetAvailablePerformanceCounters( uint32_t selectedCounterCount, const uint32_t* pSelectedCounters, uint32_t& availableCounterCount, uint32_t* pAvailableCounters ) = 0;
        virtual VkResult SetPreformanceMetricsSetIndex( uint32_t setIndex ) = 0;
        virtual uint32_t GetPerformanceMetricsSetIndex() = 0;

        virtual uint64_t GetDeviceCreateTimestamp( VkTimeDomainEXT timeDomain ) = 0;
        virtual uint64_t GetHostTimestampFrequency( VkTimeDomainEXT timeDomain ) = 0;

        virtual const DeviceProfilerConfig& GetProfilerConfig() = 0;

        virtual VkProfilerFrameDelimiterEXT GetProfilerFrameDelimiter() = 0;
        virtual VkResult SetProfilerFrameDelimiter( VkProfilerFrameDelimiterEXT frameDelimiter ) = 0;

        virtual VkProfilerModeEXT GetProfilerSamplingMode() = 0;
        virtual VkResult SetProfilerSamplingMode( VkProfilerModeEXT mode ) = 0;

        virtual std::string GetObjectName( const struct VkObject& object ) = 0;
        virtual void SetObjectName( const struct VkObject& object, const std::string& name ) = 0;

        virtual std::shared_ptr<DeviceProfilerFrameData> GetData() = 0;
        virtual void SetDataBufferSize( uint32_t maxFrames ) = 0;
    };

    /***********************************************************************************\

    Class:
        DeviceProfilerOutput

    Description:
        An output interface for presenting profiling data.
        This can be a GUI overlay, a file output, etc.

    \***********************************************************************************/
    class DeviceProfilerOutput
    {
    public:
        explicit DeviceProfilerOutput( DeviceProfilerFrontend& frontend )
            : m_Frontend( frontend )
        {
        }

        virtual ~DeviceProfilerOutput() = default;

        virtual bool IsAvailable() = 0;

        virtual bool Initialize() = 0;
        virtual void Destroy() = 0;

        virtual void Update() = 0;
        virtual void Present() = 0;

    protected:
        DeviceProfilerFrontend& m_Frontend;
    };
}

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

        virtual const std::vector<VkProfilerPerformanceMetricsSetPropertiesEXT>& GetPerformanceMetricsSets() = 0;
        virtual const std::vector<VkProfilerPerformanceCounterPropertiesEXT>& GetPerformanceCounterProperties( uint32_t setIndex ) = 0;
        virtual VkResult SetPreformanceMetricsSetIndex( uint32_t setIndex ) = 0;
        virtual uint32_t GetPerformanceMetricsSetIndex() = 0;

        virtual VkProfilerSyncModeEXT GetProfilerSyncMode() = 0;
        virtual VkResult SetProfilerSyncMode( VkProfilerSyncModeEXT mode ) = 0;

        virtual VkProfilerModeEXT GetProfilerSamplingMode() = 0;
        virtual VkResult SetProfilerSamplingMode( VkProfilerModeEXT mode ) = 0;

        virtual std::string GetObjectName( const struct VkObject& object ) = 0;
        virtual void SetObjectName( const struct VkObject& object, const std::string& name ) = 0;

        virtual std::shared_ptr<DeviceProfilerFrameData> GetData() = 0;
    };
}

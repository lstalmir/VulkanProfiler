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
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <string_view>

namespace Profiler
{
    struct VkDevice_Object;
    struct VkQueue_Object;
    class DeviceProfiler;

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

        virtual const std::unordered_set<std::string>& GetEnabledInstanceExtensions() = 0;
        virtual const std::unordered_set<std::string>& GetEnabledDeviceExtensions() = 0;

        virtual const std::unordered_map<VkQueue, VkQueue_Object>& GetDeviceQueues() = 0;

        virtual const std::vector<VkProfilerPerformanceMetricsSetPropertiesEXT>& GetPerformanceMetricsSets() = 0;
        virtual const std::vector<VkProfilerPerformanceCounterPropertiesEXT>& GetPerformanceCounterProperties( uint32_t setIndex ) = 0;
        virtual VkResult SetPreformanceMetricsSetIndex( uint32_t setIndex ) = 0;
        virtual uint32_t GetPerformanceMetricsSetIndex() = 0;

        virtual VkProfilerSyncModeEXT GetProfilerSyncMode() = 0;
        virtual VkResult SetProfilerSyncMode( VkProfilerSyncModeEXT mode ) = 0;

        virtual VkProfilerModeEXT GetProfilerSamplingMode() = 0;
        virtual VkResult SetProfilerSamplingMode( VkProfilerModeEXT mode ) = 0;

        virtual std::string_view GetObjectName( uint64_t object, VkObjectType objectType ) = 0;

        virtual std::shared_ptr<DeviceProfilerFrameData> GetData() = 0;
    };

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
        DeviceProfilerLayerFrontend();

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

        const std::vector<VkProfilerPerformanceMetricsSetPropertiesEXT>& GetPerformanceMetricsSets() final;
        const std::vector<VkProfilerPerformanceCounterPropertiesEXT>& GetPerformanceCounterProperties( uint32_t setIndex ) final;
        VkResult SetPreformanceMetricsSetIndex( uint32_t setIndex ) final;
        uint32_t GetPerformanceMetricsSetIndex() final;

        VkProfilerSyncModeEXT GetProfilerSyncMode() final;
        VkResult SetProfilerSyncMode( VkProfilerSyncModeEXT mode ) final;

        VkProfilerModeEXT GetProfilerSamplingMode() final;
        VkResult SetProfilerSamplingMode( VkProfilerModeEXT mode ) final;

        std::string_view GetObjectName( uint64_t object, VkObjectType objectType ) final;

        std::shared_ptr<DeviceProfilerFrameData> GetData() final;

    private:
        VkDevice_Object* m_pDevice;
        DeviceProfiler* m_pProfiler;
    };
}

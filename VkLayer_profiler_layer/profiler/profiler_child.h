#pragma once
#include <vk_layer.h>
#include <vk_dispatch_table_helper.h>

namespace Profiler
{
    class DeviceProfiler;

    class ProfilerChild
    {
    protected:
        DeviceProfiler& m_Profiler;

        ProfilerChild( DeviceProfiler& profiler );

        VkDevice Device() const;
        VkInstance Instance() const;
        VkPhysicalDevice PhysicalDevice() const;

        const VkLayerDispatchTable& Dispatch() const;
        const VkLayerInstanceDispatchTable& InstanceDispatch() const;
    };
}

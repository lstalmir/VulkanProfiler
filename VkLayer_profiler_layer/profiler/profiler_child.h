#pragma once
#include <vk_layer.h>
#include <vk_dispatch_table_helper.h>

namespace Profiler
{
    class Profiler;

    class ProfilerChild
    {
    protected:
        Profiler& m_Profiler;

        ProfilerChild( Profiler& profiler );

        VkDevice Device() const;
        VkInstance Instance() const;
        VkPhysicalDevice PhysicalDevice() const;

        const VkLayerDispatchTable& Dispatch() const;
        const VkLayerInstanceDispatchTable& InstanceDispatch() const;
    };
}

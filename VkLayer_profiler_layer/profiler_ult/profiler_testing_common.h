#pragma once

// Vulkan headers
#include <vulkan/vulkan.h>

#include "VkLayer_profiler_layer.generated.h"
#include "profiler_vulkan_state.h"

// Profiler extension
#include "profiler_ext/VkProfilerEXT.h"

// Linux: Undefine names conflicting in gtest
#undef None
#undef Bool

// GTest headers
#include <gtest/gtest.h>

// Windows: Undefine names conflicting in tests
#undef SetEnvironmentVariable

namespace Profiler
{
    /***********************************************************************************\

    Class:
        ProfilerBaseULT

    Description:
        Base class for all profiler tests.

    \***********************************************************************************/
    class ProfilerBaseULT : public testing::Test
    {
    protected:
        VulkanState* Vk = {};

        VkLayerDispatchTable DT = {};
        VkLayerInstanceDispatchTable IDT = {};

        DeviceProfiler* Prof = {};

        // Executed before each test
        inline void SetUp() override
        {
            Test::SetUp();

            Vk = new VulkanState;
            DT = Vk->GetLayerDispatchTable();
            IDT = Vk->GetLayerInstanceDispatchTable();

            auto& dd = VkDevice_Functions::DeviceDispatch.Get( Vk->Device );
            Prof = &dd.Profiler;
        }

        // Executed after each test
        inline void TearDown() override
        {
            delete Vk;

            Test::TearDown();
        }
    };
}

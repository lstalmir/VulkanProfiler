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

#include "profiler_testing_common.h"

#include "profiler/profiler.h"

namespace Profiler
{
    class ProfilerConfigULT : public testing::Test
    {
    protected:
        vk::UniqueInstance m_Instance;
        vk::PhysicalDevice m_PhysicalDevice;
        vk::UniqueDevice m_Device;
        DeviceProfiler* m_pProfiler;

        void SetUpVulkan( const void* pNextInstance = nullptr, const void* pNextDevice = nullptr )
        {
            const char* const instanceLayers[] = {
                VK_LAYER_profiler_name
            };

            vk::ApplicationInfo applicationInfo;
            applicationInfo.setApiVersion( VK_API_VERSION_1_1 );

            vk::InstanceCreateInfo instanceCreateInfo;
            instanceCreateInfo.setPNext( pNextInstance );
            instanceCreateInfo.setPApplicationInfo( &applicationInfo );
            instanceCreateInfo.setPEnabledLayerNames( instanceLayers );

            m_Instance = vk::createInstanceUnique( instanceCreateInfo );
            m_PhysicalDevice = m_Instance->enumeratePhysicalDevices().front();

            std::vector<float> queuePriorities = { 1.0f };
            vk::DeviceQueueCreateInfo queueCreateInfo;
            queueCreateInfo.setQueueFamilyIndex( 0 );
            queueCreateInfo.setQueueCount( 1 );
            queueCreateInfo.setQueuePriorities( queuePriorities );

            vk::DeviceCreateInfo deviceCreateInfo;
            deviceCreateInfo.setPNext( pNextDevice );
            deviceCreateInfo.setPQueueCreateInfos( &queueCreateInfo );

            m_Device = m_PhysicalDevice.createDeviceUnique( deviceCreateInfo );

            // Get the profiler instance and verify the settings.
            auto vkGetProfilerEXT = reinterpret_cast<PFN_vkGetProfilerEXT>( m_Device->getProcAddr( "vkGetProfilerEXT" ) );
            if( !vkGetProfilerEXT )
            {
                throw VulkanError( VK_ERROR_EXTENSION_NOT_PRESENT, "Failed to get vkGetProfilerEXT function address" );
            }

            VkProfilerEXT profiler = VK_NULL_HANDLE;
            vkGetProfilerEXT( m_Device.get(), &profiler );
            if( !profiler )
            {
                throw VulkanError( VK_ERROR_EXTENSION_NOT_PRESENT, "Failed to read VkProfilerEXT object" );
            }

            m_pProfiler = reinterpret_cast<DeviceProfiler*>( profiler );
        }

        const ProfilerLayerSettings& GetProfilerConfig() const
        {
            return m_pProfiler->m_Config;
        }
    };

    /***********************************************************************************\

    Test:
        ConfigureWithLayerSettingsExt

    Description:
        Configure the profiler using official VK_EXT_layer_settings extension.

    \***********************************************************************************/
    TEST_F( ProfilerConfigULT, ConfigureWithLayerSettingsExt )
    {
        const char* sampling_mode = "pipeline";
        const vk::Bool32 enable_threading = vk::False;
        const int32_t frame_count = 100;

        ProfilerLayerSettings expected_settings = {};
        expected_settings.m_SamplingMode = sampling_mode_t::pipeline;
        expected_settings.m_EnableThreading = false;
        expected_settings.m_FrameCount = frame_count;

        const vk::LayerSettingEXT settings[] = {
            vk::LayerSettingEXT( VK_LAYER_profiler_name, "sampling_mode", vk::LayerSettingTypeEXT::eString, 1, &sampling_mode ),
            vk::LayerSettingEXT( VK_LAYER_profiler_name, "enable_threading", vk::LayerSettingTypeEXT::eBool32, 1, &enable_threading ),
            vk::LayerSettingEXT( VK_LAYER_profiler_name, "frame_count", vk::LayerSettingTypeEXT::eInt32, 1, &frame_count )
        };

        vk::LayerSettingsCreateInfoEXT layerSettingsCreateInfo;
        layerSettingsCreateInfo.setSettings( settings );

        SetUpVulkan( &layerSettingsCreateInfo, nullptr );

        const ProfilerLayerSettings& actual_settings = GetProfilerConfig();
        EXPECT_EQ( expected_settings.m_SamplingMode.value, actual_settings.m_SamplingMode.value );
        EXPECT_EQ( expected_settings.m_EnableThreading, actual_settings.m_EnableThreading );
        EXPECT_EQ( expected_settings.m_FrameCount, actual_settings.m_FrameCount );
    }

    /***********************************************************************************\

    Test:
        ConfigureWithProfilerExt

    Description:
        Configure the profiler using custom VK_EXT_profiler extension.

    \***********************************************************************************/
    TEST_F( ProfilerConfigULT, ConfigureWithProfilerExt )
    {
        ProfilerLayerSettings expected_settings = {};
        expected_settings.m_Output = output_t::none;
        expected_settings.m_SamplingMode = sampling_mode_t::pipeline;
        expected_settings.m_EnableThreading = false;

        VkProfilerCreateInfoEXT profilerCreateInfo = {};
        profilerCreateInfo.sType = VK_STRUCTURE_TYPE_PROFILER_CREATE_INFO_EXT;
        profilerCreateInfo.flags = VK_PROFILER_CREATE_NO_OVERLAY_BIT_EXT | VK_PROFILER_CREATE_NO_THREADING_BIT_EXT;
        profilerCreateInfo.samplingMode = VK_PROFILER_MODE_PER_PIPELINE_EXT;

        SetUpVulkan( nullptr, &profilerCreateInfo );

        const ProfilerLayerSettings& actual_settings = GetProfilerConfig();
        EXPECT_EQ( expected_settings.m_Output.value, actual_settings.m_Output.value );
        EXPECT_EQ( expected_settings.m_SamplingMode.value, actual_settings.m_SamplingMode.value );
        EXPECT_EQ( expected_settings.m_EnableThreading, actual_settings.m_EnableThreading );
    }

    /***********************************************************************************\

    Test:
        ConfigureWithEnvironmentVars

    Description:
        Configure the profiler using environment variables.

    \***********************************************************************************/
    TEST_F( ProfilerConfigULT, ConfigureWithEnvironmentVars )
    {
        EnvironmentVariableScope enable_threading_var( "VKPROF_enable_threading", "false" );
        EnvironmentVariableScope sampling_mode_var( "VKPROF_sampling_mode", "renderpass" );
        EnvironmentVariableScope frame_count_var( "VKPROF_frame_count", "50" );

        ProfilerLayerSettings expected_settings = {};
        expected_settings.m_EnableThreading = false;
        expected_settings.m_SamplingMode = sampling_mode_t::renderpass;
        expected_settings.m_FrameCount = 50;

        SetUpVulkan();

        const ProfilerLayerSettings& actual_settings = GetProfilerConfig();
        EXPECT_EQ( expected_settings.m_EnableThreading, actual_settings.m_EnableThreading );
        EXPECT_EQ( expected_settings.m_SamplingMode.value, actual_settings.m_SamplingMode.value );
        EXPECT_EQ( expected_settings.m_FrameCount, actual_settings.m_FrameCount );
    }

    /***********************************************************************************\

    Test:
        CheckEnvironmentVarsCaseInsensitivity

    Description:
        Verify that environment variable values are case-insensitive.

    \***********************************************************************************/
    TEST_F( ProfilerConfigULT, CheckEnvironmentVarsCaseInsensitivity )
    {
        EnvironmentVariableScope enable_threading_var( "VKPROF_enable_threading", "FaLse" );
        EnvironmentVariableScope sampling_mode_var( "VKPROF_sampling_mode", "ReNdeRpASs" );

        ProfilerLayerSettings expected_settings = {};
        expected_settings.m_EnableThreading = false;
        expected_settings.m_SamplingMode = sampling_mode_t::renderpass;

        ProfilerLayerSettings actual_settings = {};
        actual_settings.LoadFromEnvironment();

        EXPECT_EQ( expected_settings.m_EnableThreading, actual_settings.m_EnableThreading );
        EXPECT_EQ( expected_settings.m_SamplingMode.value, actual_settings.m_SamplingMode.value );
    }

    /***********************************************************************************\

    Test:
        CheckEnvironmentVarsBooleanValues

    Description:
        Verify that boolean values accept various representations
        (i.e. "0", "1", "true", "false", "yes", "no").

    \***********************************************************************************/
    TEST_F( ProfilerConfigULT, CheckEnvironmentVarsBooleanValues )
    {
        struct TestCase
        {
            std::string value;
            bool expected_result;
        };

        const TestCase test_cases[] = {
            { "0", false },
            { "1", true },
            { "false", false },
            { "true", true },
            { "no", false },
            { "yes", true }
        };

        for( const TestCase& test_case : test_cases )
        {
            EnvironmentVariableScope enable_threading_var( "VKPROF_enable_threading", test_case.value );

            ProfilerLayerSettings actual_settings = {};
            actual_settings.m_EnableThreading = !test_case.expected_result;
            actual_settings.LoadFromEnvironment();

            EXPECT_EQ( test_case.expected_result, actual_settings.m_EnableThreading ) << "Failed for value: " << test_case.value;
        }
    }
}

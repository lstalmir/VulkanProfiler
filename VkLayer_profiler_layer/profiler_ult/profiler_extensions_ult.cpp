#include "profiler_testing_common.h"

#include <set>
#include <string>

namespace Profiler
{
    class ProfilerExtensionsULT : public testing::Test
    {
    protected:
        std::set<std::string> Variables;

        // Helper functions for environment manipulation
        inline void SetEnvironmentVariable( const char* pName, const char* pValue )
        {
            #if defined WIN32
            SetEnvironmentVariableA( pName, pValue );
            #elif defined __linux__
            setenv( pName, pValue, true );
            #else
            #error SetEnvironmentVariable not implemented for this OS
            #endif

            Variables.insert( pName );
        }

        inline void ResetEnvironmentVariable( const char* pName )
        {
            #if defined WIN32
            SetEnvironmentVariableA( pName, nullptr );
            #elif defined __linux__
            unsetenv( pName );
            #else
            #error ResetEnvironmentVariable not implemented for this OS
            #endif

            Variables.erase( pName );
        }

        inline void SetUp() override
        {
            Test::SetUp();

            // Setup default environemnt for extension testing
            // Assume tests are being run in default directory (VkLayer_profiler_layer/profiler_ult)
            const std::filesystem::path layerPath = std::filesystem::current_path().parent_path();

            SetEnvironmentVariable( "VK_INSTANCE_LAYERS", VK_LAYER_profiler_name );
            SetEnvironmentVariable( "VK_LAYER_PATH", layerPath.string().c_str() );
        }

        inline void TearDown() override
        {
            std::set<std::string> variables = Variables;

            // Cleanup environment before the next run
            for( const std::string& variable : variables )
            {
                ResetEnvironmentVariable( variable.c_str() );
            }
        }
    };

    TEST_F( ProfilerExtensionsULT, EnumerateInstanceExtensionProperties )
    {
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties( VK_LAYER_profiler_name, &extensionCount, nullptr );

        std::vector<VkExtensionProperties> extensions( extensionCount );
        vkEnumerateInstanceExtensionProperties( VK_LAYER_profiler_name, &extensionCount, extensions.data() );

        std::set<std::string> unimplementedExtensions = {
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME };

        std::set<std::string> unexpectedExtensions;

        for( const VkExtensionProperties& ext : extensions )
            unexpectedExtensions.insert( ext.extensionName );

        for( const std::string& ext : unimplementedExtensions )
            unexpectedExtensions.erase( ext );

        for( const VkExtensionProperties& ext : extensions )
            unimplementedExtensions.erase( ext.extensionName );

        EXPECT_EQ( 1, extensionCount );
        EXPECT_TRUE( unexpectedExtensions.empty() );
        EXPECT_TRUE( unimplementedExtensions.empty() );
    }

    TEST_F( ProfilerExtensionsULT, EnumerateDeviceExtensionProperties )
    {
        // Create simple vulkan instance
        VulkanState Vk;
        
        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties( Vk.PhysicalDevice, VK_LAYER_profiler_name, &extensionCount, nullptr );

        std::vector<VkExtensionProperties> extensions( extensionCount );
        vkEnumerateDeviceExtensionProperties( Vk.PhysicalDevice, VK_LAYER_profiler_name, &extensionCount, extensions.data() );

        std::set<std::string> unimplementedExtensions = {
            VK_EXT_PROFILER_EXTENSION_NAME,
            VK_EXT_DEBUG_MARKER_EXTENSION_NAME };

        std::set<std::string> unexpectedExtensions;

        for( const VkExtensionProperties& ext : extensions )
            unexpectedExtensions.insert( ext.extensionName );

        for( const std::string& ext : unimplementedExtensions )
            unexpectedExtensions.erase( ext );

        for( const VkExtensionProperties& ext : extensions )
            unimplementedExtensions.erase( ext.extensionName );

        EXPECT_EQ( 2, extensionCount );
        EXPECT_TRUE( unexpectedExtensions.empty() );
        EXPECT_TRUE( unimplementedExtensions.empty() );
    }

    TEST_F( ProfilerExtensionsULT, DebugMarkerEXT )
    {
        // Create vulkan instance with profiler layer enabled externally
        VulkanState Vk;

        EXPECT_NE( nullptr, vkGetDeviceProcAddr( Vk.Device, "vkCmdDebugMarkerBeginEXT" ) );
        EXPECT_NE( nullptr, vkGetDeviceProcAddr( Vk.Device, "vkCmdDebugMarkerEndEXT" ) );
        EXPECT_NE( nullptr, vkGetDeviceProcAddr( Vk.Device, "vkCmdDebugMarkerInsertEXT" ) );
        EXPECT_NE( nullptr, vkGetDeviceProcAddr( Vk.Device, "vkDebugMarkerSetObjectNameEXT" ) );
        EXPECT_NE( nullptr, vkGetDeviceProcAddr( Vk.Device, "vkDebugMarkerSetObjectTagEXT" ) );
    }

    TEST_F( ProfilerExtensionsULT, DebugUtilsEXT )
    {
        // Create vulkan instance with profiler layer enabled externally
        VulkanState Vk;

        EXPECT_NE( nullptr, vkGetDeviceProcAddr( Vk.Device, "vkCmdBeginDebugUtilsLabelEXT" ) );
        EXPECT_NE( nullptr, vkGetDeviceProcAddr( Vk.Device, "vkCmdEndDebugUtilsLabelEXT" ) );
        EXPECT_NE( nullptr, vkGetDeviceProcAddr( Vk.Device, "vkCmdInsertDebugUtilsLabelEXT" ) );
        EXPECT_NE( nullptr, vkGetDeviceProcAddr( Vk.Device, "vkSetDebugUtilsObjectNameEXT" ) );
        EXPECT_NE( nullptr, vkGetDeviceProcAddr( Vk.Device, "vkSetDebugUtilsObjectTagEXT" ) );
    }
}

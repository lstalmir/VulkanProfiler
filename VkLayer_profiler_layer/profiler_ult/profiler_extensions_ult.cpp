#include "VkLayer_profiler_layer.generated.h"
#include "profiler_vulkan_state.h"

#include "profiler_ext/VkProfilerEXT.h"

#include <vulkan/vulkan.h>

// Undefine names conflicting in gtest
#undef None
#undef Bool

#include <gtest/gtest.h>

#include <set>
#include <string>

namespace
{
    static inline std::set<std::string> Separate( const std::string& string, const char* pSeparators )
    {
        std::set<std::string> components;
        size_t begin = 0;
        while( begin < string.length() )
        {
            const size_t end = string.find_first_of( pSeparators, begin );
            components.insert( string.substr( begin, end ) );
            begin = end + 1;
        }
        return components;
    }
}

namespace Profiler
{
    TEST( ProfilerExtensionsULT, EnumerateInstanceExtensionProperties )
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

    TEST( ProfilerExtensionsULT, EnumerateDeviceExtensionProperties )
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

    TEST( ProfilerExtensionsULT, DebugMarkerEXT )
    {
        #ifdef WIN32
        SetEnvironmentVariableA( "VK_INSTANCE_LAYERS", VK_LAYER_profiler_name );
        #endif

        // Create vulkan instance with profiler layer enabled externally
        VulkanState Vk;

        EXPECT_NE( nullptr, vkGetDeviceProcAddr( Vk.Device, "vkCmdDebugMarkerBeginEXT" ) );
        EXPECT_NE( nullptr, vkGetDeviceProcAddr( Vk.Device, "vkCmdDebugMarkerEndEXT" ) );
        EXPECT_NE( nullptr, vkGetDeviceProcAddr( Vk.Device, "vkCmdDebugMarkerInsertEXT" ) );
        EXPECT_NE( nullptr, vkGetDeviceProcAddr( Vk.Device, "vkDebugMarkerSetObjectNameEXT" ) );
        EXPECT_NE( nullptr, vkGetDeviceProcAddr( Vk.Device, "vkDebugMarkerSetObjectTagEXT" ) );

        #ifdef WIN32
        SetEnvironmentVariableA( "VK_INSTANCE_LAYERS", nullptr );
        #endif
    }
}

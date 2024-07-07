// Copyright (c) 2019-2021 Lukasz Stalmirski
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
#include "profiler_vulkan_simple_triangle.h"

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

            SkipIfLayerNotPresent();
            SetEnvironmentVariable( "VK_INSTANCE_LAYERS", VK_LAYER_profiler_name );
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

        inline void SkipIfLayerNotPresent()
        {
            uint32_t layerCount = 0;
            vkEnumerateInstanceLayerProperties( &layerCount, nullptr );

            std::vector<VkLayerProperties> layers( layerCount );
            vkEnumerateInstanceLayerProperties( &layerCount, layers.data() );

            for( const VkLayerProperties& layer : layers )
            {
                if( ( strcmp( layer.layerName, VK_LAYER_profiler_name ) == 0 ) &&
                    ( layer.implementationVersion == VK_LAYER_profiler_impl_ver ) )
                {
                    // Profiler layer found.
                    return;
                }
            }

            GTEST_SKIP() << VK_LAYER_profiler_name " not found.";
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

    TEST_F( ProfilerExtensionsULT, vkGetProfilerFrameDataEXT )
    {
        // Create vulkan instance with profiler layer enabled externally
        VulkanState Vk;

        // Load entry points to loader
        VkLayerInstanceDispatchTable IDT;
        VkLayerDeviceDispatchTable DT;

        IDT.Initialize( Vk.Instance, vkGetInstanceProcAddr );
        DT.Initialize( Vk.Device, vkGetDeviceProcAddr );

        // Initialize simple triangle app
        VulkanSimpleTriangle simpleTriangle( &Vk, IDT, DT );

        VkCommandBuffer commandBuffer;

        { // Allocate command buffer
            VkCommandBufferAllocateInfo allocateInfo = {};
            allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocateInfo.commandPool = Vk.CommandPool;
            allocateInfo.commandBufferCount = 1;
            ASSERT_EQ( VK_SUCCESS, DT.AllocateCommandBuffers( Vk.Device, &allocateInfo, &commandBuffer ) );
        }
        { // Begin command buffer
            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            ASSERT_EQ( VK_SUCCESS, DT.BeginCommandBuffer( commandBuffer, &beginInfo ) );
        }
        { // Image layout transitions
            VkImageMemoryBarrier barrier = {};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            barrier.srcQueueFamilyIndex = Vk.QueueFamilyIndex;
            barrier.dstQueueFamilyIndex = Vk.QueueFamilyIndex;
            barrier.image = simpleTriangle.FramebufferImage;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
            barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;

            DT.CmdPipelineBarrier( commandBuffer,
                VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                VK_DEPENDENCY_BY_REGION_BIT,
                0, nullptr,
                0, nullptr,
                1, &barrier );
        }
        { // Begin render pass
            VkRenderPassBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            beginInfo.renderPass = simpleTriangle.RenderPass;
            beginInfo.framebuffer = simpleTriangle.Framebuffer;
            beginInfo.renderArea = simpleTriangle.RenderArea;
            DT.CmdBeginRenderPass( commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE );
        }
        { // Draw triangles
            DT.CmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, simpleTriangle.Pipeline );
            DT.CmdDraw( commandBuffer, 3, 1, 0, 0 );
            DT.CmdDraw( commandBuffer, 3, 1, 0, 0 );
        }
        { // End render pass
            DT.CmdEndRenderPass( commandBuffer );
        }
        { // End command buffer
            ASSERT_EQ( VK_SUCCESS, DT.EndCommandBuffer( commandBuffer ) );
        }
        { // Submit command buffer
            VkSubmitInfo submitInfo = {};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffer;
            ASSERT_EQ( VK_SUCCESS, DT.QueueSubmit( Vk.Queue, 1, &submitInfo, VK_NULL_HANDLE ) );
        }

        VkProfilerDataEXT data = {};
        data.sType = VK_STRUCTURE_TYPE_PROFILER_DATA_EXT;

        PFN_vkFlushProfilerEXT flushProfilerEXT = (PFN_vkFlushProfilerEXT)DT.GetDeviceProcAddr( Vk.Device, "vkFlushProfilerEXT" );
        PFN_vkFreeProfilerFrameDataEXT freeProfilerFrameDataEXT = (PFN_vkFreeProfilerFrameDataEXT)DT.GetDeviceProcAddr( Vk.Device, "vkFreeProfilerFrameDataEXT" );
        PFN_vkGetProfilerFrameDataEXT getProfilerFrameDataEXT = (PFN_vkGetProfilerFrameDataEXT)DT.GetDeviceProcAddr( Vk.Device, "vkGetProfilerFrameDataEXT" );

        ASSERT_NE( nullptr, flushProfilerEXT );
        ASSERT_NE( nullptr, freeProfilerFrameDataEXT );
        ASSERT_NE( nullptr, getProfilerFrameDataEXT );

        { // Collect data
            ASSERT_EQ( VK_SUCCESS, flushProfilerEXT( Vk.Device ) );
            ASSERT_EQ( VK_SUCCESS, getProfilerFrameDataEXT( Vk.Device, &data ) );
        }
        { // Validate data
            EXPECT_EQ( VK_STRUCTURE_TYPE_PROFILER_REGION_DATA_EXT, data.frame.sType );
            EXPECT_EQ( nullptr, data.frame.pNext );
            EXPECT_EQ( VK_PROFILER_REGION_TYPE_FRAME_EXT, data.frame.regionType );
            EXPECT_EQ( 1, data.frame.subregionCount );
            EXPECT_LT( 0, data.frame.duration );
            ASSERT_NE( nullptr, data.frame.pSubregions );

            const VkProfilerRegionDataEXT& submitData = data.frame.pSubregions[ 0 ];
            EXPECT_EQ( VK_STRUCTURE_TYPE_PROFILER_REGION_DATA_EXT, submitData.sType );
            EXPECT_EQ( nullptr, submitData.pNext );
            EXPECT_EQ( VK_PROFILER_REGION_TYPE_SUBMIT_EXT, submitData.regionType );
            EXPECT_EQ( 1, submitData.subregionCount );
            EXPECT_EQ( 0, submitData.duration );
            EXPECT_EQ( Vk.Queue, submitData.properties.submit.queue );
            ASSERT_NE( nullptr, submitData.pSubregions );

            const VkProfilerRegionDataEXT& submitInfoData = submitData.pSubregions[ 0 ];
            EXPECT_EQ( VK_STRUCTURE_TYPE_PROFILER_REGION_DATA_EXT, submitInfoData.sType );
            EXPECT_EQ( nullptr, submitInfoData.pNext );
            EXPECT_EQ( VK_PROFILER_REGION_TYPE_SUBMIT_INFO_EXT, submitInfoData.regionType );
            EXPECT_EQ( 1, submitInfoData.subregionCount );
            EXPECT_EQ( 0, submitInfoData.duration );
            ASSERT_NE( nullptr, submitInfoData.pSubregions );

            const VkProfilerRegionDataEXT& commandBufferData = submitInfoData.pSubregions[ 0 ];
            EXPECT_EQ( VK_STRUCTURE_TYPE_PROFILER_REGION_DATA_EXT, commandBufferData.sType );
            EXPECT_EQ( nullptr, commandBufferData.pNext );
            EXPECT_EQ( VK_PROFILER_REGION_TYPE_COMMAND_BUFFER_EXT, commandBufferData.regionType );
            EXPECT_EQ( 1, commandBufferData.subregionCount );
            EXPECT_LT( 0, commandBufferData.duration );
            EXPECT_EQ( commandBuffer, commandBufferData.properties.commandBuffer.handle );
            EXPECT_EQ( VK_COMMAND_BUFFER_LEVEL_PRIMARY, commandBufferData.properties.commandBuffer.level );
            ASSERT_NE( nullptr, commandBufferData.pSubregions );

            const VkProfilerRegionDataEXT& renderPassData = commandBufferData.pSubregions[ 0 ];
            EXPECT_EQ( VK_STRUCTURE_TYPE_PROFILER_REGION_DATA_EXT, renderPassData.sType );
            EXPECT_EQ( VK_PROFILER_REGION_TYPE_RENDER_PASS_EXT, renderPassData.regionType );
            EXPECT_EQ( 1, renderPassData.subregionCount );
            EXPECT_LT( 0, renderPassData.duration );
            EXPECT_EQ( simpleTriangle.RenderPass, renderPassData.properties.renderPass.handle );
            ASSERT_NE( nullptr, renderPassData.pSubregions );
            ASSERT_NE( nullptr, renderPassData.pNext );

            const VkProfilerRenderPassDataEXT& renderPassDataSpec = *(const VkProfilerRenderPassDataEXT*)renderPassData.pNext;
            EXPECT_EQ( VK_STRUCTURE_TYPE_PROFILER_RENDER_PASS_DATA_EXT, renderPassDataSpec.sType );
            EXPECT_EQ( nullptr, renderPassDataSpec.pNext );
            EXPECT_LE( 0, renderPassDataSpec.beginDuration );
            EXPECT_LE( 0, renderPassDataSpec.endDuration );

            const VkProfilerRegionDataEXT& subpassData = renderPassData.pSubregions[ 0 ];
            EXPECT_EQ( VK_STRUCTURE_TYPE_PROFILER_REGION_DATA_EXT, subpassData.sType );
            EXPECT_EQ( nullptr, subpassData.pNext );
            EXPECT_EQ( VK_PROFILER_REGION_TYPE_SUBPASS_EXT, subpassData.regionType );
            EXPECT_EQ( 1, subpassData.subregionCount );
            EXPECT_LT( 0, subpassData.duration );
            EXPECT_EQ( 0, subpassData.properties.subpass.index );
            EXPECT_EQ( VK_SUBPASS_CONTENTS_INLINE, subpassData.properties.subpass.contents );
            ASSERT_NE( nullptr, subpassData.pSubregions );

            const VkProfilerRegionDataEXT& pipelineData = subpassData.pSubregions[ 0 ];
            EXPECT_EQ( VK_STRUCTURE_TYPE_PROFILER_REGION_DATA_EXT, pipelineData.sType );
            EXPECT_EQ( nullptr, pipelineData.pNext );
            EXPECT_EQ( VK_PROFILER_REGION_TYPE_PIPELINE_EXT, pipelineData.regionType );
            EXPECT_EQ( 2, pipelineData.subregionCount );
            EXPECT_LT( 0, pipelineData.duration );
            EXPECT_EQ( simpleTriangle.Pipeline, pipelineData.properties.pipeline.handle );
            ASSERT_NE( nullptr, pipelineData.pSubregions );

            const VkProfilerRegionDataEXT& drawcall0 = pipelineData.pSubregions[ 0 ];
            EXPECT_EQ( VK_STRUCTURE_TYPE_PROFILER_REGION_DATA_EXT, drawcall0.sType );
            EXPECT_EQ( nullptr, drawcall0.pNext );
            EXPECT_EQ( VK_PROFILER_REGION_TYPE_COMMAND_EXT, drawcall0.regionType );
            EXPECT_EQ( VK_PROFILER_COMMAND_DRAW_EXT, drawcall0.properties.command.type );
            EXPECT_EQ( 0, drawcall0.subregionCount );
            EXPECT_EQ( nullptr, drawcall0.pSubregions );
            EXPECT_LT( 0, drawcall0.duration );

            const VkProfilerRegionDataEXT& drawcall1 = pipelineData.pSubregions[ 1 ];
            EXPECT_EQ( VK_STRUCTURE_TYPE_PROFILER_REGION_DATA_EXT, drawcall1.sType );
            EXPECT_EQ( nullptr, drawcall1.pNext );
            EXPECT_EQ( VK_PROFILER_REGION_TYPE_COMMAND_EXT, drawcall1.regionType );
            EXPECT_EQ( VK_PROFILER_COMMAND_DRAW_EXT, drawcall1.properties.command.type );
            EXPECT_EQ( 0, drawcall1.subregionCount );
            EXPECT_EQ( nullptr, drawcall1.pSubregions );
            EXPECT_LT( 0, drawcall1.duration );
        }
        { // Free data
            freeProfilerFrameDataEXT( Vk.Device, &data );
        }
    }
}

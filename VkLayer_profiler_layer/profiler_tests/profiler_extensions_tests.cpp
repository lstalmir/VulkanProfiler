// Copyright (c) 2019-2024 Lukasz Stalmirski
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
        inline void VerifyExtensions( const std::set<std::string>& expected, const std::vector<VkExtensionProperties>& actual )
        {
            std::set<std::string> unimplementedExtensions = expected;
            std::set<std::string> unexpectedExtensions;

            for( const VkExtensionProperties& extension : actual )
            {
                unimplementedExtensions.erase( extension.extensionName );
                unexpectedExtensions.insert( extension.extensionName );
            }

            for( const std::string& extension : expected )
            {
                unexpectedExtensions.erase( extension );
            }

            for( const std::string& extension : unimplementedExtensions )
            {
                ADD_FAILURE() << "Extension " << extension << " not implemented.";
            }

            for( const std::string& extension : unexpectedExtensions )
            {
                ADD_FAILURE() << "Unexpected extension " << extension << ".";
            }
        }
    };

    TEST_F( ProfilerExtensionsULT, EnumerateInstanceExtensionProperties )
    {
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties( VK_LAYER_profiler_name, &extensionCount, nullptr );

        std::vector<VkExtensionProperties> extensions( extensionCount );
        vkEnumerateInstanceExtensionProperties( VK_LAYER_profiler_name, &extensionCount, extensions.data() );

        std::set<std::string> expectedExtensions = {
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
            VK_EXT_LAYER_SETTINGS_EXTENSION_NAME };

        VerifyExtensions( expectedExtensions, extensions );
    }

    TEST_F( ProfilerExtensionsULT, EnumerateDeviceExtensionProperties )
    {
        // Create simple vulkan instance
        VulkanState Vk;

        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties( Vk.PhysicalDevice, VK_LAYER_profiler_name, &extensionCount, nullptr );

        std::vector<VkExtensionProperties> extensions( extensionCount );
        vkEnumerateDeviceExtensionProperties( Vk.PhysicalDevice, VK_LAYER_profiler_name, &extensionCount, extensions.data() );

        std::set<std::string> expectedExtensions = {
            VK_EXT_PROFILER_EXTENSION_NAME,
            VK_EXT_DEBUG_MARKER_EXTENSION_NAME,
            VK_EXT_TOOLING_INFO_EXTENSION_NAME };

        VerifyExtensions( expectedExtensions, extensions );
    }

    TEST_F( ProfilerExtensionsULT, DebugMarkerEXT )
    {
        VulkanState::CreateInfo vulkanCreateInfo;
        VulkanExtension debugReportExtension( VK_EXT_DEBUG_REPORT_EXTENSION_NAME, true );
        vulkanCreateInfo.InstanceExtensions.push_back( &debugReportExtension );
        VulkanExtension debugMarkerExtension( VK_EXT_DEBUG_MARKER_EXTENSION_NAME, true );
        vulkanCreateInfo.DeviceExtensions.push_back( &debugMarkerExtension );

        // Create vulkan instance with profiler layer enabled externally
        VulkanState Vk( vulkanCreateInfo );

        EXPECT_NE( nullptr, vkGetDeviceProcAddr( Vk.Device, "vkCmdDebugMarkerBeginEXT" ) );
        EXPECT_NE( nullptr, vkGetDeviceProcAddr( Vk.Device, "vkCmdDebugMarkerEndEXT" ) );
        EXPECT_NE( nullptr, vkGetDeviceProcAddr( Vk.Device, "vkCmdDebugMarkerInsertEXT" ) );
        EXPECT_NE( nullptr, vkGetDeviceProcAddr( Vk.Device, "vkDebugMarkerSetObjectNameEXT" ) );
        EXPECT_NE( nullptr, vkGetDeviceProcAddr( Vk.Device, "vkDebugMarkerSetObjectTagEXT" ) );
    }

    TEST_F( ProfilerExtensionsULT, DebugUtilsEXT )
    {
        VulkanState::CreateInfo vulkanCreateInfo;
        VulkanExtension debugUtilsExtension( VK_EXT_DEBUG_UTILS_EXTENSION_NAME, true );
        vulkanCreateInfo.InstanceExtensions.push_back( &debugUtilsExtension );

        // Create vulkan instance with profiler layer enabled externally
        VulkanState Vk( vulkanCreateInfo );

        EXPECT_NE( nullptr, vkGetDeviceProcAddr( Vk.Device, "vkCmdBeginDebugUtilsLabelEXT" ) );
        EXPECT_NE( nullptr, vkGetDeviceProcAddr( Vk.Device, "vkCmdEndDebugUtilsLabelEXT" ) );
        EXPECT_NE( nullptr, vkGetDeviceProcAddr( Vk.Device, "vkCmdInsertDebugUtilsLabelEXT" ) );
        EXPECT_NE( nullptr, vkGetDeviceProcAddr( Vk.Device, "vkSetDebugUtilsObjectNameEXT" ) );
        EXPECT_NE( nullptr, vkGetDeviceProcAddr( Vk.Device, "vkSetDebugUtilsObjectTagEXT" ) );
    }

    TEST_F( ProfilerExtensionsULT, vkGetProfilerFrameDataEXT )
    {
        VulkanState::CreateInfo vulkanCreateInfo;
        VulkanExtension profilerExtension( VK_EXT_PROFILER_EXTENSION_NAME, true );
        vulkanCreateInfo.DeviceExtensions.push_back( &profilerExtension );

        // Create vulkan instance with profiler layer enabled externally
        VulkanState Vk( vulkanCreateInfo );

        // Initialize simple triangle app
        VulkanSimpleTriangle simpleTriangle( &Vk );

        VkCommandBuffer commandBuffer;

        { // Allocate command buffer
            VkCommandBufferAllocateInfo allocateInfo = {};
            allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocateInfo.commandPool = Vk.CommandPool;
            allocateInfo.commandBufferCount = 1;
            ASSERT_EQ( VK_SUCCESS, vkAllocateCommandBuffers( Vk.Device, &allocateInfo, &commandBuffer ) );
        }
        { // Begin command buffer
            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            ASSERT_EQ( VK_SUCCESS, vkBeginCommandBuffer( commandBuffer, &beginInfo ) );
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
            vkCmdPipelineBarrier( commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &barrier );
        }
        { // Begin render pass
            VkRenderPassBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            beginInfo.renderPass = simpleTriangle.RenderPass;
            beginInfo.framebuffer = simpleTriangle.Framebuffer;
            beginInfo.renderArea = simpleTriangle.RenderArea;
            vkCmdBeginRenderPass( commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE );
        }
        { // Draw triangles
            vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, simpleTriangle.Pipeline );
            vkCmdDraw( commandBuffer, 3, 1, 0, 0 );
            vkCmdDraw( commandBuffer, 3, 1, 0, 0 );
        }
        { // End render pass
            vkCmdEndRenderPass( commandBuffer );
        }
        { // End command buffer
            ASSERT_EQ( VK_SUCCESS, vkEndCommandBuffer( commandBuffer ) );
        }
        { // Submit command buffer
            VkSubmitInfo submitInfo = {};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffer;
            ASSERT_EQ( VK_SUCCESS, vkQueueSubmit( Vk.Queue, 1, &submitInfo, VK_NULL_HANDLE ) );
        }

        VkProfilerDataEXT data = {};
        data.sType = VK_STRUCTURE_TYPE_PROFILER_DATA_EXT;

        PFN_vkFlushProfilerEXT flushProfilerEXT = (PFN_vkFlushProfilerEXT)vkGetDeviceProcAddr( Vk.Device, "vkFlushProfilerEXT" );
        PFN_vkFreeProfilerFrameDataEXT freeProfilerFrameDataEXT = (PFN_vkFreeProfilerFrameDataEXT)vkGetDeviceProcAddr( Vk.Device, "vkFreeProfilerFrameDataEXT" );
        PFN_vkGetProfilerFrameDataEXT getProfilerFrameDataEXT = (PFN_vkGetProfilerFrameDataEXT)vkGetDeviceProcAddr( Vk.Device, "vkGetProfilerFrameDataEXT" );

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

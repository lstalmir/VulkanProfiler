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

#ifndef VK_OBJECT_FN
#error VK_OBJECT_FN macro must be defined before including VkObject_Types.inl
#endif

VK_OBJECT_FN(
    /* typename                   */ VkInstance,
    /* VkObjectType               */ VK_OBJECT_TYPE_INSTANCE,
    /* VkDebugReportObjectTypeEXT */ VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
    /* ShouldHaveDebugName        */ 0 )

VK_OBJECT_FN(
    /* typename                   */ VkPhysicalDevice,
    /* VkObjectType               */ VK_OBJECT_TYPE_PHYSICAL_DEVICE,
    /* VkDebugReportObjectTypeEXT */ VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT,
    /* ShouldHaveDebugName        */ 0 )

VK_OBJECT_FN(
    /* typename                   */ VkDevice,
    /* VkObjectType               */ VK_OBJECT_TYPE_DEVICE,
    /* VkDebugReportObjectTypeEXT */ VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
    /* ShouldHaveDebugName        */ 0 )

VK_OBJECT_FN(
    /* typename                   */ VkQueue,
    /* VkObjectType               */ VK_OBJECT_TYPE_QUEUE,
    /* VkDebugReportObjectTypeEXT */ VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT,
    /* ShouldHaveDebugName        */ true )

VK_OBJECT_FN(
    /* typename                   */ VkSemaphore,
    /* VkObjectType               */ VK_OBJECT_TYPE_SEMAPHORE,
    /* VkDebugReportObjectTypeEXT */ VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT,
    /* ShouldHaveDebugName        */ 0 )

VK_OBJECT_FN(
    /* typename                   */ VkCommandBuffer,
    /* VkObjectType               */ VK_OBJECT_TYPE_COMMAND_BUFFER,
    /* VkDebugReportObjectTypeEXT */ VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
    /* ShouldHaveDebugName        */ true )

VK_OBJECT_FN(
    /* typename                   */ VkFence,
    /* VkObjectType               */ VK_OBJECT_TYPE_FENCE,
    /* VkDebugReportObjectTypeEXT */ VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT,
    /* ShouldHaveDebugName        */ 0 )

VK_OBJECT_FN(
    /* typename                   */ VkDeviceMemory,
    /* VkObjectType               */ VK_OBJECT_TYPE_DEVICE_MEMORY,
    /* VkDebugReportObjectTypeEXT */ VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT,
    /* ShouldHaveDebugName        */ 0 )

VK_OBJECT_FN(
    /* typename                   */ VkBuffer,
    /* VkObjectType               */ VK_OBJECT_TYPE_BUFFER,
    /* VkDebugReportObjectTypeEXT */ VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT,
    /* ShouldHaveDebugName        */ true )

VK_OBJECT_FN(
    /* typename                   */ VkImage,
    /* VkObjectType               */ VK_OBJECT_TYPE_IMAGE,
    /* VkDebugReportObjectTypeEXT */ VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
    /* ShouldHaveDebugName        */ true )

VK_OBJECT_FN(
    /* typename                   */ VkEvent,
    /* VkObjectType               */ VK_OBJECT_TYPE_EVENT,
    /* VkDebugReportObjectTypeEXT */ VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT,
    /* ShouldHaveDebugName        */ 0 )

VK_OBJECT_FN(
    /* typename                   */ VkQueryPool,
    /* VkObjectType               */ VK_OBJECT_TYPE_QUERY_POOL,
    /* VkDebugReportObjectTypeEXT */ VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT,
    /* ShouldHaveDebugName        */ 0 )

VK_OBJECT_FN(
    /* typename                   */ VkBufferView,
    /* VkObjectType               */ VK_OBJECT_TYPE_BUFFER_VIEW,
    /* VkDebugReportObjectTypeEXT */ VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT,
    /* ShouldHaveDebugName        */ 0 )

VK_OBJECT_FN(
    /* typename                   */ VkImageView,
    /* VkObjectType               */ VK_OBJECT_TYPE_IMAGE_VIEW,
    /* VkDebugReportObjectTypeEXT */ VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT,
    /* ShouldHaveDebugName        */ 0 )

VK_OBJECT_FN(
    /* typename                   */ VkShaderModule,
    /* VkObjectType               */ VK_OBJECT_TYPE_SHADER_MODULE,
    /* VkDebugReportObjectTypeEXT */ VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT,
    /* ShouldHaveDebugName        */ 0 )

VK_OBJECT_FN(
    /* typename                   */ VkPipelineCache,
    /* VkObjectType               */ VK_OBJECT_TYPE_PIPELINE_CACHE,
    /* VkDebugReportObjectTypeEXT */ VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT,
    /* ShouldHaveDebugName        */ 0 )

VK_OBJECT_FN(
    /* typename                   */ VkPipelineLayout,
    /* VkObjectType               */ VK_OBJECT_TYPE_PIPELINE_LAYOUT,
    /* VkDebugReportObjectTypeEXT */ VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT,
    /* ShouldHaveDebugName        */ 0 )

VK_OBJECT_FN(
    /* typename                   */ VkRenderPass,
    /* VkObjectType               */ VK_OBJECT_TYPE_RENDER_PASS,
    /* VkDebugReportObjectTypeEXT */ VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT,
    /* ShouldHaveDebugName        */ true )

VK_OBJECT_FN(
    /* typename                   */ VkPipeline,
    /* VkObjectType               */ VK_OBJECT_TYPE_PIPELINE,
    /* VkDebugReportObjectTypeEXT */ VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT,
    /* ShouldHaveDebugName        */ true )

VK_OBJECT_FN(
    /* typename                   */ VkDescriptorSetLayout,
    /* VkObjectType               */ VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
    /* VkDebugReportObjectTypeEXT */ VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT,
    /* ShouldHaveDebugName        */ 0 )

VK_OBJECT_FN(
    /* typename                   */ VkSampler,
    /* VkObjectType               */ VK_OBJECT_TYPE_SAMPLER,
    /* VkDebugReportObjectTypeEXT */ VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT,
    /* ShouldHaveDebugName        */ 0 )

VK_OBJECT_FN(
    /* typename                   */ VkDescriptorPool,
    /* VkObjectType               */ VK_OBJECT_TYPE_DESCRIPTOR_POOL,
    /* VkDebugReportObjectTypeEXT */ VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT,
    /* ShouldHaveDebugName        */ 0 )

VK_OBJECT_FN(
    /* typename                   */ VkDescriptorSet,
    /* VkObjectType               */ VK_OBJECT_TYPE_DESCRIPTOR_SET,
    /* VkDebugReportObjectTypeEXT */ VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT,
    /* ShouldHaveDebugName        */ 0 )

VK_OBJECT_FN(
    /* typename                   */ VkFramebuffer,
    /* VkObjectType               */ VK_OBJECT_TYPE_FRAMEBUFFER,
    /* VkDebugReportObjectTypeEXT */ VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT,
    /* ShouldHaveDebugName        */ 0 )

VK_OBJECT_FN(
    /* typename                   */ VkCommandPool,
    /* VkObjectType               */ VK_OBJECT_TYPE_COMMAND_POOL,
    /* VkDebugReportObjectTypeEXT */ VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT,
    /* ShouldHaveDebugName        */ 0 )

VK_OBJECT_FN(
    /* typename                   */ VkSamplerYcbcrConversion,
    /* VkObjectType               */ VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION,
    /* VkDebugReportObjectTypeEXT */ VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION_EXT,
    /* ShouldHaveDebugName        */ 0 )

VK_OBJECT_FN(
    /* typename                   */ VkDescriptorUpdateTemplate,
    /* VkObjectType               */ VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE,
    /* VkDebugReportObjectTypeEXT */ VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_EXT,
    /* ShouldHaveDebugName        */ 0 )

VK_OBJECT_FN(
    /* typename                   */ VkSurfaceKHR,
    /* VkObjectType               */ VK_OBJECT_TYPE_SURFACE_KHR,
    /* VkDebugReportObjectTypeEXT */ VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT,
    /* ShouldHaveDebugName        */ 0 )

VK_OBJECT_FN(
    /* typename                   */ VkSwapchainKHR,
    /* VkObjectType               */ VK_OBJECT_TYPE_SWAPCHAIN_KHR,
    /* VkDebugReportObjectTypeEXT */ VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT,
    /* ShouldHaveDebugName        */ 0 )

VK_OBJECT_FN(
    /* typename                   */ VkDisplayKHR,
    /* VkObjectType               */ VK_OBJECT_TYPE_DISPLAY_KHR,
    /* VkDebugReportObjectTypeEXT */ VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_KHR_EXT,
    /* ShouldHaveDebugName        */ 0 )

VK_OBJECT_FN(
    /* typename                   */ VkDisplayModeKHR,
    /* VkObjectType               */ VK_OBJECT_TYPE_DISPLAY_MODE_KHR,
    /* VkDebugReportObjectTypeEXT */ VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_MODE_KHR_EXT,
    /* ShouldHaveDebugName        */ 0 )

VK_OBJECT_FN(
    /* typename                   */ VkDebugReportCallbackEXT,
    /* VkObjectType               */ VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT,
    /* VkDebugReportObjectTypeEXT */ VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT_EXT,
    /* ShouldHaveDebugName        */ 0 )

VK_OBJECT_FN(
    /* typename                   */ VkDebugUtilsMessengerEXT,
    /* VkObjectType               */ VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT,
    /* VkDebugReportObjectTypeEXT */ VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT,
    /* ShouldHaveDebugName        */ 0 )

VK_OBJECT_FN(
    /* typename                   */ VkAccelerationStructureKHR,
    /* VkObjectType               */ VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR,
    /* VkDebugReportObjectTypeEXT */ VK_DEBUG_REPORT_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR_EXT,
    /* ShouldHaveDebugName        */ 0 )

VK_OBJECT_FN(
    /* typename                   */ VkValidationCacheEXT,
    /* VkObjectType               */ VK_OBJECT_TYPE_VALIDATION_CACHE_EXT,
    /* VkDebugReportObjectTypeEXT */ VK_DEBUG_REPORT_OBJECT_TYPE_VALIDATION_CACHE_EXT,
    /* ShouldHaveDebugName        */ 0 )

VK_OBJECT_FN(
    /* typename                   */ VkPerformanceConfigurationINTEL,
    /* VkObjectType               */ VK_OBJECT_TYPE_PERFORMANCE_CONFIGURATION_INTEL,
    /* VkDebugReportObjectTypeEXT */ VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT,
    /* ShouldHaveDebugName        */ 0 )

//VK_OBJECT_FN( 
    /* typename                   */ // <>,
    /* VkObjectType               */ // VK_OBJECT_TYPE_DEFERRED_OPERATION_KHR,
    /* VkDebugReportObjectTypeEXT */ // VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT,
    /* ShouldHaveDebugName        */ // 0 )

VK_OBJECT_FN(
    /* typename                   */ VkIndirectCommandsLayoutNV,
    /* VkObjectType               */ VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NV,
    /* VkDebugReportObjectTypeEXT */ VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT,
    /* ShouldHaveDebugName        */ 0 )

VK_OBJECT_FN(
    /* typename                   */ VkPrivateDataSlotEXT,
    /* VkObjectType               */ VK_OBJECT_TYPE_PRIVATE_DATA_SLOT_EXT,
    /* VkDebugReportObjectTypeEXT */ VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT,
    /* ShouldHaveDebugName        */ 0 )

VK_OBJECT_FN(
    /* typename                   */ VkMicromapEXT,
    /* VkObjectType               */ VK_OBJECT_TYPE_MICROMAP_EXT,
    /* VkDebugReportObjectTypeEXT */ VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT,
    /* ShouldHaveDebugName        */ true )

#undef VK_OBJECT_FN

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

#pragma once
#include <imgui_impl_vulkan.h>
#include "vk_dispatch_tables.h"

// Initialization data, for ImGui_ImplVulkan_Init()
// [Please zero-clear before use!]
struct ImGui_ImplVulkanLayer_InitInfo : ImGui_ImplVulkan_InitInfo
{
    const VkLayerInstanceDispatchTable* pInstanceDispatchTable;
    const VkLayerDeviceDispatchTable* pDispatchTable;
};

class ImGui_ImplVulkan_Context
{
public:
    ImGui_ImplVulkan_Context( ImGui_ImplVulkanLayer_InitInfo* info );
    ~ImGui_ImplVulkan_Context();

    void NewFrame();
    void RenderDrawData( ImDrawData* draw_data, VkCommandBuffer command_buffer );
    bool CreateFontsTexture();
    void SetMinImageCount( uint32_t min_image_count );

    VkDescriptorSet AddTexture( VkSampler sampler, VkImageView view, VkImageLayout layout );
    void RemoveTexture( VkDescriptorSet descriptor_set );

private:
    static PFN_vkVoidFunction FunctionLoader( const char* pFunctionName, void* pUserData );
    static VkResult AllocateCommandBuffers( VkDevice, const VkCommandBufferAllocateInfo*, VkCommandBuffer* );
};

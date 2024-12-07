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

#include "imgui_impl_vulkan_layer.h"
#include "profiler_layer_functions/core/VkDevice_functions_base.h"

PFN_vkVoidFunction ImGui_ImplVulkan_Context::FunctionLoader( const char* pFunctionName, void* pUserData )
{
    auto* info = static_cast<ImGui_ImplVulkanLayer_InitInfo*>( pUserData );
    assert( info );

    // If function creates a dispatchable object, it must also set loader data.
    if( !strcmp( pFunctionName, "vkAllocateCommandBuffers" ) )
    {
        return reinterpret_cast<PFN_vkVoidFunction>( AllocateCommandBuffers );
    }

    // Nullopt if the function is not known.
    // Nullptr if the function is known but not found/supported.
    std::optional<PFN_vkVoidFunction> pfnKnownFunction;

    // Try to return a known device function first.
    pfnKnownFunction = info->pDispatchTable->Get(
        info->Device,
        pFunctionName,
        VkLayerFunctionNotFoundBehavior::eReturnNullopt );

    if( pfnKnownFunction.has_value() )
    {
        return pfnKnownFunction.value();
    }

    // If the function is not found in the device dispatch table, try to find it in the instance dispatch table.
    pfnKnownFunction = info->pInstanceDispatchTable->Get(
        info->Instance,
        pFunctionName,
        VkLayerFunctionNotFoundBehavior::eReturnNullopt );

    if( pfnKnownFunction.has_value() )
    {
        return pfnKnownFunction.value();
    }

    // If the function is not known, try to get it from the next layer.
    PFN_vkVoidFunction pfnUnknownFunction = info->pDispatchTable->GetDeviceProcAddr(
        info->Device,
        pFunctionName );

    if( pfnUnknownFunction != nullptr )
    {
        return pfnUnknownFunction;
    }

    // Unknown function not found in the device chain, try to get it from the instance chain.
    pfnUnknownFunction = info->pInstanceDispatchTable->GetInstanceProcAddr(
        info->Instance,
        pFunctionName );

    return pfnUnknownFunction;
}

VkResult ImGui_ImplVulkan_Context::AllocateCommandBuffers( VkDevice device, const VkCommandBufferAllocateInfo* pAllocateInfo, VkCommandBuffer* pCommandBuffers )
{
    auto& dd = Profiler::VkDevice_Functions_Base::DeviceDispatch.Get( device );

    // Allocate the command buffers.
    VkResult result = dd.Device.Callbacks.AllocateCommandBuffers(
        device, pAllocateInfo, pCommandBuffers );

    if( result == VK_SUCCESS )
    {
        for( uint32_t i = 0; i < pAllocateInfo->commandBufferCount; ++i )
        {
            // Initialize dispatchable handles.
            dd.Device.SetDeviceLoaderData( device, pCommandBuffers[i] );
        }
    }

    return result;
}

ImGui_ImplVulkan_Context::ImGui_ImplVulkan_Context( ImGui_ImplVulkanLayer_InitInfo* info )
{
    // Load functions for the new context.
    if( !ImGui_ImplVulkan_LoadFunctions( FunctionLoader, info ) )
    {
        assert( false );
        throw;
    }

    // Create Vulkan context.
    if( !ImGui_ImplVulkan_Init( info ) )
    {
        assert( false );
        throw;
    }
}

ImGui_ImplVulkan_Context::~ImGui_ImplVulkan_Context()
{
    ImGui_ImplVulkan_Shutdown();
}

void ImGui_ImplVulkan_Context::RenderDrawData( ImDrawData* draw_data, VkCommandBuffer command_buffer )
{
    ImGui_ImplVulkan_RenderDrawData( draw_data, command_buffer );
}

bool ImGui_ImplVulkan_Context::CreateFontsTexture()
{
    return ImGui_ImplVulkan_CreateFontsTexture();
}

void ImGui_ImplVulkan_Context::NewFrame()
{
    ImGui_ImplVulkan_NewFrame();
}

void ImGui_ImplVulkan_Context::SetMinImageCount( uint32_t min_image_count )
{
    ImGui_ImplVulkan_SetMinImageCount( min_image_count );
}

VkDescriptorSet ImGui_ImplVulkan_Context::AddTexture( VkSampler sampler, VkImageView view, VkImageLayout layout )
{
    return ImGui_ImplVulkan_AddTexture( sampler, view, layout );
}

void ImGui_ImplVulkan_Context::RemoveTexture( VkDescriptorSet descriptor_set )
{
    ImGui_ImplVulkan_RemoveTexture( descriptor_set );
}

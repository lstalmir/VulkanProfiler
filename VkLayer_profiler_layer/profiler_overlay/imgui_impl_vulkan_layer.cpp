// dear imgui: Renderer for Vulkan
// This needs to be used along with a Platform Binding (e.g. GLFW, SDL, Win32, custom..)

// Implemented features:
//  [X] Renderer: Support for large meshes (64k+ vertices) with 16-bit indices.
// Missing features:
//  [ ] Renderer: User texture binding. Changes of ImTextureID aren't supported by this binding! See https://github.com/ocornut/imgui/pull/914

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you are new to dear imgui, read examples/README.txt and read the documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

// The aim of imgui_impl_vulkan.h/.cpp is to be usable in your engine without any modification.
// IF YOU FEEL YOU NEED TO MAKE ANY CHANGE TO THIS CODE, please share them and your feedback at https://github.com/ocornut/imgui/

// Important note to the reader who wish to integrate imgui_impl_vulkan.cpp/.h in their own engine/app.
// - Common ImGui_ImplVulkan_XXX functions and structures are used to interface with imgui_impl_vulkan.cpp/.h.
//   You will use those if you want to use this rendering back-end in your engine/app.
// - Helper ImGui_ImplVulkanH_XXX functions and structures are only used by this example (main.cpp) and by
//   the back-end itself (imgui_impl_vulkan.cpp), but should PROBABLY NOT be used by your own engine/app code.
// Read comments in imgui_impl_vulkan.h.

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2019-08-01: Vulkan: Added support for specifying multisample count. Set ImGui_ImplVulkan_InitInfo::MSAASamples to one of the VkSampleCountFlagBits values to use, default is non-multisampled as before.
//  2019-05-29: Vulkan: Added support for large mesh (64K+ vertices), enable ImGuiBackendFlags_RendererHasVtxOffset flag.
//  2019-04-30: Vulkan: Added support for special ImDrawCallback_ResetRenderState callback to reset render state.
//  2019-04-04: *BREAKING CHANGE*: Vulkan: Added ImageCount/MinImageCount fields in ImGui_ImplVulkan_InitInfo, required for initialization (was previously a hard #define IMGUI_VK_QUEUED_FRAMES 2). Added ImGui_ImplVulkan_SetMinImageCount().
//  2019-04-04: Vulkan: Added VkInstance argument to ImGui_ImplVulkanH_CreateWindow() optional helper.
//  2019-04-04: Vulkan: Avoid passing negative coordinates to vkCmdSetScissor, which debug validation layers do not like.
//  2019-04-01: Vulkan: Support for 32-bit index buffer (#define ImDrawIdx unsigned int).
//  2019-02-16: Vulkan: Viewport and clipping rectangles correctly using draw_data->FramebufferScale to allow retina display.
//  2018-11-30: Misc: Setting up io.BackendRendererName so it can be displayed in the About Window.
//  2018-08-25: Vulkan: Fixed mishandled VkSurfaceCapabilitiesKHR::maxImageCount=0 case.
//  2018-06-22: Inverted the parameters to ImGui_ImplVulkan_RenderDrawData() to be consistent with other bindings.
//  2018-06-08: Misc: Extracted imgui_impl_vulkan.cpp/.h away from the old combined GLFW+Vulkan example.
//  2018-06-08: Vulkan: Use draw_data->DisplayPos and draw_data->DisplaySize to setup projection matrix and clipping rectangle.
//  2018-03-03: Vulkan: Various refactor, created a couple of ImGui_ImplVulkanH_XXX helper that the example can use and that viewport support will use.
//  2018-03-01: Vulkan: Renamed ImGui_ImplVulkan_Init_Info to ImGui_ImplVulkan_InitInfo and fields to match more closely Vulkan terminology.
//  2018-02-16: Misc: Obsoleted the io.RenderDrawListsFn callback, ImGui_ImplVulkan_Render() calls ImGui_ImplVulkan_RenderDrawData() itself.
//  2018-02-06: Misc: Removed call to ImGui::Shutdown() which is not available from 1.60 WIP, user needs to call CreateContext/DestroyContext themselves.
//  2017-05-15: Vulkan: Fix scissor offset being negative. Fix new Vulkan validation warnings. Set required depth member for buffer image copy.
//  2016-11-13: Vulkan: Fix validation layer warnings and errors and redeclare gl_PerVertex.
//  2016-10-18: Vulkan: Add location decorators & change to use structs as in/out in glsl, update embedded spv (produced with glslangValidator -x). Null the released resources.
//  2016-08-27: Vulkan: Fix Vulkan example for use when a depth buffer is active.

#include "imgui_impl_vulkan_layer.h"
#include <imgui.h>
#include <stdio.h>

//-----------------------------------------------------------------------------
// SHADERS
//-----------------------------------------------------------------------------

// glsl_shader.vert, compiled with:
// # glslangValidator -V -x -o glsl_shader.vert.u32 glsl_shader.vert
/*
#version 450 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aColor;
layout(push_constant) uniform uPushConstant { vec2 uScale; vec2 uTranslate; } pc;

out gl_PerVertex { vec4 gl_Position; };
layout(location = 0) out struct { vec4 Color; vec2 UV; } Out;

void main()
{
    Out.Color = aColor;
    Out.UV = aUV;
    gl_Position = vec4(aPos * pc.uScale + pc.uTranslate, 0, 1);
}
*/
static uint32_t __glsl_shader_vert_spv[] =
{
    0x07230203,0x00010000,0x00080001,0x0000002e,0x00000000,0x00020011,0x00000001,0x0006000b,
    0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
    0x000a000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x0000000b,0x0000000f,0x00000015,
    0x0000001b,0x0000001c,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,
    0x00000000,0x00030005,0x00000009,0x00000000,0x00050006,0x00000009,0x00000000,0x6f6c6f43,
    0x00000072,0x00040006,0x00000009,0x00000001,0x00005655,0x00030005,0x0000000b,0x0074754f,
    0x00040005,0x0000000f,0x6c6f4361,0x0000726f,0x00030005,0x00000015,0x00565561,0x00060005,
    0x00000019,0x505f6c67,0x65567265,0x78657472,0x00000000,0x00060006,0x00000019,0x00000000,
    0x505f6c67,0x7469736f,0x006e6f69,0x00030005,0x0000001b,0x00000000,0x00040005,0x0000001c,
    0x736f5061,0x00000000,0x00060005,0x0000001e,0x73755075,0x6e6f4368,0x6e617473,0x00000074,
    0x00050006,0x0000001e,0x00000000,0x61635375,0x0000656c,0x00060006,0x0000001e,0x00000001,
    0x61725475,0x616c736e,0x00006574,0x00030005,0x00000020,0x00006370,0x00040047,0x0000000b,
    0x0000001e,0x00000000,0x00040047,0x0000000f,0x0000001e,0x00000002,0x00040047,0x00000015,
    0x0000001e,0x00000001,0x00050048,0x00000019,0x00000000,0x0000000b,0x00000000,0x00030047,
    0x00000019,0x00000002,0x00040047,0x0000001c,0x0000001e,0x00000000,0x00050048,0x0000001e,
    0x00000000,0x00000023,0x00000000,0x00050048,0x0000001e,0x00000001,0x00000023,0x00000008,
    0x00030047,0x0000001e,0x00000002,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,
    0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040017,
    0x00000008,0x00000006,0x00000002,0x0004001e,0x00000009,0x00000007,0x00000008,0x00040020,
    0x0000000a,0x00000003,0x00000009,0x0004003b,0x0000000a,0x0000000b,0x00000003,0x00040015,
    0x0000000c,0x00000020,0x00000001,0x0004002b,0x0000000c,0x0000000d,0x00000000,0x00040020,
    0x0000000e,0x00000001,0x00000007,0x0004003b,0x0000000e,0x0000000f,0x00000001,0x00040020,
    0x00000011,0x00000003,0x00000007,0x0004002b,0x0000000c,0x00000013,0x00000001,0x00040020,
    0x00000014,0x00000001,0x00000008,0x0004003b,0x00000014,0x00000015,0x00000001,0x00040020,
    0x00000017,0x00000003,0x00000008,0x0003001e,0x00000019,0x00000007,0x00040020,0x0000001a,
    0x00000003,0x00000019,0x0004003b,0x0000001a,0x0000001b,0x00000003,0x0004003b,0x00000014,
    0x0000001c,0x00000001,0x0004001e,0x0000001e,0x00000008,0x00000008,0x00040020,0x0000001f,
    0x00000009,0x0000001e,0x0004003b,0x0000001f,0x00000020,0x00000009,0x00040020,0x00000021,
    0x00000009,0x00000008,0x0004002b,0x00000006,0x00000028,0x00000000,0x0004002b,0x00000006,
    0x00000029,0x3f800000,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,
    0x00000005,0x0004003d,0x00000007,0x00000010,0x0000000f,0x00050041,0x00000011,0x00000012,
    0x0000000b,0x0000000d,0x0003003e,0x00000012,0x00000010,0x0004003d,0x00000008,0x00000016,
    0x00000015,0x00050041,0x00000017,0x00000018,0x0000000b,0x00000013,0x0003003e,0x00000018,
    0x00000016,0x0004003d,0x00000008,0x0000001d,0x0000001c,0x00050041,0x00000021,0x00000022,
    0x00000020,0x0000000d,0x0004003d,0x00000008,0x00000023,0x00000022,0x00050085,0x00000008,
    0x00000024,0x0000001d,0x00000023,0x00050041,0x00000021,0x00000025,0x00000020,0x00000013,
    0x0004003d,0x00000008,0x00000026,0x00000025,0x00050081,0x00000008,0x00000027,0x00000024,
    0x00000026,0x00050051,0x00000006,0x0000002a,0x00000027,0x00000000,0x00050051,0x00000006,
    0x0000002b,0x00000027,0x00000001,0x00070050,0x00000007,0x0000002c,0x0000002a,0x0000002b,
    0x00000028,0x00000029,0x00050041,0x00000011,0x0000002d,0x0000001b,0x0000000d,0x0003003e,
    0x0000002d,0x0000002c,0x000100fd,0x00010038
};

// glsl_shader.frag, compiled with:
// # glslangValidator -V -x -o glsl_shader.frag.u32 glsl_shader.frag
/*
#version 450 core
layout(location = 0) out vec4 fColor;
layout(set=0, binding=0) uniform sampler2D sTexture;
layout(location = 0) in struct { vec4 Color; vec2 UV; } In;
void main()
{
    fColor = In.Color * texture(sTexture, In.UV.st);
}
*/
static uint32_t __glsl_shader_frag_spv[] =
{
    0x07230203,0x00010000,0x00080001,0x0000001e,0x00000000,0x00020011,0x00000001,0x0006000b,
    0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
    0x0007000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x00000009,0x0000000d,0x00030010,
    0x00000004,0x00000007,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,
    0x00000000,0x00040005,0x00000009,0x6c6f4366,0x0000726f,0x00030005,0x0000000b,0x00000000,
    0x00050006,0x0000000b,0x00000000,0x6f6c6f43,0x00000072,0x00040006,0x0000000b,0x00000001,
    0x00005655,0x00030005,0x0000000d,0x00006e49,0x00050005,0x00000016,0x78655473,0x65727574,
    0x00000000,0x00040047,0x00000009,0x0000001e,0x00000000,0x00040047,0x0000000d,0x0000001e,
    0x00000000,0x00040047,0x00000016,0x00000022,0x00000000,0x00040047,0x00000016,0x00000021,
    0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,
    0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040020,0x00000008,0x00000003,
    0x00000007,0x0004003b,0x00000008,0x00000009,0x00000003,0x00040017,0x0000000a,0x00000006,
    0x00000002,0x0004001e,0x0000000b,0x00000007,0x0000000a,0x00040020,0x0000000c,0x00000001,
    0x0000000b,0x0004003b,0x0000000c,0x0000000d,0x00000001,0x00040015,0x0000000e,0x00000020,
    0x00000001,0x0004002b,0x0000000e,0x0000000f,0x00000000,0x00040020,0x00000010,0x00000001,
    0x00000007,0x00090019,0x00000013,0x00000006,0x00000001,0x00000000,0x00000000,0x00000000,
    0x00000001,0x00000000,0x0003001b,0x00000014,0x00000013,0x00040020,0x00000015,0x00000000,
    0x00000014,0x0004003b,0x00000015,0x00000016,0x00000000,0x0004002b,0x0000000e,0x00000018,
    0x00000001,0x00040020,0x00000019,0x00000001,0x0000000a,0x00050036,0x00000002,0x00000004,
    0x00000000,0x00000003,0x000200f8,0x00000005,0x00050041,0x00000010,0x00000011,0x0000000d,
    0x0000000f,0x0004003d,0x00000007,0x00000012,0x00000011,0x0004003d,0x00000014,0x00000017,
    0x00000016,0x00050041,0x00000019,0x0000001a,0x0000000d,0x00000018,0x0004003d,0x0000000a,
    0x0000001b,0x0000001a,0x00050057,0x00000007,0x0000001c,0x00000017,0x0000001b,0x00050085,
    0x00000007,0x0000001d,0x00000012,0x0000001c,0x0003003e,0x00000009,0x0000001d,0x000100fd,
    0x00010038
};

//-----------------------------------------------------------------------------
// FUNCTIONS
//-----------------------------------------------------------------------------

ImGui_ImplVulkan_Context::ImGui_ImplVulkan_Context( ImGui_ImplVulkan_InitInfo* info, VkRenderPass render_pass )
    : m_InstanceDispatchTable()
    , m_DispatchTable()
    , m_VulkanInitInfo()
    , m_RenderPass( VK_NULL_HANDLE )
    , m_BufferMemoryAlignment( 256 )
    , m_PipelineCreateFlags( 0x00 )
    , m_DescriptorSetLayout( VK_NULL_HANDLE )
    , m_PipelineLayout( VK_NULL_HANDLE )
    , m_DescriptorSet( VK_NULL_HANDLE )
    , m_Pipeline( VK_NULL_HANDLE )
    , m_FontSampler( VK_NULL_HANDLE )
    , m_FontMemory( VK_NULL_HANDLE )
    , m_FontImage( VK_NULL_HANDLE )
    , m_FontView( VK_NULL_HANDLE )
    , m_UploadBufferMemory( VK_NULL_HANDLE )
    , m_UploadBuffer( VK_NULL_HANDLE )
    , m_MainWindowRenderBuffers()
{
    // Clear structures
    memset( &m_InstanceDispatchTable, 0, sizeof( m_InstanceDispatchTable ) );
    memset( &m_DispatchTable, 0, sizeof( m_DispatchTable ) );
    memset( &m_VulkanInitInfo, 0, sizeof( m_VulkanInitInfo ) );
    memset( &m_MainWindowRenderBuffers, 0, sizeof( m_MainWindowRenderBuffers ) );

    // Setup back-end capabilities flags
    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "imgui_impl_vulkan";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.

    IM_ASSERT( info->Instance != VK_NULL_HANDLE );
    IM_ASSERT( info->PhysicalDevice != VK_NULL_HANDLE );
    IM_ASSERT( info->Device != VK_NULL_HANDLE );
    IM_ASSERT( info->Queue != VK_NULL_HANDLE );
    IM_ASSERT( info->DescriptorPool != VK_NULL_HANDLE );
    IM_ASSERT( info->MinImageCount >= 2 );
    IM_ASSERT( info->ImageCount >= info->MinImageCount );
    IM_ASSERT( render_pass != VK_NULL_HANDLE );

    m_InstanceDispatchTable = *info->pInstanceDispatchTable;
    m_DispatchTable = *info->pDispatchTable;

    m_VulkanInitInfo = *info;
    m_RenderPass = render_pass;
    CreateDeviceObjects();
}

ImGui_ImplVulkan_Context::~ImGui_ImplVulkan_Context()
{
    DestroyDeviceObjects();
}

uint32_t ImGui_ImplVulkan_Context::MemoryType( VkMemoryPropertyFlags properties, uint32_t type_bits )
{
    ImGui_ImplVulkan_InitInfo* v = &m_VulkanInitInfo;
    VkPhysicalDeviceMemoryProperties prop;
    m_InstanceDispatchTable.GetPhysicalDeviceMemoryProperties( v->PhysicalDevice, &prop );
    for( uint32_t i = 0; i < prop.memoryTypeCount; i++ )
        if( (prop.memoryTypes[ i ].propertyFlags & properties) == properties && type_bits & (1 << i) )
            return i;
    return 0xFFFFFFFF; // Unable to find memoryType
}

void ImGui_ImplVulkan_Context::check_vk_result( VkResult err )
{
    ImGui_ImplVulkan_InitInfo* v = &m_VulkanInitInfo;
    if( v->CheckVkResultFn )
        v->CheckVkResultFn( err );
}

void ImGui_ImplVulkan_Context::CreateOrResizeBuffer( VkBuffer& buffer, VkDeviceMemory& buffer_memory, VkDeviceSize& p_buffer_size, size_t new_size, VkBufferUsageFlagBits usage )
{
    ImGui_ImplVulkan_InitInfo* v = &m_VulkanInitInfo;
    VkResult err;
    if( buffer != VK_NULL_HANDLE )
        m_DispatchTable.DestroyBuffer( v->Device, buffer, v->Allocator );
    if( buffer_memory != VK_NULL_HANDLE )
        m_DispatchTable.FreeMemory( v->Device, buffer_memory, v->Allocator );

    VkDeviceSize vertex_buffer_size_aligned = ((new_size - 1) / m_BufferMemoryAlignment + 1) * m_BufferMemoryAlignment;
    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = vertex_buffer_size_aligned;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    err = m_DispatchTable.CreateBuffer( v->Device, &buffer_info, v->Allocator, &buffer );
    check_vk_result( err );

    VkMemoryRequirements req;
    m_DispatchTable.GetBufferMemoryRequirements( v->Device, buffer, &req );
    m_BufferMemoryAlignment = (m_BufferMemoryAlignment > req.alignment) ? m_BufferMemoryAlignment : req.alignment;
    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = req.size;
    alloc_info.memoryTypeIndex = MemoryType( VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, req.memoryTypeBits );
    err = m_DispatchTable.AllocateMemory( v->Device, &alloc_info, v->Allocator, &buffer_memory );
    check_vk_result( err );

    err = m_DispatchTable.BindBufferMemory( v->Device, buffer, buffer_memory, 0 );
    check_vk_result( err );
    p_buffer_size = new_size;
}

void ImGui_ImplVulkan_Context::SetupRenderState( ImDrawData* draw_data, VkCommandBuffer command_buffer, ImGui_ImplVulkanH_FrameRenderBuffers* rb, int fb_width, int fb_height )
{
    // Bind pipeline and descriptor sets:
    {
        m_DispatchTable.CmdBindPipeline( command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline );
        VkDescriptorSet desc_set[ 1 ] = { m_DescriptorSet };
        m_DispatchTable.CmdBindDescriptorSets( command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, desc_set, 0, NULL );
    }

    // Bind Vertex And Index Buffer:
    {
        VkBuffer vertex_buffers[ 1 ] = { rb->VertexBuffer };
        VkDeviceSize vertex_offset[ 1 ] = { 0 };
        m_DispatchTable.CmdBindVertexBuffers( command_buffer, 0, 1, vertex_buffers, vertex_offset );
        m_DispatchTable.CmdBindIndexBuffer( command_buffer, rb->IndexBuffer, 0, sizeof( ImDrawIdx ) == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32 );
    }

    // Setup viewport:
    {
        VkViewport viewport;
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = (float)fb_width;
        viewport.height = (float)fb_height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        m_DispatchTable.CmdSetViewport( command_buffer, 0, 1, &viewport );
    }

    // Setup scale and translation:
    // Our visible imgui space lies from draw_data->DisplayPps (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
    {
        float scale[ 2 ];
        scale[ 0 ] = 2.0f / draw_data->DisplaySize.x;
        scale[ 1 ] = 2.0f / draw_data->DisplaySize.y;
        float translate[ 2 ];
        translate[ 0 ] = -1.0f - draw_data->DisplayPos.x * scale[ 0 ];
        translate[ 1 ] = -1.0f - draw_data->DisplayPos.y * scale[ 1 ];
        m_DispatchTable.CmdPushConstants( command_buffer, m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof( float ) * 0, sizeof( float ) * 2, scale );
        m_DispatchTable.CmdPushConstants( command_buffer, m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof( float ) * 2, sizeof( float ) * 2, translate );
    }
}

// Render function
// (this used to be set in io.RenderDrawListsFn and called by ImGui::Render(), but you can now call this directly from your main loop)
void ImGui_ImplVulkan_Context::RenderDrawData( ImDrawData* draw_data, VkCommandBuffer command_buffer )
{
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if( fb_width <= 0 || fb_height <= 0 || draw_data->TotalVtxCount == 0 )
        return;

    ImGui_ImplVulkan_InitInfo* v = &m_VulkanInitInfo;

    // Allocate array to store enough vertex/index buffers
    ImGui_ImplVulkanH_WindowRenderBuffers* wrb = &m_MainWindowRenderBuffers;
    if( wrb->FrameRenderBuffers == NULL )
    {
        wrb->Index = 0;
        wrb->Count = v->ImageCount;
        wrb->FrameRenderBuffers = (ImGui_ImplVulkanH_FrameRenderBuffers*)IM_ALLOC( sizeof( ImGui_ImplVulkanH_FrameRenderBuffers ) * wrb->Count );
        memset( wrb->FrameRenderBuffers, 0, sizeof( ImGui_ImplVulkanH_FrameRenderBuffers ) * wrb->Count );
    }
    IM_ASSERT( wrb->Count == v->ImageCount );
    wrb->Index = (wrb->Index + 1) % wrb->Count;
    ImGui_ImplVulkanH_FrameRenderBuffers* rb = &wrb->FrameRenderBuffers[ wrb->Index ];

    VkResult err;

    // Create or resize the vertex/index buffers
    size_t vertex_size = draw_data->TotalVtxCount * sizeof( ImDrawVert );
    size_t index_size = draw_data->TotalIdxCount * sizeof( ImDrawIdx );
    if( rb->VertexBuffer == VK_NULL_HANDLE || rb->VertexBufferSize < vertex_size )
        CreateOrResizeBuffer( rb->VertexBuffer, rb->VertexBufferMemory, rb->VertexBufferSize, vertex_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT );
    if( rb->IndexBuffer == VK_NULL_HANDLE || rb->IndexBufferSize < index_size )
        CreateOrResizeBuffer( rb->IndexBuffer, rb->IndexBufferMemory, rb->IndexBufferSize, index_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT );

    // Upload vertex/index data into a single contiguous GPU buffer
    {
        ImDrawVert* vtx_dst = NULL;
        ImDrawIdx* idx_dst = NULL;
        err = m_DispatchTable.MapMemory( m_VulkanInitInfo.Device, rb->VertexBufferMemory, 0, vertex_size, 0, (void**)(&vtx_dst) );
        check_vk_result( err );
        err = m_DispatchTable.MapMemory( v->Device, rb->IndexBufferMemory, 0, index_size, 0, (void**)(&idx_dst) );
        check_vk_result( err );
        for( int n = 0; n < draw_data->CmdListsCount; n++ )
        {
            const ImDrawList* cmd_list = draw_data->CmdLists[ n ];
            memcpy( vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof( ImDrawVert ) );
            memcpy( idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof( ImDrawIdx ) );
            vtx_dst += cmd_list->VtxBuffer.Size;
            idx_dst += cmd_list->IdxBuffer.Size;
        }
        VkMappedMemoryRange range[ 2 ] = {};
        range[ 0 ].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range[ 0 ].memory = rb->VertexBufferMemory;
        range[ 0 ].size = VK_WHOLE_SIZE;
        range[ 1 ].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range[ 1 ].memory = rb->IndexBufferMemory;
        range[ 1 ].size = VK_WHOLE_SIZE;
        err = m_DispatchTable.FlushMappedMemoryRanges( v->Device, 2, range );
        check_vk_result( err );
        m_DispatchTable.UnmapMemory( v->Device, rb->VertexBufferMemory );
        m_DispatchTable.UnmapMemory( v->Device, rb->IndexBufferMemory );
    }

    // Setup desired Vulkan state
    SetupRenderState( draw_data, command_buffer, rb, fb_width, fb_height );

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
    ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

    // Render command lists
    // (Because we merged all buffers into a single one, we maintain our own offset into them)
    int global_vtx_offset = 0;
    int global_idx_offset = 0;
    for( int n = 0; n < draw_data->CmdListsCount; n++ )
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[ n ];
        for( int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++ )
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[ cmd_i ];
            if( pcmd->UserCallback != NULL )
            {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                if( pcmd->UserCallback == ImDrawCallback_ResetRenderState )
                    SetupRenderState( draw_data, command_buffer, rb, fb_width, fb_height );
                else
                    pcmd->UserCallback( cmd_list, pcmd );
            }
            else
            {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec4 clip_rect;
                clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
                clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
                clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
                clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

                if( clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f )
                {
                    // Negative offsets are illegal for vkCmdSetScissor
                    if( clip_rect.x < 0.0f )
                        clip_rect.x = 0.0f;
                    if( clip_rect.y < 0.0f )
                        clip_rect.y = 0.0f;

                    // Apply scissor/clipping rectangle
                    VkRect2D scissor;
                    scissor.offset.x = (int32_t)(clip_rect.x);
                    scissor.offset.y = (int32_t)(clip_rect.y);
                    scissor.extent.width = (uint32_t)(clip_rect.z - clip_rect.x);
                    scissor.extent.height = (uint32_t)(clip_rect.w - clip_rect.y);
                    m_DispatchTable.CmdSetScissor( command_buffer, 0, 1, &scissor );

                    // Draw
                    m_DispatchTable.CmdDrawIndexed( command_buffer, pcmd->ElemCount, 1, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset, 0 );
                }
            }
        }
        global_idx_offset += cmd_list->IdxBuffer.Size;
        global_vtx_offset += cmd_list->VtxBuffer.Size;
    }
}

bool ImGui_ImplVulkan_Context::CreateFontsTexture( VkCommandBuffer command_buffer )
{
    ImGui_ImplVulkan_InitInfo* v = &m_VulkanInitInfo;
    ImGuiIO& io = ImGui::GetIO();

    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32( &pixels, &width, &height );
    size_t upload_size = width * height * 4 * sizeof( char );

    VkResult err;

    // Create the Image:
    {
        VkImageCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        info.imageType = VK_IMAGE_TYPE_2D;
        info.format = VK_FORMAT_R8G8B8A8_UNORM;
        info.extent.width = width;
        info.extent.height = height;
        info.extent.depth = 1;
        info.mipLevels = 1;
        info.arrayLayers = 1;
        info.samples = VK_SAMPLE_COUNT_1_BIT;
        info.tiling = VK_IMAGE_TILING_OPTIMAL;
        info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        err = m_DispatchTable.CreateImage( v->Device, &info, v->Allocator, &m_FontImage );
        check_vk_result( err );
        VkMemoryRequirements req;
        m_DispatchTable.GetImageMemoryRequirements( v->Device, m_FontImage, &req );
        VkMemoryAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = req.size;
        alloc_info.memoryTypeIndex = MemoryType( VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, req.memoryTypeBits );
        err = m_DispatchTable.AllocateMemory( v->Device, &alloc_info, v->Allocator, &m_FontMemory );
        check_vk_result( err );
        err = m_DispatchTable.BindImageMemory( v->Device, m_FontImage, m_FontMemory, 0 );
        check_vk_result( err );
    }

    // Create the Image View:
    {
        VkImageViewCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.image = m_FontImage;
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info.format = VK_FORMAT_R8G8B8A8_UNORM;
        info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        info.subresourceRange.levelCount = 1;
        info.subresourceRange.layerCount = 1;
        err = m_DispatchTable.CreateImageView( v->Device, &info, v->Allocator, &m_FontView );
        check_vk_result( err );
    }

    // Update the Descriptor Set:
    {
        VkDescriptorImageInfo desc_image[ 1 ] = {};
        desc_image[ 0 ].sampler = m_FontSampler;
        desc_image[ 0 ].imageView = m_FontView;
        desc_image[ 0 ].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        VkWriteDescriptorSet write_desc[ 1 ] = {};
        write_desc[ 0 ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_desc[ 0 ].dstSet = m_DescriptorSet;
        write_desc[ 0 ].descriptorCount = 1;
        write_desc[ 0 ].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write_desc[ 0 ].pImageInfo = desc_image;
        m_DispatchTable.UpdateDescriptorSets( v->Device, 1, write_desc, 0, NULL );
    }

    // Create the Upload Buffer:
    {
        VkBufferCreateInfo buffer_info = {};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = upload_size;
        buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        err = m_DispatchTable.CreateBuffer( v->Device, &buffer_info, v->Allocator, &m_UploadBuffer );
        check_vk_result( err );
        VkMemoryRequirements req;
        m_DispatchTable.GetBufferMemoryRequirements( v->Device, m_UploadBuffer, &req );
        m_BufferMemoryAlignment = (m_BufferMemoryAlignment > req.alignment) ? m_BufferMemoryAlignment : req.alignment;
        VkMemoryAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = req.size;
        alloc_info.memoryTypeIndex = MemoryType( VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, req.memoryTypeBits );
        err = m_DispatchTable.AllocateMemory( v->Device, &alloc_info, v->Allocator, &m_UploadBufferMemory );
        check_vk_result( err );
        err = m_DispatchTable.BindBufferMemory( v->Device, m_UploadBuffer, m_UploadBufferMemory, 0 );
        check_vk_result( err );
    }

    // Upload to Buffer:
    {
        char* map = NULL;
        err = m_DispatchTable.MapMemory( v->Device, m_UploadBufferMemory, 0, upload_size, 0, (void**)(&map) );
        check_vk_result( err );
        memcpy( map, pixels, upload_size );
        VkMappedMemoryRange range[ 1 ] = {};
        range[ 0 ].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range[ 0 ].memory = m_UploadBufferMemory;
        range[ 0 ].size = upload_size;
        err = m_DispatchTable.FlushMappedMemoryRanges( v->Device, 1, range );
        check_vk_result( err );
        m_DispatchTable.UnmapMemory( v->Device, m_UploadBufferMemory );
    }

    // Copy to Image:
    {
        VkImageMemoryBarrier copy_barrier[ 1 ] = {};
        copy_barrier[ 0 ].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        copy_barrier[ 0 ].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        copy_barrier[ 0 ].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        copy_barrier[ 0 ].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        copy_barrier[ 0 ].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        copy_barrier[ 0 ].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        copy_barrier[ 0 ].image = m_FontImage;
        copy_barrier[ 0 ].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy_barrier[ 0 ].subresourceRange.levelCount = 1;
        copy_barrier[ 0 ].subresourceRange.layerCount = 1;
        m_DispatchTable.CmdPipelineBarrier( command_buffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, copy_barrier );

        VkBufferImageCopy region = {};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageExtent.width = width;
        region.imageExtent.height = height;
        region.imageExtent.depth = 1;
        m_DispatchTable.CmdCopyBufferToImage( command_buffer, m_UploadBuffer, m_FontImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region );

        VkImageMemoryBarrier use_barrier[ 1 ] = {};
        use_barrier[ 0 ].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        use_barrier[ 0 ].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        use_barrier[ 0 ].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        use_barrier[ 0 ].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        use_barrier[ 0 ].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        use_barrier[ 0 ].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        use_barrier[ 0 ].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        use_barrier[ 0 ].image = m_FontImage;
        use_barrier[ 0 ].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        use_barrier[ 0 ].subresourceRange.levelCount = 1;
        use_barrier[ 0 ].subresourceRange.layerCount = 1;
        m_DispatchTable.CmdPipelineBarrier( command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, use_barrier );
    }

    // Store our identifier
    io.Fonts->TexID = (ImTextureID)(intptr_t)m_FontImage;

    return true;
}

bool ImGui_ImplVulkan_Context::CreateDeviceObjects()
{
    ImGui_ImplVulkan_InitInfo* v = &m_VulkanInitInfo;
    VkResult err;
    VkShaderModule vert_module;
    VkShaderModule frag_module;

    // Create The Shader Modules:
    {
        VkShaderModuleCreateInfo vert_info = {};
        vert_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        vert_info.codeSize = sizeof( __glsl_shader_vert_spv );
        vert_info.pCode = (uint32_t*)__glsl_shader_vert_spv;
        err = m_DispatchTable.CreateShaderModule( v->Device, &vert_info, v->Allocator, &vert_module );
        check_vk_result( err );
        VkShaderModuleCreateInfo frag_info = {};
        frag_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        frag_info.codeSize = sizeof( __glsl_shader_frag_spv );
        frag_info.pCode = (uint32_t*)__glsl_shader_frag_spv;
        err = m_DispatchTable.CreateShaderModule( v->Device, &frag_info, v->Allocator, &frag_module );
        check_vk_result( err );
    }

    if( !m_FontSampler )
    {
        VkSamplerCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        info.magFilter = VK_FILTER_LINEAR;
        info.minFilter = VK_FILTER_LINEAR;
        info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.minLod = -1000;
        info.maxLod = 1000;
        info.maxAnisotropy = 1.0f;
        err = m_DispatchTable.CreateSampler( v->Device, &info, v->Allocator, &m_FontSampler );
        check_vk_result( err );
    }

    if( !m_DescriptorSetLayout )
    {
        VkSampler sampler[ 1 ] = { m_FontSampler };
        VkDescriptorSetLayoutBinding binding[ 1 ] = {};
        binding[ 0 ].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding[ 0 ].descriptorCount = 1;
        binding[ 0 ].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        binding[ 0 ].pImmutableSamplers = sampler;
        VkDescriptorSetLayoutCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.bindingCount = 1;
        info.pBindings = binding;
        err = m_DispatchTable.CreateDescriptorSetLayout( v->Device, &info, v->Allocator, &m_DescriptorSetLayout );
        check_vk_result( err );
    }

    // Create Descriptor Set:
    {
        VkDescriptorSetAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = v->DescriptorPool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &m_DescriptorSetLayout;
        err = m_DispatchTable.AllocateDescriptorSets( v->Device, &alloc_info, &m_DescriptorSet );
        check_vk_result( err );
    }

    if( !m_PipelineLayout )
    {
        // Constants: we are using 'vec2 offset' and 'vec2 scale' instead of a full 3d projection matrix
        VkPushConstantRange push_constants[ 1 ] = {};
        push_constants[ 0 ].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        push_constants[ 0 ].offset = sizeof( float ) * 0;
        push_constants[ 0 ].size = sizeof( float ) * 4;
        VkDescriptorSetLayout set_layout[ 1 ] = { m_DescriptorSetLayout };
        VkPipelineLayoutCreateInfo layout_info = {};
        layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layout_info.setLayoutCount = 1;
        layout_info.pSetLayouts = set_layout;
        layout_info.pushConstantRangeCount = 1;
        layout_info.pPushConstantRanges = push_constants;
        err = m_DispatchTable.CreatePipelineLayout( v->Device, &layout_info, v->Allocator, &m_PipelineLayout );
        check_vk_result( err );
    }

    VkPipelineShaderStageCreateInfo stage[ 2 ] = {};
    stage[ 0 ].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage[ 0 ].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stage[ 0 ].module = vert_module;
    stage[ 0 ].pName = "main";
    stage[ 1 ].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage[ 1 ].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stage[ 1 ].module = frag_module;
    stage[ 1 ].pName = "main";

    VkVertexInputBindingDescription binding_desc[ 1 ] = {};
    binding_desc[ 0 ].stride = sizeof( ImDrawVert );
    binding_desc[ 0 ].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attribute_desc[ 3 ] = {};
    attribute_desc[ 0 ].location = 0;
    attribute_desc[ 0 ].binding = binding_desc[ 0 ].binding;
    attribute_desc[ 0 ].format = VK_FORMAT_R32G32_SFLOAT;
    attribute_desc[ 0 ].offset = IM_OFFSETOF( ImDrawVert, pos );
    attribute_desc[ 1 ].location = 1;
    attribute_desc[ 1 ].binding = binding_desc[ 0 ].binding;
    attribute_desc[ 1 ].format = VK_FORMAT_R32G32_SFLOAT;
    attribute_desc[ 1 ].offset = IM_OFFSETOF( ImDrawVert, uv );
    attribute_desc[ 2 ].location = 2;
    attribute_desc[ 2 ].binding = binding_desc[ 0 ].binding;
    attribute_desc[ 2 ].format = VK_FORMAT_R8G8B8A8_UNORM;
    attribute_desc[ 2 ].offset = IM_OFFSETOF( ImDrawVert, col );

    VkPipelineVertexInputStateCreateInfo vertex_info = {};
    vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_info.vertexBindingDescriptionCount = 1;
    vertex_info.pVertexBindingDescriptions = binding_desc;
    vertex_info.vertexAttributeDescriptionCount = 3;
    vertex_info.pVertexAttributeDescriptions = attribute_desc;

    VkPipelineInputAssemblyStateCreateInfo ia_info = {};
    ia_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewport_info = {};
    viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_info.viewportCount = 1;
    viewport_info.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo raster_info = {};
    raster_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster_info.polygonMode = VK_POLYGON_MODE_FILL;
    raster_info.cullMode = VK_CULL_MODE_NONE;
    raster_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster_info.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo ms_info = {};
    ms_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    if( v->MSAASamples != 0 )
        ms_info.rasterizationSamples = v->MSAASamples;
    else
        ms_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState color_attachment[ 1 ] = {};
    color_attachment[ 0 ].blendEnable = VK_TRUE;
    color_attachment[ 0 ].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_attachment[ 0 ].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_attachment[ 0 ].colorBlendOp = VK_BLEND_OP_ADD;
    color_attachment[ 0 ].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_attachment[ 0 ].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_attachment[ 0 ].alphaBlendOp = VK_BLEND_OP_ADD;
    color_attachment[ 0 ].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineDepthStencilStateCreateInfo depth_info = {};
    depth_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

    VkPipelineColorBlendStateCreateInfo blend_info = {};
    blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend_info.attachmentCount = 1;
    blend_info.pAttachments = color_attachment;

    VkDynamicState dynamic_states[ 2 ] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamic_state = {};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = (uint32_t)IM_ARRAYSIZE( dynamic_states );
    dynamic_state.pDynamicStates = dynamic_states;

    VkGraphicsPipelineCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    info.flags = m_PipelineCreateFlags;
    info.stageCount = 2;
    info.pStages = stage;
    info.pVertexInputState = &vertex_info;
    info.pInputAssemblyState = &ia_info;
    info.pViewportState = &viewport_info;
    info.pRasterizationState = &raster_info;
    info.pMultisampleState = &ms_info;
    info.pDepthStencilState = &depth_info;
    info.pColorBlendState = &blend_info;
    info.pDynamicState = &dynamic_state;
    info.layout = m_PipelineLayout;
    info.renderPass = m_RenderPass;
    err = m_DispatchTable.CreateGraphicsPipelines( v->Device, v->PipelineCache, 1, &info, v->Allocator, &m_Pipeline );
    check_vk_result( err );

    m_DispatchTable.DestroyShaderModule( v->Device, vert_module, v->Allocator );
    m_DispatchTable.DestroyShaderModule( v->Device, frag_module, v->Allocator );

    return true;
}

void ImGui_ImplVulkan_Context::DestroyFontUploadObjects()
{
    ImGui_ImplVulkan_InitInfo* v = &m_VulkanInitInfo;
    if( m_UploadBuffer )
    {
        m_DispatchTable.DestroyBuffer( v->Device, m_UploadBuffer, v->Allocator );
        m_UploadBuffer = VK_NULL_HANDLE;
    }
    if( m_UploadBufferMemory )
    {
        m_DispatchTable.FreeMemory( v->Device, m_UploadBufferMemory, v->Allocator );
        m_UploadBufferMemory = VK_NULL_HANDLE;
    }
}

void ImGui_ImplVulkan_Context::DestroyDeviceObjects()
{
    ImGui_ImplVulkan_InitInfo* v = &m_VulkanInitInfo;
    DestroyWindowRenderBuffers( v->Device, &m_MainWindowRenderBuffers, v->Allocator );
    DestroyFontUploadObjects();

    if( m_FontView ) { m_DispatchTable.DestroyImageView( v->Device, m_FontView, v->Allocator ); m_FontView = VK_NULL_HANDLE; }
    if( m_FontImage ) { m_DispatchTable.DestroyImage( v->Device, m_FontImage, v->Allocator ); m_FontImage = VK_NULL_HANDLE; }
    if( m_FontMemory ) { m_DispatchTable.FreeMemory( v->Device, m_FontMemory, v->Allocator ); m_FontMemory = VK_NULL_HANDLE; }
    if( m_FontSampler ) { m_DispatchTable.DestroySampler( v->Device, m_FontSampler, v->Allocator ); m_FontSampler = VK_NULL_HANDLE; }
    if( m_DescriptorSetLayout ) { m_DispatchTable.DestroyDescriptorSetLayout( v->Device, m_DescriptorSetLayout, v->Allocator ); m_DescriptorSetLayout = VK_NULL_HANDLE; }
    if( m_PipelineLayout ) { m_DispatchTable.DestroyPipelineLayout( v->Device, m_PipelineLayout, v->Allocator ); m_PipelineLayout = VK_NULL_HANDLE; }
    if( m_Pipeline ) { m_DispatchTable.DestroyPipeline( v->Device, m_Pipeline, v->Allocator ); m_Pipeline = VK_NULL_HANDLE; }
}

void ImGui_ImplVulkan_Context::NewFrame()
{
}

void ImGui_ImplVulkan_Context::SetMinImageCount( uint32_t min_image_count )
{
    IM_ASSERT( min_image_count >= 2 );
    if( m_VulkanInitInfo.MinImageCount == min_image_count )
        return;

    ImGui_ImplVulkan_InitInfo* v = &m_VulkanInitInfo;
    VkResult err = m_DispatchTable.DeviceWaitIdle( v->Device );
    check_vk_result( err );
    DestroyWindowRenderBuffers( v->Device, &m_MainWindowRenderBuffers, v->Allocator );
    m_VulkanInitInfo.MinImageCount = min_image_count;
}

void ImGui_ImplVulkan_Context::DestroyFrameRenderBuffers( VkDevice device, ImGui_ImplVulkanH_FrameRenderBuffers* buffers, const VkAllocationCallbacks* allocator )
{
    if( buffers->VertexBuffer ) { m_DispatchTable.DestroyBuffer( device, buffers->VertexBuffer, allocator ); buffers->VertexBuffer = VK_NULL_HANDLE; }
    if( buffers->VertexBufferMemory ) { m_DispatchTable.FreeMemory( device, buffers->VertexBufferMemory, allocator ); buffers->VertexBufferMemory = VK_NULL_HANDLE; }
    if( buffers->IndexBuffer ) { m_DispatchTable.DestroyBuffer( device, buffers->IndexBuffer, allocator ); buffers->IndexBuffer = VK_NULL_HANDLE; }
    if( buffers->IndexBufferMemory ) { m_DispatchTable.FreeMemory( device, buffers->IndexBufferMemory, allocator ); buffers->IndexBufferMemory = VK_NULL_HANDLE; }
    buffers->VertexBufferSize = 0;
    buffers->IndexBufferSize = 0;
}

void ImGui_ImplVulkan_Context::DestroyWindowRenderBuffers( VkDevice device, ImGui_ImplVulkanH_WindowRenderBuffers* buffers, const VkAllocationCallbacks* allocator )
{
    for( uint32_t n = 0; n < buffers->Count; n++ )
        DestroyFrameRenderBuffers( device, &buffers->FrameRenderBuffers[ n ], allocator );
    IM_FREE( buffers->FrameRenderBuffers );
    buffers->FrameRenderBuffers = NULL;
    buffers->Index = 0;
    buffers->Count = 0;
}

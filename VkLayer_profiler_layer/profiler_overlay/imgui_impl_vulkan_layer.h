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

#pragma once
#include <imgui.h>      // IMGUI_IMPL_API
#include "vk_dispatch_tables.h"

// Initialization data, for ImGui_ImplVulkan_Init()
// [Please zero-clear before use!]
struct ImGui_ImplVulkan_InitInfo
{
    VkInstance          Instance;
    VkPhysicalDevice    PhysicalDevice;
    VkDevice            Device;
    uint32_t            QueueFamily;
    VkQueue             Queue;
    VkPipelineCache     PipelineCache;
    VkDescriptorPool    DescriptorPool;
    uint32_t            MinImageCount;          // >= 2
    uint32_t            ImageCount;             // >= MinImageCount
    VkSampleCountFlagBits        MSAASamples;   // >= VK_SAMPLE_COUNT_1_BIT
    const VkAllocationCallbacks* Allocator;
    void                (*CheckVkResultFn)(VkResult err);

    VkLayerInstanceDispatchTable* pInstanceDispatchTable;
    VkLayerDeviceDispatchTable* pDispatchTable;
};

class ImGui_ImplVulkan_Context
{
public:
    ImGui_ImplVulkan_Context( ImGui_ImplVulkan_InitInfo* info, VkRenderPass render_pass );
    ~ImGui_ImplVulkan_Context();

    void NewFrame();
    void RenderDrawData( ImDrawData* draw_data, VkCommandBuffer command_buffer );
    bool CreateFontsTexture( VkCommandBuffer command_buffer );
    void DestroyFontUploadObjects();
    void SetMinImageCount( uint32_t min_image_count );

private:
    // Callbacks
    VkLayerInstanceDispatchTable m_InstanceDispatchTable;
    VkLayerDeviceDispatchTable m_DispatchTable;

    // Vulkan data
    ImGui_ImplVulkan_InitInfo m_VulkanInitInfo;
    VkRenderPass             m_RenderPass;
    VkDeviceSize             m_BufferMemoryAlignment;
    VkPipelineCreateFlags    m_PipelineCreateFlags;
    VkDescriptorSetLayout    m_DescriptorSetLayout;
    VkPipelineLayout         m_PipelineLayout;
    VkDescriptorSet          m_DescriptorSet;
    VkPipeline               m_Pipeline;

    // Font data
    VkSampler                m_FontSampler;
    VkDeviceMemory           m_FontMemory;
    VkImage                  m_FontImage;
    VkImageView              m_FontView;
    VkDeviceMemory           m_UploadBufferMemory;
    VkBuffer                 m_UploadBuffer;

    // Reusable buffers used for rendering 1 current in-flight frame, for ImGui_ImplVulkan_RenderDrawData()
    // [Please zero-clear before use!]
    struct ImGui_ImplVulkanH_FrameRenderBuffers
    {
        VkDeviceMemory      VertexBufferMemory;
        VkDeviceMemory      IndexBufferMemory;
        VkDeviceSize        VertexBufferSize;
        VkDeviceSize        IndexBufferSize;
        VkBuffer            VertexBuffer;
        VkBuffer            IndexBuffer;
    };

    // Each viewport will hold 1 ImGui_ImplVulkanH_WindowRenderBuffers
    // [Please zero-clear before use!]
    struct ImGui_ImplVulkanH_WindowRenderBuffers
    {
        uint32_t            Index;
        uint32_t            Count;
        ImGui_ImplVulkanH_FrameRenderBuffers* FrameRenderBuffers;
    };

    // Render buffers
    ImGui_ImplVulkanH_WindowRenderBuffers m_MainWindowRenderBuffers;

    uint32_t MemoryType( VkMemoryPropertyFlags properties, uint32_t type_bits );
    void check_vk_result( VkResult );
    void CreateOrResizeBuffer( VkBuffer& buffer, VkDeviceMemory& buffer_memory, VkDeviceSize& p_buffer_size, size_t new_size, VkBufferUsageFlagBits usage );
    void SetupRenderState( ImDrawData* draw_data, VkCommandBuffer command_buffer, ImGui_ImplVulkanH_FrameRenderBuffers* rb, int fb_width, int fb_height );

    bool CreateDeviceObjects();
    void DestroyDeviceObjects();
    void DestroyFrameRenderBuffers( VkDevice device, ImGui_ImplVulkanH_FrameRenderBuffers* buffers, const VkAllocationCallbacks* allocator );
    void DestroyWindowRenderBuffers( VkDevice device, ImGui_ImplVulkanH_WindowRenderBuffers* buffers, const VkAllocationCallbacks* allocator );
};

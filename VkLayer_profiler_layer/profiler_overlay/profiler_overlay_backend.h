// Copyright (c) 2025 Lukasz Stalmirski
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
#include <memory>
#include <vector>

#include <vulkan/vulkan.h>
#include <vulkan/vk_layer.h>
#include <vk_mem_alloc.h>

struct ImGuiContext;
struct ImDrawData;
struct ImGui_ImplVulkan_Context;
struct ImGui_Window_Context;

namespace Profiler
{
    struct VkDevice_Object;
    struct VkQueue_Object;
    struct VkSwapchainKhr_Object;

    /***********************************************************************************\

    Class:
        OverlayGraphicsBackend

    Description:
        Graphics backend interface for the overlay.

    \***********************************************************************************/
    class OverlayGraphicsBackend
    {
    public:
        struct ImageCreateInfo
        {
            uint32_t Width;
            uint32_t Height;
            const uint8_t* pData;
        };

        virtual ~OverlayGraphicsBackend() = default;

        virtual bool Initialize() = 0;
        virtual void Destroy() = 0;

        virtual void WaitIdle() = 0;

        virtual bool NewFrame() = 0;
        virtual void RenderDrawData( ImDrawData* draw_data ) = 0;
        virtual void GetRenderArea( uint32_t& width, uint32_t& height ) = 0;

        virtual void* CreateImage( const ImageCreateInfo& createInfo ) = 0;
        virtual void DestroyImage( void* pImage ) = 0;
        virtual void CreateFontsImage() = 0;
    };

    /***********************************************************************************\

    Class:
        OverlayWindowBackend

    Description:
        Window backend interface for the overlay.

    \***********************************************************************************/
    class OverlayWindowBackend
    {
    public:
        virtual ~OverlayWindowBackend() = default;

        virtual bool Initialize() = 0;
        virtual void Destroy() = 0;

        virtual bool NewFrame() = 0;
        virtual void AddInputCaptureRect( int x, int y, int width, int height ) = 0;
        virtual float GetDPIScale() const = 0;
        virtual const char* GetName() const = 0;
    };

    /***********************************************************************************\

    Class:
        OverlayVulkanBackend

    Description:
        Implementation of the backend for Vulkan.

    \***********************************************************************************/
    class OverlayVulkanBackend
        : public OverlayGraphicsBackend
    {
    public:
        struct CreateInfo
        {
            VkInstance Instance;
            VkPhysicalDevice PhysicalDevice;
            VkDevice Device;
            VkQueue Queue;
            uint32_t QueueFamilyIndex;
            uint32_t ApiVersion;
            PFN_vkGetDeviceProcAddr pfn_vkGetDeviceProcAddr;
            PFN_vkGetInstanceProcAddr pfn_vkGetInstanceProcAddr;
        };

        explicit OverlayVulkanBackend( const CreateInfo& createInfo );

        bool Initialize() override;
        void Destroy() override;

        bool SetSwapchain( VkSwapchainKHR swapchain, const VkSwapchainCreateInfoKHR& createInfo );
        void SetFramePresentInfo( const VkPresentInfoKHR& presentInfo );
        const VkPresentInfoKHR& GetFramePresentInfo() const;

        void WaitIdle() override;

        bool NewFrame() override;
        void RenderDrawData( ImDrawData* pDrawData ) override;

        void* CreateImage( const ImageCreateInfo& createInfo ) override;
        void DestroyImage( void* pImage ) override;
        void CreateFontsImage() override;

    protected:
        void LoadFunctions();
        void ResetMembers();

        void DestroySwapchainResources();
        void ResetSwapchainMembers();

        bool PrepareImGuiBackend();
        void DestroyImGuiBackend();
        static PFN_vkVoidFunction FunctionLoader( const char* pFunctionName, void* pUserData );
        virtual PFN_vkVoidFunction LoadFunction( const char* pFunctionName );

        virtual VkResult AllocateCommandBuffers(
            const VkCommandBufferAllocateInfo& allocateInfo, VkCommandBuffer* pCommandBuffers );

        void RecordUploadCommands( VkCommandBuffer commandBuffer );

    protected:
        VkInstance m_Instance;
        VkPhysicalDevice m_PhysicalDevice;
        VkDevice m_Device;
        VkQueue m_Queue;
        uint32_t m_QueueFamilyIndex;
        uint32_t m_ApiVersion;

        VmaAllocator m_Allocator;
        VkCommandPool m_CommandPool;
        VkDescriptorPool m_DescriptorPool;

        bool m_ImGuiBackendResetBeforeNextFrame : 1;
        bool m_ImGuiBackendInitialized : 1;

        VkSwapchainKHR m_Swapchain;
        VkPresentInfoKHR m_PresentInfo;

        VkRenderPass m_RenderPass;
        VkExtent2D m_RenderArea;
        VkFormat m_ImageFormat;
        uint32_t m_MinImageCount;
        std::vector<VkImage> m_Images;
        std::vector<VkImageView> m_ImageViews;
        std::vector<VkFramebuffer> m_Framebuffers;
        std::vector<VkCommandBuffer> m_CommandBuffers;
        std::vector<VkFence> m_CommandFences;
        std::vector<VkSemaphore> m_CommandSemaphores;
        VkFence m_LastSubmittedFence;

        VkEvent m_ResourcesUploadEvent;
        VkSampler m_LinearSampler;

        struct ImageResource;
        std::vector<ImageResource> m_ImageResources;

        PFN_vkGetDeviceProcAddr pfn_vkGetDeviceProcAddr;
        PFN_vkGetInstanceProcAddr pfn_vkGetInstanceProcAddr;
        PFN_vkQueueSubmit pfn_vkQueueSubmit;
        PFN_vkCreateRenderPass pfn_vkCreateRenderPass;
        PFN_vkDestroyRenderPass pfn_vkDestroyRenderPass;
        PFN_vkCreateFramebuffer pfn_vkCreateFramebuffer;
        PFN_vkDestroyFramebuffer pfn_vkDestroyFramebuffer;
        PFN_vkCreateImageView pfn_vkCreateImageView;
        PFN_vkDestroyImageView pfn_vkDestroyImageView;
        PFN_vkCreateSampler pfn_vkCreateSampler;
        PFN_vkDestroySampler pfn_vkDestroySampler;
        PFN_vkCreateFence pfn_vkCreateFence;
        PFN_vkDestroyFence pfn_vkDestroyFence;
        PFN_vkWaitForFences pfn_vkWaitForFences;
        PFN_vkResetFences pfn_vkResetFences;
        PFN_vkCreateEvent pfn_vkCreateEvent;
        PFN_vkDestroyEvent pfn_vkDestroyEvent;
        PFN_vkCmdSetEvent pfn_vkCmdSetEvent;
        PFN_vkCreateSemaphore pfn_vkCreateSemaphore;
        PFN_vkDestroySemaphore pfn_vkDestroySemaphore;
        PFN_vkCreateDescriptorPool pfn_vkCreateDescriptorPool;
        PFN_vkDestroyDescriptorPool pfn_vkDestroyDescriptorPool;
        PFN_vkAllocateDescriptorSets pfn_vkAllocateDescriptorSets;
        PFN_vkCreateCommandPool pfn_vkCreateCommandPool;
        PFN_vkDestroyCommandPool pfn_vkDestroyCommandPool;
        PFN_vkAllocateCommandBuffers pfn_vkAllocateCommandBuffers;
        PFN_vkFreeCommandBuffers pfn_vkFreeCommandBuffers;
        PFN_vkBeginCommandBuffer pfn_vkBeginCommandBuffer;
        PFN_vkEndCommandBuffer pfn_vkEndCommandBuffer;
        PFN_vkGetSwapchainImagesKHR pfn_vkGetSwapchainImagesKHR;
        PFN_vkCmdBeginRenderPass pfn_vkCmdBeginRenderPass;
        PFN_vkCmdEndRenderPass pfn_vkCmdEndRenderPass;
        PFN_vkCmdPipelineBarrier pfn_vkCmdPipelineBarrier;
        PFN_vkCmdCopyBufferToImage pfn_vkCmdCopyBufferToImage;

        struct ImageResource
        {
            VkImage Image = VK_NULL_HANDLE;
            VkImageView ImageView = VK_NULL_HANDLE;
            VkDescriptorSet ImageDescriptorSet = VK_NULL_HANDLE;
            VmaAllocation ImageAllocation = VK_NULL_HANDLE;
            VkExtent2D ImageExtent = {};

            VkBuffer UploadBuffer = VK_NULL_HANDLE;
            VmaAllocation UploadBufferAllocation = VK_NULL_HANDLE;

            bool RequiresUpload = false;
        };

        VkResult InitializeImage( ImageResource& image, const ImageCreateInfo& createInfo );
        void DestroyImage( ImageResource& image );

        void RecordImageUploadCommands( VkCommandBuffer commandBuffer, ImageResource& image );
        void TransitionImageLayout( VkCommandBuffer commandBuffer, ImageResource& image, VkImageLayout oldLayout, VkImageLayout newLayout );
    };

    /***********************************************************************************\

    Class:
        OverlayVulkanLayerBackend

    Description:
        Implementation of the backend for Vulkan layer environment.

    \***********************************************************************************/
    class OverlayVulkanLayerBackend
        : public OverlayVulkanBackend
    {
    public:
        OverlayVulkanLayerBackend( VkDevice_Object& device, VkQueue_Object& queue );

        bool SetSwapchain( VkSwapchainKhr_Object& swapchain, const VkSwapchainCreateInfoKHR& createInfo );

    protected:
        PFN_vkVoidFunction LoadFunction( const char* pFunctionName ) override;
        static VkResult vkAllocateCommandBuffers(
            VkDevice device,
            const VkCommandBufferAllocateInfo* pAllocateInfo,
            VkCommandBuffer* pCommandBuffers );

        VkResult AllocateCommandBuffers(
            const VkCommandBufferAllocateInfo& allocateInfo, VkCommandBuffer* pCommandBuffers ) override;

        static CreateInfo GetCreateInfo( VkDevice_Object& device, VkQueue_Object& queue );

    protected:
        VkDevice_Object& m_DeviceObject;

        PFN_vkSetDeviceLoaderData pfn_vkSetDeviceLoaderData;
    };
}

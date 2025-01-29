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
#include "profiler_overlay_backend.h"
#include "profiler/profiler_memory_manager.h"

#include <vector>

#include <vulkan/vulkan.h>

namespace Profiler
{
    struct VkDevice_Object;
    struct VkQueue_Object;

    /***********************************************************************************\

    Class:
        OverlayLayerPlatformBackend

    Description:
        Platform-specific backend interface for Vulkan layer environment.

    \***********************************************************************************/
    class OverlayLayerPlatformBackend
    {
    public:
        virtual ~OverlayLayerPlatformBackend() = default;

        virtual void NewFrame() = 0;

        virtual void AddInputCaptureRect( int x, int y, int width, int height ) {}
        virtual float GetDPIScale() const { return 1.0f; }
    };

    /***********************************************************************************\

    Class:
        OverlayLayerBackend

    Description:
        Implementation of the backend for Vulkan layer environment.

    \***********************************************************************************/
    class OverlayLayerBackend final
        : public OverlayBackend
    {
    public:
        OverlayLayerBackend();

        VkResult Initialize( VkDevice_Object& device );
        void Destroy();

        bool IsInitialized() const;

        VkResult SetSwapchain( VkSwapchainKHR swapchain, const VkSwapchainCreateInfoKHR& createInfo );
        VkSwapchainKHR GetSwapchain() const;

        void SetFramePresentInfo( const VkPresentInfoKHR& presentInfo );
        const VkPresentInfoKHR& GetFramePresentInfo() const;

        bool PrepareImGuiBackend() override;
        void DestroyImGuiBackend() override;

        void WaitIdle() override;

        bool NewFrame() override;
        void RenderDrawData( ImDrawData* pDrawData ) override;

        void AddInputCaptureRect( int x, int y, int width, int height ) override;
        float GetDPIScale() const override;
        ImVec2 GetRenderArea() const override;

        void* CreateImage( int width, int height, const void* pData ) override;
        void DestroyImage( void* pImage ) override;
        void CreateFontsImage() override;
        void DestroyFontsImage() override;

    private:
        VkDevice_Object* m_pDevice;
        VkQueue_Object* m_pGraphicsQueue;

        VkCommandPool m_CommandPool;
        VkDescriptorPool m_DescriptorPool;

        DeviceProfilerMemoryManager m_MemoryManager;

        bool m_Initialized : 1;

        bool m_ResetBackendsBeforeNextFrame : 1;
        bool m_VulkanBackendInitialized : 1;

        OverlayLayerPlatformBackend* m_pPlatformBackend;

        VkSurfaceKHR m_Surface;
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

        void ResetMembers();

        void DestroySwapchainResources();
        void ResetSwapchainMembers();

        void RecordUploadCommands( VkCommandBuffer commandBuffer );
        void DestroyUploadResources();
        void DestroyResources();

        VkResult InitializeImage( ImageResource& image, int width, int height, const void* pData );
        void DestroyImage( ImageResource& image );

        void RecordImageUploadCommands( VkCommandBuffer commandBuffer, ImageResource& image );
        void TransitionImageLayout( VkCommandBuffer commandBuffer, ImageResource& image, VkImageLayout oldLayout, VkImageLayout newLayout );

        static PFN_vkVoidFunction FunctionLoader( const char* pFunctionName, void* pUserData );
        static VkResult AllocateCommandBuffers(
            VkDevice device,
            const VkCommandBufferAllocateInfo* pAllocateInfo,
            VkCommandBuffer* pCommandBuffers );
    };
}

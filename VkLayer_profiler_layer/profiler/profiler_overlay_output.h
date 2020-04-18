#pragma once
#include "profiler_mode.h"
#include "imgui/imgui.h"
#include <vulkan/vk_layer.h>
#include <vector>

namespace Profiler
{
    class Profiler;
    struct ProfilerSubmitData;

    /***********************************************************************************\

    Class:
        ProfilerOverlayOutput

    Description:
        Writes profiling output to the overlay.

    \***********************************************************************************/
    class ProfilerOverlayOutput
    {
    public:
        ProfilerOverlayOutput( Profiler& profiler );

        VkResult Initialize(
            const VkSwapchainCreateInfoKHR* pCreateInfo,
            VkSwapchainKHR swapchain );

        void Destroy();

        //void PostSubmitCommandBuffers( VkQueue, uint32_t, const VkSubmitInfo*, VkFence );

        void AcquireNextImage( uint32_t acquiredImageIndex );
        void Present( VkPresentInfoKHR* pPresentInfo );

        void BeginWindow();
        void WriteSubmit( const ProfilerSubmitData& submit );
        void EndWindow();

        void Flush();

        struct
        {
            uint32_t Width;
            uint32_t Height;
            uint32_t Version;
            ProfilerMode Mode;
        }   Summary;

    private:
        Profiler& m_Profiler;

        ImGuiContext* m_pContext;

        VkQueue m_GraphicsQueue;

        VkDescriptorPool m_DescriptorPool;

        VkRenderPass m_RenderPass;
        VkExtent2D m_RenderArea;
        // Image view and framebuffer for each swapchain image
        std::vector<VkImageView> m_ImageViews;
        std::vector<VkFramebuffer> m_Framebuffers;

        VkCommandPool m_CommandPool;
        std::vector<VkCommandBuffer> m_CommandBuffers;
        std::vector<VkFence> m_CommandFences;
        std::vector<VkSemaphore> m_CommandSemaphores;
        uint32_t m_CommandBufferIndex;
        uint32_t m_AcquiredImageIndex;

        std::vector<VkSemaphore> m_WaitSemaphores;
        std::vector<VkPipelineStageFlags> m_WaitStages;

        inline static WNDPROC OriginalWindowProc;
        static LRESULT CALLBACK WindowProc( HWND, UINT, WPARAM, LPARAM );
    };
}

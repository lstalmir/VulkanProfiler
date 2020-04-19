#pragma once
#include "profiler_data_aggregator.h"
#include "profiler_layer_objects/VkQueue_object.h"
#include "profiler_layer_objects/VkSwapchainKHR_object.h"
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

        void Present( const VkQueue_Object& queue, VkPresentInfoKHR* pPresentInfo );

        void Update( const ProfilerAggregatedData& data );

        void Flush();

        struct
        {
            uint32_t Width;
            uint32_t Height;
            uint32_t Version;
            std::string Message;
        }   Summary;

    private:
        Profiler& m_Profiler;

        ImGuiContext* m_pContext;

        const VkQueue_Object* m_pGraphicsQueue;
        const VkSwapchainKHR_Object* m_pSwapchain;

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

        bool m_HasNewFrame;

        inline static WNDPROC OriginalWindowProc;
        static LRESULT CALLBACK WindowProc( HWND, UINT, WPARAM, LPARAM );

        void UpdatePerformanceTab( const ProfilerAggregatedData& data );
        void UpdateMemoryTab( const ProfilerAggregatedData& data );

        void TextAlignRight( const char* fmt, ... );
    };
}

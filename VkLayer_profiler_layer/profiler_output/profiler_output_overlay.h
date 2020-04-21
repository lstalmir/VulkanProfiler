#pragma once
#include "profiler/profiler_data_aggregator.h"
#include "profiler/profiler_helpers.h"
#include "profiler_layer_objects/VkDevice_object.h"
#include "profiler_layer_objects/VkQueue_object.h"
#include "profiler_layer_objects/VkSwapchainKHR_object.h"
#include <vulkan/vk_layer.h>
#include <vector>
#include <mutex>

// Public interface
#include "profiler_ext/VkProfilerEXT.h"

struct ImGuiContext;

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
    class ProfilerOverlayOutput final
    {
    public:
        ProfilerOverlayOutput(
            VkDevice_Object& device,
            VkQueue_Object& graphicsQueue,
            VkSwapchainKHR_Object& swapchain,
            const VkSwapchainCreateInfoKHR* pCreateInfo );

        ~ProfilerOverlayOutput();

        void Present(
            const ProfilerAggregatedData& data,
            const VkQueue_Object& presentQueue,
            VkPresentInfoKHR* pPresentInfo );

    private:
        VkDevice_Object& m_Device;
        VkQueue_Object& m_GraphicsQueue;
        VkSwapchainKHR_Object& m_Swapchain;

        void* m_pWindowHandle;

        static std::mutex s_ImGuiMutex;
        ImGuiContext* m_pImGuiContext;

        VkDescriptorPool m_DescriptorPool;

        VkRenderPass m_RenderPass;
        VkExtent2D m_RenderArea;
        std::vector<VkImage> m_Images;
        std::vector<VkImageView> m_ImageViews;
        std::vector<VkFramebuffer> m_Framebuffers;

        VkCommandPool m_CommandPool;
        std::vector<VkCommandBuffer> m_CommandBuffers;
        std::vector<VkFence> m_CommandFences;
        std::vector<VkSemaphore> m_CommandSemaphores;

        // Dispatch overriden window procedures
        static LockableUnorderedMap<void*, WNDPROC> s_pfnWindowProc;

        // Common window procedure
        static LRESULT CALLBACK WindowProc( HWND, UINT, WPARAM, LPARAM );

        void Update( const ProfilerAggregatedData& data );
        void UpdatePerformanceTab( const ProfilerAggregatedData& data );
        void UpdateMemoryTab( const ProfilerAggregatedData& data );
        void UpdateStatisticsTab( const ProfilerAggregatedData& data );

        void TextAlignRight( const char* fmt, ... );

        std::string GetDebugObjectName( VkObjectType type, uint64_t handle ) const;
    };
}

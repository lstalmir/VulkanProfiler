#pragma once
#include "profiler/profiler_data_aggregator.h"
#include "profiler/profiler_helpers.h"
#include "profiler/profiler_stat_comparators.h"
#include "profiler_layer_objects/VkDevice_object.h"
#include "profiler_layer_objects/VkQueue_object.h"
#include "profiler_layer_objects/VkSwapchainKHR_object.h"
#include <vulkan/vk_layer.h>
#include <list>
#include <vector>
#include <mutex>

// Public interface
#include "profiler_ext/VkProfilerEXT.h"

struct ImGuiContext;
class ImGui_ImplVulkan_Context;
class ImGui_Window_Context;

namespace Profiler
{
    class DeviceProfiler;
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

        VkSwapchainKHR GetSwapchain() const;

        void ResetSwapchain(
            VkSwapchainKHR_Object& swapchain,
            const VkSwapchainCreateInfoKHR* pCreateInfo );

        void Present(
            const ProfilerAggregatedData& data,
            const VkQueue_Object& presentQueue,
            VkPresentInfoKHR* pPresentInfo );

    private:
        VkDevice_Object& m_Device;
        VkQueue_Object& m_GraphicsQueue;
        VkSwapchainKHR_Object* m_pSwapchain;

        OSWindowHandle m_Window;

        static std::mutex s_ImGuiMutex;
        ImGuiContext* m_pImGuiContext;
        ImGui_ImplVulkan_Context* m_pImGuiVulkanContext;
        ImGui_Window_Context* m_pImGuiWindowContext;

        VkDescriptorPool m_DescriptorPool;

        VkRenderPass m_RenderPass;
        VkExtent2D m_RenderArea;
        VkFormat m_ImageFormat;
        std::vector<VkImage> m_Images;
        std::vector<VkImageView> m_ImageViews;
        std::vector<VkFramebuffer> m_Framebuffers;

        VkCommandPool m_CommandPool;
        std::vector<VkCommandBuffer> m_CommandBuffers;
        std::vector<VkFence> m_CommandFences;
        std::vector<VkSemaphore> m_CommandSemaphores;

        std::vector<VkProfilerPerformanceCounterPropertiesEXT> m_VendorMetricProperties;

        const float m_TimestampPeriod;

        enum class FrameBrowserSortMode
        {
            eSubmissionOrder,
            eDurationDescending,
            eDurationAscending
        };

        FrameBrowserSortMode m_FrameBrowserSortMode;

        enum class HistogramGroupMode
        {
            eRenderPass,
            ePipeline,
            eDrawcall
        };

        HistogramGroupMode m_HistogramGroupMode;

        struct FrameBrowserTreeNodeIndex
        {
            uint16_t SubmitIndex;
            uint16_t PrimaryCommandBufferIndex;
            uint16_t SecondaryCommandBufferIndex;
            uint16_t RenderPassIndex;
            uint16_t SubpassIndex;
            uint16_t PipelineIndex;
        };

        ProfilerAggregatedData m_Data;
        bool m_Pause;

        void InitializeImGuiWindowHooks( const VkSwapchainCreateInfoKHR* );
        void InitializeImGuiVulkanContext( const VkSwapchainCreateInfoKHR* );

        void Update( const ProfilerAggregatedData& );
        void UpdatePerformanceTab();
        void UpdateMemoryTab();
        void UpdateStatisticsTab();
        void UpdateSelfTab();
        void UpdateSettingsTab();

        // Frame browser helpers
        void PrintCommandBuffer( const ProfilerCommandBufferData&, FrameBrowserTreeNodeIndex, uint64_t );
        void PrintRenderPass( const ProfilerRenderPass&, FrameBrowserTreeNodeIndex, uint64_t );
        void PrintSubpass( const ProfilerSubpass&, FrameBrowserTreeNodeIndex, uint64_t );
        void PrintPipeline( const ProfilerPipeline&, FrameBrowserTreeNodeIndex, uint64_t );

        void TextAlignRight( float, const char*, ... );
        void TextAlignRight( const char*, ... );
        void DrawSignificanceRect( float );

        std::string GetDebugObjectName( VkObjectType, uint64_t ) const;

        // Sort frame browser data
        template<typename Data>
        std::list<const Data*> SortFrameBrowserData( const std::vector<Data>& data ) const
        {
            std::list<const Data*> pSortedData;

            for( const Data& subdata : data )
                pSortedData.push_back( &subdata );

            switch( m_FrameBrowserSortMode )
            {
            case FrameBrowserSortMode::eSubmissionOrder:
                break; // No sorting in submission order view

            case FrameBrowserSortMode::eDurationDescending:
                pSortedData.sort( []( const Data* a, const Data* b )
                    { return DurationDesc( *a, *b ); } ); break;

            case FrameBrowserSortMode::eDurationAscending:
                pSortedData.sort( []( const Data* a, const Data* b )
                    { return DurationAsc( *a, *b ); } ); break;
            }

            return pSortedData;
        }
    };
}

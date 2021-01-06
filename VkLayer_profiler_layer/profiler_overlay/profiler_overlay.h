// Copyright (c) 2020 Lukasz Stalmirski
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
#include "profiler/profiler_data_aggregator.h"
#include "profiler/profiler_helpers.h"
#include "profiler/profiler_stat_comparators.h"
#include "profiler_layer_objects/VkDevice_object.h"
#include "profiler_layer_objects/VkQueue_object.h"
#include "profiler_layer_objects/VkSwapchainKhr_object.h"
#include "profiler_helpers/profiler_time_helpers.h"
#include <vulkan/vk_layer.h>
#include <list>
#include <vector>
#include <stack>
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
        ProfilerOverlayOutput();

        VkResult Initialize(
            VkDevice_Object& device,
            VkQueue_Object& graphicsQueue,
            VkSwapchainKhr_Object& swapchain,
            const VkSwapchainCreateInfoKHR* pCreateInfo );

        void Destroy();

        bool IsAvailable() const;

        VkSwapchainKHR GetSwapchain() const;

        VkResult ResetSwapchain(
            VkSwapchainKhr_Object& swapchain,
            const VkSwapchainCreateInfoKHR* pCreateInfo );

        void Present(
            const DeviceProfilerFrameData& data,
            const VkQueue_Object& presentQueue,
            VkPresentInfoKHR* pPresentInfo );

    private:
        VkDevice_Object* m_pDevice;
        VkQueue_Object* m_pGraphicsQueue;
        VkSwapchainKhr_Object* m_pSwapchain;

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

        Milliseconds m_TimestampPeriod;

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
            uint16_t SubmitBatchIndex;
            uint16_t SubmitIndex;
            uint16_t PrimaryCommandBufferIndex;
            uint16_t SecondaryCommandBufferIndex;
            uint16_t RenderPassIndex;
            uint16_t SubpassIndex;
            uint16_t PipelineIndex;
        };

        DeviceProfilerFrameData m_Data;
        bool m_Pause;
        bool m_ShowDebugLabels;

        class DeviceProfilerStringSerializer* m_pStringSerializer;

        VkResult InitializeImGuiWindowHooks( const VkSwapchainCreateInfoKHR* );
        VkResult InitializeImGuiVulkanContext( const VkSwapchainCreateInfoKHR* );

        void InitializeImGuiDefaultFont();

        void Update( const DeviceProfilerFrameData& );
        void UpdatePerformanceTab();
        void UpdateMemoryTab();
        void UpdateStatisticsTab();
        void UpdateSettingsTab();

        // Frame browser helpers
        void PrintCommandBuffer( const DeviceProfilerCommandBufferData&, FrameBrowserTreeNodeIndex );
        void PrintRenderPass( const DeviceProfilerRenderPassData&, FrameBrowserTreeNodeIndex );
        void PrintSubpass( const DeviceProfilerSubpassData&, FrameBrowserTreeNodeIndex, bool );
        void PrintPipeline( const DeviceProfilerPipelineData&, FrameBrowserTreeNodeIndex );
        void PrintDrawcall( const DeviceProfilerDrawcall& );
        void PrintDebugLabel( const char*, const float[ 4 ] );

        void DrawSignificanceRect( float );

        // Sort frame browser data
        template<typename Data>
        std::list<const Data*> SortFrameBrowserData( const ContainerType<Data>& data ) const
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

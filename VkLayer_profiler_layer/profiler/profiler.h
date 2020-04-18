#pragma once
#include "profiler_allocator.h"
#include "profiler_command_buffer.h"
#include "profiler_console_output.h"
#include "profiler_counters.h"
#include "profiler_data_aggregator.h"
#include "profiler_debug_utils.h"
#include "profiler_frame_stats.h"
#include "profiler_helpers.h"
#include "profiler_mode.h"
#include "profiler_overlay_output.h"
#include "profiler_layer_objects/VkDevice_object.h"
#include <unordered_map>

namespace Profiler
{
    /***********************************************************************************\

    Structure:
        ProfilerConfig

    Description:
        Profiler configuration

    \***********************************************************************************/
    struct ProfilerConfig
    {
        ProfilerMode              m_Mode;
        uint32_t                  m_NumQueriesPerCommandBuffer;
        std::chrono::milliseconds m_OutputUpdateInterval;
    };

    /***********************************************************************************\

    Class:
        Profiler

    Description:

    \***********************************************************************************/
    class Profiler
    {
    public:
        Profiler();

        VkResult Initialize( VkDevice_Object* pDevice );

        void Destroy();

        VkResult SetMode( ProfilerMode mode );

        void CreateSwapchain( const VkSwapchainCreateInfoKHR*, VkSwapchainKHR );
        void DestroySwapchain( VkSwapchainKHR );

        void SetDebugObjectName( uint64_t, const char* );

        void PreDraw( VkCommandBuffer );
        void PostDraw( VkCommandBuffer );

        void CreatePipelines( uint32_t, const VkGraphicsPipelineCreateInfo*, VkPipeline* );
        void DestroyPipeline( VkPipeline );
        void BindPipeline( VkCommandBuffer, VkPipeline );

        void CreateShaderModule( VkShaderModule, const VkShaderModuleCreateInfo* );
        void DestroyShaderModule( VkShaderModule );

        void BeginRenderPass( VkCommandBuffer, VkRenderPass );
        void EndRenderPass( VkCommandBuffer );

        void BeginCommandBuffer( VkCommandBuffer, const VkCommandBufferBeginInfo* );
        void EndCommandBuffer( VkCommandBuffer );
        void FreeCommandBuffers( uint32_t, const VkCommandBuffer* );

        void PostSubmitCommandBuffers( VkQueue, uint32_t, const VkSubmitInfo*, VkFence );

        void AcquireNextImage( uint32_t );
        void Present( VkPresentInfoKHR* );

        void OnAllocateMemory( VkDeviceMemory, const VkMemoryAllocateInfo* );
        void OnFreeMemory( VkDeviceMemory );

        FrameStats& GetCurrentFrameStats();
        const FrameStats& GetPreviousFrameStats() const;

    public:
        VkDevice_Object*        m_pDevice;

        ProfilerConfig          m_Config;

        ProfilerConsoleOutput   m_Output;
        ProfilerOverlayOutput   m_Overlay;
        ProfilerDebugUtils      m_Debug;

        ProfilerDataAggregator  m_DataAggregator;

        FrameStats*             m_pCurrentFrameStats;
        FrameStats*             m_pPreviousFrameStats;

        uint32_t                m_CurrentFrame;

        CpuTimestampCounter     m_CpuTimestampCounter;

        std::unordered_map<VkDeviceMemory, VkMemoryAllocateInfo> m_Allocations;
        std::atomic_uint64_t    m_AllocatedMemorySize;

        LockableUnorderedMap<VkCommandBuffer, ProfilerCommandBuffer> m_ProfiledCommandBuffers;

        LockableUnorderedMap<VkShaderModule, uint32_t> m_ProfiledShaderModules;
        LockableUnorderedMap<VkPipeline, ProfilerPipeline> m_ProfiledPipelines;

        VkFence                 m_SubmitFence;

        bool                    m_IsFirstSubmitInFrame;

        float                   m_TimestampPeriod;

        void PresentResults();

        ProfilerShaderTuple CreateShaderTuple( const VkGraphicsPipelineCreateInfo& createInfo );
    };
}

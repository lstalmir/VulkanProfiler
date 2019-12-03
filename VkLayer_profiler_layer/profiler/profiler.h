#pragma once
#include "profiler_allocator.h"
#include "profiler_command_buffer.h"
#include "profiler_console_output.h"
#include "profiler_counters.h"
#include "profiler_debug_utils.h"
#include "profiler_frame_stats.h"
#include "profiler_mode.h"
#include <unordered_map>
#include <vk_layer.h>
#include <vk_layer_dispatch_table.h>

namespace Profiler
{
    /***********************************************************************************\

    Class:
        Profiler

    Description:

    \***********************************************************************************/
    class Profiler
    {
    public:
        Profiler();

        VkResult Initialize( const VkApplicationInfo*,
            VkPhysicalDevice, const VkLayerInstanceDispatchTable*,
            VkDevice, const VkLayerDispatchTable* );

        void Destroy();

        void SetDebugObjectName( uint64_t, const char* );

        void PreDraw( VkCommandBuffer );
        void PostDraw( VkCommandBuffer );

        void CreatePipelines( uint32_t, const VkGraphicsPipelineCreateInfo*, VkPipeline* );
        void DestroyPipeline( VkPipeline );
        void BindPipeline( VkCommandBuffer, VkPipeline );

        void BeginRenderPass( VkCommandBuffer, VkRenderPass );
        void EndRenderPass( VkCommandBuffer );

        void BeginCommandBuffer( VkCommandBuffer, const VkCommandBufferBeginInfo* );
        void EndCommandBuffer( VkCommandBuffer );
        void FreeCommandBuffers( uint32_t, const VkCommandBuffer* );

        void PostSubmitCommandBuffers( VkQueue, uint32_t, const VkSubmitInfo*, VkFence );

        void PrePresent( VkQueue );
        void PostPresent( VkQueue );

        void OnAllocateMemory( VkDeviceMemory, const VkMemoryAllocateInfo* );
        void OnFreeMemory( VkDeviceMemory );

        FrameStats& GetCurrentFrameStats();
        const FrameStats& GetPreviousFrameStats() const;

    public:
        VkDevice                m_Device;
        VkLayerDispatchTable    m_Callbacks;

        ProfilerMode            m_Mode;

        ProfilerConsoleOutput   m_Output;
        ProfilerDebugUtils      m_Debug;

        FrameStats*             m_pCurrentFrameStats;
        FrameStats*             m_pPreviousFrameStats;

        uint32_t                m_CurrentFrame;

        CpuTimestampCounter*    m_pCpuTimestampQueryPool;
        uint32_t                m_TimestampQueryPoolSize;
        uint32_t                m_CurrentCpuTimestampQuery;

        std::unordered_map<VkDeviceMemory, VkMemoryAllocateInfo> m_Allocations;
        std::atomic_uint64_t    m_AllocatedMemorySize;

        std::unordered_map<VkCommandBuffer, ProfilerCommandBuffer> m_ProfiledCommandBuffers;

        struct Submit
        {
            std::vector<VkCommandBuffer> m_CommandBuffers;
            std::vector<ProfilerCommandBufferData> m_ProfilingData;
        };

        std::vector<Submit>     m_Submits;

        VkCommandPool           m_HelperCommandPool;
        VkCommandBuffer         m_HelperCommandBuffer;
        VkSemaphore             m_HelperCommandBufferExecutionSemaphore;

        VkFence                 m_SubmitFence;

        bool                    m_IsFirstSubmitInFrame;

        float                   m_TimestampPeriod;

        void PresentResults();
    };
}

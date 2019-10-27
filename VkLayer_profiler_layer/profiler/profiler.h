#pragma once
#include "profiler_allocator.h"
#include "profiler_command_buffer.h"
#include "profiler_counters.h"
#include "profiler_frame_stats.h"
#include <unordered_map>
#include <vk_layer.h>
#include <vk_layer_dispatch_table.h>

namespace Profiler
{
    /***********************************************************************************\

    Enum:
        ProfilerMode

    Description:
        Profiling frequency

    \***********************************************************************************/
    enum class ProfilerMode
    {
        ePerDrawcall,
        ePerPipeline,
        ePerRenderPass,
        ePerFrame
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

        VkResult Initialize(
            VkPhysicalDevice, const VkLayerInstanceDispatchTable*,
            VkDevice, const VkLayerDispatchTable* );

        void Destroy();

        void PreDraw( VkCommandBuffer );
        void PostDraw( VkCommandBuffer );

        void BindPipeline( VkCommandBuffer, VkPipeline );

        void BeginRenderPass( VkCommandBuffer, VkRenderPass );
        void EndRenderPass( VkCommandBuffer );

        void BeginCommandBuffer( VkCommandBuffer );
        void EndCommandBuffer( VkCommandBuffer );

        void SubmitCommandBuffers( VkQueue, uint32_t, const VkSubmitInfo* );

        void PrePresent( VkQueue );
        void PostPresent( VkQueue );

        void OnAllocateMemory( VkDeviceMemory, const VkMemoryAllocateInfo* );
        void OnFreeMemory( VkDeviceMemory );

        FrameStats& GetCurrentFrameStats();
        const FrameStats& GetPreviousFrameStats() const;

    protected:
        VkDevice                m_Device;
        VkLayerDispatchTable    m_Callbacks;

        ProfilerMode            m_Mode;

        FrameStats*             m_pCurrentFrameStats;
        FrameStats*             m_pPreviousFrameStats;

        uint32_t                m_CurrentFrame;

        CpuTimestampCounter*    m_pCpuTimestampQueryPool;
        uint32_t                m_TimestampQueryPoolSize;
        uint32_t                m_CurrentCpuTimestampQuery;

        std::unordered_map<VkDeviceMemory, VkMemoryAllocateInfo> m_Allocations;
        std::atomic_uint64_t    m_AllocatedMemorySize;

        std::unordered_map<VkCommandBuffer, ProfilerCommandBuffer> m_ProfiledCommandBuffers;

        float                   m_TimestampPeriod;

        void SendTimestampQuery( ProfilerCommandBuffer&, VkPipelineStageFlagBits );

        void PresentResults( const ProfilerCommandBuffer& );
    };
}

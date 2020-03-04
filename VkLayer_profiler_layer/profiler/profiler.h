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
#include <unordered_map>
#include <vk_layer.h>
#include <vk_layer_dispatch_table.h>

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
        ProfilerMode              m_DisplayMode;
        ProfilerMode              m_SamplingMode;
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

        VkResult Initialize( const VkApplicationInfo*,
            VkPhysicalDevice, const VkLayerInstanceDispatchTable*,
            VkDevice, const VkLayerDispatchTable* );

        void Destroy();

        void SetDebugObjectName( uint64_t, const char* );

        void PreDraw( VkCommandBuffer );
        void PostDraw( VkCommandBuffer );
        void PreDrawIndirect( VkCommandBuffer );
        void PostDrawIndirect( VkCommandBuffer );
        void PreDispatch( VkCommandBuffer );
        void PostDispatch( VkCommandBuffer );
        void PreDispatchIndirect( VkCommandBuffer );
        void PostDispatchIndirect( VkCommandBuffer );
        void PreCopy( VkCommandBuffer );
        void PostCopy( VkCommandBuffer );
        void PreClear( VkCommandBuffer );
        void PostClear( VkCommandBuffer, uint32_t );
        void OnPipelineBarrier( VkCommandBuffer,
            uint32_t, const VkMemoryBarrier*,
            uint32_t, const VkBufferMemoryBarrier*,
            uint32_t, const VkImageMemoryBarrier* );

        void CreatePipelines( uint32_t, const VkGraphicsPipelineCreateInfo*, VkPipeline* );
        void DestroyPipeline( VkPipeline );
        void BindPipeline( VkCommandBuffer, VkPipeline );

        void CreateShaderModule( VkShaderModule, const VkShaderModuleCreateInfo* );
        void DestroyShaderModule( VkShaderModule );

        void BeginRenderPass( VkCommandBuffer, const VkRenderPassBeginInfo* );
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

        ProfilerConfig          m_Config;

        ProfilerConsoleOutput   m_Output;
        ProfilerDebugUtils      m_Debug;

        ProfilerDataAggregator  m_DataAggregator;

        FrameStats*             m_pCurrentFrameStats;
        FrameStats*             m_pPreviousFrameStats;

        uint32_t                m_CurrentFrame;
        uint64_t                m_LastFrameBeginTimestamp;

        CpuTimestampCounter     m_CpuTimestampCounter;

        std::unordered_map<VkDeviceMemory, VkMemoryAllocateInfo> m_Allocations;
        std::atomic_uint64_t    m_AllocatedMemorySize;

        LockableUnorderedMap<VkCommandBuffer, ProfilerCommandBuffer> m_ProfiledCommandBuffers;

        LockableUnorderedMap<VkShaderModule, uint32_t> m_ProfiledShaderModules;
        LockableUnorderedMap<VkPipeline, ProfilerPipeline> m_ProfiledPipelines;

        VkFence                 m_SubmitFence;

        float                   m_TimestampPeriod;

        void PresentResults( const ProfilerAggregatedData& );
        void PresentSubmit( uint32_t, const ProfilerSubmitData& );
        void PresentCommandBuffer( uint32_t, const ProfilerCommandBufferData& );

        ProfilerShaderTuple CreateShaderTuple( const VkGraphicsPipelineCreateInfo& createInfo );
    };
}

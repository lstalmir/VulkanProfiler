#pragma once
#include "profiler_allocator.h"
#include "profiler_command_buffer.h"
#include "profiler_console_output.h"
#include "profiler_counters.h"
#include "profiler_data_aggregator.h"
#include "profiler_debug_utils.h"
#include "profiler_frame_stats.h"
#include "profiler_helpers.h"
#include "profiler_overlay_output.h"
#include "profiler_layer_objects/VkDevice_object.h"
#include "profiler_layer_objects/VkQueue_object.h"
#include <unordered_map>

// Public interface
#include "profiler_ext/VkProfilerEXT.h"

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
        VkProfilerModeEXT         m_DisplayMode;
        VkProfilerModeEXT         m_SamplingMode;
        uint32_t                  m_NumQueriesPerCommandBuffer;
        std::chrono::milliseconds m_OutputUpdateInterval;
        VkProfilerOutputFlagsEXT  m_OutputFlags;
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

        VkResult SetMode( VkProfilerModeEXT mode );

        void CreateSwapchain( const VkSwapchainCreateInfoKHR*, VkSwapchainKHR );
        void DestroySwapchain( VkSwapchainKHR );

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

        void Present( const VkQueue_Object&, VkPresentInfoKHR* );

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
        uint64_t                m_LastFrameBeginTimestamp;

        CpuTimestampCounter     m_CpuTimestampCounter;

        VkPhysicalDeviceProperties m_DeviceProperties;
        VkPhysicalDeviceMemoryProperties m_MemoryProperties;
        VkPhysicalDeviceMemoryProperties2 m_MemoryProperties2;

        std::unordered_map<VkDeviceMemory, VkMemoryAllocateInfo> m_Allocations;
        std::atomic_uint64_t    m_DeviceLocalAllocatedMemorySize;
        std::atomic_uint64_t    m_HostVisibleAllocatedMemorySize;
        std::atomic_uint64_t    m_TotalAllocatedMemorySize;
        std::atomic_uint64_t    m_DeviceLocalAllocationCount;
        std::atomic_uint64_t    m_HostVisibleAllocationCount;
        std::atomic_uint64_t    m_TotalAllocationCount;

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

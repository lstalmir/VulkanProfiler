#pragma once
#include "profiler_allocator.h"
#include "profiler_command_buffer.h"
#include "profiler_counters.h"
#include "profiler_data_aggregator.h"
#include "profiler_debug_utils.h"
#include "profiler_frame_stats.h"
#include "profiler_helpers.h"
#include "profiler_layer_objects/VkDevice_object.h"
#include "profiler_layer_objects/VkQueue_object.h"
#include <unordered_map>
#include <sstream>

#include "lockable_unordered_map.h"

// Vendor APIs
#include "intel/profiler_metrics_api.h"

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
        VkProfilerCreateFlagsEXT  m_Flags;
        VkProfilerModeEXT         m_Mode;
        VkProfilerSyncModeEXT     m_SyncMode;
    };

    /***********************************************************************************\

    Class:
        DeviceProfiler

    Description:

    \***********************************************************************************/
    class DeviceProfiler
    {
    public:
        DeviceProfiler();

        VkResult Initialize( VkDevice_Object* pDevice, const VkProfilerCreateInfoEXT* pCreateInfo );

        void Destroy();

        // Public interface
        VkResult SetMode( VkProfilerModeEXT mode );
        VkResult SetSyncMode( VkProfilerSyncModeEXT syncMode );
        ProfilerAggregatedData GetData() const;

        void RegisterCommandBuffers( VkCommandPool, VkCommandBufferLevel, uint32_t, VkCommandBuffer* );
        void UnregisterCommandBuffers( uint32_t, const VkCommandBuffer* );
        void UnregisterCommandBuffers( VkCommandPool );

        ProfilerCommandBuffer& GetCommandBuffer( VkCommandBuffer );
        // TODO: Wrap render passes
        ProfilerPipeline& GetPipeline( VkPipeline );

        // Deprecate
        void OnPipelineBarrier( VkCommandBuffer,
            uint32_t, const VkMemoryBarrier*,
            uint32_t, const VkBufferMemoryBarrier*,
            uint32_t, const VkImageMemoryBarrier* );

        void CreatePipelines( uint32_t, const VkGraphicsPipelineCreateInfo*, VkPipeline* );
        void CreatePipelines( uint32_t, const VkComputePipelineCreateInfo*, VkPipeline* );
        void DestroyPipeline( VkPipeline );

        void CreateShaderModule( VkShaderModule, const VkShaderModuleCreateInfo* );
        void DestroyShaderModule( VkShaderModule );

        void PreSubmitCommandBuffers( VkQueue, uint32_t, const VkSubmitInfo*, VkFence );
        void PostSubmitCommandBuffers( VkQueue, uint32_t, const VkSubmitInfo*, VkFence );

        void Present( const VkQueue_Object&, VkPresentInfoKHR* );

        void OnAllocateMemory( VkDeviceMemory, const VkMemoryAllocateInfo* );
        void OnFreeMemory( VkDeviceMemory );

    public:
        VkDevice_Object*        m_pDevice;

        ProfilerConfig          m_Config;

        mutable std::mutex      m_DataMutex;
        ProfilerAggregatedData  m_Data;

        ProfilerDataAggregator  m_DataAggregator;

        uint32_t                m_CurrentFrame;
        uint64_t                m_LastFrameBeginTimestamp;

        CpuTimestampCounter     m_CpuTimestampCounter;

        VkPhysicalDeviceProperties m_DeviceProperties;
        VkPhysicalDeviceMemoryProperties m_MemoryProperties;
        VkPhysicalDeviceMemoryProperties2 m_MemoryProperties2;

        ProfilerAggregatedSelfData m_SelfData;

        std::unordered_map<VkDeviceMemory, VkMemoryAllocateInfo> m_Allocations;
        std::atomic_uint64_t    m_DeviceLocalAllocatedMemorySize;
        std::atomic_uint64_t    m_HostVisibleAllocatedMemorySize;
        std::atomic_uint64_t    m_TotalAllocatedMemorySize;
        std::atomic_uint64_t    m_DeviceLocalAllocationCount;
        std::atomic_uint64_t    m_HostVisibleAllocationCount;
        std::atomic_uint64_t    m_TotalAllocationCount;

        LockableUnorderedMap<VkCommandBuffer, ProfilerCommandBuffer> m_CommandBuffers;

        LockableUnorderedMap<VkShaderModule, uint32_t> m_ShaderModuleHashes;
        LockableUnorderedMap<VkPipeline, ProfilerPipeline> m_Pipelines;

        VkFence                 m_SubmitFence;

        float                   m_TimestampPeriod;

        VkPerformanceConfigurationINTEL m_PerformanceConfigurationINTEL;

        ProfilerMetricsApi_INTEL m_MetricsApiINTEL;


        VkResult InitializeINTEL();

        ProfilerShaderTuple CreateShaderTuple( const VkGraphicsPipelineCreateInfo& createInfo );
        ProfilerShaderTuple CreateShaderTuple( const VkComputePipelineCreateInfo& createInfo );
    };
}

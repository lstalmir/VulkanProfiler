#pragma once
#include "profiler_counters.h"
#include "profiler_data_aggregator.h"
#include "profiler_debug_utils.h"
#include "profiler_helpers.h"
#include "profiler_data.h"
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
    class ProfilerCommandBuffer;

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

        VkResult Initialize( VkDevice_Object*, const VkProfilerCreateInfoEXT* );

        void Destroy();

        // Public interface
        VkResult SetMode( VkProfilerModeEXT );
        VkResult SetSyncMode( VkProfilerSyncModeEXT );
        DeviceProfilerFrameData GetData() const;

        void AllocateCommandBuffers( VkCommandPool, VkCommandBufferLevel, uint32_t, VkCommandBuffer* );
        void FreeCommandBuffers( uint32_t, const VkCommandBuffer* );
        void FreeCommandBuffers( VkCommandPool );

        ProfilerCommandBuffer& GetCommandBuffer( VkCommandBuffer );
        DeviceProfilerPipeline& GetPipeline( VkPipeline );
        DeviceProfilerRenderPass& GetRenderPass( VkRenderPass );

        void CreatePipelines( uint32_t, const VkGraphicsPipelineCreateInfo*, VkPipeline* );
        void CreatePipelines( uint32_t, const VkComputePipelineCreateInfo*, VkPipeline* );
        void DestroyPipeline( VkPipeline );

        void CreateShaderModule( VkShaderModule, const VkShaderModuleCreateInfo* );
        void DestroyShaderModule( VkShaderModule );

        void CreateRenderPass( VkRenderPass, const VkRenderPassCreateInfo* );
        void CreateRenderPass( VkRenderPass, const VkRenderPassCreateInfo2* );
        void DestroyRenderPass( VkRenderPass );

        void PreSubmitCommandBuffers( VkQueue, uint32_t, const VkSubmitInfo*, VkFence );
        void PostSubmitCommandBuffers( VkQueue, uint32_t, const VkSubmitInfo*, VkFence );

        void Present( const VkQueue_Object&, VkPresentInfoKHR* );

        void OnAllocateMemory( VkDeviceMemory, const VkMemoryAllocateInfo* );
        void OnFreeMemory( VkDeviceMemory );

    public:
        VkDevice_Object*        m_pDevice;

        ProfilerConfig          m_Config;

        mutable std::mutex      m_DataMutex;
        DeviceProfilerFrameData m_Data;

        ProfilerDataAggregator  m_DataAggregator;

        uint32_t                m_CurrentFrame;
        uint64_t                m_LastFrameBeginTimestamp;

        CpuTimestampCounter     m_CpuTimestampCounter;

        VkPhysicalDeviceProperties m_DeviceProperties;
        VkPhysicalDeviceMemoryProperties m_MemoryProperties;
        VkPhysicalDeviceMemoryProperties2 m_MemoryProperties2;

        uint64_t                m_CommandBufferLookupTimeNs;
        uint64_t                m_PipelineLookupTimeNs;
        uint64_t                m_RenderPassLookupTimeNs;

        std::unordered_map<VkDeviceMemory, VkMemoryAllocateInfo> m_Allocations;
        std::atomic_uint64_t    m_DeviceLocalAllocatedMemorySize;
        std::atomic_uint64_t    m_HostVisibleAllocatedMemorySize;
        std::atomic_uint64_t    m_TotalAllocatedMemorySize;
        std::atomic_uint64_t    m_DeviceLocalAllocationCount;
        std::atomic_uint64_t    m_HostVisibleAllocationCount;
        std::atomic_uint64_t    m_TotalAllocationCount;

        LockableUnorderedMap<VkCommandBuffer, ProfilerCommandBuffer> m_CommandBuffers;

        LockableUnorderedMap<VkShaderModule, uint32_t> m_ShaderModuleHashes;
        LockableUnorderedMap<VkPipeline, DeviceProfilerPipeline> m_Pipelines;

        LockableUnorderedMap<VkRenderPass, DeviceProfilerRenderPass> m_RenderPasses;

        VkFence                 m_SubmitFence;

        float                   m_TimestampPeriod;

        VkPerformanceConfigurationINTEL m_PerformanceConfigurationINTEL;

        ProfilerMetricsApi_INTEL m_MetricsApiINTEL;


        VkResult InitializeINTEL();

        ProfilerShaderTuple CreateShaderTuple( const VkGraphicsPipelineCreateInfo& );
        ProfilerShaderTuple CreateShaderTuple( const VkComputePipelineCreateInfo& );

        void CreateInternalPipeline( DeviceProfilerPipelineType, const char* );

        decltype(m_CommandBuffers)::iterator FreeCommandBuffer( VkCommandBuffer );
        decltype(m_CommandBuffers)::iterator FreeCommandBuffer( decltype(m_CommandBuffers)::iterator );
    };
}

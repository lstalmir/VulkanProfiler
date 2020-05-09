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

        // Public interface
        VkResult SetMode( VkProfilerModeEXT mode );
        ProfilerAggregatedData GetData() const;

        void SetDebugObjectName( uint64_t, const char* );

        // TODO: Create wrappers in AllocateCommandBuffers()
        ProfilerCommandBuffer& GetCommandBuffer( VkCommandBuffer );
        // TODO: Wrap render passes
        ProfilerPipeline& GetPipeline( VkPipeline );

        // Deprecate
        void OnPipelineBarrier( VkCommandBuffer,
            uint32_t, const VkMemoryBarrier*,
            uint32_t, const VkBufferMemoryBarrier*,
            uint32_t, const VkImageMemoryBarrier* );

        void CreatePipelines( uint32_t, const VkGraphicsPipelineCreateInfo*, VkPipeline* );
        void DestroyPipeline( VkPipeline );
        void BindPipeline( VkCommandBuffer, VkPipeline );

        void CreateShaderModule( VkShaderModule, const VkShaderModuleCreateInfo* );
        void DestroyShaderModule( VkShaderModule );

        void PreBeginRenderPass( VkCommandBuffer, const VkRenderPassBeginInfo* );
        void PostBeginRenderPass( VkCommandBuffer );
        void PreEndRenderPass( VkCommandBuffer );
        void PostEndRenderPass( VkCommandBuffer );
        void NextSubpass( VkCommandBuffer, VkSubpassContents );

        void BeginCommandBuffer( VkCommandBuffer, const VkCommandBufferBeginInfo* );
        void EndCommandBuffer( VkCommandBuffer );
        void FreeCommandBuffers( uint32_t, const VkCommandBuffer* );

        void PostSubmitCommandBuffers( VkQueue, uint32_t, const VkSubmitInfo*, VkFence );

        void Present( const VkQueue_Object&, VkPresentInfoKHR* );

        void OnAllocateMemory( VkDeviceMemory, const VkMemoryAllocateInfo* );
        void OnFreeMemory( VkDeviceMemory );

    public:
        VkDevice_Object*        m_pDevice;

        ProfilerConfig          m_Config;
        ProfilerDebugUtils      m_Debug;

        mutable std::mutex      m_DataMutex;
        ProfilerAggregatedData  m_Data;

        ProfilerDataAggregator  m_DataAggregator;

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

        #if 0
        void PresentResults( const ProfilerAggregatedData& );
        void PresentSubmit( uint32_t, const ProfilerSubmitData& );
        void PresentCommandBuffer( uint32_t, const ProfilerCommandBufferData& );
        #endif

        void FreeProfilerData( VkProfilerRegionDataEXT* pData ) const;

        template<typename... Args>
        inline char* CreateRegionName( Args... args ) const
        {
            std::stringstream sstr;
            sstr << args << ...;
            std::string str = sstr.str();
            return _strdup( str.c_str() );
        }

        void FillProfilerData( VkProfilerRegionDataEXT* pData, const ProfilerRangeStats& stats ) const;

        ProfilerShaderTuple CreateShaderTuple( const VkGraphicsPipelineCreateInfo& createInfo );

        inline VkDevice Device() const { return m_pDevice->Handle; }
        inline VkInstance Instance() const { return m_pDevice->pInstance->Handle; }
        inline VkPhysicalDevice PhysicalDevice() const { return m_pDevice->PhysicalDevice; }

        inline const VkLayerDispatchTable& Dispatch() const { return m_pDevice->Callbacks; }
        inline const VkLayerInstanceDispatchTable& InstanceDispatch() const { return m_pDevice->pInstance->Callbacks; }
    };
}

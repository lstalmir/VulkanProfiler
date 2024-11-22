// Copyright (c) 2019-2023 Lukasz Stalmirski
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
#include "profiler_counters.h"
#include "profiler_command_pool.h"
#include "profiler_config.h"
#include "profiler_data_aggregator.h"
#include "profiler_helpers.h"
#include "profiler_memory_manager.h"
#include "profiler_data.h"
#include "profiler_sync.h"
#include "profiler_layer_objects/VkObject.h"
#include "profiler_layer_objects/VkDevice_object.h"
#include "profiler_layer_objects/VkQueue_object.h"
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <string>
#include <functional>

#include "lockable_unordered_map.h"

// Vendor APIs
#include "intel/profiler_metrics_api.h"

// Public interface
#include "profiler_ext/VkProfilerEXT.h"

namespace Profiler
{
    class ProfilerCommandBuffer;

    // Deferred operation callback.
    using DeferredOperationCallback = std::function<void( VkDeferredOperationKHR )>;

    /***********************************************************************************\

    Class:
        DeviceProfiler

    Description:

    \***********************************************************************************/
    class DeviceProfiler
    {
    public:
        DeviceProfiler();

        static std::unordered_set<std::string> EnumerateOptionalDeviceExtensions( const ProfilerLayerSettings&, const VkProfilerCreateInfoEXT* );
        static std::unordered_set<std::string> EnumerateOptionalInstanceExtensions();

        static void LoadConfiguration( const ProfilerLayerSettings&, const VkProfilerCreateInfoEXT*, DeviceProfilerConfig* );

        VkResult Initialize( VkDevice_Object*, const VkProfilerCreateInfoEXT* );

        void Destroy();

        // Public interface
        VkResult SetMode( VkProfilerModeEXT );
        VkResult SetSyncMode( VkProfilerSyncModeEXT );
        std::shared_ptr<DeviceProfilerFrameData> GetData() const;

        ProfilerCommandBuffer& GetCommandBuffer( VkCommandBuffer commandBuffer );
        DeviceProfilerCommandPool& GetCommandPool( VkCommandPool commandPool );
        DeviceProfilerPipeline& GetPipeline( VkPipeline pipeline );
        DeviceProfilerRenderPass& GetRenderPass( VkRenderPass renderPass );
        ProfilerShader& GetShader( VkShaderEXT shader );

        bool ShouldCapturePipelineExecutableProperties() const;

        void CreateCommandPool( VkCommandPool, const VkCommandPoolCreateInfo* );
        void DestroyCommandPool( VkCommandPool );

        void AllocateCommandBuffers( VkCommandPool, VkCommandBufferLevel, uint32_t, VkCommandBuffer* );
        void FreeCommandBuffers( uint32_t, const VkCommandBuffer* );

        void CreateDeferredOperation( VkDeferredOperationKHR );
        void DestroyDeferredOperation( VkDeferredOperationKHR );
        void SetDeferredOperationCallback( VkDeferredOperationKHR, DeferredOperationCallback );
        void ExecuteDeferredOperationCallback( VkDeferredOperationKHR );

        void CreatePipelines( uint32_t, const VkGraphicsPipelineCreateInfo*, VkPipeline* );
        void CreatePipelines( uint32_t, const VkComputePipelineCreateInfo*, VkPipeline* );
        void CreatePipelines( uint32_t, const VkRayTracingPipelineCreateInfoKHR*, VkPipeline* );
        void DestroyPipeline( VkPipeline );

        void CreateShaderModule( VkShaderModule, const VkShaderModuleCreateInfo* );
        void DestroyShaderModule( VkShaderModule );
        void CreateShader( VkShaderEXT, const VkShaderCreateInfoEXT* );
        void DestroyShader( VkShaderEXT );

        void CreateRenderPass( VkRenderPass, const VkRenderPassCreateInfo* );
        void CreateRenderPass( VkRenderPass, const VkRenderPassCreateInfo2* );
        void DestroyRenderPass( VkRenderPass );

        void CreateSubmitBatchInfo( VkQueue, uint32_t, const VkSubmitInfo*, DeviceProfilerSubmitBatch* );
        void CreateSubmitBatchInfo( VkQueue, uint32_t, const VkSubmitInfo2*, DeviceProfilerSubmitBatch* );
        void PreSubmitCommandBuffers( const DeviceProfilerSubmitBatch& );
        void PostSubmitCommandBuffers( const DeviceProfilerSubmitBatch& );

        void FinishFrame();

        void AllocateMemory( VkDeviceMemory, const VkMemoryAllocateInfo* );
        void FreeMemory( VkDeviceMemory );
        void BindBufferMemory( VkBuffer, VkDeviceMemory, VkDeviceSize );
        void BindImageMemory( VkImage, VkDeviceMemory, VkDeviceSize );
        void FlushMappedMemoryRanges( VkDeviceMemory, uint32_t, const VkMappedMemoryRange* );
        void InvalidateMappedMemoryRanges( VkDeviceMemory, uint32_t, const VkMappedMemoryRange* );

        void CreateBuffer( VkBuffer, const VkBufferCreateInfo* );
        void DestroyBuffer( VkBuffer );

        void CreateImage( VkImage, const VkImageCreateInfo* );
        void DestroyImage( VkImage );

        void SetObjectName( VkObject, const char* );
        void SetDefaultObjectName( VkObject );
        void SetDefaultObjectName( VkPipeline );

        template<typename VkObjectTypeEnumT>
        void SetObjectName( uint64_t, VkObjectTypeEnumT, const char* );

    public:
        VkDevice_Object*        m_pDevice;

        DeviceProfilerConfig    m_Config;

        mutable std::mutex      m_SubmitMutex;
        mutable std::mutex      m_PresentMutex;
        std::shared_ptr<DeviceProfilerFrameData> m_pData;

        DeviceProfilerMemoryManager m_MemoryManager;
        ProfilerDataAggregator  m_DataAggregator;

        uint32_t                m_CurrentFrame;
        uint64_t                m_LastFrameBeginTimestamp;

        CpuTimestampCounter     m_CpuTimestampCounter;
        CpuEventFrequencyCounter m_CpuFpsCounter;

        struct DeviceMemoryInfo
        {
            VkDeviceSize        m_Size;
            uint32_t            m_TypeIndex;
            uint32_t            m_HeapIndex;
        };

        std::mutex              m_MemoryProfilerMutex;
        std::unordered_map<VkDeviceMemory, DeviceMemoryInfo> m_Allocations;
        std::unordered_map<VkBuffer, VkDeviceMemory>         m_Buffers;
        std::unordered_map<VkImage, VkDeviceMemory>          m_Images;
        DeviceProfilerMemoryData m_MemoryData;

        ConcurrentMap<VkCommandBuffer, std::unique_ptr<ProfilerCommandBuffer>> m_pCommandBuffers;
        ConcurrentMap<VkCommandPool, std::unique_ptr<DeviceProfilerCommandPool>> m_pCommandPools;

        ConcurrentMap<VkShaderModule, std::shared_ptr<ProfilerShaderModule>> m_pShaderModules;
        ConcurrentMap<VkPipeline, DeviceProfilerPipeline> m_Pipelines;
        ConcurrentMap<VkShaderEXT, ProfilerShader> m_Shaders;

        ConcurrentMap<VkDeferredOperationKHR, DeferredOperationCallback> m_DeferredOperationCallbacks;

        ConcurrentMap<VkRenderPass, DeviceProfilerRenderPass> m_RenderPasses;

        VkFence                 m_SubmitFence;

        VkPerformanceConfigurationINTEL m_PerformanceConfigurationINTEL;

        ProfilerMetricsApi_INTEL m_MetricsApiINTEL;

        DeviceProfilerSynchronization m_Synchronization;

        // Whether VK_KHR_pipeline_executable_properties is available for the profiled device.
        // In such case the internal representations of pipelines may be inspected to give more insight on potential performance issues.
        bool                    m_PipelineExecutablePropertiesEnabled;

        void*                   m_pStablePowerStateHandle;


        VkResult InitializeINTEL();
        void AcquirePerformanceConfigurationINTEL( VkQueue );
        void ReleasePerformanceConfigurationINTEL();

        void CreateInternalPipeline( DeviceProfilerPipelineType, const char* );

        void SetPipelineShaderProperties( DeviceProfilerPipeline& pipeline, uint32_t stageCount, const VkPipelineShaderStageCreateInfo* pStages );
        void SetDefaultObjectName( const DeviceProfilerPipeline& pipeline );

        decltype(m_pCommandBuffers)::iterator FreeCommandBuffer( VkCommandBuffer );
        decltype(m_pCommandBuffers)::iterator FreeCommandBuffer( decltype(m_pCommandBuffers)::iterator );

        template<typename SubmitInfoT>
        void CreateSubmitBatchInfoImpl( VkQueue, uint32_t, const SubmitInfoT*, DeviceProfilerSubmitBatch* );

        void BeginNextFrame();

        template<typename ResourceT>
        void BindResourceMemoryImpl( ResourceT, VkDeviceMemory, VkDeviceSize, VkMemoryRequirements, DeviceProfilerResourceInfo& );

        template<typename ResourceT>
        void DestroyResourceImpl( ResourceT );

        template<typename ResourceT>
        std::unordered_map<ResourceT, VkDeviceMemory>& GetResourceMap()
        {
            if constexpr( std::is_same_v<ResourceT, VkBuffer> ) return m_Buffers;
            if constexpr( std::is_same_v<ResourceT, VkImage> ) return m_Images;
        }

        template<typename ResourceT>
        std::pair<typename std::unordered_map<ResourceT, VkDeviceMemory>::iterator, bool> GetResourceMemoryEntry( ResourceT resource )
        {
            auto& resources = GetResourceMap<ResourceT>();
            auto found = resources.find( resource );
            return std::pair( found, found != resources.end() );
        }
    };

    /***********************************************************************************\

    Function:
        SetObjectName

    Description:
        Sets or restores default object name.

    \***********************************************************************************/
    template<typename VkObjectTypeEnumT>
    inline void DeviceProfiler::SetObjectName( uint64_t objectHandle, VkObjectTypeEnumT objectType, const char* pObjectName )
    {
        const auto objectTypeTraits = VkObject_Runtime_Traits::FromObjectType( objectType );

        // Don't waste memory for storing unnecessary debug names
        if( objectTypeTraits.ShouldHaveDebugName )
        {
            VkObject object( objectHandle, objectTypeTraits );

            // VK_EXT_debug_utils
            // Revision 2 (2020-04-03): pObjectName can be nullptr
            if( (pObjectName) && (std::strlen( pObjectName ) > 0) )
            {
                // Set custom object name
                SetObjectName( object, pObjectName );
            }
            else
            {
                // Restore default debug name
                SetDefaultObjectName( object );
            }
        }
    }
}

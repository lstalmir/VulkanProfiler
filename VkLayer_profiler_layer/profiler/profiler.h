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
#define PROFILER_DISABLE_CRITICAL_SECTION_OPTIMIZATION 0
#include "profiler_counters.h"
#include "profiler_data_aggregator.h"
#include "profiler_helpers.h"
#include "profiler_data.h"
#include "profiler_layer_objects/VkDevice_object.h"
#include "profiler_layer_objects/VkQueue_object.h"
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <string>

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

        static std::unordered_set<std::string> EnumerateOptionalDeviceExtensions();
        static std::unordered_set<std::string> EnumerateOptionalInstanceExtensions();

        VkResult Initialize( VkDevice_Object*, const VkProfilerCreateInfoEXT* );

        void Destroy();

        bool IsAvailable() const;

        // Public interface
        VkResult SetMode( VkProfilerModeEXT );
        VkResult SetSyncMode( VkProfilerSyncModeEXT );
        DeviceProfilerFrameData GetData() const;

        ProfilerCommandBuffer& GetCommandBuffer( VkCommandBuffer commandBuffer );
        DeviceProfilerPipeline& GetPipeline( VkPipeline pipeline );
        DeviceProfilerRenderPass& GetRenderPass( VkRenderPass renderPass );

        void AllocateCommandBuffers( VkCommandPool, VkCommandBufferLevel, uint32_t, VkCommandBuffer* );
        void FreeCommandBuffers( uint32_t, const VkCommandBuffer* );
        void FreeCommandBuffers( VkCommandPool );

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

        void FinishFrame();

        void AllocateMemory( VkDeviceMemory, const VkMemoryAllocateInfo* );
        void FreeMemory( VkDeviceMemory );

    public:
        VkDevice_Object*        m_pDevice;

        ProfilerConfig          m_Config;

        mutable std::mutex      m_SubmitMutex;
        mutable std::mutex      m_PresentMutex;
        mutable std::mutex      m_DataMutex;
        DeviceProfilerFrameData m_Data;

        ProfilerDataAggregator  m_DataAggregator;

        uint32_t                m_CurrentFrame;
        uint64_t                m_LastFrameBeginTimestamp;

        CpuTimestampCounter     m_CpuTimestampCounter;
        CpuEventFrequencyCounter m_CpuFpsCounter;

        uint64_t                m_CommandBufferAccessTimeNs;
        uint64_t                m_PipelineAccessTimeNs;
        uint64_t                m_RenderPassAccessTimeNs;
        uint64_t                m_ShaderModuleAccessTimeNs;

        ConcurrentMap<VkDeviceMemory, VkMemoryAllocateInfo> m_Allocations;
        DeviceProfilerMemoryData m_MemoryData;

        ConcurrentMap<VkCommandBuffer, ProfilerCommandBuffer> m_CommandBuffers;

        ConcurrentMap<VkShaderModule, uint32_t> m_ShaderModuleHashes;
        ConcurrentMap<VkPipeline, DeviceProfilerPipeline> m_Pipelines;

        ConcurrentMap<VkRenderPass, DeviceProfilerRenderPass> m_RenderPasses;

        VkFence                 m_SubmitFence;

        VkPerformanceConfigurationINTEL m_PerformanceConfigurationINTEL;

        ProfilerMetricsApi_INTEL m_MetricsApiINTEL;


        VkResult InitializeINTEL();

        ProfilerShaderTuple CreateShaderTuple( const VkGraphicsPipelineCreateInfo& );
        ProfilerShaderTuple CreateShaderTuple( const VkComputePipelineCreateInfo& );

        void SetDefaultPipelineObjectName( const DeviceProfilerPipeline& );

        void CreateInternalPipeline( DeviceProfilerPipelineType, const char* );

        decltype(m_CommandBuffers)::iterator FreeCommandBuffer( VkCommandBuffer );
        decltype(m_CommandBuffers)::iterator FreeCommandBuffer( decltype(m_CommandBuffers)::iterator );
    };
}

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
#include "profiler_commands.h"
#include "profiler_shader.h"
#include <assert.h>
#include <vector>
#include <list>
#include <new>
#include <memory>
#include <string>
#include <cstring>
#include <vulkan/vulkan.h>
// Import extension structures
#include "profiler_ext/VkProfilerEXT.h"

namespace Profiler
{
    template<typename T> using ContainerType = std::vector<T>;

    /***********************************************************************************\

    Structure:
        DeviceProfilerDrawcallStats

    Description:
        Stores number of drawcalls.

    \***********************************************************************************/
    struct DeviceProfilerDrawcallStats
    {
        uint32_t m_DrawCount = {};
        uint32_t m_DrawIndirectCount = {};
        uint32_t m_DispatchCount = {};
        uint32_t m_DispatchIndirectCount = {};
        uint32_t m_CopyBufferCount = {};
        uint32_t m_CopyBufferToImageCount = {};
        uint32_t m_CopyImageCount = {};
        uint32_t m_CopyImageToBufferCount = {};
        uint32_t m_ClearColorCount = {};
        uint32_t m_ClearDepthStencilCount = {};
        uint32_t m_ResolveCount = {};
        uint32_t m_BlitImageCount = {};
        uint32_t m_FillBufferCount = {};
        uint32_t m_UpdateBufferCount = {};
        uint32_t m_PipelineBarrierCount = {};

        // Stat aggregation helper
        inline DeviceProfilerDrawcallStats& operator+=( const DeviceProfilerDrawcallStats& rh )
        {
            m_DrawCount += rh.m_DrawCount;
            m_DrawIndirectCount += rh.m_DrawIndirectCount;
            m_DispatchCount += rh.m_DispatchCount;
            m_DispatchIndirectCount += rh.m_DispatchIndirectCount;
            m_CopyBufferCount += rh.m_CopyBufferCount;
            m_CopyBufferToImageCount += rh.m_CopyBufferToImageCount;
            m_CopyImageCount += rh.m_CopyImageCount;
            m_CopyImageToBufferCount += rh.m_CopyImageToBufferCount;
            m_ClearColorCount += rh.m_ClearColorCount;
            m_ClearDepthStencilCount += rh.m_ClearDepthStencilCount;
            m_ResolveCount += rh.m_ResolveCount;
            m_BlitImageCount += rh.m_BlitImageCount;
            m_FillBufferCount += rh.m_FillBufferCount;
            m_UpdateBufferCount += rh.m_UpdateBufferCount;
            m_PipelineBarrierCount += rh.m_PipelineBarrierCount;
            return *this;
        }
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerPipeline

    Description:
        Represents VkPipeline object.

    \***********************************************************************************/
    struct DeviceProfilerPipeline
    {
        VkPipeline                                          m_Handle = {};
        VkPipelineBindPoint                                 m_BindPoint = {};
        ProfilerShaderTuple                                 m_ShaderTuple = {};
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerSubpass

    Description:
        Contains captured GPU timestamp data for render pass subpass.

    \***********************************************************************************/
    struct DeviceProfilerSubpass
    {
        uint32_t                                            m_Index = {};
        uint32_t                                            m_ResolveCount = {};
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerRenderPass

    Description:
        Represents VkRenderPass object.

    \***********************************************************************************/
    struct DeviceProfilerRenderPass
    {
        VkRenderPass                                        m_Handle = {};
        std::vector<struct DeviceProfilerSubpass>           m_Subpasses = {};
        uint32_t                                            m_ClearColorAttachmentCount = {};
        uint32_t                                            m_ClearDepthStencilAttachmentCount = {};
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerCommandBufferData

    Description:
        Contains captured GPU timestamp data for single command buffer.

    \***********************************************************************************/
    struct CommandBufferData
    {
        VkCommandBuffer                                     m_Handle = {};
        VkCommandBufferLevel                                m_Level = {};
        VkCommandBufferUsageFlags                           m_Usage = {};
        DeviceProfilerDrawcallStats                         m_Stats = {};
        uint64_t                                            m_BeginTimestamp = {};
        uint64_t                                            m_EndTimestamp = {};

        //ContainerType<struct DeviceProfilerRenderPassData>  m_RenderPasses = {};
        std::shared_ptr<CommandGroup>                       m_pCommands = {};

        std::vector<char>                                   m_PerformanceQueryReportINTEL = {};

        uint64_t                                            m_ProfilerCpuOverheadNs = {};
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerSubmitData

    Description:
        Contains captured command buffers data for single submit.

    \***********************************************************************************/
    struct DeviceProfilerSubmitData
    {
        ContainerType<std::shared_ptr<const CommandBufferData>> m_pCommandBuffers = {};
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerSubmitBatchData

    Description:
        Stores data for whole vkQueueSubmit.

    \***********************************************************************************/
    struct DeviceProfilerSubmitBatchData
    {
        VkQueue                                             m_Handle = {};
        ContainerType<struct DeviceProfilerSubmitData>      m_Submits = {};
        uint64_t                                            m_Timestamp = {};
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerHeapMemoryData

    Description:

    \***********************************************************************************/
    struct DeviceProfilerMemoryHeapData
    {
        uint64_t m_AllocationSize = {};
        uint64_t m_AllocationCount = {};
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerHeapMemoryData

    Description:

    \***********************************************************************************/
    struct DeviceProfilerMemoryTypeData
    {
        uint64_t m_AllocationSize = {};
        uint64_t m_AllocationCount = {};
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerMemoryData

    Description:

    \***********************************************************************************/
    struct DeviceProfilerMemoryData
    {
        uint64_t m_TotalAllocationSize = {};
        uint64_t m_TotalAllocationCount = {};

        std::vector<struct DeviceProfilerMemoryHeapData> m_Heaps = {};
        std::vector<struct DeviceProfilerMemoryTypeData> m_Types = {};
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerCPUData

    Description:

    \***********************************************************************************/
    struct DeviceProfilerCPUData
    {
        uint64_t m_TimeNs = {};
        float    m_FramesPerSec = {};
    };

    /***********************************************************************************\
    \***********************************************************************************/
    struct AggregatedPipelineData
    {
        VkPipeline                                          m_Handle = {};
        VkPipelineBindPoint                                 m_BindPoint = {};
        uint64_t                                            m_Ticks = {};
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerFrameData

    Description:

    \***********************************************************************************/
    struct DeviceProfilerFrameData
    {
        ContainerType<struct DeviceProfilerSubmitBatchData> m_Submits = {};
        ContainerType<struct AggregatedPipelineData>        m_TopPipelines = {};

        DeviceProfilerDrawcallStats                         m_Stats = {};

        uint64_t                                            m_Timestamp = {};
        uint64_t                                            m_Ticks = {};

        DeviceProfilerMemoryData                            m_Memory = {};
        DeviceProfilerCPUData                               m_CPU = {};

        std::vector<VkProfilerPerformanceCounterResultEXT>  m_VendorMetrics = {};
    };
}

namespace std
{
    template<>
    struct hash<Profiler::DeviceProfilerPipeline>
    {
        inline size_t operator()( const Profiler::DeviceProfilerPipeline& pipeline ) const
        {
            return pipeline.m_ShaderTuple.m_Hash;
        }
    };
}

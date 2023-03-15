// Copyright (c) 2019-2021 Lukasz Stalmirski
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
#include <vulkan/vulkan.h>

#ifndef VK_EXT_profiler
#define VK_EXT_profiler 1
#define VK_EXT_PROFILER_SPEC_VERSION 1
#define VK_EXT_PROFILER_EXTENSION_NAME "VK_EXT_profiler"

enum VkProfilerResultEXT
{
};

enum VkProfilerStructureTypeEXT
{
    VK_STRUCTURE_TYPE_PROFILER_CREATE_INFO_EXT = 1000999000,
    VK_STRUCTURE_TYPE_PROFILER_DATA_EXT = 1000999001,
    VK_STRUCTURE_TYPE_PROFILER_REGION_DATA_EXT = 1000999002,
    VK_STRUCTURE_TYPE_PROFILER_RENDER_PASS_DATA_EXT = 1000999003,
    VK_PROFILER_STRUCTURE_TYPE_MAX_ENUM_EXT = 0x7FFFFFFF
};

enum VkProfilerCreateFlagBitsEXT
{
    VK_PROFILER_CREATE_NO_OVERLAY_BIT_EXT = 1,
    VK_PROFILER_CREATE_NO_PERFORMANCE_QUERY_EXTENSION_BIT_EXT = 2,
    VK_PROFILER_CREATE_RENDER_PASS_BEGIN_END_PROFILING_ENABLED_BIT_EXT = 4,
    VK_PROFILER_CREATE_NO_STABLE_POWER_STATE = 8,
    VK_PROFILER_CREATE_FLAG_BITS_MAX_ENUM_EXT = 0x7FFFFFFF
};

typedef VkFlags VkProfilerCreateFlagsEXT;

enum VkProfilerModeEXT
{
    VK_PROFILER_MODE_PER_DRAWCALL_EXT,
    VK_PROFILER_MODE_PER_PIPELINE_EXT,
    VK_PROFILER_MODE_PER_RENDER_PASS_EXT,
    VK_PROFILER_MODE_PER_COMMAND_BUFFER_EXT,
    VK_PROFILER_MODE_PER_SUBMIT_EXT,
    VK_PROFILER_MODE_PER_FRAME_EXT,
    VK_PROFILER_MODE_MAX_ENUM_EXT = 0x7FFFFFFF
};

enum VkProfilerRegionTypeEXT
{
    VK_PROFILER_REGION_TYPE_FRAME_EXT,
    VK_PROFILER_REGION_TYPE_SUBMIT_EXT,
    VK_PROFILER_REGION_TYPE_SUBMIT_INFO_EXT,
    VK_PROFILER_REGION_TYPE_COMMAND_BUFFER_EXT,
    VK_PROFILER_REGION_TYPE_RENDER_PASS_EXT,
    VK_PROFILER_REGION_TYPE_SUBPASS_EXT,
    VK_PROFILER_REGION_TYPE_PIPELINE_EXT,
    VK_PROFILER_REGION_TYPE_COMMAND_EXT,
    VK_PROFILER_REGION_TYPE_MAX_ENUM_EXT = 0x7FFFFFFF
};

enum VkProfilerCommandTypeEXT
{
    VK_PROFILER_COMMAND_UNKNOWN_EXT,
    VK_PROFILER_COMMAND_DRAW_EXT,
    VK_PROFILER_COMMAND_DRAW_INDEXED_EXT,
    VK_PROFILER_COMMAND_DRAW_INDIRECT_EXT,
    VK_PROFILER_COMMAND_DRAW_INDEXED_INDIRECT_EXT,
    VK_PROFILER_COMMAND_DRAW_INDIRECT_COUNT_EXT,
    VK_PROFILER_COMMAND_DRAW_INDEXED_INDIRECT_COUNT_EXT,
    VK_PROFILER_COMMAND_DISPATCH_EXT,
    VK_PROFILER_COMMAND_DISPATCH_INDIRECT_EXT,
    VK_PROFILER_COMMAND_COPY_BUFFER_EXT,
    VK_PROFILER_COMMAND_COPY_BUFFER_TO_IMAGE_EXT,
    VK_PROFILER_COMMAND_COPY_IMAGE_EXT,
    VK_PROFILER_COMMAND_COPY_IMAGE_TO_BUFFER_EXT,
    VK_PROFILER_COMMAND_CLEAR_ATTACHMENTS_EXT,
    VK_PROFILER_COMMAND_CLEAR_COLOR_IMAGE_EXT,
    VK_PROFILER_COMMAND_CLEAR_DEPTH_STENCIL_IMAGE_EXT,
    VK_PROFILER_COMMAND_RESOLVE_IMAGE_EXT,
    VK_PROFILER_COMMAND_BLIT_IMAGE_EXT,
    VK_PROFILER_COMMAND_FILL_BUFFER_EXT,
    VK_PROFILER_COMMAND_UPDATE_BUFFER_EXT,
    VK_PROFILER_COMMAND_MAX_ENUM_EXT = 0x7FFFFFFF
};

enum VkProfilerSyncModeEXT
{
    VK_PROFILER_SYNC_MODE_PRESENT_EXT,
    VK_PROFILER_SYNC_MODE_SUBMIT_EXT,
    VK_PROFILER_SYNC_MODE_MAX_ENUM_EXT = 0x7FFFFFFF
};

enum VkProfilerPerformanceCounterUnitEXT
{
    VK_PROFILER_PERFORMANCE_COUNTER_UNIT_GENERIC_EXT,
    VK_PROFILER_PERFORMANCE_COUNTER_UNIT_PERCENTAGE_EXT,
    VK_PROFILER_PERFORMANCE_COUNTER_UNIT_NANOSECONDS_EXT,
    VK_PROFILER_PERFORMANCE_COUNTER_UNIT_BYTES_EXT,
    VK_PROFILER_PERFORMANCE_COUNTER_UNIT_BYTES_PER_SECOND_EXT,
    VK_PROFILER_PERFORMANCE_COUNTER_UNIT_KELVIN_EXT,
    VK_PROFILER_PERFORMANCE_COUNTER_UNIT_WATTS_EXT,
    VK_PROFILER_PERFORMANCE_COUNTER_UNIT_VOLTS_EXT,
    VK_PROFILER_PERFORMANCE_COUNTER_UNIT_AMPS_EXT,
    VK_PROFILER_PERFORMANCE_COUNTER_UNIT_HERTZ_EXT,
    VK_PROFILER_PERFORMANCE_COUNTER_UNIT_CYCLES_EXT,
    VK_PROFILER_PERFORMANCE_COUNTER_UNIT_MAX_ENUM_EXT = 0x7FFFFFFF
};

enum VkProfilerPerformanceCounterStorageEXT
{
    VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_INT32_EXT,
    VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_INT64_EXT,
    VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_UINT32_EXT,
    VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_UINT64_EXT,
    VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_FLOAT32_EXT,
    VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_FLOAT64_EXT,
    VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_MAX_ENUM_EXT = 0x7FFFFFFF
};

typedef struct VkProfilerCreateInfoEXT
{
    VkProfilerStructureTypeEXT sType;
    const void* pNext;
    VkProfilerCreateFlagsEXT flags;
    VkProfilerModeEXT samplingMode;
    VkProfilerSyncModeEXT syncMode;
} VkProfilerCreateInfoEXT;

typedef struct VkProfilerCommandPropertiesEXT
{
    VkProfilerCommandTypeEXT type;
} VkProfilerCommandPropertiesEXT;

typedef struct VkProfilerPipelinePropertiesEXT
{
    VkPipeline handle;
} VkProfilerPipelinePropertiesEXT;

typedef struct VkProfilerSubpassPropertiesEXT
{
    uint32_t index;
    VkSubpassContents contents;
} VkProfilerSubpassPropertiesEXT;

typedef struct VkProfilerRenderPassPropertiesEXT
{
    VkRenderPass handle;
} VkProfilerRenderPassPropertiesEXT;

typedef struct VkProfilerCommandBufferPropertiesEXT
{
    VkCommandBuffer handle;
    VkCommandBufferLevel level;
} VkProfilerCommandBufferPropertiesEXT;

typedef struct VkProfilerSubmitInfoPropertiesEXT
{
} VkProfilerSubmitInfoPropertiesEXT;

typedef struct VkProfilerSubmitPropertiesEXT
{
    VkQueue queue;
} VkProfilerSubmitPropertiesEXT;

typedef struct VkProfilerFramePropertiesEXT
{
} VkProfilerFramePropertiesEXT;

typedef union VkProfilerRegionPropertiesEXT
{
    VkProfilerCommandPropertiesEXT command;
    VkProfilerPipelinePropertiesEXT pipeline;
    VkProfilerSubpassPropertiesEXT subpass;
    VkProfilerRenderPassPropertiesEXT renderPass;
    VkProfilerCommandBufferPropertiesEXT commandBuffer;
    VkProfilerSubmitInfoPropertiesEXT submitInfo;
    VkProfilerSubmitPropertiesEXT submit;
    VkProfilerFramePropertiesEXT frame;
} VkProfilerRegionPropertiesEXT;

typedef struct VkProfilerRegionDataEXT
{
    VkProfilerStructureTypeEXT sType;
    void* pNext;
    VkProfilerRegionTypeEXT regionType;
    VkProfilerRegionPropertiesEXT properties;
    float duration;
    uint32_t subregionCount;
    struct VkProfilerRegionDataEXT* pSubregions;
} VkProfilerRegionDataEXT;

typedef struct VkProfilerRenderPassDataEXT
{
    VkProfilerStructureTypeEXT sType;
    void* pNext;
    float beginDuration;
    float endDuration;
} VkProfilerRenderPassDataEXT;

typedef struct VkProfilerDataEXT
{
    VkProfilerStructureTypeEXT sType;
    void* pNext;
    VkProfilerRegionDataEXT frame;
} VkProfilerDataEXT;

typedef struct VkProfilerPerformanceCounterPropertiesEXT
{
    char shortName[ 64 ];
    char description[ 256 ];
    VkProfilerPerformanceCounterUnitEXT unit;
    VkProfilerPerformanceCounterStorageEXT storage;
} VkProfilerPerformanceCounterPropertiesEXT;

typedef union VkProfilerPerformanceCounterResultEXT
{
    int32_t     int32;
    int64_t     int64;
    uint32_t    uint32;
    uint64_t    uint64;
    float       float32;
    double      float64;
} VkProfilerPerformanceCounterResultEXT;

typedef struct VkProfilerPerformanceMetricsSetPropertiesEXT
{
    char     name[ 64 ];
    uint32_t metricsCount;
} VkProfilerPerformanceMetricsSetPropertiesEXT;

typedef VKAPI_ATTR VkResult( VKAPI_CALL* PFN_vkSetProfilerModeEXT )(VkDevice, VkProfilerModeEXT);
typedef VKAPI_ATTR VkResult( VKAPI_CALL* PFN_vkSetProfilerSyncModeEXT )(VkDevice, VkProfilerSyncModeEXT);
typedef VKAPI_ATTR VkResult( VKAPI_CALL* PFN_vkGetProfilerFrameDataEXT )(VkDevice, VkProfilerDataEXT*);
typedef VKAPI_ATTR void( VKAPI_CALL* PFN_vkFreeProfilerFrameDataEXT )(VkDevice, VkProfilerDataEXT*);
typedef VKAPI_ATTR VkResult( VKAPI_CALL* PFN_vkFlushProfilerEXT )(VkDevice);
typedef VKAPI_ATTR VkResult( VKAPI_CALL* PFN_vkEnumerateProfilerPerformanceMetricsSetsEXT )(VkDevice, uint32_t*, VkProfilerPerformanceMetricsSetPropertiesEXT*);
typedef VKAPI_ATTR VkResult( VKAPI_CALL* PFN_vkEnumerateProfilerPerformanceCounterPropertiesEXT )(VkDevice, uint32_t, uint32_t*, VkProfilerPerformanceCounterPropertiesEXT*);
typedef VKAPI_ATTR VkResult( VKAPI_CALL* PFN_vkSetProfilerPerformanceMetricsSetEXT )(VkDevice, uint32_t);
typedef VKAPI_ATTR void( VKAPI_CALL* PFN_vkGetProfilerActivePerformanceMetricsSetIndexEXT )(VkDevice, uint32_t*);

#ifndef VK_NO_PROTOTYPES
VKAPI_ATTR VkResult VKAPI_CALL vkSetProfilerModeEXT(
    VkDevice device,
    VkProfilerModeEXT mode );

VKAPI_ATTR VkResult VKAPI_CALL vkSetProfilerSyncModeEXT(
    VkDevice device,
    VkProfilerSyncModeEXT syncMode );

VKAPI_ATTR VkResult VKAPI_CALL vkGetProfilerFrameDataEXT(
    VkDevice device,
    VkProfilerDataEXT* pData );

VKAPI_ATTR void VKAPI_CALL vkFreeProfilerFrameDataEXT(
    VkDevice device,
    VkProfilerDataEXT* pData );

VKAPI_ATTR VkResult VKAPI_CALL vkFlushProfilerEXT(
    VkDevice device );

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateProfilerPerformanceMetricsSetsEXT(
    VkDevice device,
    uint32_t* pProfilerMetricSetCount,
    VkProfilerPerformanceMetricsSetPropertiesEXT* pProfilerMetricsSetNameInfos );

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateProfilerPerformanceCounterPropertiesEXT(
    VkDevice device,
    uint32_t metricsSetIndex,
    uint32_t* pProfilerMetricCount,
    VkProfilerPerformanceCounterPropertiesEXT* pProfilerMetricProperties );

VKAPI_ATTR VkResult VKAPI_CALL vkSetProfilerPerformanceMetricsSetEXT(
    VkDevice device,
    uint32_t metricsSetIndex );

VKAPI_ATTR void VKAPI_CALL vkGetProfilerActivePerformanceMetricsSetIndexEXT(
    VkDevice device,
    uint32_t* pMetricsSetIndex );
#endif // VK_NO_PROTOTYPES
#endif // VK_EXT_profiler

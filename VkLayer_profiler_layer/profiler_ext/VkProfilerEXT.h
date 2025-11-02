// Copyright (c) 2019-2025 Lukasz Stalmirski
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

#ifdef __cplusplus
extern "C" {
#endif

#ifndef VK_EXT_profiler
#define VK_EXT_profiler 1
#define VK_EXT_PROFILER_SPEC_VERSION 5
#define VK_EXT_PROFILER_EXTENSION_NAME "VK_EXT_profiler"

#define VK_STRUCTURE_TYPE_PROFILER_CREATE_INFO_EXT ((VkStructureType)1000999000)
#define VK_STRUCTURE_TYPE_PROFILER_DATA_EXT ((VkStructureType)1000999001)
#define VK_STRUCTURE_TYPE_PROFILER_REGION_DATA_EXT ((VkStructureType)1000999002)
#define VK_STRUCTURE_TYPE_PROFILER_RENDER_PASS_DATA_EXT ((VkStructureType)1000999003)
#define VK_STRUCTURE_TYPE_PROFILER_PERFORMANCE_COUNTER_PROPERTIES_2_EXT ((VkStructureType)1000999004)
#define VK_STRUCTURE_TYPE_PROFILER_PERFORMANCE_METRICS_SET_PROPERTIES_2_EXT ((VkStructureType)1000999005)
#define VK_STRUCTURE_TYPE_PROFILER_CUSTOM_PERFORMANCE_METRICS_SET_CREATE_INFO_EXT ((VkStructureType)1000999006)
#define VK_STRUCTURE_TYPE_PROFILER_CUSTOM_PERFORMANCE_METRICS_SET_UPDATE_INFO_EXT ((VkStructureType)1000999007)

typedef enum VkProfilerCreateFlagBitsEXT
{
    VK_PROFILER_CREATE_NO_OVERLAY_BIT_EXT = 1,
    VK_PROFILER_CREATE_NO_PERFORMANCE_QUERY_EXTENSION_BIT_EXT = 2,
    VK_PROFILER_CREATE_RENDER_PASS_BEGIN_END_PROFILING_ENABLED_BIT_EXT = 4,
    VK_PROFILER_CREATE_NO_STABLE_POWER_STATE_BIT_EXT = 8,
    VK_PROFILER_CREATE_NO_THREADING_BIT_EXT = 16,
    VK_PROFILER_CREATE_NO_MEMORY_PROFILING_BIT_EXT = 32,
    VK_PROFILER_CREATE_FLAG_BITS_MAX_ENUM_EXT = 0x7FFFFFFF
} VkProfilerCreateFlagBitsEXT;
typedef VkFlags VkProfilerCreateFlagsEXT;

typedef enum VkProfilerModeEXT
{
    VK_PROFILER_MODE_PER_DRAWCALL_EXT,
    VK_PROFILER_MODE_PER_PIPELINE_EXT,
    VK_PROFILER_MODE_PER_RENDER_PASS_EXT,
    VK_PROFILER_MODE_PER_COMMAND_BUFFER_EXT,
    VK_PROFILER_MODE_PER_SUBMIT_EXT,
    VK_PROFILER_MODE_PER_FRAME_EXT,
    VK_PROFILER_MODE_MAX_ENUM_EXT = 0x7FFFFFFF
} VkProfilerModeEXT;

typedef enum VkProfilerRegionTypeEXT
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
} VkProfilerRegionTypeEXT;

typedef enum VkProfilerCommandTypeEXT
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
    VK_PROFILER_COMMAND_DRAW_MESH_TASKS_EXT,
    VK_PROFILER_COMMAND_DRAW_MESH_TASKS_INDIRECT_EXT,
    VK_PROFILER_COMMAND_DRAW_MESH_TASKS_INDIRECT_COUNT_EXT,
    VK_PROFILER_COMMAND_DRAW_MESH_TASKS_NV_EXT,
    VK_PROFILER_COMMAND_DRAW_MESH_TASKS_INDIRECT_NV_EXT,
    VK_PROFILER_COMMAND_DRAW_MESH_TASKS_INDIRECT_COUNT_NV_EXT,
    VK_PROFILER_COMMAND_TRACE_RAYS_EXT,
    VK_PROFILER_COMMAND_TRACE_RAYS_INDIRECT_EXT,
    VK_PROFILER_COMMAND_TRACE_RAYS_INDIRECT2_EXT,
    VK_PROFILER_COMMAND_BUILD_ACCELERATION_STRUCTURES_EXT,
    VK_PROFILER_COMMAND_BUILD_ACCELERATION_STRUCTURES_INDIRECT_EXT,
    VK_PROFILER_COMMAND_COPY_ACCELERATION_STRUCTURE_EXT,
    VK_PROFILER_COMMAND_COPY_ACCELERATION_STRUCTURE_TO_MEMORY_EXT,
    VK_PROFILER_COMMAND_COPY_MEMORY_TO_ACCELERATION_STRUCTURE_EXT,
    VK_PROFILER_COMMAND_BUILD_MICROMAP_EXT,
    VK_PROFILER_COMMAND_COPY_MICROMAP_EXT,
    VK_PROFILER_COMMAND_COPY_MICROMAP_TO_MEMORY_EXT,
    VK_PROFILER_COMMAND_COPY_MEMORY_TO_MICROMAP_EXT,
    VK_PROFILER_COMMAND_MAX_ENUM_EXT = 0x7FFFFFFF
} VkProfilerCommandTypeEXT;

typedef enum VkProfilerFrameDelimiterEXT
{
    VK_PROFILER_FRAME_DELIMITER_PRESENT_EXT,
    VK_PROFILER_FRAME_DELIMITER_SUBMIT_EXT,
    VK_PROFILER_FRAME_DELIMITER_MAX_ENUM_EXT = 0x7FFFFFFF
} VkProfilerFrameDelimiterEXT;

typedef enum VkProfilerPerformanceCounterFlagBitsEXT
{
    VK_PROFILER_PERFORMANCE_COUNTER_PERFORMANCE_IMPACTING_BIT_EXT = 1,
    VK_PROFILER_PERFORMANCE_COUNTER_CONCURRENTLY_IMPACTED_BIT_EXT = 2,
    VK_PROFILER_PERFORMANCE_COUNTER_SCOPE_MAX_ENUM_EXT = 0x7FFFFFFF
} VkProfilerPerformanceCounterFlagBitsEXT;
typedef VkFlags VkProfilerPerformanceCounterFlagsEXT;

typedef enum VkProfilerPerformanceCounterUnitEXT
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
} VkProfilerPerformanceCounterUnitEXT;

typedef enum VkProfilerPerformanceCounterStorageEXT
{
    VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_INT32_EXT,
    VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_INT64_EXT,
    VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_UINT32_EXT,
    VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_UINT64_EXT,
    VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_FLOAT32_EXT,
    VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_FLOAT64_EXT,
    VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_MAX_ENUM_EXT = 0x7FFFFFFF
} VkProfilerPerformanceCounterStorageEXT;

typedef struct VkProfilerCreateInfoEXT
{
    VkStructureType sType;
    const void* pNext;
    VkProfilerCreateFlagsEXT flags;
    VkProfilerModeEXT samplingMode;
    VkProfilerFrameDelimiterEXT frameDelimiter;
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
    VkStructureType sType;
    void* pNext;
    VkProfilerRegionTypeEXT regionType;
    VkProfilerRegionPropertiesEXT properties;
    float duration;
    uint32_t subregionCount;
    struct VkProfilerRegionDataEXT* pSubregions;
} VkProfilerRegionDataEXT;

typedef struct VkProfilerRenderPassDataEXT
{
    VkStructureType sType;
    void* pNext;
    float beginDuration;
    float endDuration;
} VkProfilerRenderPassDataEXT;

typedef struct VkProfilerDataEXT
{
    VkStructureType sType;
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

typedef struct VkProfilerPerformanceCounterProperties2EXT
{
    VkStructureType sType;
    void* pNext;
    VkProfilerPerformanceCounterFlagsEXT flags;
    VkProfilerPerformanceCounterUnitEXT unit;
    VkProfilerPerformanceCounterStorageEXT storage;
    uint8_t uuid[ VK_UUID_SIZE ];
    char shortName[ VK_MAX_DESCRIPTION_SIZE ];
    char category[ VK_MAX_DESCRIPTION_SIZE ];
    char description[ VK_MAX_DESCRIPTION_SIZE ];
} VkProfilerPerformanceCounterProperties2EXT;

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

typedef struct VkProfilerPerformanceMetricsSetProperties2EXT
{
    VkStructureType sType;
    void* pNext;
    uint32_t metricsCount;
    char name[ VK_MAX_DESCRIPTION_SIZE ];
    char description[ VK_MAX_DESCRIPTION_SIZE ];
} VkProfilerPerformanceMetricsSetProperties2EXT;

typedef struct VkProfilerCustomPerformanceMetricsSetCreateInfoEXT
{
    VkStructureType sType;
    const void* pNext;
    uint32_t metricsCount;
    const uint32_t* pMetricsIndices;
    const char* pName;
    const char* pDescription;
} VkProfilerCustomPerformanceMetricsSetCreateInfoEXT;

typedef struct VkProfilerCustomPerformanceMetricsSetUpdateInfoEXT
{
    VkStructureType sType;
    const void* pNext;
    uint32_t metricsSetIndex;
    const char* pName;
    const char* pDescription;
} VkProfilerCustomPerformanceMetricsSetUpdateInfoEXT;

typedef VkResult( VKAPI_PTR* PFN_vkSetProfilerSamplingModeEXT )(VkDevice, VkProfilerModeEXT);
typedef void( VKAPI_PTR* PFN_vkGetProfilerSamplingModeEXT )(VkDevice, VkProfilerModeEXT*);
typedef VkResult( VKAPI_PTR* PFN_vkSetProfilerFrameDelimiterEXT )(VkDevice, VkProfilerFrameDelimiterEXT);
typedef void( VKAPI_PTR* PFN_vkGetProfilerFrameDelimiterEXT )(VkDevice, VkProfilerFrameDelimiterEXT*);
typedef VkResult( VKAPI_PTR* PFN_vkGetProfilerFrameDataEXT )(VkDevice, VkProfilerDataEXT*);
typedef void( VKAPI_PTR* PFN_vkFreeProfilerFrameDataEXT )(VkDevice, VkProfilerDataEXT*);
typedef VkResult( VKAPI_PTR* PFN_vkFlushProfilerEXT )(VkDevice);
typedef void( VKAPI_PTR* PFN_vkGetProfilerCustomPerfomanceMetricsSetsSupportEXT )( VkDevice, VkBool32* );
typedef VkResult( VKAPI_PTR* PFN_vkCreateProfilerCustomPerformanceMetricsSetEXT )( VkDevice, const VkProfilerCustomPerformanceMetricsSetCreateInfoEXT*, const VkAllocationCallbacks*, uint32_t* );
typedef void( VKAPI_PTR* PFN_vkDestroyProfilerCustomPerformanceMetricsSetEXT )( VkDevice, uint32_t, const VkAllocationCallbacks* );
typedef void( VKAPI_PTR* PFN_vkUpdateProfilerCustomPerformanceMetricsSetsEXT )( VkDevice, uint32_t, const VkProfilerCustomPerformanceMetricsSetUpdateInfoEXT* );
typedef VkResult( VKAPI_PTR* PFN_vkEnumerateProfilerPerformanceMetricsEXT )( VkDevice, uint32_t*, VkProfilerPerformanceCounterProperties2EXT* );
typedef VkResult( VKAPI_PTR* PFN_vkEnumerateProfilerPerformanceMetricsSets2EXT )( VkDevice, uint32_t*, VkProfilerPerformanceMetricsSetProperties2EXT* );
typedef VkResult( VKAPI_PTR* PFN_vkEnumerateProfilerPerformanceMetricsSetMetricsEXT )( VkDevice, uint32_t, uint32_t*, VkProfilerPerformanceCounterProperties2EXT* );
typedef VkResult( VKAPI_PTR* PFN_vkGetProfilerPerformanceMetricsRequiredPassesEXT )( VkDevice, uint32_t, uint32_t*, uint32_t* );
typedef VkResult( VKAPI_PTR* PFN_vkEnumerateProfilerPerformanceMetricsSetsEXT )(VkDevice, uint32_t*, VkProfilerPerformanceMetricsSetPropertiesEXT*);
typedef VkResult( VKAPI_PTR* PFN_vkEnumerateProfilerPerformanceCounterPropertiesEXT )(VkDevice, uint32_t, uint32_t*, VkProfilerPerformanceCounterPropertiesEXT*);
typedef VkResult( VKAPI_PTR* PFN_vkSetProfilerPerformanceMetricsSetEXT )(VkDevice, uint32_t);
typedef void( VKAPI_PTR* PFN_vkGetProfilerActivePerformanceMetricsSetIndexEXT )(VkDevice, uint32_t*);

#ifndef VK_NO_PROTOTYPES
VKAPI_ATTR VkResult VKAPI_CALL vkSetProfilerSamplingModeEXT(
    VkDevice device,
    VkProfilerModeEXT mode );

VKAPI_ATTR void VKAPI_CALL vkGetProfilerSamplingModeEXT(
    VkDevice device,
    VkProfilerModeEXT* pMode );

VKAPI_ATTR VkResult VKAPI_CALL vkSetProfilerFrameDelimiterEXT(
    VkDevice device,
    VkProfilerFrameDelimiterEXT frameDelimiter );

VKAPI_ATTR void VKAPI_CALL vkGetProfilerFrameDelimiterEXT(
    VkDevice device,
    VkProfilerFrameDelimiterEXT* pFrameDelimiter );

VKAPI_ATTR VkResult VKAPI_CALL vkGetProfilerFrameDataEXT(
    VkDevice device,
    VkProfilerDataEXT* pData );

VKAPI_ATTR void VKAPI_CALL vkFreeProfilerFrameDataEXT(
    VkDevice device,
    VkProfilerDataEXT* pData );

VKAPI_ATTR VkResult VKAPI_CALL vkFlushProfilerEXT(
    VkDevice device );

VKAPI_ATTR void VKAPI_CALL vkGetProfilerCustomPerfomanceMetricsSetsSupportEXT(
    VkDevice device,
    VkBool32* pSupported );

VKAPI_ATTR VkResult VKAPI_CALL vkCreateProfilerCustomPerformanceMetricsSetEXT(
    VkDevice device,
    const VkProfilerCustomPerformanceMetricsSetCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    uint32_t* pMetricsSetIndex );

VKAPI_ATTR void VKAPI_CALL vkDestroyProfilerCustomPerformanceMetricsSetEXT(
    VkDevice device,
    uint32_t metricsSetIndex,
    const VkAllocationCallbacks* pAllocator );

VKAPI_ATTR void VKAPI_CALL vkUpdateProfilerCustomPerformanceMetricsSetsEXT(
    VkDevice device,
    uint32_t updateInfosCount,
    const VkProfilerCustomPerformanceMetricsSetUpdateInfoEXT* pUpdateInfos );

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateProfilerPerformanceMetricsEXT(
    VkDevice device,
    uint32_t* pProfilerMetricCount,
    VkProfilerPerformanceCounterProperties2EXT* pProfilerMetricProperties );

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateProfilerPerformanceMetricsSets2EXT(
    VkDevice device,
    uint32_t* pProfilerMetricSetCount,
    VkProfilerPerformanceMetricsSetProperties2EXT* pProfilerMetricSetProperties );

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateProfilerPerformanceMetricsSetMetricsEXT(
    VkDevice device,
    uint32_t metricsSetIndex,
    uint32_t* pProfilerMetricCount,
    VkProfilerPerformanceCounterProperties2EXT* pProfilerMetricProperties );

VKAPI_ATTR VkResult VKAPI_CALL vkGetProfilerPerformanceMetricsRequiredPassesEXT(
    VkDevice device,
    uint32_t metricsCount,
    uint32_t* pMetricsIndices,
    uint32_t* pRequiredPassCount );

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateProfilerPerformanceMetricsSetsEXT(
    VkDevice device,
    uint32_t* pProfilerMetricSetCount,
    VkProfilerPerformanceMetricsSetPropertiesEXT* pProfilerMetricSetProperties );

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

#ifdef __cplusplus
} // extern "C"
#endif

#pragma once
#include <vulkan/vulkan.h>

#ifndef VK_EXT_profiler
#define VK_EXT_profiler 1
#define VK_EXT_PROFILER_SPEC_VERSION 1
#define VK_EXT_PROFILER_EXTENSION_NAME "VK_EXT_profiler"

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
    VK_PROFILER_CREATE_DISABLE_OVERLAY_BIT_EXT = 1,
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
    VK_PROFILER_PERFORMANCE_COUNTER_UNIT_GENERIC_EXT = VK_PERFORMANCE_COUNTER_UNIT_GENERIC_KHR,
    VK_PROFILER_PERFORMANCE_COUNTER_UNIT_PERCENTAGE_EXT = VK_PERFORMANCE_COUNTER_UNIT_PERCENTAGE_KHR,
    VK_PROFILER_PERFORMANCE_COUNTER_UNIT_NANOSECONDS_EXT = VK_PERFORMANCE_COUNTER_UNIT_NANOSECONDS_KHR,
    VK_PROFILER_PERFORMANCE_COUNTER_UNIT_BYTES_EXT = VK_PERFORMANCE_COUNTER_UNIT_BYTES_KHR,
    VK_PROFILER_PERFORMANCE_COUNTER_UNIT_BYTES_PER_SECOND_EXT = VK_PERFORMANCE_COUNTER_UNIT_BYTES_PER_SECOND_KHR,
    VK_PROFILER_PERFORMANCE_COUNTER_UNIT_KELVIN_EXT = VK_PERFORMANCE_COUNTER_UNIT_KELVIN_KHR,
    VK_PROFILER_PERFORMANCE_COUNTER_UNIT_WATTS_EXT = VK_PERFORMANCE_COUNTER_UNIT_WATTS_KHR,
    VK_PROFILER_PERFORMANCE_COUNTER_UNIT_VOLTS_EXT = VK_PERFORMANCE_COUNTER_UNIT_VOLTS_KHR,
    VK_PROFILER_PERFORMANCE_COUNTER_UNIT_AMPS_EXT = VK_PERFORMANCE_COUNTER_UNIT_AMPS_KHR,
    VK_PROFILER_PERFORMANCE_COUNTER_UNIT_HERTZ_EXT = VK_PERFORMANCE_COUNTER_UNIT_HERTZ_KHR,
    VK_PROFILER_PERFORMANCE_COUNTER_UNIT_CYCLES_EXT = VK_PERFORMANCE_COUNTER_UNIT_CYCLES_KHR,
    VK_PROFILER_PERFORMANCE_COUNTER_UNIT_MAX_ENUM_EXT = VK_PERFORMANCE_COUNTER_UNIT_MAX_ENUM_KHR
};

enum VkProfilerPerformanceCounterStorageEXT
{
    VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_INT32_EXT = VK_PERFORMANCE_COUNTER_STORAGE_INT32_KHR,
    VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_INT64_EXT = VK_PERFORMANCE_COUNTER_STORAGE_INT64_KHR,
    VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_UINT32_EXT = VK_PERFORMANCE_COUNTER_STORAGE_UINT32_KHR,
    VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_UINT64_EXT = VK_PERFORMANCE_COUNTER_STORAGE_UINT64_KHR,
    VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_FLOAT32_EXT = VK_PERFORMANCE_COUNTER_STORAGE_FLOAT32_KHR,
    VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_FLOAT64_EXT = VK_PERFORMANCE_COUNTER_STORAGE_FLOAT64_KHR,
    VK_PROFILER_PERFORMANCE_COUNTER_STORAGE_MAX_ENUM_EXT = VK_PERFORMANCE_COUNTER_STORAGE_MAX_ENUM_KHR
};

typedef struct VkProfilerCreateInfoEXT
{
    VkProfilerStructureTypeEXT sType;
    const void* pNext;
    VkProfilerCreateFlagsEXT flags;
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

typedef VkPerformanceCounterResultKHR VkProfilerPerformanceCounterResultEXT;

typedef VKAPI_ATTR VkResult( VKAPI_CALL* PFN_vkSetProfilerModeEXT )(VkDevice, VkProfilerModeEXT);
typedef VKAPI_ATTR VkResult( VKAPI_CALL* PFN_vkSetProfilerSyncModeEXT )(VkDevice, VkProfilerSyncModeEXT);
typedef VKAPI_ATTR VkResult( VKAPI_CALL* PFN_vkGetProfilerFrameDataEXT )(VkDevice, VkProfilerDataEXT*);
typedef VKAPI_ATTR void( VKAPI_CALL* PFN_vkFreeProfilerFrameDataEXT )(VkDevice, VkProfilerDataEXT*);
typedef VKAPI_ATTR VkResult( VKAPI_CALL* PFN_vkEnumerateProfilerMetricPropertiesEXT )(VkDevice, uint32_t*, VkProfilerPerformanceCounterPropertiesEXT*);
typedef VKAPI_ATTR VkResult( VKAPI_CALL* PFN_vkFlushProfilerEXT )(VkDevice);

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

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateProfilerPerformanceCounterPropertiesEXT(
    VkDevice device,
    uint32_t* pProfilerMetricCount,
    VkProfilerPerformanceCounterPropertiesEXT* pProfilerMetricProperties );

VKAPI_ATTR VkResult VKAPI_CALL vkFlushProfilerEXT(
    VkDevice device );
#endif // VK_NO_PROTOTYPES
#endif // VK_EXT_profiler

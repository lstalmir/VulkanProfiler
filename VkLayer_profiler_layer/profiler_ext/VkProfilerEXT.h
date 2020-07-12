#pragma once
#include <vulkan/vulkan.h>

#define VK_EXT_PROFILER_SPEC_VERSION 1
#define VK_EXT_PROFILER_EXTENSION_NAME "VK_EXT_profiler"

enum VkProfilerStructureTypeEXT
{
    VK_STRUCTURE_TYPE_PROFILER_CREATE_INFO_EXT = 1000999000,
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
    VK_PROFILER_REGION_TYPE_COMMAND_BUFFER_EXT,
    VK_PROFILER_REGION_TYPE_DEBUG_MARKER_EXT,
    VK_PROFILER_REGION_TYPE_RENDER_PASS_EXT,
    VK_PROFILER_REGION_TYPE_PIPELINE_EXT,
    VK_PROFILER_REGION_TYPE_MAX_ENUM_EXT = 0x7FFFFFFF
};

enum VkProfilerSyncModeEXT
{
    VK_PROFILER_SYNC_MODE_PRESENT_EXT,
    VK_PROFILER_SYNC_MODE_SUBMIT_EXT,
    VK_PROFILER_SYNC_MODE_MAX_ENUM_EXT = 0x7FFFFFFF
};

enum VkProfilerMetricTypeEXT
{
    VK_PROFILER_METRIC_TYPE_FLOAT_EXT,
    VK_PROFILER_METRIC_TYPE_UINT32_EXT,
    VK_PROFILER_METRIC_TYPE_UINT64_EXT,
    VK_PROFILER_METRIC_TYPE_BOOL_EXT,
    VK_PROFILER_METRIC_TYPE_MAX_ENUM_EXT = 0x7FFFFFFF
};

typedef struct VkProfilerCreateInfoEXT
{
    VkProfilerStructureTypeEXT sType;
    const void* pNext;
    VkProfilerCreateFlagsEXT flags;
} VkProfilerCreateInfoEXT;

typedef struct VkProfilerRegionDataEXT
{
    VkProfilerRegionTypeEXT regionType;
    char regionName[ 256 ];
    float duration;
} VkProfilerRegionDataEXT;

typedef struct VkProfilerMemoryDataEXT
{
    uint64_t deviceLocalMemoryAllocated;
} VkProfilerMemoryDataEXT;

typedef struct VkProfilerDataEXT
{
    VkProfilerRegionDataEXT frame;
    VkProfilerMemoryDataEXT memory;
} VkProfilerDataEXT;

typedef struct VkProfilerMetricPropertiesEXT
{
    char shortName[ 64 ];
    char description[ 256 ];
    char unit[ 32 ];
    VkProfilerMetricTypeEXT type;
} VkProfilerMetricPropertiesEXT;

typedef union VkProfilerMetricEXT
{
    float floatValue;
    uint32_t uint32Value;
    uint64_t uint64Value;
    VkBool32 boolValue;
} VkProfilerTypedMetricEXT;

typedef VKAPI_ATTR VkResult( VKAPI_CALL* PFN_vkSetProfilerModeEXT )(VkDevice, VkProfilerModeEXT);
typedef VKAPI_ATTR VkResult( VKAPI_CALL* PFN_vkSetProfilerSyncModeEXT )(VkDevice, VkProfilerSyncModeEXT);
typedef VKAPI_ATTR VkResult( VKAPI_CALL* PFN_vkGetProfilerFrameDataEXT )(VkDevice, VkProfilerRegionDataEXT*);
typedef VKAPI_ATTR VkResult( VKAPI_CALL* PFN_vkGetProfilerCommandBufferDataEXT )(VkDevice, VkCommandBuffer, VkProfilerRegionDataEXT*);
typedef VKAPI_ATTR VkResult( VKAPI_CALL* PFN_vkEnumerateProfilerMetricPropertiesEXT )(VkDevice, uint32_t*, VkProfilerMetricPropertiesEXT*);

#ifndef VK_NO_PROTOTYPES
VKAPI_ATTR VkResult VKAPI_CALL vkSetProfilerModeEXT(
    VkDevice device,
    VkProfilerModeEXT mode );

VKAPI_ATTR VkResult VKAPI_CALL vkSetProfilerSyncModeEXT(
    VkDevice device,
    VkProfilerSyncModeEXT syncMode );

VKAPI_ATTR VkResult VKAPI_CALL vkGetProfilerFrameDataEXT(
    VkDevice device,
    VkProfilerRegionDataEXT* pData );

VKAPI_ATTR VkResult VKAPI_CALL vkGetProfilerCommandBufferDataEXT(
    VkDevice device,
    VkCommandBuffer commandBuffer,
    VkProfilerRegionDataEXT* pData );

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateProfilerMetricPropertiesEXT(
    VkDevice device,
    uint32_t* pProfilerMetricCount,
    VkProfilerMetricPropertiesEXT* pProfilerMetricProperties );
#endif // VK_NO_PROTOTYPES

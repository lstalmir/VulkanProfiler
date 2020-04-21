#pragma once
#include <vulkan/vulkan.h>

#define VK_EXT_PROFILER_SPEC_VERSION 1
#define VK_EXT_PROFILER_EXTENSION_NAME "VK_EXT_profiler"

enum VkProfilerModeEXT
{
    VK_PROFILER_MODE_PER_DRAWCALL_EXT,
    VK_PROFILER_MODE_PER_PIPELINE_EXT,
    VK_PROFILER_MODE_PER_RENDER_PASS_EXT,
    VK_PROFILER_MODE_PER_COMMAND_BUFFER_EXT,
    VK_PROFILER_MODE_PER_SUBMIT_EXT,
    VK_PROFILER_MODE_PER_FRAME_EXT,
    VK_PROFILER_MODE_MAX_ENUM_EXT = UINT32_MAX
};

enum VkProfilerOutputFlagBitsEXT
{
    VK_PROFILER_OUTPUT_FLAG_CONSOLE_BIT_EXT = 1,
    VK_PROFILER_OUTPUT_FLAG_OVERLAY_BIT_EXT = 2,
    VK_PROFILER_OUTPUT_FLAG_MAX_ENUM_EXT = UINT32_MAX
};

typedef VkFlags VkProfilerOutputFlagsEXT;

enum VkProfilerRegionTypeEXT
{
    VK_PROFILER_REGION_TYPE_FRAME_EXT,
    VK_PROFILER_REGION_TYPE_SUBMIT_EXT,
    VK_PROFILER_REGION_TYPE_COMMAND_BUFFER_EXT,
    VK_PROFILER_REGION_TYPE_DEBUG_MARKER_EXT,
    VK_PROFILER_REGION_TYPE_RENDER_PASS_EXT,
    VK_PROFILER_REGION_TYPE_PIPELINE_EXT,
    VK_PROFILER_REGION_TYPE_MAX_ENUM_EXT = UINT32_MAX
};

typedef struct VkProfilerRegionDataEXT
{
    VkProfilerRegionTypeEXT regionType;
    const char* pRegionName;
    uint64_t regionObject;
    uint32_t regionID;
    uint32_t subregionCount;
    struct VkProfilerRegionDataEXT* pSubregions;
    float duration;
    uint32_t drawCount;
    uint32_t drawIndirectCount;
    uint32_t dispatchCount;
    uint32_t dispatchIndirectCount;
    uint32_t clearCount;
    uint32_t barrierCount;
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

typedef VKAPI_ATTR VkResult( VKAPI_CALL* PFN_vkSetProfilerModeEXT )(VkDevice, VkProfilerModeEXT);
typedef VKAPI_ATTR void(VKAPI_CALL* PFN_vkGetProfilerDataEXT)(VkDevice, const VkProfilerRegionDataEXT**);

VKAPI_ATTR VkResult VKAPI_CALL vkSetProfilerModeEXT(
    VkDevice device,
    VkProfilerModeEXT mode );

VKAPI_ATTR VkResult VKAPI_CALL vkGetProfilerDataEXT(
    VkDevice device,
    VkProfilerDataEXT* pData );

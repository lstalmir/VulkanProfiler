#pragma once
#include "profiler_layer_functions/core/VkDevice_functions_base.h"

namespace Profiler
{
    struct VkDrawIndirectCountAmd_Functions : VkDevice_Functions_Base
    {
        // vkCmdDrawIndirectCountKHR
        static VKAPI_ATTR void VKAPI_CALL CmdDrawIndirectCountAMD(
            VkCommandBuffer commandBuffer,
            VkBuffer argsBuffer,
            VkDeviceSize argsOffset,
            VkBuffer countBuffer,
            VkDeviceSize countOffset,
            uint32_t maxDrawCount,
            uint32_t stride );

        // vkCmdDrawIndirectCountKHR
        static VKAPI_ATTR void VKAPI_CALL CmdDrawIndexedIndirectCountAMD(
            VkCommandBuffer commandBuffer,
            VkBuffer argsBuffer,
            VkDeviceSize argsOffset,
            VkBuffer countBuffer,
            VkDeviceSize countOffset,
            uint32_t maxDrawCount,
            uint32_t stride );
    };
}

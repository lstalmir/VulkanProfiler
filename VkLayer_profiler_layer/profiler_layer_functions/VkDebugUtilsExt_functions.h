#pragma once
#include "VkDevice_functions_base.h"

namespace Profiler
{
    struct VkDebugUtilsExt_Functions : VkDevice_Functions_Base
    {
        // vkSetDebugUtilsObjectNameEXT
        static VKAPI_ATTR VkResult VKAPI_CALL SetDebugUtilsObjectNameEXT(
            VkDevice device,
            const VkDebugUtilsObjectNameInfoEXT* pObjectInfo );

        // vkSetDebugUtilsObjectTagEXT
        static VKAPI_ATTR VkResult VKAPI_CALL SetDebugUtilsObjectTagEXT(
            VkDevice device,
            const VkDebugUtilsObjectTagInfoEXT* pObjectInfo );

        // vkCmdInsertDebugUtilsLabelEXT
        static VKAPI_ATTR void VKAPI_CALL CmdInsertDebugUtilsLabelEXT(
            VkCommandBuffer commandBuffer,
            const VkDebugUtilsLabelEXT* pLabelInfo );

        // vkCmdBeginDebugUtilsLabelEXT
        static VKAPI_ATTR void VKAPI_CALL CmdBeginDebugUtilsLabelEXT(
            VkCommandBuffer commandBuffer,
            const VkDebugUtilsLabelEXT* pLabelInfo );

        // vkCmdEndDebugUtilsLabelEXT
        static VKAPI_ATTR void VKAPI_CALL CmdEndDebugUtilsLabelEXT(
            VkCommandBuffer commandBuffer );
    };
}

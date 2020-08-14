#pragma once
#include "profiler_layer_functions/core/VkDevice_functions_base.h"

namespace Profiler
{
    struct VkDebugMarkerExt_Functions : VkDevice_Functions_Base
    {
        // vkDebugMarkerSetObjectNameEXT
        static VKAPI_ATTR VkResult VKAPI_CALL DebugMarkerSetObjectNameEXT(
            VkDevice device,
            const VkDebugMarkerObjectNameInfoEXT* pObjectInfo );

        // vkDebugMarkerSetObjectTagEXT
        static VKAPI_ATTR VkResult VKAPI_CALL DebugMarkerSetObjectTagEXT(
            VkDevice device,
            const VkDebugMarkerObjectTagInfoEXT* pObjectInfo );

        // vkCmdDebugMarkerInsertEXT
        static VKAPI_ATTR void VKAPI_CALL CmdDebugMarkerInsertEXT(
            VkCommandBuffer commandBuffer,
            const VkDebugMarkerMarkerInfoEXT* pMarkerInfo );

        // vkCmdDebugMarkerBeginEXT
        static VKAPI_ATTR void VKAPI_CALL CmdDebugMarkerBeginEXT(
            VkCommandBuffer commandBuffer,
            const VkDebugMarkerMarkerInfoEXT* pMarkerInfo );

        // vkCmdDebugMarkerEndEXT
        static VKAPI_ATTR void VKAPI_CALL CmdDebugMarkerEndEXT(
            VkCommandBuffer commandBuffer );
    };
}

#include "VkDebugMarkerExt_Functions.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        DebugMarkerSetObjectNameEXT

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkDebugMarkerExt_Functions::DebugMarkerSetObjectNameEXT(
        VkDevice device,
        const VkDebugMarkerObjectNameInfoEXT* pObjectInfo )
    {
        auto& dd = DeviceDispatch.Get( device );

        VkResult result = VK_SUCCESS;

        // Call next layer
        if( dd.Device.Callbacks.DebugMarkerSetObjectNameEXT )
        {
            result = dd.Device.Callbacks.DebugMarkerSetObjectNameEXT( device, pObjectInfo );
        }

        // Store object name
        if( result == VK_SUCCESS )
        {
            dd.Device.Debug.ObjectNames.emplace( pObjectInfo->object, pObjectInfo->pObjectName );
        }

        return result;
    }

    /***********************************************************************************\

    Function:
        DebugMarkerSetObjectTagEXT

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkDebugMarkerExt_Functions::DebugMarkerSetObjectTagEXT(
        VkDevice device,
        const VkDebugMarkerObjectTagInfoEXT* pObjectInfo )
    {
        auto& dd = DeviceDispatch.Get( device );

        VkResult result = VK_SUCCESS;

        // Call next layer
        if( dd.Device.Callbacks.DebugMarkerSetObjectTagEXT )
        {
            result = dd.Device.Callbacks.DebugMarkerSetObjectTagEXT( device, pObjectInfo );
        }

        // Object tags not supported

        return result;
    }

    /***********************************************************************************\

    Function:
        CmdDebugMarkerInsertEXT

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkDebugMarkerExt_Functions::CmdDebugMarkerInsertEXT(
        VkCommandBuffer commandBuffer,
        const VkDebugMarkerMarkerInfoEXT* pMarkerInfo )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        profiledCommandBuffer.DebugLabel( pMarkerInfo->pMarkerName, pMarkerInfo->color );

        // Invoke next layer (if available)
        if( dd.Device.Callbacks.CmdDebugMarkerInsertEXT )
        {
            dd.Device.Callbacks.CmdDebugMarkerInsertEXT( commandBuffer, pMarkerInfo );
        }
    }

    /***********************************************************************************\

    Function:
        CmdDebugMarkerBeginEXT

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkDebugMarkerExt_Functions::CmdDebugMarkerBeginEXT(
        VkCommandBuffer commandBuffer,
        const VkDebugMarkerMarkerInfoEXT* pMarkerInfo )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        profiledCommandBuffer.DebugLabel( pMarkerInfo->pMarkerName, pMarkerInfo->color );

        // Invoke next layer (if available)
        if( dd.Device.Callbacks.CmdDebugMarkerBeginEXT )
        {
            dd.Device.Callbacks.CmdDebugMarkerBeginEXT( commandBuffer, pMarkerInfo );
        }
    }

    /***********************************************************************************\

    Function:
        CmdDebugMarkerEndEXT

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkDebugMarkerExt_Functions::CmdDebugMarkerEndEXT(
        VkCommandBuffer commandBuffer )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );

        // Ranged debug labels not supported

        if( dd.Device.Callbacks.CmdDebugMarkerEndEXT )
        {
            dd.Device.Callbacks.CmdDebugMarkerEndEXT( commandBuffer );
        }
    }
}

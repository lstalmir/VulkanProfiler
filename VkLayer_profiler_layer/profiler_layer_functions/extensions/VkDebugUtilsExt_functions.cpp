#include "VkDebugUtilsExt_functions.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        SetDebugUtilsObjectNameEXT

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkDebugUtilsExt_Functions::SetDebugUtilsObjectNameEXT(
        VkDevice device,
        const VkDebugUtilsObjectNameInfoEXT* pObjectInfo )
    {
        auto& dd = DeviceDispatch.Get( device );

        VkResult result = VK_SUCCESS; 

        // Call next layer
        if( dd.Device.Callbacks.SetDebugUtilsObjectNameEXT )
        {
            result = dd.Device.Callbacks.SetDebugUtilsObjectNameEXT( device, pObjectInfo );
        }

        // Store object name
        if( result == VK_SUCCESS )
        {
            // Revision 2 (2020-04-03): pObjectName can be nullptr
            if( (pObjectInfo->pObjectName) &&
                (std::strlen( pObjectInfo->pObjectName ) > 0) )
            {
                dd.Device.Debug.ObjectNames.insert_or_assign( pObjectInfo->objectHandle, pObjectInfo->pObjectName );
            }
            else
            {
                // Clear debug name
                dd.Device.Debug.ObjectNames.erase( pObjectInfo->objectHandle );

                // Restore pipeline hash as debug name
                if( pObjectInfo->objectType == VK_OBJECT_TYPE_PIPELINE )
                {
                    dd.Profiler.SetDefaultPipelineObjectName(
                        dd.Profiler.GetPipeline( (VkPipeline)pObjectInfo->objectHandle ) );
                }
            }
        }

        return result;
    }

    /***********************************************************************************\

    Function:
        SetDebugUtilsObjectTagEXT

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkDebugUtilsExt_Functions::SetDebugUtilsObjectTagEXT(
        VkDevice device,
        const VkDebugUtilsObjectTagInfoEXT* pObjectInfo )
    {
        auto& dd = DeviceDispatch.Get( device );

        VkResult result = VK_SUCCESS;

        // Call next layer
        if( dd.Device.Callbacks.SetDebugUtilsObjectTagEXT )
        {
            result = dd.Device.Callbacks.SetDebugUtilsObjectTagEXT( device, pObjectInfo );
        }

        // Object tags not supported

        return result;
    }

    /***********************************************************************************\

    Function:
        CmdInsertDebugUtilsLabelEXT

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkDebugUtilsExt_Functions::CmdInsertDebugUtilsLabelEXT(
        VkCommandBuffer commandBuffer,
        const VkDebugUtilsLabelEXT* pLabelInfo )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        profiledCommandBuffer.DebugLabel( pLabelInfo->pLabelName, pLabelInfo->color );

        // Invoke next layer (if available)
        if( dd.Device.Callbacks.CmdInsertDebugUtilsLabelEXT )
        {
            dd.Device.Callbacks.CmdInsertDebugUtilsLabelEXT( commandBuffer, pLabelInfo );
        }
    }

    /***********************************************************************************\

    Function:
        CmdBeginDebugUtilsLabelEXT

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkDebugUtilsExt_Functions::CmdBeginDebugUtilsLabelEXT(
        VkCommandBuffer commandBuffer,
        const VkDebugUtilsLabelEXT* pLabelInfo )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        profiledCommandBuffer.DebugLabel( pLabelInfo->pLabelName, pLabelInfo->color );

        // Invoke next layer (if available)
        if( dd.Device.Callbacks.CmdBeginDebugUtilsLabelEXT )
        {
            dd.Device.Callbacks.CmdBeginDebugUtilsLabelEXT( commandBuffer, pLabelInfo );
        }
    }

    /***********************************************************************************\

    Function:
        CmdEndDebugUtilsLabelEXT

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkDebugUtilsExt_Functions::CmdEndDebugUtilsLabelEXT(
        VkCommandBuffer commandBuffer )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );

        // Ranged debug labels not supported

        if( dd.Device.Callbacks.CmdEndDebugUtilsLabelEXT )
        {
            dd.Device.Callbacks.CmdEndDebugUtilsLabelEXT( commandBuffer );
        }
    }
}

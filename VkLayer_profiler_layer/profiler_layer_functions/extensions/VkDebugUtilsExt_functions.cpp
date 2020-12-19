// Copyright (c) 2020 Lukasz Stalmirski
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

        auto pCommand = DebugLabelCommand::Create(
            CommandId::eInsertDebugLabel,
            pLabelInfo->pLabelName,
            pLabelInfo->color );

        profiledCommandBuffer.PreCommand( pCommand );

        // Invoke next layer (if available)
        if( dd.Device.Callbacks.CmdInsertDebugUtilsLabelEXT )
        {
            dd.Device.Callbacks.CmdInsertDebugUtilsLabelEXT( commandBuffer, pLabelInfo );
        }

        profiledCommandBuffer.PostCommand( pCommand );
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

        auto pCommand = DebugLabelCommand::Create(
            CommandId::eBeginDebugLabel,
            pLabelInfo->pLabelName,
            pLabelInfo->color );

        profiledCommandBuffer.PreCommand( pCommand );

        // Invoke next layer (if available)
        if( dd.Device.Callbacks.CmdBeginDebugUtilsLabelEXT )
        {
            dd.Device.Callbacks.CmdBeginDebugUtilsLabelEXT( commandBuffer, pLabelInfo );
        }

        profiledCommandBuffer.PostCommand( pCommand );
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
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        auto pCommand = DebugLabelCommand::Create( CommandId::eEndDebugLabel );

        profiledCommandBuffer.PreCommand( pCommand );

        if( dd.Device.Callbacks.CmdEndDebugUtilsLabelEXT )
        {
            dd.Device.Callbacks.CmdEndDebugUtilsLabelEXT( commandBuffer );
        }

        profiledCommandBuffer.PostCommand( pCommand );
    }
}

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

#include "VkDebugMarkerExt_functions.h"

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
            if( (std::strlen( pObjectInfo->pObjectName ) > 0) )
            {
                dd.Device.Debug.ObjectNames.insert_or_assign( pObjectInfo->object, pObjectInfo->pObjectName );
            }
            else
            {
                // Clear debug name
                dd.Device.Debug.ObjectNames.erase( pObjectInfo->object );

                // Restore pipeline hash as debug name
                if( pObjectInfo->objectType == VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT )
                {
                    dd.Profiler.SetDefaultPipelineObjectName(
                        dd.Profiler.GetPipeline( (VkPipeline)pObjectInfo->object ) );
                }
            }
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

        auto pCommand = DebugLabelCommand::Create(
            CommandId::eInsertDebugLabel,
            pMarkerInfo->pMarkerName,
            pMarkerInfo->color );

        profiledCommandBuffer.PreCommand( pCommand );

        // Invoke next layer (if available)
        if( dd.Device.Callbacks.CmdDebugMarkerInsertEXT )
        {
            dd.Device.Callbacks.CmdDebugMarkerInsertEXT( commandBuffer, pMarkerInfo );
        }

        profiledCommandBuffer.PostCommand( pCommand );
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

        auto pCommand = DebugLabelCommand::Create(
            CommandId::eBeginDebugLabel,
            pMarkerInfo->pMarkerName,
            pMarkerInfo->color );

        profiledCommandBuffer.PreCommand( pCommand );

        // Invoke next layer (if available)
        if( dd.Device.Callbacks.CmdDebugMarkerBeginEXT )
        {
            dd.Device.Callbacks.CmdDebugMarkerBeginEXT( commandBuffer, pMarkerInfo );
        }

        profiledCommandBuffer.PostCommand( pCommand );
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
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        auto pCommand = DebugLabelCommand::Create( CommandId::eEndDebugLabel );

        profiledCommandBuffer.PreCommand( pCommand );

        if( dd.Device.Callbacks.CmdDebugMarkerEndEXT )
        {
            dd.Device.Callbacks.CmdDebugMarkerEndEXT( commandBuffer );
        }

        profiledCommandBuffer.PostCommand( pCommand );
    }
}

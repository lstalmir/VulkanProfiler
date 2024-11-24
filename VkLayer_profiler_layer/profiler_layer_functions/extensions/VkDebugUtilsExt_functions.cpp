// Copyright (c) 2019-2021 Lukasz Stalmirski
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
            dd.Profiler.template SetObjectName<VkObjectType>(
                pObjectInfo->objectHandle,
                pObjectInfo->objectType,
                pObjectInfo->pObjectName );
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

        // Setup debug label drawcall
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eInsertDebugLabel;
        drawcall.m_Extension = DeviceProfilerExtensionType::eEXT;
        drawcall.m_Payload.m_DebugLabel.m_pName = ProfilerStringFunctions::DuplicateString( pLabelInfo->pLabelName );
        drawcall.m_Payload.m_DebugLabel.m_Color[ 0 ] = pLabelInfo->color[ 0 ];
        drawcall.m_Payload.m_DebugLabel.m_Color[ 1 ] = pLabelInfo->color[ 1 ];
        drawcall.m_Payload.m_DebugLabel.m_Color[ 2 ] = pLabelInfo->color[ 2 ];
        drawcall.m_Payload.m_DebugLabel.m_Color[ 3 ] = pLabelInfo->color[ 3 ];

        profiledCommandBuffer.PreCommand( drawcall );

        // Invoke next layer (if available)
        if( dd.Device.Callbacks.CmdInsertDebugUtilsLabelEXT )
        {
            dd.Device.Callbacks.CmdInsertDebugUtilsLabelEXT( commandBuffer, pLabelInfo );
        }

        profiledCommandBuffer.PostCommand( drawcall );
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

        // Setup debug label drawcall
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eBeginDebugLabel;
        drawcall.m_Extension = DeviceProfilerExtensionType::eEXT;
        drawcall.m_Payload.m_DebugLabel.m_pName = ProfilerStringFunctions::DuplicateString( pLabelInfo->pLabelName );
        drawcall.m_Payload.m_DebugLabel.m_Color[ 0 ] = pLabelInfo->color[ 0 ];
        drawcall.m_Payload.m_DebugLabel.m_Color[ 1 ] = pLabelInfo->color[ 1 ];
        drawcall.m_Payload.m_DebugLabel.m_Color[ 2 ] = pLabelInfo->color[ 2 ];
        drawcall.m_Payload.m_DebugLabel.m_Color[ 3 ] = pLabelInfo->color[ 3 ];

        profiledCommandBuffer.PreCommand( drawcall );

        // Invoke next layer (if available)
        if( dd.Device.Callbacks.CmdBeginDebugUtilsLabelEXT )
        {
            dd.Device.Callbacks.CmdBeginDebugUtilsLabelEXT( commandBuffer, pLabelInfo );
        }

        profiledCommandBuffer.PostCommand( drawcall );
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

        // Setup debug label drawcall
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eEndDebugLabel;
        drawcall.m_Extension = DeviceProfilerExtensionType::eEXT;
        drawcall.m_Payload.m_DebugLabel.m_pName = nullptr;

        profiledCommandBuffer.PreCommand( drawcall );

        if( dd.Device.Callbacks.CmdEndDebugUtilsLabelEXT )
        {
            dd.Device.Callbacks.CmdEndDebugUtilsLabelEXT( commandBuffer );
        }

        profiledCommandBuffer.PostCommand( drawcall );
    }
}

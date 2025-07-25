// Copyright (c) 2025 Lukasz Stalmirski
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

#include "VkOpacityMicromapExt_functions.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        CreateMicromapEXT

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkOpacityMicromapExt_Functions::CreateMicromapEXT(
        VkDevice device,
        const VkMicromapCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkMicromapEXT* pMicromap )
    {
        auto& dd = DeviceDispatch.Get( device );
        TipGuard tip( dd.Device.TIP, __func__ );

        // Invoke next layer's implementation
        VkResult result = dd.Device.Callbacks.CreateMicromapEXT(
            device,
            pCreateInfo,
            pAllocator,
            pMicromap );

        if( result == VK_SUCCESS )
        {
            // Register the micromap
            dd.Profiler.CreateMicromap( *pMicromap, pCreateInfo );
        }

        return result;
    }

    /***********************************************************************************\

    Function:
        DestroyMicromapEXT

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkOpacityMicromapExt_Functions::DestroyMicromapEXT(
        VkDevice device,
        VkMicromapEXT micromap,
        const VkAllocationCallbacks* pAllocator )
    {
        auto& dd = DeviceDispatch.Get( device );
        TipGuard tip( dd.Device.TIP, __func__ );

        // Unregister the micromap
        dd.Profiler.DestroyMicromap( micromap );

        // Invoke next layer's implementation
        dd.Device.Callbacks.DestroyMicromapEXT(
            device,
            micromap,
            pAllocator );
    }

    /***********************************************************************************\

    Function:
        CmdBuildMicromapsEXT

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkOpacityMicromapExt_Functions::CmdBuildMicromapsEXT(
        VkCommandBuffer commandBuffer,
        uint32_t infoCount,
        const VkMicromapBuildInfoEXT* pInfos )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        TipGuard tip( dd.Device.TIP, __func__ );

        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        // Setup drawcall descriptor
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eBuildMicromapsEXT;
        drawcall.m_Payload.m_BuildMicromaps.m_InfoCount = infoCount;
        drawcall.m_Payload.m_BuildMicromaps.m_pInfos = pInfos;

        profiledCommandBuffer.PreCommand( drawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdBuildMicromapsEXT(
            commandBuffer,
            infoCount,
            pInfos );

        profiledCommandBuffer.PostCommand( drawcall );
    }

    /***********************************************************************************\

    Function:
        CmdCopyMicromapEXT

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkOpacityMicromapExt_Functions::CmdCopyMicromapEXT(
        VkCommandBuffer commandBuffer,
        const VkCopyMicromapInfoEXT* pInfo )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        TipGuard tip( dd.Device.TIP, __func__ );

        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        // Setup drawcall descriptor
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eCopyMicromapEXT;
        drawcall.m_Payload.m_CopyMicromap.m_Src = pInfo->src;
        drawcall.m_Payload.m_CopyMicromap.m_Dst = pInfo->dst;
        drawcall.m_Payload.m_CopyMicromap.m_Mode = pInfo->mode;

        profiledCommandBuffer.PreCommand( drawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdCopyMicromapEXT(
            commandBuffer,
            pInfo );

        profiledCommandBuffer.PostCommand( drawcall );
    }

    /***********************************************************************************\

    Function:
        CmdCopyMemoryToMicromapEXT

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkOpacityMicromapExt_Functions::CmdCopyMemoryToMicromapEXT(
        VkCommandBuffer commandBuffer,
        const VkCopyMemoryToMicromapInfoEXT* pInfo )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        TipGuard tip( dd.Device.TIP, __func__ );

        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        // Setup drawcall descriptor
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eCopyMemoryToMicromapEXT;
        drawcall.m_Payload.m_CopyMemoryToMicromap.m_Src = pInfo->src;
        drawcall.m_Payload.m_CopyMemoryToMicromap.m_Dst = pInfo->dst;
        drawcall.m_Payload.m_CopyMemoryToMicromap.m_Mode = pInfo->mode;

        profiledCommandBuffer.PreCommand( drawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdCopyMemoryToMicromapEXT(
            commandBuffer,
            pInfo );

        profiledCommandBuffer.PostCommand( drawcall );
    }

    /***********************************************************************************\

    Function:
        CmdCopyMicromapToMemoryEXT

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkOpacityMicromapExt_Functions::CmdCopyMicromapToMemoryEXT(
        VkCommandBuffer commandBuffer,
        const VkCopyMicromapToMemoryInfoEXT* pInfo )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        TipGuard tip( dd.Device.TIP, __func__ );

        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        // Setup drawcall descriptor
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eCopyMicromapToMemoryEXT;
        drawcall.m_Payload.m_CopyMicromapToMemory.m_Src = pInfo->src;
        drawcall.m_Payload.m_CopyMicromapToMemory.m_Dst = pInfo->dst;
        drawcall.m_Payload.m_CopyMicromapToMemory.m_Mode = pInfo->mode;

        profiledCommandBuffer.PreCommand( drawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdCopyMicromapToMemoryEXT(
            commandBuffer,
            pInfo );

        profiledCommandBuffer.PostCommand( drawcall );
    }
}

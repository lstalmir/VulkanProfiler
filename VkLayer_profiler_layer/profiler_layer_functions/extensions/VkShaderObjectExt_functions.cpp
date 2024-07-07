// Copyright (c) 2023-2023 Lukasz Stalmirski
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

#include "VkShaderObjectExt_functions.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        CreateShadersEXT

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkShaderObjectExt_Functions::CreateShadersEXT(
        VkDevice device,
        uint32_t createInfoCount,
        const VkShaderCreateInfoEXT* pCreateInfos,
        const VkAllocationCallbacks* pAllocator,
        VkShaderEXT* pShaders )
    {
        auto& dd = DeviceDispatch.Get( device );

        // Create the shaders.
        VkResult result = dd.Device.Callbacks.CreateShadersEXT(
            device,
            createInfoCount,
            pCreateInfos,
            pAllocator,
            pShaders );

        // The function above may return an error code, yet still some of the shaders may compile successfully.
        // Register those shaders regardless of the result, skipping the null handles.
        for( uint32_t i = 0; i < createInfoCount; ++i )
        {
            if( pShaders[i] == VK_NULL_HANDLE )
            {
                // Compilation of this shader failed.
                continue;
            }

            dd.Profiler.CreateShader( pShaders[i], &pCreateInfos[i] );
        }

        return result;
    }

    /***********************************************************************************\

    Function:
        DestroyShaderEXT

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkShaderObjectExt_Functions::DestroyShaderEXT(
        VkDevice device,
        VkShaderEXT shader,
        const VkAllocationCallbacks* pAllocator )
    {
        auto& dd = DeviceDispatch.Get( device );

        // Unregister the shader.
        dd.Profiler.DestroyShader( shader );

        dd.Device.Callbacks.DestroyShaderEXT(
            device,
            shader,
            pAllocator );
    }

    /***********************************************************************************\

    Function:
        CmdBindShadersEXT

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkShaderObjectExt_Functions::CmdBindShadersEXT(
        VkCommandBuffer commandBuffer,
        uint32_t stageCount,
        const VkShaderStageFlagBits* pStages,
        const VkShaderEXT* pShaders )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        auto& cmd = dd.Profiler.GetCommandBuffer( commandBuffer );

        // Update profiled command buffer state for the next draw.
        cmd.BindShaders( stageCount, pStages, pShaders );

        dd.Device.Callbacks.CmdBindShadersEXT(
            commandBuffer,
            stageCount,
            pStages,
            pShaders );
    }
}

// Copyright (c) 2024-2024 Lukasz Stalmirski
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

#pragma once
#include "VkDevice_functions_base.h"

namespace Profiler
{
    struct VkMeshShaderExt_Functions : VkDevice_Functions_Base
    {
        // vkCmdDrawMeshTasksEXT
        static VKAPI_ATTR void VKAPI_CALL CmdDrawMeshTasksEXT(
            VkCommandBuffer commandBuffer,
            uint32_t groupCountX,
            uint32_t groupCountY,
            uint32_t groupCountZ );

        // vkCmdDrawMeshTasksIndirectEXT
        static VKAPI_ATTR void VKAPI_CALL CmdDrawMeshTasksIndirectEXT(
            VkCommandBuffer commandBuffer,
            VkBuffer buffer,
            VkDeviceSize offset,
            uint32_t drawCount,
            uint32_t stride );

        // vkCmdDrawMeshTasksIndirectCountEXT
        static VKAPI_ATTR void VKAPI_CALL CmdDrawMeshTasksIndirectCountEXT(
            VkCommandBuffer commandBuffer,
            VkBuffer buffer,
            VkDeviceSize offset,
            VkBuffer countBuffer,
            VkDeviceSize countBufferOffset,
            uint32_t maxDrawCount,
            uint32_t stride );
    };
}

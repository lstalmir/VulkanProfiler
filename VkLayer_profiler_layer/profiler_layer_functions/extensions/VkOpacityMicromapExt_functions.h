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

#pragma once
#include "VkDevice_functions_base.h"

namespace Profiler
{
    struct VkOpacityMicromapExt_Functions : VkDevice_Functions_Base
    {
        // vkCmdBuildMicromapsEXT
        static VKAPI_ATTR void VKAPI_CALL CmdBuildMicromapsEXT(
            VkCommandBuffer commandBuffer,
            uint32_t infoCount,
            const VkMicromapBuildInfoEXT* pInfos );

        // vkCmdCopyMicromapEXT
        static VKAPI_ATTR void VKAPI_CALL CmdCopyMicromapEXT(
            VkCommandBuffer commandBuffer,
            const VkCopyMicromapInfoEXT* pInfo );

        // vkCmdCopyMemoryToMicromapEXT
        static VKAPI_ATTR void VKAPI_CALL CmdCopyMemoryToMicromapEXT(
            VkCommandBuffer commandBuffer,
            const VkCopyMemoryToMicromapInfoEXT* pInfo );

        // vkCmdCopyMicromapToMemoryEXT
        static VKAPI_ATTR void VKAPI_CALL CmdCopyMicromapToMemoryEXT(
            VkCommandBuffer commandBuffer,
            const VkCopyMicromapToMemoryInfoEXT* pInfo );
    };
}

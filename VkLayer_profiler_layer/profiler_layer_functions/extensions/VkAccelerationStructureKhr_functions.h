// Copyright (c) 2019-2022 Lukasz Stalmirski
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
    struct VkAccelerationStructureKhr_Functions : VkDevice_Functions_Base
    {
        // vkCmdBuildAccelerationStructuresKHR
        static VKAPI_ATTR void VKAPI_CALL CmdBuildAccelerationStructuresKHR(
            VkCommandBuffer commandBuffer,
            uint32_t infoCount,
            const VkAccelerationStructureBuildGeometryInfoKHR* pInfos,
            const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos);

        // vkCmdBuildAccelerationStructuresIndirectKHR
        static VKAPI_ATTR void VKAPI_CALL CmdBuildAccelerationStructuresIndirectKHR(
            VkCommandBuffer commandBuffer,
            uint32_t infoCount,
            const VkAccelerationStructureBuildGeometryInfoKHR* pInfos,
            const VkDeviceAddress* pIndirectDeviceAddresses,
            const uint32_t* pIndirectStrides,
            const uint32_t* const* ppMaxPrimitiveCounts);

        // vkCmdCopyAccelerationStructureKHR
        static VKAPI_ATTR void VKAPI_CALL CmdCopyAccelerationStructureKHR(
            VkCommandBuffer commandBuffer,
            const VkCopyAccelerationStructureInfoKHR* pInfo);

        // vkCmdCopyAccelerationStructureToMemoryKHR
        static VKAPI_ATTR void VKAPI_CALL CmdCopyAccelerationStructureToMemoryKHR(
            VkCommandBuffer commandBuffer,
            const VkCopyAccelerationStructureToMemoryInfoKHR* pInfo);

        // vkCmdCopyMemoryToAccelerationStructureKHR
        static VKAPI_ATTR void VKAPI_CALL CmdCopyMemoryToAccelerationStructureKHR(
            VkCommandBuffer commandBuffer,
            const VkCopyMemoryToAccelerationStructureInfoKHR* pInfo);
    };
}

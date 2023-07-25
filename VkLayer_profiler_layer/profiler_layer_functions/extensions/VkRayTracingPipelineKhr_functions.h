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
    struct VkRayTracingPipelineKhr_Functions : VkDevice_Functions_Base
    {
        // vkCreateRayTracingPipelinesKHR
        static VKAPI_ATTR VkResult VKAPI_CALL CreateRayTracingPipelinesKHR(
            VkDevice device,
            VkDeferredOperationKHR deferredOperation,
            VkPipelineCache pipelineCache,
            uint32_t createInfoCount,
            const VkRayTracingPipelineCreateInfoKHR* pCreateInfos,
            const VkAllocationCallbacks* pAllocator,
            VkPipeline* pPipelines );

        // vkCmdTraceRaysKHR
        static VKAPI_ATTR void VKAPI_CALL CmdTraceRaysKHR(
            VkCommandBuffer commandBuffer,
            const VkStridedDeviceAddressRegionKHR* pRaygenShaderBindingTable,
            const VkStridedDeviceAddressRegionKHR* pMissShaderBindingTable,
            const VkStridedDeviceAddressRegionKHR* pHitShaderBindingTable,
            const VkStridedDeviceAddressRegionKHR* pCallableShaderBindingTable,
            uint32_t width,
            uint32_t height,
            uint32_t depth );

        // vkCmdTraceRaysIndirectKHR
        static VKAPI_ATTR void VKAPI_CALL CmdTraceRaysIndirectKHR(
            VkCommandBuffer commandBuffer,
            const VkStridedDeviceAddressRegionKHR* pRaygenShaderBindingTable,
            const VkStridedDeviceAddressRegionKHR* pMissShaderBindingTable,
            const VkStridedDeviceAddressRegionKHR* pHitShaderBindingTable,
            const VkStridedDeviceAddressRegionKHR* pCallableShaderBindingTable,
            VkDeviceAddress indirectDeviceAddress );
    };
}

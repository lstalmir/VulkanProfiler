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
#include "profiler_vulkan_state.h"

#include "shaders/simple_triangle_rt.rgen.glsl.h"
#include "shaders/simple_triangle_rt.rmiss.glsl.h"
#include "shaders/simple_triangle_rt.rchit.glsl.h"

namespace Profiler
{
    class VulkanSimpleTriangleRT
    {
    public:
        VulkanState* Vk = nullptr;

        VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;
        VkPipeline Pipeline = VK_NULL_HANDLE;

        VkShaderModule RaygenShaderModule = VK_NULL_HANDLE;
        VkShaderModule MissShaderModule = VK_NULL_HANDLE;
        VkShaderModule HitShaderModule = VK_NULL_HANDLE;

        VkRayTracingPipelineCreateInfoKHR PipelineInfo = {};
        VkRayTracingShaderGroupCreateInfoKHR PipelineShaderGroups[3] = {};
        VkPipelineShaderStageCreateInfo PipelineShaderStages[3] = {};

        PFN_vkCreateDeferredOperationKHR vkCreateDeferredOperationKHR = nullptr;
        PFN_vkDestroyDeferredOperationKHR vkDestroyDeferredOperationKHR = nullptr;
        PFN_vkDeferredOperationJoinKHR vkDeferredOperationJoinKHR = nullptr;

        PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR = nullptr;

    public:
        static void ConfigureVulkan( VulkanState::CreateInfo& createInfo )
        {
            static VulkanExtension deferredHostOperationsExtension( VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, true );
            createInfo.DeviceExtensions.push_back( &deferredHostOperationsExtension );

            static VulkanExtension rayTracingPipelineExtension( VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, true );
            createInfo.DeviceExtensions.push_back( &rayTracingPipelineExtension );
        }

        inline VulkanSimpleTriangleRT( VulkanState* Vk )
            : Vk( Vk )
        {
            // Get function pointers
            vkCreateDeferredOperationKHR = (PFN_vkCreateDeferredOperationKHR)vkGetDeviceProcAddr( Vk->Device, "vkCreateDeferredOperationKHR" );
            vkDestroyDeferredOperationKHR = (PFN_vkDestroyDeferredOperationKHR)vkGetDeviceProcAddr( Vk->Device, "vkDestroyDeferredOperationKHR" );
            vkDeferredOperationJoinKHR = (PFN_vkDeferredOperationJoinKHR)vkGetDeviceProcAddr( Vk->Device, "vkDeferredOperationJoinKHR" );

            vkCreateRayTracingPipelinesKHR = (PFN_vkCreateRayTracingPipelinesKHR)vkGetDeviceProcAddr( Vk->Device, "vkCreateRayTracingPipelinesKHR" );
        }

        inline ~VulkanSimpleTriangleRT()
        {
            if( Pipeline ) vkDestroyPipeline( Vk->Device, Pipeline, nullptr );
            if( PipelineLayout ) vkDestroyPipelineLayout( Vk->Device, PipelineLayout, nullptr );
            if( RaygenShaderModule ) vkDestroyShaderModule( Vk->Device, RaygenShaderModule, nullptr );
            if( MissShaderModule ) vkDestroyShaderModule( Vk->Device, MissShaderModule, nullptr );
            if( HitShaderModule ) vkDestroyShaderModule( Vk->Device, HitShaderModule, nullptr );
        }

        inline void CreatePipeline( VkDeferredOperationKHR deferredOperation = VK_NULL_HANDLE )
        {
            // Create shader modules
            VkShaderModuleCreateInfo shaderModuleInfo = {};
            shaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            shaderModuleInfo.codeSize = sizeof( simple_triangle_rt_rgen_glsl );
            shaderModuleInfo.pCode = simple_triangle_rt_rgen_glsl;
            VERIFY_RESULT( Vk, vkCreateShaderModule( Vk->Device, &shaderModuleInfo, nullptr, &RaygenShaderModule ) );

            shaderModuleInfo.codeSize = sizeof( simple_triangle_rt_rmiss_glsl );
            shaderModuleInfo.pCode = simple_triangle_rt_rmiss_glsl;
            VERIFY_RESULT( Vk, vkCreateShaderModule( Vk->Device, &shaderModuleInfo, nullptr, &MissShaderModule ) );

            shaderModuleInfo.codeSize = sizeof( simple_triangle_rt_rchit_glsl );
            shaderModuleInfo.pCode = simple_triangle_rt_rchit_glsl;
            VERIFY_RESULT( Vk, vkCreateShaderModule( Vk->Device, &shaderModuleInfo, nullptr, &HitShaderModule ) );

            // Create pipeline layout
            VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            VERIFY_RESULT( Vk, vkCreatePipelineLayout( Vk->Device, &pipelineLayoutInfo, nullptr, &PipelineLayout ) );

            // Create ray tracing pipeline
            PipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
            PipelineInfo.layout = PipelineLayout;
            PipelineInfo.stageCount = 3;
            PipelineInfo.pStages = PipelineShaderStages;
            PipelineInfo.groupCount = 3;
            PipelineInfo.pGroups = PipelineShaderGroups;

            PipelineShaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            PipelineShaderStages[0].stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
            PipelineShaderStages[0].module = RaygenShaderModule;
            PipelineShaderStages[0].pName = "main";

            PipelineShaderGroups[0].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
            PipelineShaderGroups[0].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
            PipelineShaderGroups[0].generalShader = 0;
            PipelineShaderGroups[0].closestHitShader = VK_SHADER_UNUSED_KHR;
            PipelineShaderGroups[0].anyHitShader = VK_SHADER_UNUSED_KHR;
            PipelineShaderGroups[0].intersectionShader = VK_SHADER_UNUSED_KHR;

            PipelineShaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            PipelineShaderStages[1].stage = VK_SHADER_STAGE_MISS_BIT_KHR;
            PipelineShaderStages[1].module = MissShaderModule;
            PipelineShaderStages[1].pName = "main";

            PipelineShaderGroups[1].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
            PipelineShaderGroups[1].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
            PipelineShaderGroups[1].generalShader = 1;
            PipelineShaderGroups[1].closestHitShader = VK_SHADER_UNUSED_KHR;
            PipelineShaderGroups[1].anyHitShader = VK_SHADER_UNUSED_KHR;
            PipelineShaderGroups[1].intersectionShader = VK_SHADER_UNUSED_KHR;

            PipelineShaderStages[2].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            PipelineShaderStages[2].stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
            PipelineShaderStages[2].module = HitShaderModule;
            PipelineShaderStages[2].pName = "main";

            PipelineShaderGroups[2].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
            PipelineShaderGroups[2].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
            PipelineShaderGroups[2].generalShader = VK_SHADER_UNUSED_KHR;
            PipelineShaderGroups[2].closestHitShader = 2;
            PipelineShaderGroups[2].anyHitShader = VK_SHADER_UNUSED_KHR;
            PipelineShaderGroups[2].intersectionShader = VK_SHADER_UNUSED_KHR;

            VERIFY_RESULT( Vk, vkCreateRayTracingPipelinesKHR( Vk->Device, deferredOperation, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &Pipeline ) );
        }

        [[nodiscard]]
        inline VkDeferredOperationKHR CreatePipelineDeferred()
        {
            VkDeferredOperationKHR deferredOperation = VK_NULL_HANDLE;
            VERIFY_RESULT( Vk, vkCreateDeferredOperationKHR( Vk->Device, nullptr, &deferredOperation ) );

            try
            {
                CreatePipeline( deferredOperation );
            }
            catch( ... )
            {
                vkDestroyDeferredOperationKHR( Vk->Device, deferredOperation, nullptr );
                throw;
            }

            return deferredOperation;
        }

        inline void JoinDeferredOperation( VkDeferredOperationKHR deferredOperation )
        {
            VERIFY_RESULT( Vk, vkDeferredOperationJoinKHR( Vk->Device, deferredOperation ) );

            // Destroy deferred operation once it's done.
            vkDestroyDeferredOperationKHR( Vk->Device, deferredOperation, nullptr );
        }
    };
}

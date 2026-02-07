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

        VkDescriptorSetLayout DescriptorSetLayout = VK_NULL_HANDLE;

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

        struct RayTracingPipelineFeature : VulkanFeature
        {
            VkPhysicalDeviceRayTracingPipelineFeaturesKHR createInfo = {
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR
            };

            RayTracingPipelineFeature()
                : VulkanFeature( "rayTracingPipeline", VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, true )
            {
            }

            void* GetCreateInfo() override
            {
                return &createInfo;
            }

            bool CheckSupport( const VkPhysicalDeviceFeatures2* ) const override
            {
                return createInfo.rayTracingPipeline;
            }
        };

    public:
        static void ConfigureVulkan( VulkanState::CreateInfo& createInfo )
        {
            static VulkanExtension deferredHostOperationsExtension( VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, true );
            createInfo.DeviceExtensions.push_back( &deferredHostOperationsExtension );

            static VulkanExtension descriptorIndexingExtension( VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME, true );
            createInfo.DeviceExtensions.push_back( &descriptorIndexingExtension );

            static VulkanExtension bufferDeviceAddressExtension( VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, true );
            createInfo.DeviceExtensions.push_back( &bufferDeviceAddressExtension );

            static VulkanExtension accelerationStructureExtension( VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, true );
            createInfo.DeviceExtensions.push_back( &accelerationStructureExtension );

            static VulkanExtension shaderFloatControlsExtension( VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME, true );
            createInfo.DeviceExtensions.push_back( &shaderFloatControlsExtension );

            static VulkanExtension spirv14Extension( VK_KHR_SPIRV_1_4_EXTENSION_NAME, true );
            createInfo.DeviceExtensions.push_back( &spirv14Extension );

            static VulkanExtension rayTracingPipelineExtension( VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, true );
            createInfo.DeviceExtensions.push_back( &rayTracingPipelineExtension );

            static RayTracingPipelineFeature rayTracingPipelineFeature;
            createInfo.DeviceFeatures.push_back( &rayTracingPipelineFeature );
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
            if( DescriptorSetLayout ) vkDestroyDescriptorSetLayout( Vk->Device, DescriptorSetLayout, nullptr );
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

            // Create descriptor set layout
            VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[2] = {};
            descriptorSetLayoutBindings[0].binding = 0;
            descriptorSetLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
            descriptorSetLayoutBindings[0].descriptorCount = 1;
            descriptorSetLayoutBindings[0].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

            descriptorSetLayoutBindings[1].binding = 1;
            descriptorSetLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            descriptorSetLayoutBindings[1].descriptorCount = 1;
            descriptorSetLayoutBindings[1].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

            VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {};
            descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorSetLayoutInfo.bindingCount = 2;
            descriptorSetLayoutInfo.pBindings = descriptorSetLayoutBindings;
            VERIFY_RESULT( Vk, vkCreateDescriptorSetLayout( Vk->Device, &descriptorSetLayoutInfo, nullptr, &DescriptorSetLayout ) );

            // Create pipeline layout
            VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = 1;
            pipelineLayoutInfo.pSetLayouts = &DescriptorSetLayout;
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

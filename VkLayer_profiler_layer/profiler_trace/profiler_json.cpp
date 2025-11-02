// Copyright (c) 2019-2025 Lukasz Stalmirski
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

#include "profiler_json.h"
#include "profiler/profiler_data.h"
#include "profiler_helpers/profiler_string_serializer.h"
#include "profiler_layer_objects/VkObject.h"

#include <fmt/format.h>

namespace Profiler
{
    /*************************************************************************\

    Function:
        DeviceProfilerJsonSerializer

    Description:
        Constructor.

    \*************************************************************************/
    DeviceProfilerJsonSerializer::DeviceProfilerJsonSerializer( const DeviceProfilerStringSerializer* pStringSerializer )
        : m_pStringSerializer( pStringSerializer )
    {
    }

    /*************************************************************************\

    Function:
        GetCommandArgs

    Description:
        Serialize command arguments into JSON object.

    \*************************************************************************/
    nlohmann::json DeviceProfilerJsonSerializer::GetCommandArgs( const DeviceProfilerDrawcall& drawcall ) const
    {
        switch( drawcall.m_Type )
        {
        default:
        case DeviceProfilerDrawcallType::eUnknown:
        case DeviceProfilerDrawcallType::eInsertDebugLabel:
        case DeviceProfilerDrawcallType::eBeginDebugLabel:
        case DeviceProfilerDrawcallType::eEndDebugLabel:
            return {};

        case DeviceProfilerDrawcallType::eDraw:
            return {
                { "vertexCount", drawcall.m_Payload.m_Draw.m_VertexCount },
                { "instanceCount", drawcall.m_Payload.m_Draw.m_InstanceCount },
                { "firstVertex", drawcall.m_Payload.m_Draw.m_FirstVertex },
                { "firstInstance", drawcall.m_Payload.m_Draw.m_FirstInstance } };

        case DeviceProfilerDrawcallType::eDrawIndexed:
            return {
                { "indexCount", drawcall.m_Payload.m_DrawIndexed.m_IndexCount },
                { "instanceCount", drawcall.m_Payload.m_DrawIndexed.m_InstanceCount },
                { "firstIndex", drawcall.m_Payload.m_DrawIndexed.m_FirstIndex },
                { "vertexOffset", drawcall.m_Payload.m_DrawIndexed.m_VertexOffset },
                { "firstInstance", drawcall.m_Payload.m_DrawIndexed.m_FirstInstance } };

        case DeviceProfilerDrawcallType::eDrawIndirect:
            return {
                { "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawIndirect.m_Buffer ) },
                { "offset", drawcall.m_Payload.m_DrawIndirect.m_Offset },
                { "drawCount", drawcall.m_Payload.m_DrawIndirect.m_DrawCount },
                { "stride", drawcall.m_Payload.m_DrawIndirect.m_Stride } };

        case DeviceProfilerDrawcallType::eDrawIndexedIndirect:
            return {
                { "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawIndexedIndirect.m_Buffer ) },
                { "offset", drawcall.m_Payload.m_DrawIndexedIndirect.m_Offset },
                { "drawCount", drawcall.m_Payload.m_DrawIndexedIndirect.m_DrawCount },
                { "stride", drawcall.m_Payload.m_DrawIndexedIndirect.m_Stride } };

        case DeviceProfilerDrawcallType::eDrawIndirectCount:
            return {
                { "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawIndirectCount.m_Buffer ) },
                { "offset", drawcall.m_Payload.m_DrawIndirectCount.m_Offset },
                { "countBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawIndirectCount.m_CountBuffer ) },
                { "countOffset", drawcall.m_Payload.m_DrawIndirectCount.m_CountOffset },
                { "maxDrawCount", drawcall.m_Payload.m_DrawIndirectCount.m_MaxDrawCount },
                { "stride", drawcall.m_Payload.m_DrawIndirectCount.m_Stride } };

        case DeviceProfilerDrawcallType::eDrawIndexedIndirectCount:
            return {
                { "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawIndexedIndirectCount.m_Buffer ) },
                { "offset", drawcall.m_Payload.m_DrawIndexedIndirectCount.m_Offset },
                { "countBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawIndexedIndirectCount.m_CountBuffer ) },
                { "countOffset", drawcall.m_Payload.m_DrawIndexedIndirectCount.m_CountOffset },
                { "maxDrawCount", drawcall.m_Payload.m_DrawIndexedIndirectCount.m_MaxDrawCount },
                { "stride", drawcall.m_Payload.m_DrawIndexedIndirectCount.m_Stride } };

        case DeviceProfilerDrawcallType::eDrawMeshTasks:
            return {
                { "groupCountX", drawcall.m_Payload.m_DrawMeshTasks.m_GroupCountX },
                { "groupCountY", drawcall.m_Payload.m_DrawMeshTasks.m_GroupCountY },
                { "groupCountZ", drawcall.m_Payload.m_DrawMeshTasks.m_GroupCountZ }
            };

        case DeviceProfilerDrawcallType::eDrawMeshTasksIndirect:
            return {
                { "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawMeshTasksIndirect.m_Buffer ) },
                { "offset", drawcall.m_Payload.m_DrawMeshTasksIndirect.m_Offset },
                { "drawCount", drawcall.m_Payload.m_DrawMeshTasksIndirect.m_DrawCount },
                { "stride", drawcall.m_Payload.m_DrawMeshTasksIndirect.m_Stride }
            };

        case DeviceProfilerDrawcallType::eDrawMeshTasksIndirectCount:
            return {
                { "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_Buffer ) },
                { "offset", drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_Offset },
                { "countBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_CountBuffer ) },
                { "countOffset", drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_CountOffset },
                { "maxDrawCount", drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_MaxDrawCount },
                { "stride", drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_Stride }
            };

        case DeviceProfilerDrawcallType::eDrawMeshTasksNV:
            return {
                { "taskCount", drawcall.m_Payload.m_DrawMeshTasksNV.m_TaskCount },
                { "firstTask", drawcall.m_Payload.m_DrawMeshTasksNV.m_FirstTask }
            };

        case DeviceProfilerDrawcallType::eDrawMeshTasksIndirectNV:
            return {
                { "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawMeshTasksIndirectNV.m_Buffer ) },
                { "offset", drawcall.m_Payload.m_DrawMeshTasksIndirectNV.m_Offset },
                { "drawCount", drawcall.m_Payload.m_DrawMeshTasksIndirectNV.m_DrawCount },
                { "stride", drawcall.m_Payload.m_DrawMeshTasksIndirectNV.m_Stride }
            };

        case DeviceProfilerDrawcallType::eDrawMeshTasksIndirectCountNV:
            return {
                { "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_Buffer ) },
                { "offset", drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_Offset },
                { "countBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_CountBuffer ) },
                { "countOffset", drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_CountOffset },
                { "maxDrawCount", drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_MaxDrawCount },
                { "stride", drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_Stride }
            };

        case DeviceProfilerDrawcallType::eDispatch:
            return {
                { "groupCountX", drawcall.m_Payload.m_Dispatch.m_GroupCountX },
                { "groupCountY", drawcall.m_Payload.m_Dispatch.m_GroupCountY },
                { "groupCountZ", drawcall.m_Payload.m_Dispatch.m_GroupCountZ } };

        case DeviceProfilerDrawcallType::eDispatchIndirect:
            return {
                { "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DispatchIndirect.m_Buffer ) },
                { "offset", drawcall.m_Payload.m_DispatchIndirect.m_Offset } };

        case DeviceProfilerDrawcallType::eCopyBuffer:
            return {
                { "srcBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyBuffer.m_SrcBuffer ) },
                { "dstBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyBuffer.m_DstBuffer ) } };

        case DeviceProfilerDrawcallType::eCopyBufferToImage:
            return {
                { "srcBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyBufferToImage.m_SrcBuffer ) },
                { "dstImage", m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyBufferToImage.m_DstImage ) } };

        case DeviceProfilerDrawcallType::eCopyImage:
            return {
                { "srcImage", m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyImage.m_SrcImage ) },
                { "dstImage", m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyImage.m_DstImage ) } };

        case DeviceProfilerDrawcallType::eCopyImageToBuffer:
            return {
                { "srcImage", m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyImageToBuffer.m_SrcImage ) },
                { "dstBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyImageToBuffer.m_DstBuffer ) } };

        case DeviceProfilerDrawcallType::eClearAttachments:
            return {
                { "attachmentCount", drawcall.m_Payload.m_ClearAttachments.m_Count } };

        case DeviceProfilerDrawcallType::eClearColorImage:
            return {
                { "image", m_pStringSerializer->GetName( drawcall.m_Payload.m_ClearColorImage.m_Image ) },
                { "value", GetColorClearValue( drawcall.m_Payload.m_ClearColorImage.m_Value ) } };

        case DeviceProfilerDrawcallType::eClearDepthStencilImage:
            return {
                { "image", m_pStringSerializer->GetName( drawcall.m_Payload.m_ClearDepthStencilImage.m_Image ) },
                { "value", GetDepthStencilClearValue( drawcall.m_Payload.m_ClearDepthStencilImage.m_Value ) } };

        case DeviceProfilerDrawcallType::eResolveImage:
            return {
                { "srcImage", m_pStringSerializer->GetName( drawcall.m_Payload.m_ResolveImage.m_SrcImage ) },
                { "dstImage", m_pStringSerializer->GetName( drawcall.m_Payload.m_ResolveImage.m_DstImage ) } };

        case DeviceProfilerDrawcallType::eBlitImage:
            return {
                { "srcImage", m_pStringSerializer->GetName( drawcall.m_Payload.m_BlitImage.m_SrcImage ) },
                { "dstImage", m_pStringSerializer->GetName( drawcall.m_Payload.m_BlitImage.m_DstImage ) } };

        case DeviceProfilerDrawcallType::eFillBuffer:
            return {
                { "dstBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_FillBuffer.m_Buffer ) },
                { "dstOffset", drawcall.m_Payload.m_FillBuffer.m_Offset },
                { "size", drawcall.m_Payload.m_FillBuffer.m_Size },
                { "data", drawcall.m_Payload.m_FillBuffer.m_Data } };

        case DeviceProfilerDrawcallType::eUpdateBuffer:
            return {
                { "dstBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_UpdateBuffer.m_Buffer ) },
                { "dstOffset", drawcall.m_Payload.m_UpdateBuffer.m_Offset },
                { "dataSize", drawcall.m_Payload.m_UpdateBuffer.m_Size } };

        case DeviceProfilerDrawcallType::eTraceRaysKHR:
            return {
                { "width", drawcall.m_Payload.m_TraceRays.m_Width },
                { "height", drawcall.m_Payload.m_TraceRays.m_Height },
                { "depth", drawcall.m_Payload.m_TraceRays.m_Depth } };

        case DeviceProfilerDrawcallType::eTraceRaysIndirectKHR:
            return {
                { "indirectDeviceAddress", drawcall.m_Payload.m_TraceRaysIndirect.m_IndirectAddress } };

        case DeviceProfilerDrawcallType::eTraceRaysIndirect2KHR:
            return {
                { "indirectDeviceAddress", drawcall.m_Payload.m_TraceRaysIndirect2.m_IndirectAddress } };

        case DeviceProfilerDrawcallType::eBuildAccelerationStructuresKHR:
        case DeviceProfilerDrawcallType::eBuildAccelerationStructuresIndirectKHR:
        {
            const uint32_t infoCount = drawcall.m_Payload.m_BuildAccelerationStructures.m_InfoCount;

            std::vector<nlohmann::json> infos;
            infos.reserve( infoCount );

            for( uint32_t i = 0; i < infoCount; ++i )
            {
                const auto& info = drawcall.m_Payload.m_BuildAccelerationStructures.m_pInfos[ i ];

                std::vector<nlohmann::json> geometries;
                if( info.pGeometries )
                {
                    geometries.reserve( info.geometryCount );
                    for( uint32_t j = 0; j < info.geometryCount; ++j )
                    {
                        const auto& geometry = info.pGeometries[ j ];
                        nlohmann::json geometryData;

                        switch( geometry.geometryType )
                        {
                        case VK_GEOMETRY_TYPE_TRIANGLES_KHR:
                            geometryData = {
                                { "vertexFormat", m_pStringSerializer->GetFormatName( geometry.geometry.triangles.vertexFormat ) },
                                { "vertexData", m_pStringSerializer->GetPointer( geometry.geometry.triangles.vertexData.hostAddress ) },
                                { "vertexStride", geometry.geometry.triangles.vertexStride },
                                { "maxVertex", geometry.geometry.triangles.maxVertex },
                                { "indexType", m_pStringSerializer->GetIndexTypeName( geometry.geometry.triangles.indexType ) },
                                { "indexData", m_pStringSerializer->GetPointer( geometry.geometry.triangles.indexData.hostAddress ) },
                                { "transformData", m_pStringSerializer->GetPointer( geometry.geometry.triangles.transformData.hostAddress ) } };
                            break;

                        case VK_GEOMETRY_TYPE_AABBS_KHR:
                            geometryData = {
                                { "data", m_pStringSerializer->GetPointer( geometry.geometry.aabbs.data.hostAddress ) },
                                { "stride", geometry.geometry.aabbs.stride } };
                            break;

                        case VK_GEOMETRY_TYPE_INSTANCES_KHR:
                            geometryData = {
                                { "arrayOfPointers", static_cast<bool>( geometry.geometry.instances.arrayOfPointers ) },
                                { "data", m_pStringSerializer->GetPointer( geometry.geometry.instances.data.hostAddress ) } };
                            break;
                        }

                        nlohmann::json geometryRange;
                        if( drawcall.m_Type == DeviceProfilerDrawcallType::eBuildAccelerationStructuresKHR )
                        {
                            const auto& range = drawcall.m_Payload.m_BuildAccelerationStructures.m_ppRanges[ i ][ j ];
                            geometryRange = {
                                { "primitiveCount", range.primitiveCount },
                                { "primitiveOffset", range.primitiveOffset },
                                { "firstVertex", range.firstVertex },
                                { "transformOffset", range.transformOffset } };
                        }
                        else //( drawcall.m_Type == DeviceProfilerDrawcallType::eBuildAccelerationStructuresIndirectKHR )
                        {
                            geometryRange = {
                                { "maxPrimitiveCount", drawcall.m_Payload.m_BuildAccelerationStructuresIndirect.m_ppMaxPrimitiveCounts[ i ][ j ] } };
                        }

                        geometries.push_back({
                            { "type", m_pStringSerializer->GetGeometryTypeName( geometry.geometryType ) },
                            { "flags", m_pStringSerializer->GetGeometryFlagNames( geometry.flags ) },
                            { "data", geometryData },
                            { "range", geometryRange } });
                    }
                }

                infos.push_back({
                    { "type", m_pStringSerializer->GetAccelerationStructureTypeName( info.type ) },
                    { "flags", m_pStringSerializer->GetBuildAccelerationStructureFlagNames( info.flags ) },
                    { "mode", m_pStringSerializer->GetBuildAccelerationStructureModeName( info.mode ) },
                    { "src", m_pStringSerializer->GetName( VkAccelerationStructureKHRHandle( info.srcAccelerationStructure ) ) },
                    { "dst", m_pStringSerializer->GetName( VkAccelerationStructureKHRHandle( info.dstAccelerationStructure ) ) },
                    { "geometryCount", info.geometryCount },
                    { "geometries", geometries } });
            }

            return {
                { "infoCount", infoCount },
                { "infos", infos } };
        }

        case DeviceProfilerDrawcallType::eBuildMicromapsEXT:
        {
            const uint32_t infoCount = drawcall.m_Payload.m_BuildMicromaps.m_InfoCount;

            std::vector<nlohmann::json> infos;
            infos.reserve( infoCount );

            for( uint32_t i = 0; i < infoCount; ++i )
            {
                const auto& info = drawcall.m_Payload.m_BuildMicromaps.m_pInfos[i];

                std::vector<nlohmann::json> usageCounts;
                usageCounts.reserve( info.usageCountsCount );

                for( uint32_t j = 0; j < info.usageCountsCount; ++j )
                {
                    const auto& usageCount = info.pUsageCounts[j];
                    usageCounts.push_back( {
                        { "count", usageCount.count },
                        { "format", usageCount.format },
                        { "subdivisionLevel", usageCount.subdivisionLevel } } );
                }

                infos.push_back( {
                    { "type", m_pStringSerializer->GetMicromapTypeName( info.type ) },
                    { "flags", m_pStringSerializer->GetBuildMicromapFlagNames( info.flags ) },
                    { "mode", m_pStringSerializer->GetBuildMicromapModeName( info.mode ) },
                    { "dst", m_pStringSerializer->GetName( VkMicromapEXTHandle( info.dstMicromap ) ) },
                    { "usageCountsCount", info.usageCountsCount },
                    { "usageCounts", usageCounts },
                    { "data", m_pStringSerializer->GetPointer( info.data.hostAddress ) },
                    { "scratchData", m_pStringSerializer->GetPointer( info.scratchData.hostAddress ) },
                    { "triangleArray", m_pStringSerializer->GetPointer( info.triangleArray.hostAddress ) },
                    { "triangleArrayStride", info.triangleArrayStride } } );
            }

            return {
                { "infoCount", infoCount },
                { "infos", infos } };
        }
        }
    }

    /*************************************************************************\

    Function:
        GetPipelineArgs

    Description:
        Serialize pipeline state into JSON object.

    \*************************************************************************/
    nlohmann::json DeviceProfilerJsonSerializer::GetPipelineArgs( const DeviceProfilerPipeline& pipeline ) const
    {
        nlohmann::json args = {};

        // Append shader stages info.
        if( !pipeline.m_ShaderTuple.m_Shaders.empty() )
        {
            nlohmann::json& shaderStages = args["shaders"];

            for( const ProfilerShader& shader : pipeline.m_ShaderTuple.m_Shaders )
            {
                shaderStages.push_back( GetShaderStageArgs( shader ) );
            }
        }

        // Append pipeline create info details.
        if( pipeline.m_pCreateInfo )
        {
            switch( pipeline.m_Type )
            {
            case DeviceProfilerPipelineType::eGraphics:
            {
                args.update( GetGraphicsPipelineCreateInfoArgs(
                    pipeline.m_pCreateInfo->m_GraphicsPipelineCreateInfo ) );
                break;
            }
            case DeviceProfilerPipelineType::eCompute:
            {
                args.update( GetComputePipelineCreateInfoArgs(
                    pipeline.m_pCreateInfo->m_ComputePipelineCreateInfo ) );
                break;
            }
            case DeviceProfilerPipelineType::eRayTracingKHR:
            {
                args.update( GetRayTracingPipelineCreateInfoArgs(
                    pipeline.m_pCreateInfo->m_RayTracingPipelineCreateInfoKHR ) );
                break;
            }
            }
        }

        return args;
    }

    /*************************************************************************\

    Function:
        GetColorClearValue

    Description:
        Serialize VkClearColorValue struct into JSON object.

    \*************************************************************************/
    nlohmann::json DeviceProfilerJsonSerializer::GetColorClearValue( const VkClearColorValue& value ) const
    {
        return { "VkClearColorValue", {
            { "float32", {
                value.float32[ 0 ],
                value.float32[ 1 ],
                value.float32[ 2 ],
                value.float32[ 3 ] } },
            { "int32", {
                value.int32[ 0 ],
                value.int32[ 1 ],
                value.int32[ 2 ],
                value.int32[ 3 ] } },
            { "uint32", {
                value.uint32[ 0 ],
                value.uint32[ 1 ],
                value.uint32[ 2 ],
                value.uint32[ 3 ] } } } };
    }

    /*************************************************************************\

    Function:
        GetDepthStencilClearValue

    Description:
        Serialize VkClearDepthStencilValue struct into JSON object.

    \*************************************************************************/
    nlohmann::json DeviceProfilerJsonSerializer::GetDepthStencilClearValue( const VkClearDepthStencilValue& value ) const
    {
        return { "VkClearDepthStencilValue", {
            { "depth", value.depth },
            { "stencil", value.stencil } } };
    }

    /*************************************************************************\

    Function:
        GetShaderStageArgs

    Description:
        Serialize shader stage into JSON object.

    \*************************************************************************/
    nlohmann::json DeviceProfilerJsonSerializer::GetShaderStageArgs( const ProfilerShader& shader ) const
    {
        nlohmann::json shaderStage = {
            { "stage", m_pStringSerializer->GetShaderStageName( shader.m_Stage ) },
            { "entryPoint", shader.m_EntryPoint }
        };

        if( shader.m_pShaderModule )
        {
            static constexpr char hexDigits[] = "0123456789abcdef";

            const ProfilerShaderModule& shaderModule = *shader.m_pShaderModule;
            const uint32_t identifierSize = shaderModule.m_IdentifierSize;
            const uint8_t* pIdentifier = shaderModule.m_Identifier;

            std::string shaderIdentifier;
            shaderIdentifier.reserve( identifierSize * 3 );

            // Convert from the end to keep the little-endian order.
            for( uint32_t i = identifierSize; i > 0; --i )
            {
                shaderIdentifier.push_back( hexDigits[pIdentifier[i - 1] >> 4] );
                shaderIdentifier.push_back( hexDigits[pIdentifier[i - 1] & 0xF] );

                if( ( i != 1 ) && ( ( i - 1 ) % 8 ) == 0 )
                {
                    // Insert a dash separator every 8 bytes for better readability.
                    shaderIdentifier.push_back( '-' );
                }
            }

            shaderStage["shaderIdentifier"] = shaderIdentifier;
        }

        return shaderStage;
    }

    /*************************************************************************\

    Function:
        GetGraphicsPipelineCreateInfoArgs

    Description:
        Serialize graphics pipeline state into JSON object.

    \*************************************************************************/
    nlohmann::json DeviceProfilerJsonSerializer::GetGraphicsPipelineCreateInfoArgs( const VkGraphicsPipelineCreateInfo& createInfo ) const
    {
        nlohmann::json args = {};
        args["vertexInputState"] = nullptr;
        args["inputAssemblyState"] = nullptr;
        args["tessellationState"] = nullptr;
        args["viewportState"] = nullptr;
        args["rasterizationState"] = nullptr;
        args["multisampleState"] = nullptr;
        args["depthStencilState"] = nullptr;
        args["colorBlendState"] = nullptr;
        args["dynamicStates"] = nlohmann::json::array();

        // VkPipelineVertexInputStateCreateInfo
        if( createInfo.pVertexInputState )
        {
            const auto& state = *createInfo.pVertexInputState;
            args["vertexInputState"] = {
                { "attributeCount", state.vertexAttributeDescriptionCount },
                { "attributes", nullptr },
                { "bindingCount", state.vertexBindingDescriptionCount },
                { "bindings", nullptr }
            };

            if( state.pVertexAttributeDescriptions )
            {
                auto& attributesArgs = args["vertexInputState"]["attributes"] = nlohmann::json::array();
                for( uint32_t i = 0; i < state.vertexAttributeDescriptionCount; ++i )
                {
                    const auto& attribute = state.pVertexAttributeDescriptions[i];
                    attributesArgs.push_back( {
                        { "location", attribute.location },
                        { "binding", attribute.binding },
                        { "format", m_pStringSerializer->GetFormatName( attribute.format ) },
                        { "offset", attribute.offset } } );
                }
            }

            if( state.pVertexBindingDescriptions )
            {
                auto& bindingsArgs = args["vertexInputState"]["bindings"] = nlohmann::json::array();
                for( uint32_t i = 0; i < state.vertexBindingDescriptionCount; ++i )
                {
                    const auto& binding = state.pVertexBindingDescriptions[i];
                    bindingsArgs.push_back( {
                        { "binding", binding.binding },
                        { "stride", binding.stride },
                        { "inputRate", m_pStringSerializer->GetVertexInputRateName( binding.inputRate ) } } );
                }
            }
        }

        // VkPipelineInputAssemblyStateCreateInfo
        if( createInfo.pInputAssemblyState )
        {
            const auto& state = *createInfo.pInputAssemblyState;
            args["inputAssemblyState"] = {
                { "topology", m_pStringSerializer->GetPrimitiveTopologyName( state.topology ) },
                { "primitiveRestartEnable", static_cast<bool>( state.primitiveRestartEnable ) }
            };
        }

        // VkPipelineTessellationStateCreateInfo
        if( createInfo.pTessellationState )
        {
            const auto& state = *createInfo.pTessellationState;
            args["tessellationState"] = {
                { "patchControlPoints", state.patchControlPoints }
            };
        }

        // VkPipelineViewportStateCreateInfo
        if( createInfo.pViewportState )
        {
            const auto& state = *createInfo.pViewportState;
            args["viewportState"] = {
                { "viewportCount", state.viewportCount },
                { "viewports", nullptr },
                { "scissorCount", state.scissorCount },
                { "scissors", nullptr }
            };

            if( state.pViewports )
            {
                auto& viewportsArgs = args["viewportState"]["viewports"] = nlohmann::json::array();
                for( uint32_t i = 0; i < state.viewportCount; ++i )
                {
                    const auto& viewport = state.pViewports[i];
                    viewportsArgs.push_back( {
                        { "x", viewport.x },
                        { "y", viewport.y },
                        { "width", viewport.width },
                        { "height", viewport.height },
                        { "minDepth", viewport.minDepth },
                        { "maxDepth", viewport.maxDepth } } );
                }
            }

            if( state.pScissors )
            {
                auto& scissorsArgs = args["viewportState"]["scissors"] = nlohmann::json::array();
                for( uint32_t i = 0; i < state.scissorCount; ++i )
                {
                    const auto& scissor = state.pScissors[i];
                    scissorsArgs.push_back( {
                        { "offsetX", scissor.offset.x },
                        { "offsetY", scissor.offset.y },
                        { "extentWidth", scissor.extent.width },
                        { "extentHeight", scissor.extent.height } } );
                }
            }
        }

        // VkPipelineRasterizationStateCreateInfo
        if( createInfo.pRasterizationState )
        {
            const auto& state = *createInfo.pRasterizationState;
            args["rasterizationState"] = {
                { "depthClampEnable", static_cast<bool>( state.depthClampEnable ) },
                { "rasterizerDiscardEnable", static_cast<bool>( state.rasterizerDiscardEnable ) },
                { "polygonMode", m_pStringSerializer->GetPolygonModeName( state.polygonMode ) },
                { "cullMode", m_pStringSerializer->GetCullModeName( state.cullMode ) },
                { "frontFace", m_pStringSerializer->GetFrontFaceName( state.frontFace ) },
                { "depthBiasEnable", static_cast<bool>( state.depthBiasEnable ) },
                { "depthBiasConstantFactor", state.depthBiasConstantFactor },
                { "depthBiasClamp", state.depthBiasClamp },
                { "depthBiasSlopeFactor", state.depthBiasSlopeFactor },
                { "lineWidth", state.lineWidth }
            };
        }

        // VkPipelineMultisampleStateCreateInfo
        if( createInfo.pMultisampleState )
        {
            const auto& state = *createInfo.pMultisampleState;
            args["multisampleState"] = {
                { "rasterizationSamples", state.rasterizationSamples },
                { "sampleShadingEnable", static_cast<bool>( state.sampleShadingEnable ) },
                { "minSampleShading", state.minSampleShading },
                { "sampleMask", fmt::format( "0x{:08X}", state.pSampleMask ? *state.pSampleMask : 0xFFFFFFFF ) },
                { "alphaToCoverateEnable", static_cast<bool>( state.alphaToCoverageEnable ) },
                { "alphaToOneEnable", static_cast<bool>( state.alphaToOneEnable ) }
            };
        }

        // VkPipelineDepthStencilStateCreateInfo
        if( createInfo.pDepthStencilState )
        {
            const auto& state = *createInfo.pDepthStencilState;
            args["depthStencilState"] = {
                { "depthTestEnable", static_cast<bool>( state.depthTestEnable ) },
                { "depthWriteEnable", static_cast<bool>( state.depthWriteEnable ) },
                { "depthCompareOp", m_pStringSerializer->GetCompareOpName( state.depthCompareOp ) },
                { "depthBoundsTestEnable", static_cast<bool>( state.depthBoundsTestEnable ) },
                { "minDepthBounds", state.minDepthBounds },
                { "maxDepthBounds", state.maxDepthBounds },
                { "stencilTestEnable", static_cast<bool>( state.stencilTestEnable ) },
                { "front", {
                    { "failOp", state.front.failOp },
                    { "passOp", state.front.passOp },
                    { "depthFailOp", state.front.depthFailOp },
                    { "compareOp", m_pStringSerializer->GetCompareOpName( state.front.compareOp ) },
                    { "compareMask", fmt::format( "0x{:02X}", state.front.compareMask ) },
                    { "writeMask", fmt::format( "0x{:02X}", state.front.writeMask ) },
                    { "reference", fmt::format( "0x{:02X}", state.front.reference ) } } },
                { "back", {
                    { "failOp", state.back.failOp },
                    { "passOp", state.back.passOp },
                    { "depthFailOp", state.back.depthFailOp },
                    { "compareOp", m_pStringSerializer->GetCompareOpName( state.back.compareOp ) },
                    { "compareMask", fmt::format( "0x{:02X}", state.back.compareMask ) },
                    { "writeMask", fmt::format( "0x{:02X}", state.back.writeMask ) },
                    { "reference", fmt::format( "0x{:02X}", state.back.reference ) } } }
            };
        }

        // VkPipelineColorBlendStateCreateInfo
        if( createInfo.pColorBlendState )
        {
            const auto& state = *createInfo.pColorBlendState;
            args["colorBlendState"] = {
                { "logicOpEnable", static_cast<bool>( state.logicOpEnable ) },
                { "logicOp", m_pStringSerializer->GetLogicOpName( state.logicOp ) },
                { "blendConstants", state.blendConstants },
                { "attachments", nullptr }
            };

            if( state.pAttachments )
            {
                auto& attachmentsArgs = args["colorBlendState"]["attachments"] = nlohmann::json::array();
                for( uint32_t i = 0; i < state.attachmentCount; ++i )
                {
                    const auto& attachment = state.pAttachments[i];
                    attachmentsArgs.push_back( {
                        { "blendEnable", static_cast<bool>( attachment.blendEnable ) },
                        { "srcColorBlendFactor", m_pStringSerializer->GetBlendFactorName( attachment.srcColorBlendFactor ) },
                        { "dstColorBlendFactor", m_pStringSerializer->GetBlendFactorName( attachment.dstColorBlendFactor ) },
                        { "colorBlendOp", m_pStringSerializer->GetBlendOpName( attachment.colorBlendOp ) },
                        { "srcAlphaBlendFactor", m_pStringSerializer->GetBlendFactorName( attachment.srcAlphaBlendFactor ) },
                        { "dstAlphaBlendFactor", m_pStringSerializer->GetBlendFactorName( attachment.dstAlphaBlendFactor ) },
                        { "alphaBlendOp", m_pStringSerializer->GetBlendOpName( attachment.alphaBlendOp ) },
                        { "colorWriteMask", m_pStringSerializer->GetColorComponentFlagNames( attachment.colorWriteMask ) } } );
                }
            }
        }

        // VkPipelineDynamicStateCreateInfo
        if( createInfo.pDynamicState )
        {
            const auto& state = *createInfo.pDynamicState;
            for( uint32_t i = 0; i < state.dynamicStateCount; ++i )
            {
                args["dynamicStates"].push_back(
                    m_pStringSerializer->GetDynamicStateName( state.pDynamicStates[i] ) );
            }
        }

        return args;
    }

    /*************************************************************************\

    Function:
        GetComputePipelineCreateInfoArgs

    Description:
        Serialize compute pipeline state into JSON object.

    \*************************************************************************/
    nlohmann::json DeviceProfilerJsonSerializer::GetComputePipelineCreateInfoArgs( const VkComputePipelineCreateInfo& createInfo ) const
    {
        nlohmann::json args = {};

        // No additional state to serialize for compute pipelines yet.

        return args;
    }

    /*************************************************************************\

    Function:
        GetRayTracingPipelineCreateInfoArgs

    Description:
        Serialize ray-tracing pipeline state into JSON object.

    \*************************************************************************/
    nlohmann::json DeviceProfilerJsonSerializer::GetRayTracingPipelineCreateInfoArgs( const VkRayTracingPipelineCreateInfoKHR& createInfo ) const
    {
        nlohmann::json args = {};
        args["maxPipelineRayRecursionDepth"] = createInfo.maxPipelineRayRecursionDepth;
        args["libraryInterface"] = nullptr;
        args["dynamicStates"] = nlohmann::json::array();

        // VkRayTracingPipelineInterfaceCreateInfoKHR
        if( createInfo.pLibraryInterface )
        {
            const auto& state = *createInfo.pLibraryInterface;
            args["libraryInterface"] = {
                { "maxPipelineRayPayloadSize", state.maxPipelineRayPayloadSize },
                { "maxPipelineRayHitAttributeSize", state.maxPipelineRayHitAttributeSize }
            };
        }

        // VkPipelineDynamicStateCreateInfo
        if( createInfo.pDynamicState )
        {
            const auto& state = *createInfo.pDynamicState;
            for( uint32_t i = 0; i < state.dynamicStateCount; ++i )
            {
                args["dynamicStates"].push_back(
                    m_pStringSerializer->GetDynamicStateName( state.pDynamicStates[i] ) );
            }
        }

        return args;
    }
}

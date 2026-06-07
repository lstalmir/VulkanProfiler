// Copyright (c) 2019-2026 Lukasz Stalmirski
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
    void DeviceProfilerJsonSerializer::WriteCommandArgs( DeviceProfilerJsonValueBuilder& builder, const DeviceProfilerDrawcall& drawcall ) const
    {
        auto argsBuilder = builder.MakeObject();

        switch( drawcall.m_Type )
        {
        default:
        case DeviceProfilerDrawcallType::eUnknown:
        case DeviceProfilerDrawcallType::eInsertDebugLabel:
        case DeviceProfilerDrawcallType::eBeginDebugLabel:
        case DeviceProfilerDrawcallType::eEndDebugLabel:
            break;

        case DeviceProfilerDrawcallType::eDraw:
            argsBuilder
                .Add( "vertexCount", drawcall.m_Payload.m_Draw.m_VertexCount )
                .Add( "instanceCount", drawcall.m_Payload.m_Draw.m_InstanceCount )
                .Add( "firstVertex", drawcall.m_Payload.m_Draw.m_FirstVertex )
                .Add( "firstInstance", drawcall.m_Payload.m_Draw.m_FirstInstance );
            break;

        case DeviceProfilerDrawcallType::eDrawIndexed:
            argsBuilder
                .Add( "indexCount", drawcall.m_Payload.m_DrawIndexed.m_IndexCount )
                .Add( "instanceCount", drawcall.m_Payload.m_DrawIndexed.m_InstanceCount )
                .Add( "firstIndex", drawcall.m_Payload.m_DrawIndexed.m_FirstIndex )
                .Add( "vertexOffset", drawcall.m_Payload.m_DrawIndexed.m_VertexOffset )
                .Add( "firstInstance", drawcall.m_Payload.m_DrawIndexed.m_FirstInstance );
            break;

        case DeviceProfilerDrawcallType::eDrawIndirect:
            argsBuilder
                .Add( "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawIndirect.m_Buffer ) )
                .Add( "offset", drawcall.m_Payload.m_DrawIndirect.m_Offset )
                .Add( "drawCount", drawcall.m_Payload.m_DrawIndirect.m_DrawCount )
                .Add( "stride", drawcall.m_Payload.m_DrawIndirect.m_Stride );
            break;

        case DeviceProfilerDrawcallType::eDrawIndexedIndirect:
            argsBuilder
                .Add( "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawIndexedIndirect.m_Buffer ) )
                .Add( "offset", drawcall.m_Payload.m_DrawIndexedIndirect.m_Offset )
                .Add( "drawCount", drawcall.m_Payload.m_DrawIndexedIndirect.m_DrawCount )
                .Add( "stride", drawcall.m_Payload.m_DrawIndexedIndirect.m_Stride );
            break;

        case DeviceProfilerDrawcallType::eDrawIndirectCount:
            argsBuilder
                .Add( "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawIndirectCount.m_Buffer ) )
                .Add( "offset", drawcall.m_Payload.m_DrawIndirectCount.m_Offset )
                .Add( "countBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawIndirectCount.m_CountBuffer ) )
                .Add( "countOffset", drawcall.m_Payload.m_DrawIndirectCount.m_CountOffset )
                .Add( "maxDrawCount", drawcall.m_Payload.m_DrawIndirectCount.m_MaxDrawCount )
                .Add( "stride", drawcall.m_Payload.m_DrawIndirectCount.m_Stride );
            break;

        case DeviceProfilerDrawcallType::eDrawIndexedIndirectCount:
            argsBuilder
                .Add( "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawIndexedIndirectCount.m_Buffer ) )
                .Add( "offset", drawcall.m_Payload.m_DrawIndexedIndirectCount.m_Offset )
                .Add( "countBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawIndexedIndirectCount.m_CountBuffer ) )
                .Add( "countOffset", drawcall.m_Payload.m_DrawIndexedIndirectCount.m_CountOffset )
                .Add( "maxDrawCount", drawcall.m_Payload.m_DrawIndexedIndirectCount.m_MaxDrawCount )
                .Add( "stride", drawcall.m_Payload.m_DrawIndexedIndirectCount.m_Stride );
            break;

        case DeviceProfilerDrawcallType::eDrawMeshTasks:
            argsBuilder
                .Add( "groupCountX", drawcall.m_Payload.m_DrawMeshTasks.m_GroupCountX )
                .Add( "groupCountY", drawcall.m_Payload.m_DrawMeshTasks.m_GroupCountY )
                .Add( "groupCountZ", drawcall.m_Payload.m_DrawMeshTasks.m_GroupCountZ );
            break;

        case DeviceProfilerDrawcallType::eDrawMeshTasksIndirect:
            argsBuilder
                .Add( "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawMeshTasksIndirect.m_Buffer ) )
                .Add( "offset", drawcall.m_Payload.m_DrawMeshTasksIndirect.m_Offset )
                .Add( "drawCount", drawcall.m_Payload.m_DrawMeshTasksIndirect.m_DrawCount )
                .Add( "stride", drawcall.m_Payload.m_DrawMeshTasksIndirect.m_Stride );
            break;

        case DeviceProfilerDrawcallType::eDrawMeshTasksIndirectCount:
            argsBuilder
                .Add( "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_Buffer ) )
                .Add( "offset", drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_Offset )
                .Add( "countBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_CountBuffer ) )
                .Add( "countOffset", drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_CountOffset )
                .Add( "maxDrawCount", drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_MaxDrawCount )
                .Add( "stride", drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_Stride );
            break;

        case DeviceProfilerDrawcallType::eDrawMeshTasksNV:
            argsBuilder
                .Add( "taskCount", drawcall.m_Payload.m_DrawMeshTasksNV.m_TaskCount )
                .Add( "firstTask", drawcall.m_Payload.m_DrawMeshTasksNV.m_FirstTask );
            break;

        case DeviceProfilerDrawcallType::eDrawMeshTasksIndirectNV:
            argsBuilder
                .Add( "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawMeshTasksIndirectNV.m_Buffer ) )
                .Add( "offset", drawcall.m_Payload.m_DrawMeshTasksIndirectNV.m_Offset )
                .Add( "drawCount", drawcall.m_Payload.m_DrawMeshTasksIndirectNV.m_DrawCount )
                .Add( "stride", drawcall.m_Payload.m_DrawMeshTasksIndirectNV.m_Stride );
            break;

        case DeviceProfilerDrawcallType::eDrawMeshTasksIndirectCountNV:
            argsBuilder
                .Add( "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_Buffer ) )
                .Add( "offset", drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_Offset )
                .Add( "countBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_CountBuffer ) )
                .Add( "countOffset", drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_CountOffset )
                .Add( "maxDrawCount", drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_MaxDrawCount )
                .Add( "stride", drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_Stride );
            break;

        case DeviceProfilerDrawcallType::eDrawMulti:
        {
            argsBuilder
                .Add( "drawCount", drawcall.m_Payload.m_DrawMulti.m_DrawCount )
                .Add( "instanceCount", drawcall.m_Payload.m_DrawMulti.m_InstanceCount )
                .Add( "firstInstance", drawcall.m_Payload.m_DrawMulti.m_FirstInstance )
                .Add( "stride", drawcall.m_Payload.m_DrawMulti.m_Stride );

            auto vertexInfosBuilder = argsBuilder.AddArray( "vertexInfos" );
            for( uint32_t i = 0; i < drawcall.m_Payload.m_DrawMulti.m_DrawCount; i++ )
            {
                const auto& vertexInfo = drawcall.m_Payload.m_DrawMulti.m_pVertexInfo[i];
                vertexInfosBuilder.AddObject()
                    .Add( "firstVertex", vertexInfo.firstVertex )
                    .Add( "vertexCount", vertexInfo.vertexCount );
            }

            vertexInfosBuilder.End();
            break;
        }

        case DeviceProfilerDrawcallType::eDrawMultiIndexed:
        {
            argsBuilder
                .Add( "drawCount", drawcall.m_Payload.m_DrawMultiIndexed.m_DrawCount )
                .Add( "instanceCount", drawcall.m_Payload.m_DrawMultiIndexed.m_InstanceCount )
                .Add( "firstInstance", drawcall.m_Payload.m_DrawMultiIndexed.m_FirstInstance )
                .Add( "stride", drawcall.m_Payload.m_DrawMultiIndexed.m_Stride );

            auto indexInfosBuilder = argsBuilder.AddArray( "indexInfos" );
            for( uint32_t i = 0; i < drawcall.m_Payload.m_DrawMultiIndexed.m_DrawCount; i++ )
            {
                const auto& indexInfo = drawcall.m_Payload.m_DrawMultiIndexed.m_pIndexInfo[i];
                indexInfosBuilder.AddObject()
                    .Add( "firstIndex", indexInfo.firstIndex )
                    .Add( "indexCount", indexInfo.indexCount )
                    .Add( "vertexOffset", indexInfo.vertexOffset );
            }

            indexInfosBuilder.End();
            break;
        }

        case DeviceProfilerDrawcallType::eDispatch:
            argsBuilder
                .Add( "groupCountX", drawcall.m_Payload.m_Dispatch.m_GroupCountX )
                .Add( "groupCountY", drawcall.m_Payload.m_Dispatch.m_GroupCountY )
                .Add( "groupCountZ", drawcall.m_Payload.m_Dispatch.m_GroupCountZ );
            break;

        case DeviceProfilerDrawcallType::eDispatchIndirect:
            argsBuilder
                .Add( "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DispatchIndirect.m_Buffer ) )
                .Add( "offset", drawcall.m_Payload.m_DispatchIndirect.m_Offset );
            break;

        case DeviceProfilerDrawcallType::eCopyBuffer:
            argsBuilder
                .Add( "srcBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyBuffer.m_SrcBuffer ) )
                .Add( "dstBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyBuffer.m_DstBuffer ) );
            break;

        case DeviceProfilerDrawcallType::eCopyBufferToImage:
            argsBuilder
                .Add( "srcBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyBufferToImage.m_SrcBuffer ) )
                .Add( "dstImage", m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyBufferToImage.m_DstImage ) );
            break;

        case DeviceProfilerDrawcallType::eCopyImage:
            argsBuilder
                .Add( "srcImage", m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyImage.m_SrcImage ) )
                .Add( "dstImage", m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyImage.m_DstImage ) );
            break;

        case DeviceProfilerDrawcallType::eCopyImageToBuffer:
            argsBuilder
                .Add( "srcImage", m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyImageToBuffer.m_SrcImage ) )
                .Add( "dstBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyImageToBuffer.m_DstBuffer ) );
            break;

        case DeviceProfilerDrawcallType::eClearAttachments:
            argsBuilder
                .Add( "attachmentCount", drawcall.m_Payload.m_ClearAttachments.m_Count );
            break;

        case DeviceProfilerDrawcallType::eClearColorImage:
            argsBuilder
                .Add( "image", m_pStringSerializer->GetName( drawcall.m_Payload.m_ClearColorImage.m_Image ) );
            WriteColorClearValue( argsBuilder.Add( "value" ), drawcall.m_Payload.m_ClearColorImage.m_Value );
            break;

        case DeviceProfilerDrawcallType::eClearDepthStencilImage:
            argsBuilder
                .Add( "image", m_pStringSerializer->GetName( drawcall.m_Payload.m_ClearDepthStencilImage.m_Image ) );
            WriteDepthStencilClearValue( argsBuilder.Add( "value" ), drawcall.m_Payload.m_ClearDepthStencilImage.m_Value );
            break;

        case DeviceProfilerDrawcallType::eResolveImage:
            argsBuilder
                .Add( "srcImage", m_pStringSerializer->GetName( drawcall.m_Payload.m_ResolveImage.m_SrcImage ) )
                .Add( "dstImage", m_pStringSerializer->GetName( drawcall.m_Payload.m_ResolveImage.m_DstImage ) );
            break;

        case DeviceProfilerDrawcallType::eBlitImage:
            argsBuilder
                .Add( "srcImage", m_pStringSerializer->GetName( drawcall.m_Payload.m_BlitImage.m_SrcImage ) )
                .Add( "dstImage", m_pStringSerializer->GetName( drawcall.m_Payload.m_BlitImage.m_DstImage ) );
            break;

        case DeviceProfilerDrawcallType::eFillBuffer:
            argsBuilder
                .Add( "dstBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_FillBuffer.m_Buffer ) )
                .Add( "dstOffset", drawcall.m_Payload.m_FillBuffer.m_Offset )
                .Add( "size", drawcall.m_Payload.m_FillBuffer.m_Size )
                .Add( "data", drawcall.m_Payload.m_FillBuffer.m_Data );
            break;

        case DeviceProfilerDrawcallType::eUpdateBuffer:
            argsBuilder
                .Add( "dstBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_UpdateBuffer.m_Buffer ) )
                .Add( "dstOffset", drawcall.m_Payload.m_UpdateBuffer.m_Offset )
                .Add( "dataSize", drawcall.m_Payload.m_UpdateBuffer.m_Size );
            break;

        case DeviceProfilerDrawcallType::eTraceRaysKHR:
            argsBuilder
                .Add( "width", drawcall.m_Payload.m_TraceRays.m_Width )
                .Add( "height", drawcall.m_Payload.m_TraceRays.m_Height )
                .Add( "depth", drawcall.m_Payload.m_TraceRays.m_Depth );
            break;

        case DeviceProfilerDrawcallType::eTraceRaysIndirectKHR:
            argsBuilder
                .Add( "indirectDeviceAddress", drawcall.m_Payload.m_TraceRaysIndirect.m_IndirectAddress );
            break;

        case DeviceProfilerDrawcallType::eTraceRaysIndirect2KHR:
            argsBuilder
                .Add( "indirectDeviceAddress", drawcall.m_Payload.m_TraceRaysIndirect2.m_IndirectAddress );
            break;

        case DeviceProfilerDrawcallType::eBuildAccelerationStructuresKHR:
        case DeviceProfilerDrawcallType::eBuildAccelerationStructuresIndirectKHR:
        {
            const uint32_t infoCount = drawcall.m_Payload.m_BuildAccelerationStructures.m_InfoCount;

            argsBuilder
                .Add( "infoCount", infoCount );

            auto infosBuilder = argsBuilder.AddArray( "infos" );
            for( uint32_t i = 0; i < infoCount; ++i )
            {
                const auto& info = drawcall.m_Payload.m_BuildAccelerationStructures.m_pInfos[i];

                auto infoBuilder = infosBuilder.AddObject();
                infoBuilder
                    .Add( "type", m_pStringSerializer->GetAccelerationStructureTypeName( info.type ) )
                    .Add( "flags", m_pStringSerializer->GetBuildAccelerationStructureFlagNames( info.flags ) )
                    .Add( "mode", m_pStringSerializer->GetBuildAccelerationStructureModeName( info.mode ) )
                    .Add( "src", m_pStringSerializer->GetName( VkAccelerationStructureKHRHandle( info.srcAccelerationStructure ) ) )
                    .Add( "dst", m_pStringSerializer->GetName( VkAccelerationStructureKHRHandle( info.dstAccelerationStructure ) ) )
                    .Add( "geometryCount", info.geometryCount );

                if( auto geometriesBuilder = infoBuilder.AddArrayOrNull( "geometries", info.pGeometries ) )
                {
                    for( uint32_t j = 0; j < info.geometryCount; ++j )
                    {
                        const auto& geometry = info.pGeometries[j];

                        auto geometryBuilder = geometriesBuilder.AddObject();
                        geometryBuilder
                            .Add( "type", m_pStringSerializer->GetGeometryTypeName( geometry.geometryType ) )
                            .Add( "flags", m_pStringSerializer->GetGeometryFlagNames( geometry.flags ) );

                        auto dataBuilder = geometryBuilder.AddObject( "data" );
                        switch( geometry.geometryType )
                        {
                        case VK_GEOMETRY_TYPE_TRIANGLES_KHR:
                            dataBuilder
                                .Add( "vertexFormat", m_pStringSerializer->GetFormatName( geometry.geometry.triangles.vertexFormat ) )
                                .Add( "vertexData", m_pStringSerializer->GetPointer( geometry.geometry.triangles.vertexData.hostAddress ) )
                                .Add( "vertexStride", geometry.geometry.triangles.vertexStride )
                                .Add( "maxVertex", geometry.geometry.triangles.maxVertex )
                                .Add( "indexType", m_pStringSerializer->GetIndexTypeName( geometry.geometry.triangles.indexType ) )
                                .Add( "indexData", m_pStringSerializer->GetPointer( geometry.geometry.triangles.indexData.hostAddress ) )
                                .Add( "transformData", m_pStringSerializer->GetPointer( geometry.geometry.triangles.transformData.hostAddress ) );
                            break;

                        case VK_GEOMETRY_TYPE_AABBS_KHR:
                            dataBuilder
                                .Add( "data", m_pStringSerializer->GetPointer( geometry.geometry.aabbs.data.hostAddress ) )
                                .Add( "stride", geometry.geometry.aabbs.stride );
                            break;

                        case VK_GEOMETRY_TYPE_INSTANCES_KHR:
                            dataBuilder
                                .Add( "arrayOfPointers", static_cast<bool>( geometry.geometry.instances.arrayOfPointers ) )
                                .Add( "data", m_pStringSerializer->GetPointer( geometry.geometry.instances.data.hostAddress ) );
                            break;
                        }
                        dataBuilder.End();

                        auto rangeBuilder = geometryBuilder.AddObject( "range" );
                        switch( drawcall.m_Type )
                        {
                        case DeviceProfilerDrawcallType::eBuildAccelerationStructuresKHR:
                        {
                            const auto& range = drawcall.m_Payload.m_BuildAccelerationStructures.m_ppRanges[i][j];
                            rangeBuilder
                                .Add( "primitiveCount", range.primitiveCount )
                                .Add( "primitiveOffset", range.primitiveOffset )
                                .Add( "firstVertex", range.firstVertex )
                                .Add( "transformOffset", range.transformOffset );
                            break;
                        }
                        case DeviceProfilerDrawcallType::eBuildAccelerationStructuresIndirectKHR:
                        {
                            rangeBuilder
                                .Add( "maxPrimitiveCount", drawcall.m_Payload.m_BuildAccelerationStructuresIndirect.m_ppMaxPrimitiveCounts[i][j] );
                            break;
                        }
                        }
                        rangeBuilder.End();
                    }
                }
            }

            infosBuilder.End();
            break;
        }

        case DeviceProfilerDrawcallType::eBuildMicromapsEXT:
        {
            const uint32_t infoCount = drawcall.m_Payload.m_BuildMicromaps.m_InfoCount;

            argsBuilder
                .Add( "infoCount", infoCount );

            auto infosBuilder = argsBuilder.AddArray( "infos" );
            for( uint32_t i = 0; i < infoCount; ++i )
            {
                const auto& info = drawcall.m_Payload.m_BuildMicromaps.m_pInfos[i];

                auto infoBuilder = infosBuilder.AddObject();
                infoBuilder
                    .Add( "type", m_pStringSerializer->GetMicromapTypeName( info.type ) )
                    .Add( "flags", m_pStringSerializer->GetBuildMicromapFlagNames( info.flags ) )
                    .Add( "mode", m_pStringSerializer->GetBuildMicromapModeName( info.mode ) )
                    .Add( "dst", m_pStringSerializer->GetName( VkMicromapEXTHandle( info.dstMicromap ) ) )
                    .Add( "usageCountsCount", info.usageCountsCount );

                auto usageCountsBuilder = infoBuilder.AddArray( "usageCounts" );
                for( uint32_t j = 0; j < info.usageCountsCount; ++j )
                {
                    const auto& usageCount = info.pUsageCounts[j];
                    usageCountsBuilder.AddObject()
                        .Add( "count", usageCount.count )
                        .Add( "format", usageCount.format )
                        .Add( "subdivisionLevel", usageCount.subdivisionLevel );
                }
                usageCountsBuilder.End();

                infoBuilder
                    .Add( "data", m_pStringSerializer->GetPointer( info.data.hostAddress ) )
                    .Add( "scratchData", m_pStringSerializer->GetPointer( info.scratchData.hostAddress ) )
                    .Add( "triangleArray", m_pStringSerializer->GetPointer( info.triangleArray.hostAddress ) )
                    .Add( "triangleArrayStride", info.triangleArrayStride );
            }

            infosBuilder.End();
            break;
        }
        }
    }

    /*************************************************************************\

    Function:
        GetPipelineArgs

    Description:
        Serialize pipeline state into JSON object.

    \*************************************************************************/
    void DeviceProfilerJsonSerializer::WritePipelineArgs( DeviceProfilerJsonValueBuilder& builder, const DeviceProfilerPipeline& pipeline ) const
    {
        auto argsBuilder = builder.MakeObject();

        // Append shader stages info.
        if( !pipeline.m_ShaderTuple.m_Shaders.empty() )
        {
            auto shadersBuilder = argsBuilder.AddArray( "shaderStages" );
            for( const ProfilerShader& shader : pipeline.m_ShaderTuple.m_Shaders )
            {
                WriteShaderStageArgs( shadersBuilder.Add(), shader );
            }
        }

        // Append pipeline create info details.
        if( pipeline.m_pCreateInfo )
        {
            switch( pipeline.m_Type )
            {
            case DeviceProfilerPipelineType::eGraphics:
            {
                WriteGraphicsPipelineCreateInfoArgs( argsBuilder,
                    pipeline.m_pCreateInfo->m_GraphicsPipelineCreateInfo );
                break;
            }
            case DeviceProfilerPipelineType::eCompute:
            {
                WriteComputePipelineCreateInfoArgs( argsBuilder,
                    pipeline.m_pCreateInfo->m_ComputePipelineCreateInfo );
                break;
            }
            case DeviceProfilerPipelineType::eRayTracingKHR:
            {
                WriteRayTracingPipelineCreateInfoArgs( argsBuilder,
                    pipeline.m_pCreateInfo->m_RayTracingPipelineCreateInfoKHR );
                break;
            }
            }
        }
    }

    /*************************************************************************\

    Function:
        GetColorClearValue

    Description:
        Serialize VkClearColorValue struct into JSON object.

    \*************************************************************************/
    void DeviceProfilerJsonSerializer::WriteColorClearValue( DeviceProfilerJsonValueBuilder& builder, const VkClearColorValue& value ) const
    {
        auto valueBuilder = builder.MakeObject();
        auto clearColorValueBuilder = valueBuilder.AddObject( "VkClearColorValue" );

        auto float32Builder = clearColorValueBuilder.AddArray( "float32" );
        for( int i = 0; i < 4; ++i )
        {
            float32Builder.Add( value.float32[i] );
        }
        float32Builder.End();

        auto int32Builder = clearColorValueBuilder.AddArray( "int32" );
        for( int i = 0; i < 4; ++i )
        {
            int32Builder.Add( value.int32[i] );
        }
        int32Builder.End();

        auto uint32Builder = clearColorValueBuilder.AddArray( "uint32" );
        for( int i = 0; i < 4; ++i )
        {
            uint32Builder.Add( value.uint32[i] );
        }
        uint32Builder.End();
    }

    /*************************************************************************\

    Function:
        GetDepthStencilClearValue

    Description:
        Serialize VkClearDepthStencilValue struct into JSON object.

    \*************************************************************************/
    void DeviceProfilerJsonSerializer::WriteDepthStencilClearValue( DeviceProfilerJsonValueBuilder& builder, const VkClearDepthStencilValue& value ) const
    {
        builder.MakeObject().AddObject( "VkClearDepthStencilValue" )
            .Add( "depth", value.depth )
            .Add( "stencil", value.stencil );
    }

    /*************************************************************************\

    Function:
        GetShaderStageArgs

    Description:
        Serialize shader stage into JSON object.

    \*************************************************************************/
    void DeviceProfilerJsonSerializer::WriteShaderStageArgs( DeviceProfilerJsonValueBuilder& builder, const ProfilerShader& shader ) const
    {
        auto shaderBuilder = builder.MakeObject();
        shaderBuilder
            .Add( "stage", m_pStringSerializer->GetShaderStageName( shader.m_Stage ) )
            .Add( "entryPoint", shader.m_EntryPoint );

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

            shaderBuilder.Add( "shaderIdentifier", shaderIdentifier );
        }
    }

    /*************************************************************************\

    Function:
        GetGraphicsPipelineCreateInfoArgs

    Description:
        Serialize graphics pipeline state into JSON object.

    \*************************************************************************/
    void DeviceProfilerJsonSerializer::WriteGraphicsPipelineCreateInfoArgs( DeviceProfilerJsonObjectBuilder& builder, const VkGraphicsPipelineCreateInfo& createInfo ) const
    {
        // VkPipelineVertexInputStateCreateInfo
        if( auto vertexInputStateBuilder = builder.AddObjectOrNull( "vertexInputState", createInfo.pVertexInputState ) )
        {
            const auto& state = *createInfo.pVertexInputState;
            vertexInputStateBuilder
                .Add( "attributeCount", state.vertexAttributeDescriptionCount )
                .Add( "bindingCount", state.vertexBindingDescriptionCount );

            if( auto attributesBuilder = vertexInputStateBuilder.AddArrayOrNull( "attributes", state.pVertexAttributeDescriptions ) )
            {
                for( uint32_t i = 0; i < state.vertexAttributeDescriptionCount; ++i )
                {
                    const auto& attribute = state.pVertexAttributeDescriptions[i];
                    attributesBuilder.AddObject()
                        .Add( "location", attribute.location )
                        .Add( "binding", attribute.binding )
                        .Add( "format", m_pStringSerializer->GetFormatName( attribute.format ) )
                        .Add( "offset", attribute.offset );
                }
            }

            if( auto bindingsBuilder = vertexInputStateBuilder.AddArrayOrNull( "bindings", state.pVertexBindingDescriptions ) )
            {
                for( uint32_t i = 0; i < state.vertexBindingDescriptionCount; ++i )
                {
                    const auto& binding = state.pVertexBindingDescriptions[i];
                    bindingsBuilder.AddObject()
                        .Add( "binding", binding.binding )
                        .Add( "stride", binding.stride )
                        .Add( "inputRate", m_pStringSerializer->GetVertexInputRateName( binding.inputRate ) );
                }
            }
        }

        // VkPipelineInputAssemblyStateCreateInfo
        if( auto inputAssemblyStateBuilder = builder.AddObjectOrNull( "inputAssemblyState", createInfo.pInputAssemblyState ) )
        {
            const auto& state = *createInfo.pInputAssemblyState;
            inputAssemblyStateBuilder
                .Add( "topology", m_pStringSerializer->GetPrimitiveTopologyName( state.topology ) )
                .Add( "primitiveRestartEnable", static_cast<bool>( state.primitiveRestartEnable ) );
        }

        // VkPipelineTessellationStateCreateInfo
        if( auto tessellationStateBuilder = builder.AddObjectOrNull( "tessellationState", createInfo.pTessellationState ) )
        {
            const auto& state = *createInfo.pTessellationState;
            tessellationStateBuilder
                .Add( "patchControlPoints", state.patchControlPoints );
        }

        // VkPipelineViewportStateCreateInfo
        if( auto viewportStateBuilder = builder.AddObjectOrNull( "viewportState", createInfo.pViewportState ) )
        {
            const auto& state = *createInfo.pViewportState;
            viewportStateBuilder
                .Add( "viewportCount", state.viewportCount )
                .Add( "scissorCount", state.scissorCount );

            if( auto viewportsBuilder = viewportStateBuilder.AddArrayOrNull( "viewports", state.pViewports ) )
            {
                for( uint32_t i = 0; i < state.viewportCount; ++i )
                {
                    const auto& viewport = state.pViewports[i];
                    viewportsBuilder.AddObject()
                        .Add( "x", viewport.x )
                        .Add( "y", viewport.y )
                        .Add( "width", viewport.width )
                        .Add( "height", viewport.height )
                        .Add( "minDepth", viewport.minDepth )
                        .Add( "maxDepth", viewport.maxDepth );
                }
            }

            if( auto scissorsBuilder = viewportStateBuilder.AddArrayOrNull( "scissors", state.pScissors ) )
            {
                for( uint32_t i = 0; i < state.scissorCount; ++i )
                {
                    const auto& scissor = state.pScissors[i];
                    scissorsBuilder.AddObject()
                        .Add( "offsetX", scissor.offset.x )
                        .Add( "offsetY", scissor.offset.y )
                        .Add( "extentWidth", scissor.extent.width )
                        .Add( "extentHeight", scissor.extent.height );
                }
            }
        }

        // VkPipelineRasterizationStateCreateInfo
        if( auto rasterizationStateBuilder = builder.AddObjectOrNull( "rasterizationState", createInfo.pRasterizationState ) )
        {
            const auto& state = *createInfo.pRasterizationState;
            rasterizationStateBuilder
                .Add( "depthClampEnable", static_cast<bool>( state.depthClampEnable ) )
                .Add( "rasterizerDiscardEnable", static_cast<bool>( state.rasterizerDiscardEnable ) )
                .Add( "polygonMode", m_pStringSerializer->GetPolygonModeName( state.polygonMode ) )
                .Add( "cullMode", m_pStringSerializer->GetCullModeName( state.cullMode ) )
                .Add( "frontFace", m_pStringSerializer->GetFrontFaceName( state.frontFace ) )
                .Add( "depthBiasEnable", static_cast<bool>( state.depthBiasEnable ) )
                .Add( "depthBiasConstantFactor", state.depthBiasConstantFactor )
                .Add( "depthBiasClamp", state.depthBiasClamp )
                .Add( "depthBiasSlopeFactor", state.depthBiasSlopeFactor )
                .Add( "lineWidth", state.lineWidth );
        }

        // VkPipelineMultisampleStateCreateInfo
        if( auto multisampleStateBuilder = builder.AddObjectOrNull( "multisampleState", createInfo.pMultisampleState ) )
        {
            const auto& state = *createInfo.pMultisampleState;
            multisampleStateBuilder
                .Add( "rasterizationSamples", static_cast<uint32_t>( state.rasterizationSamples ) )
                .Add( "sampleShadingEnable", static_cast<bool>( state.sampleShadingEnable ) )
                .Add( "minSampleShading", state.minSampleShading )
                .Add( "sampleMask", fmt::format( "0x{:08X}", state.pSampleMask ? *state.pSampleMask : 0xFFFFFFFF ) )
                .Add( "alphaToCoverageEnable", static_cast<bool>( state.alphaToCoverageEnable ) )
                .Add( "alphaToOneEnable", static_cast<bool>( state.alphaToOneEnable ) );
        }

        // VkPipelineDepthStencilStateCreateInfo
        if( auto depthStencilStateBuilder = builder.AddObjectOrNull( "depthStencilState", createInfo.pDepthStencilState ) )
        {
            const auto& state = *createInfo.pDepthStencilState;
            depthStencilStateBuilder
                .Add( "depthTestEnable", static_cast<bool>( state.depthTestEnable ) )
                .Add( "depthWriteEnable", static_cast<bool>( state.depthWriteEnable ) )
                .Add( "depthCompareOp", m_pStringSerializer->GetCompareOpName( state.depthCompareOp ) )
                .Add( "depthBoundsTestEnable", static_cast<bool>( state.depthBoundsTestEnable ) )
                .Add( "minDepthBounds", state.minDepthBounds )
                .Add( "maxDepthBounds", state.maxDepthBounds )
                .Add( "stencilTestEnable", static_cast<bool>( state.stencilTestEnable ) );

            depthStencilStateBuilder
                .AddObject( "front" )
                .Add( "failOp", static_cast<uint32_t>( state.front.failOp ) )
                .Add( "passOp", static_cast<uint32_t>( state.front.passOp ) )
                .Add( "depthFailOp", static_cast<uint32_t>( state.front.depthFailOp ) )
                .Add( "compareOp", m_pStringSerializer->GetCompareOpName( state.front.compareOp ) )
                .Add( "compareMask", fmt::format( "0x{:02X}", state.front.compareMask ) )
                .Add( "writeMask", fmt::format( "0x{:02X}", state.front.writeMask ) )
                .Add( "reference", fmt::format( "0x{:02X}", state.front.reference ) );

            depthStencilStateBuilder
                .AddObject( "back" )
                .Add( "failOp", static_cast<uint32_t>( state.back.failOp ) )
                .Add( "passOp", static_cast<uint32_t>( state.back.passOp ) )
                .Add( "depthFailOp", static_cast<uint32_t>( state.back.depthFailOp ) )
                .Add( "compareOp", m_pStringSerializer->GetCompareOpName( state.back.compareOp ) )
                .Add( "compareMask", fmt::format( "0x{:02X}", state.back.compareMask ) )
                .Add( "writeMask", fmt::format( "0x{:02X}", state.back.writeMask ) )
                .Add( "reference", fmt::format( "0x{:02X}", state.back.reference ) );
        }

        // VkPipelineColorBlendStateCreateInfo
        if( auto colorBlendStateBuilder = builder.AddObjectOrNull( "colorBlendState", createInfo.pColorBlendState ) )
        {
            const auto& state = *createInfo.pColorBlendState;
            colorBlendStateBuilder
                .Add( "logicOpEnable", static_cast<bool>( state.logicOpEnable ) )
                .Add( "logicOp", m_pStringSerializer->GetLogicOpName( state.logicOp ) );

            colorBlendStateBuilder
                .AddArray( "blendConstants" )
                .Add( state.blendConstants[0] )
                .Add( state.blendConstants[1] )
                .Add( state.blendConstants[2] )
                .Add( state.blendConstants[3] );

            if( auto attachmentsBuilder = colorBlendStateBuilder.AddArrayOrNull( "attachments", state.pAttachments ) )
            {
                for( uint32_t i = 0; i < state.attachmentCount; ++i )
                {
                    const auto& attachment = state.pAttachments[i];
                    attachmentsBuilder.AddObject()
                        .Add( "blendEnable", static_cast<bool>( attachment.blendEnable ) )
                        .Add( "srcColorBlendFactor", m_pStringSerializer->GetBlendFactorName( attachment.srcColorBlendFactor ) )
                        .Add( "dstColorBlendFactor", m_pStringSerializer->GetBlendFactorName( attachment.dstColorBlendFactor ) )
                        .Add( "colorBlendOp", m_pStringSerializer->GetBlendOpName( attachment.colorBlendOp ) )
                        .Add( "srcAlphaBlendFactor", m_pStringSerializer->GetBlendFactorName( attachment.srcAlphaBlendFactor ) )
                        .Add( "dstAlphaBlendFactor", m_pStringSerializer->GetBlendFactorName( attachment.dstAlphaBlendFactor ) )
                        .Add( "alphaBlendOp", m_pStringSerializer->GetBlendOpName( attachment.alphaBlendOp ) )
                        .Add( "colorWriteMask", m_pStringSerializer->GetColorComponentFlagNames( attachment.colorWriteMask ) );
                }
            }
        }

        // VkPipelineDynamicStateCreateInfo
        if( auto dynamicStatesBuilder = builder.AddArrayOrNull( "dynamicStates", createInfo.pDynamicState ) )
        {
            const auto& state = *createInfo.pDynamicState;
            for( uint32_t i = 0; i < state.dynamicStateCount; ++i )
            {
                dynamicStatesBuilder.Add(
                    m_pStringSerializer->GetDynamicStateName( state.pDynamicStates[i] ) );
            }
        }
    }

    /*************************************************************************\

    Function:
        GetComputePipelineCreateInfoArgs

    Description:
        Serialize compute pipeline state into JSON object.

    \*************************************************************************/
    void DeviceProfilerJsonSerializer::WriteComputePipelineCreateInfoArgs( DeviceProfilerJsonObjectBuilder& builder, const VkComputePipelineCreateInfo& createInfo ) const
    {
        // No additional state to serialize for compute pipelines yet.
    }

    /*************************************************************************\

    Function:
        GetRayTracingPipelineCreateInfoArgs

    Description:
        Serialize ray-tracing pipeline state into JSON object.

    \*************************************************************************/
    void DeviceProfilerJsonSerializer::WriteRayTracingPipelineCreateInfoArgs( DeviceProfilerJsonObjectBuilder& builder, const VkRayTracingPipelineCreateInfoKHR& createInfo ) const
    {
        builder.Add( "maxPipelineRayRecursionDepth", createInfo.maxPipelineRayRecursionDepth );

        // VkRayTracingPipelineInterfaceCreateInfoKHR
        if( auto libraryInterfaceBuilder = builder.AddObjectOrNull( "libraryInterface", createInfo.pLibraryInterface ) )
        {
            const auto& state = *createInfo.pLibraryInterface;
            libraryInterfaceBuilder
                .Add( "maxPipelineRayPayloadSize", state.maxPipelineRayPayloadSize )
                .Add( "maxPipelineRayHitAttributeSize", state.maxPipelineRayHitAttributeSize );
        }

        // VkPipelineDynamicStateCreateInfo
        if( auto dynamicStatesBuilder = builder.AddArrayOrNull( "dynamicStates", createInfo.pDynamicState ) )
        {
            const auto& state = *createInfo.pDynamicState;
            for( uint32_t i = 0; i < state.dynamicStateCount; ++i )
            {
                dynamicStatesBuilder.Add(
                    m_pStringSerializer->GetDynamicStateName( state.pDynamicStates[i] ) );
            }
        }
    }
}

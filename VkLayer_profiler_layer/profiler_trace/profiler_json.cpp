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
#include "profiler/profiler_frontend.h"
#include "profiler_helpers/profiler_data_helpers.h"
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
    DeviceProfilerJsonSerializer::DeviceProfilerJsonSerializer( DeviceProfilerFrontend* pFrontend, const DeviceProfilerStringSerializer* pStringSerializer )
        : m_pFrontend( pFrontend )
        , m_pStringSerializer( pStringSerializer )
    {
    }

    /*************************************************************************\

    Function:
        GetCommandArgs

    Description:
        Serialize command arguments into JSON object.

    \*************************************************************************/
    nlohmann::json DeviceProfilerJsonSerializer::GetCommandArgs( const DeviceProfilerDrawcall& drawcall, const DeviceProfilerPipeline& pipeline, const DeviceProfilerCommandBufferData& commandBuffer ) const
    {
        const bool indirectPayloadPresent = ( drawcall.HasIndirectPayload() && commandBuffer.m_pIndirectPayload );

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
        {
            const DeviceProfilerDrawcallDrawIndirectPayload& payload = drawcall.m_Payload.m_DrawIndirect;

            nlohmann::json args = {
                { "buffer", m_pStringSerializer->GetName( payload.m_Buffer ) },
                { "offset", payload.m_Offset },
                { "drawCount", payload.m_DrawCount },
                { "stride", payload.m_Stride }
            };

            if( indirectPayloadPresent )
            {
                std::vector<nlohmann::json> indirectCommands;

                const uint8_t* pIndirectData = commandBuffer.m_pIndirectPayload.get() + payload.m_IndirectArgsOffset;
                for( uint32_t drawIndex = 0; drawIndex < payload.m_DrawCount; ++drawIndex )
                {
                    const VkDrawIndirectCommand& cmd =
                        *reinterpret_cast<const VkDrawIndirectCommand*>( pIndirectData + drawIndex * payload.m_Stride );

                    indirectCommands.push_back( {
                        { "vertexCount", cmd.vertexCount },
                        { "instanceCount", cmd.instanceCount },
                        { "firstVertex", cmd.firstVertex },
                        { "firstInstance", cmd.firstInstance } } );
                }

                args["indirectCommands"] = indirectCommands;
            }

            return args;
        }

        case DeviceProfilerDrawcallType::eDrawIndexedIndirect:
        {
            const DeviceProfilerDrawcallDrawIndexedIndirectPayload& payload = drawcall.m_Payload.m_DrawIndexedIndirect;

            nlohmann::json args = {
                { "buffer", m_pStringSerializer->GetName( payload.m_Buffer ) },
                { "offset", payload.m_Offset },
                { "drawCount", payload.m_DrawCount },
                { "stride", payload.m_Stride }
            };

            if( indirectPayloadPresent )
            {
                std::vector<nlohmann::json> indirectCommands;

                const uint8_t* pIndirectData = commandBuffer.m_pIndirectPayload.get() + payload.m_IndirectArgsOffset;
                for( uint32_t drawIndex = 0; drawIndex < payload.m_DrawCount; ++drawIndex )
                {
                    const VkDrawIndexedIndirectCommand& cmd =
                        *reinterpret_cast<const VkDrawIndexedIndirectCommand*>( pIndirectData + drawIndex * payload.m_Stride );

                    indirectCommands.push_back( {
                        { "indexCount", cmd.indexCount },
                        { "instanceCount", cmd.instanceCount },
                        { "firstIndex", cmd.firstIndex },
                        { "vertexOffset", cmd.vertexOffset },
                        { "firstInstance", cmd.firstInstance } } );
                }

                args["indirectCommands"] = indirectCommands;
            }

            return args;
        }

        case DeviceProfilerDrawcallType::eDrawIndirectCount:
        {
            const DeviceProfilerDrawcallDrawIndirectCountPayload& payload = drawcall.m_Payload.m_DrawIndirectCount;

            nlohmann::json args = {
                { "buffer", m_pStringSerializer->GetName( payload.m_Buffer ) },
                { "offset", payload.m_Offset },
                { "countBuffer", m_pStringSerializer->GetName( payload.m_CountBuffer ) },
                { "countOffset", payload.m_CountOffset },
                { "maxDrawCount", payload.m_MaxDrawCount },
                { "stride", payload.m_Stride }
            };

            if( indirectPayloadPresent )
            {
                std::vector<nlohmann::json> indirectCommands;

                const uint8_t* pIndirectData = commandBuffer.m_pIndirectPayload.get() + payload.m_IndirectArgsOffset;
                const uint8_t* pIndirectCount = commandBuffer.m_pIndirectPayload.get() + payload.m_IndirectCountOffset;

                const uint32_t drawCount = *reinterpret_cast<const uint32_t*>( pIndirectCount );
                for( uint32_t drawIndex = 0; drawIndex < drawCount; ++drawIndex )
                {
                    const VkDrawIndirectCommand& cmd =
                        *reinterpret_cast<const VkDrawIndirectCommand*>( pIndirectData + drawIndex * payload.m_Stride );

                    indirectCommands.push_back( {
                        { "vertexCount", cmd.vertexCount },
                        { "instanceCount", cmd.instanceCount },
                        { "firstVertex", cmd.firstVertex },
                        { "firstInstance", cmd.firstInstance } } );
                }

                args["indirectCount"] = drawCount;
                args["indirectCommands"] = indirectCommands;
            }

            return args;
        }

        case DeviceProfilerDrawcallType::eDrawIndexedIndirectCount:
        {
            const DeviceProfilerDrawcallDrawIndexedIndirectCountPayload& payload = drawcall.m_Payload.m_DrawIndexedIndirectCount;

            nlohmann::json args = {
                { "buffer", m_pStringSerializer->GetName( payload.m_Buffer ) },
                { "offset", payload.m_Offset },
                { "countBuffer", m_pStringSerializer->GetName( payload.m_CountBuffer ) },
                { "countOffset", payload.m_CountOffset },
                { "maxDrawCount", payload.m_MaxDrawCount },
                { "stride", payload.m_Stride }
            };

            if( indirectPayloadPresent )
            {
                std::vector<nlohmann::json> indirectCommands;

                const uint8_t* pIndirectData = commandBuffer.m_pIndirectPayload.get() + payload.m_IndirectArgsOffset;
                const uint8_t* pIndirectCount = commandBuffer.m_pIndirectPayload.get() + payload.m_IndirectCountOffset;

                const uint32_t drawCount = *reinterpret_cast<const uint32_t*>( pIndirectCount );
                for( uint32_t drawIndex = 0; drawIndex < drawCount; ++drawIndex )
                {
                    const VkDrawIndexedIndirectCommand& cmd =
                        *reinterpret_cast<const VkDrawIndexedIndirectCommand*>( pIndirectData + drawIndex * payload.m_Stride );

                    indirectCommands.push_back( {
                        { "indexCount", cmd.indexCount },
                        { "instanceCount", cmd.instanceCount },
                        { "firstIndex", cmd.firstIndex },
                        { "vertexOffset", cmd.vertexOffset },
                        { "firstInstance", cmd.firstInstance } } );
                }

                args["indirectCount"] = drawCount;
                args["indirectCommands"] = indirectCommands;
            }

            return args;
        }

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
        {
            const DeviceProfilerDrawcallDispatchIndirectPayload& payload = drawcall.m_Payload.m_DispatchIndirect;

            nlohmann::json args = {
                { "buffer", m_pStringSerializer->GetName( payload.m_Buffer ) },
                { "offset", payload.m_Offset }
            };

            if( indirectPayloadPresent )
            {
                const uint8_t* pIndirectData = commandBuffer.m_pIndirectPayload.get() + payload.m_IndirectArgsOffset;
                const VkDispatchIndirectCommand& cmd =
                    *reinterpret_cast<const VkDispatchIndirectCommand*>( pIndirectData );

                args["indirectCommand"] = {
                    { "groupCountX", cmd.x },
                    { "groupCountY", cmd.y },
                    { "groupCountZ", cmd.z }
                };
            }

            return args;
        }

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
        {
            const DeviceProfilerDrawcallTraceRaysPayload& payload = drawcall.m_Payload.m_TraceRays;

            nlohmann::json args = {
                { "width", payload.m_Width },
                { "height", payload.m_Height },
                { "depth", payload.m_Depth }
            };

            if( indirectPayloadPresent )
            {
                const uint8_t* pIndirectData = commandBuffer.m_pIndirectPayload.get();
                const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& rayTracingPipelineProperties =
                    m_pFrontend->GetRayTracingPipelineProperties();

                auto GetShaderHash = []( const ProfilerShader* pShader ) -> std::string {
                    char hash[9] = "00000000";
                    snprintf( hash, sizeof( hash ), "%08X", pShader ? pShader->m_Hash : 0 );
                    return hash;
                };

                auto GetShaderRecordData = []( const void* pData, size_t size ) -> std::string {
                    std::string dataStr( size * 2, '0' );
                    for( size_t i = 0; i < size; ++i )
                        snprintf( &dataStr[i * 2], 3, "%02X", static_cast<const uint8_t*>( pData )[i] );
                    return dataStr;
                };

                auto SerializeShaderBindingTable = [&]( const VkStridedDeviceAddressRegionKHR& table, size_t tableOffset ) -> nlohmann::json {
                    nlohmann::json sbt = {
                        { "deviceAddress", m_pStringSerializer->GetPointer( (void*)table.deviceAddress ) },
                        { "stride", table.stride },
                        { "size", table.size }
                    };

                    std::vector<nlohmann::json> sbtRecords;

                    size_t groupIndex = 0;
                    size_t groupOffset = 0;

                    // Print shader groups.
                    while( groupOffset < table.size )
                    {
                        const ProfilerShaderGroup* pShaderGroup =
                            pipeline.m_ShaderTuple.GetShaderGroupAtHandle(
                                pIndirectData + tableOffset + groupOffset,
                                rayTracingPipelineProperties.shaderGroupHandleSize );

                        if( pShaderGroup == nullptr )
                        {
                            sbtRecords.emplace_back();
                            groupIndex++;
                            groupOffset += table.stride;
                            continue;
                        }

                        // Print shaders in the group.
                        switch( pShaderGroup->m_Type )
                        {
                        case VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR:
                        {
                            sbtRecords.push_back( {
                                { "general", GetShaderHash( pipeline.m_ShaderTuple.GetShaderAtIndex( pShaderGroup->m_GeneralShader ) ) } } );
                            break;
                        }

                        case VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR:
                        {
                            nlohmann::json shaderRecord = {
                                { "closestHit", GetShaderHash( pipeline.m_ShaderTuple.GetShaderAtIndex( pShaderGroup->m_ClosestHitShader ) ) },
                                { "anyHit", GetShaderHash( pipeline.m_ShaderTuple.GetShaderAtIndex( pShaderGroup->m_AnyHitShader ) ) }
                            };

                            if( table.stride > rayTracingPipelineProperties.shaderGroupHandleSize )
                            {
                                // Append application data following the shader group handle.
                                shaderRecord["data"] = GetShaderRecordData(
                                    pIndirectData + tableOffset + groupOffset + rayTracingPipelineProperties.shaderGroupHandleSize,
                                    table.stride - rayTracingPipelineProperties.shaderGroupHandleSize );
                            }

                            sbtRecords.push_back( std::move( shaderRecord ) );
                            break;
                        }

                        case VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR:
                        {
                            nlohmann::json shaderRecord = {
                                { "closestHit", GetShaderHash( pipeline.m_ShaderTuple.GetShaderAtIndex( pShaderGroup->m_ClosestHitShader ) ) },
                                { "anyHit", GetShaderHash( pipeline.m_ShaderTuple.GetShaderAtIndex( pShaderGroup->m_AnyHitShader ) ) },
                                { "intersection", GetShaderHash( pipeline.m_ShaderTuple.GetShaderAtIndex( pShaderGroup->m_IntersectionShader ) ) }
                            };

                            if( table.stride > rayTracingPipelineProperties.shaderGroupHandleSize )
                            {
                                // Append application data following the shader group handle.
                                shaderRecord["data"] = GetShaderRecordData(
                                    pIndirectData + tableOffset + groupOffset + rayTracingPipelineProperties.shaderGroupHandleSize,
                                    table.stride - rayTracingPipelineProperties.shaderGroupHandleSize );
                            }

                            sbtRecords.push_back( std::move( shaderRecord ) );
                            break;
                        }
                        }

                        groupIndex++;
                        groupOffset += table.stride;
                    }

                    // Remove unused records from the end of the list.
                    while( !sbtRecords.empty() && sbtRecords.back().empty() )
                    {
                        sbtRecords.pop_back();
                    }

                    for( nlohmann::json& record : sbtRecords )
                    {
                        if( record.empty() )
                        {
                            // Mark unused records.
                            record = "Unused";
                        }
                    }

                    sbt["records"] = std::move( sbtRecords );
                    return sbt;
                };

                args["raygenShaderBindingTable"] = SerializeShaderBindingTable( payload.m_RaygenShaderBindingTable, payload.m_RaygenShaderBindingTableOffset );
                args["missShaderBindingTable"] = SerializeShaderBindingTable( payload.m_MissShaderBindingTable, payload.m_MissShaderBindingTableOffset );
                args["hitShaderBindingTable"] = SerializeShaderBindingTable( payload.m_HitShaderBindingTable, payload.m_HitShaderBindingTableOffset );
                args["callableShaderBindingTable"] = SerializeShaderBindingTable( payload.m_CallableShaderBindingTable, payload.m_CallableShaderBindingTableOffset );
            }

            return args;
        }

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
                    { "src", m_pStringSerializer->GetName( info.srcAccelerationStructure ) },
                    { "dst", m_pStringSerializer->GetName( info.dstAccelerationStructure ) },
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
                    { "dst", m_pStringSerializer->GetName( info.dstMicromap ) },
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
}

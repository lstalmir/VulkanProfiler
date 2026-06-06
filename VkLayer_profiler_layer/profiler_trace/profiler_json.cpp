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
    void DeviceProfilerJsonSerializer::WriteCommandArgs( simdjson::builder::string_builder& builder, const DeviceProfilerDrawcall& drawcall ) const
    {
        builder.start_object();

        switch( drawcall.m_Type )
        {
        default:
        case DeviceProfilerDrawcallType::eUnknown:
        case DeviceProfilerDrawcallType::eInsertDebugLabel:
        case DeviceProfilerDrawcallType::eBeginDebugLabel:
        case DeviceProfilerDrawcallType::eEndDebugLabel:
            break;

        case DeviceProfilerDrawcallType::eDraw:
            builder.append_key_value( "vertexCount", drawcall.m_Payload.m_Draw.m_VertexCount );
            builder.append_comma();
            builder.append_key_value( "instanceCount", drawcall.m_Payload.m_Draw.m_InstanceCount );
            builder.append_comma();
            builder.append_key_value( "firstVertex", drawcall.m_Payload.m_Draw.m_FirstVertex );
            builder.append_comma();
            builder.append_key_value( "firstInstance", drawcall.m_Payload.m_Draw.m_FirstInstance );
            break;

        case DeviceProfilerDrawcallType::eDrawIndexed:
            builder.append_key_value( "indexCount", drawcall.m_Payload.m_DrawIndexed.m_IndexCount );
            builder.append_comma();
            builder.append_key_value( "instanceCount", drawcall.m_Payload.m_DrawIndexed.m_InstanceCount );
            builder.append_comma();
            builder.append_key_value( "firstIndex", drawcall.m_Payload.m_DrawIndexed.m_FirstIndex );
            builder.append_comma();
            builder.append_key_value( "vertexOffset", drawcall.m_Payload.m_DrawIndexed.m_VertexOffset );
            builder.append_comma();
            builder.append_key_value( "firstInstance", drawcall.m_Payload.m_DrawIndexed.m_FirstInstance );
            break;

        case DeviceProfilerDrawcallType::eDrawIndirect:
            builder.append_key_value( "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawIndirect.m_Buffer ) );
            builder.append_comma();
            builder.append_key_value( "offset", drawcall.m_Payload.m_DrawIndirect.m_Offset );
            builder.append_comma();
            builder.append_key_value( "drawCount", drawcall.m_Payload.m_DrawIndirect.m_DrawCount );
            builder.append_comma();
            builder.append_key_value( "stride", drawcall.m_Payload.m_DrawIndirect.m_Stride );
            break;

        case DeviceProfilerDrawcallType::eDrawIndexedIndirect:
            builder.append_key_value( "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawIndexedIndirect.m_Buffer ) );
            builder.append_comma();
            builder.append_key_value( "offset", drawcall.m_Payload.m_DrawIndexedIndirect.m_Offset );
            builder.append_comma();
            builder.append_key_value( "drawCount", drawcall.m_Payload.m_DrawIndexedIndirect.m_DrawCount );
            builder.append_comma();
            builder.append_key_value( "stride", drawcall.m_Payload.m_DrawIndexedIndirect.m_Stride );
            break;

        case DeviceProfilerDrawcallType::eDrawIndirectCount:
            builder.append_key_value( "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawIndirectCount.m_Buffer ) );
            builder.append_comma();
            builder.append_key_value( "offset", drawcall.m_Payload.m_DrawIndirectCount.m_Offset );
            builder.append_comma();
            builder.append_key_value( "countBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawIndirectCount.m_CountBuffer ) );
            builder.append_comma();
            builder.append_key_value( "countOffset", drawcall.m_Payload.m_DrawIndirectCount.m_CountOffset );
            builder.append_comma();
            builder.append_key_value( "maxDrawCount", drawcall.m_Payload.m_DrawIndirectCount.m_MaxDrawCount );
            builder.append_comma();
            builder.append_key_value( "stride", drawcall.m_Payload.m_DrawIndirectCount.m_Stride );
            break;

        case DeviceProfilerDrawcallType::eDrawIndexedIndirectCount:
            builder.append_key_value( "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawIndexedIndirectCount.m_Buffer ) );
            builder.append_comma();
            builder.append_key_value( "offset", drawcall.m_Payload.m_DrawIndexedIndirectCount.m_Offset );
            builder.append_comma();
            builder.append_key_value( "countBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawIndexedIndirectCount.m_CountBuffer ) );
            builder.append_comma();
            builder.append_key_value( "countOffset", drawcall.m_Payload.m_DrawIndexedIndirectCount.m_CountOffset );
            builder.append_comma();
            builder.append_key_value( "maxDrawCount", drawcall.m_Payload.m_DrawIndexedIndirectCount.m_MaxDrawCount );
            builder.append_comma();
            builder.append_key_value( "stride", drawcall.m_Payload.m_DrawIndexedIndirectCount.m_Stride );
            break;

        case DeviceProfilerDrawcallType::eDrawMeshTasks:
            builder.append_key_value( "groupCountX", drawcall.m_Payload.m_DrawMeshTasks.m_GroupCountX );
            builder.append_comma();
            builder.append_key_value( "groupCountY", drawcall.m_Payload.m_DrawMeshTasks.m_GroupCountY );
            builder.append_comma();
            builder.append_key_value( "groupCountZ", drawcall.m_Payload.m_DrawMeshTasks.m_GroupCountZ );
            break;

        case DeviceProfilerDrawcallType::eDrawMeshTasksIndirect:
            builder.append_key_value( "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawMeshTasksIndirect.m_Buffer ) );
            builder.append_comma();
            builder.append_key_value( "offset", drawcall.m_Payload.m_DrawMeshTasksIndirect.m_Offset );
            builder.append_comma();
            builder.append_key_value( "drawCount", drawcall.m_Payload.m_DrawMeshTasksIndirect.m_DrawCount );
            builder.append_comma();
            builder.append_key_value( "stride", drawcall.m_Payload.m_DrawMeshTasksIndirect.m_Stride );
            break;

        case DeviceProfilerDrawcallType::eDrawMeshTasksIndirectCount:
            builder.append_key_value( "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_Buffer ) );
            builder.append_comma();
            builder.append_key_value( "offset", drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_Offset );
            builder.append_comma();
            builder.append_key_value( "countBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_CountBuffer ) );
            builder.append_comma();
            builder.append_key_value( "countOffset", drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_CountOffset );
            builder.append_comma();
            builder.append_key_value( "maxDrawCount", drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_MaxDrawCount );
            builder.append_comma();
            builder.append_key_value( "stride", drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_Stride );
            break;

        case DeviceProfilerDrawcallType::eDrawMeshTasksNV:
            builder.append_key_value( "taskCount", drawcall.m_Payload.m_DrawMeshTasksNV.m_TaskCount );
            builder.append_comma();
            builder.append_key_value( "firstTask", drawcall.m_Payload.m_DrawMeshTasksNV.m_FirstTask );
            break;

        case DeviceProfilerDrawcallType::eDrawMeshTasksIndirectNV:
            builder.append_key_value( "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawMeshTasksIndirectNV.m_Buffer ) );
            builder.append_comma();
            builder.append_key_value( "offset", drawcall.m_Payload.m_DrawMeshTasksIndirectNV.m_Offset );
            builder.append_comma();
            builder.append_key_value( "drawCount", drawcall.m_Payload.m_DrawMeshTasksIndirectNV.m_DrawCount );
            builder.append_comma();
            builder.append_key_value( "stride", drawcall.m_Payload.m_DrawMeshTasksIndirectNV.m_Stride );
            break;

        case DeviceProfilerDrawcallType::eDrawMeshTasksIndirectCountNV:
            builder.append_key_value( "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_Buffer ) );
            builder.append_comma();
            builder.append_key_value( "offset", drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_Offset );
            builder.append_comma();
            builder.append_key_value( "countBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_CountBuffer ) );
            builder.append_comma();
            builder.append_key_value( "countOffset", drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_CountOffset );
            builder.append_comma();
            builder.append_key_value( "maxDrawCount", drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_MaxDrawCount );
            builder.append_comma();
            builder.append_key_value( "stride", drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_Stride );
            break;

        case DeviceProfilerDrawcallType::eDrawMulti:
            builder.append_key_value( "drawCount", drawcall.m_Payload.m_DrawMulti.m_DrawCount );
            builder.append_comma();
            builder.append_key_value( "instanceCount", drawcall.m_Payload.m_DrawMulti.m_InstanceCount );
            builder.append_comma();
            builder.append_key_value( "firstInstance", drawcall.m_Payload.m_DrawMulti.m_FirstInstance );
            builder.append_comma();
            builder.append_key_value( "stride", drawcall.m_Payload.m_DrawMulti.m_Stride );
            builder.append_comma();

            builder.escape_and_append_with_quotes( "vertexInfos" );
            builder.append_colon();
            builder.start_array();

            for( uint32_t i = 0; i < drawcall.m_Payload.m_DrawMulti.m_DrawCount; i++ )
            {
                if( i > 0 ) builder.append_comma();

                const VkMultiDrawInfoEXT& vertexInfo = drawcall.m_Payload.m_DrawMulti.m_pVertexInfo[i];
                builder.start_object();
                builder.append_key_value( "firstVertex", vertexInfo.firstVertex );
                builder.append_comma();
                builder.append_key_value( "vertexCount", vertexInfo.vertexCount );
                builder.end_object();
            }

            builder.end_array();
            break;
        
        case DeviceProfilerDrawcallType::eDrawMultiIndexed:
            builder.append_key_value( "drawCount", drawcall.m_Payload.m_DrawMultiIndexed.m_DrawCount );
            builder.append_comma();
            builder.append_key_value( "instanceCount", drawcall.m_Payload.m_DrawMultiIndexed.m_InstanceCount );
            builder.append_comma();
            builder.append_key_value( "firstInstance", drawcall.m_Payload.m_DrawMultiIndexed.m_FirstInstance );
            builder.append_comma();
            builder.append_key_value( "stride", drawcall.m_Payload.m_DrawMultiIndexed.m_Stride );
            builder.append_comma();

            builder.escape_and_append_with_quotes( "indexInfos" );
            builder.append_colon();
            builder.start_array();

            for( uint32_t i = 0; i < drawcall.m_Payload.m_DrawMultiIndexed.m_DrawCount; i++ )
            {
                if( i > 0 ) builder.append_comma();

                const VkMultiDrawIndexedInfoEXT& indexInfo = drawcall.m_Payload.m_DrawMultiIndexed.m_pIndexInfo[i];
                builder.start_object();
                builder.append_key_value( "firstIndex", indexInfo.firstIndex );
                builder.append_comma();
                builder.append_key_value( "indexCount", indexInfo.indexCount );
                builder.append_comma();
                builder.append_key_value( "vertexOffset", indexInfo.vertexOffset );
                builder.end_object();
            }

            builder.end_array();
            break;

        case DeviceProfilerDrawcallType::eDispatch:
            builder.append_key_value( "groupCountX", drawcall.m_Payload.m_Dispatch.m_GroupCountX );
            builder.append_comma();
            builder.append_key_value( "groupCountY", drawcall.m_Payload.m_Dispatch.m_GroupCountY );
            builder.append_comma();
            builder.append_key_value( "groupCountZ", drawcall.m_Payload.m_Dispatch.m_GroupCountZ );
            break;

        case DeviceProfilerDrawcallType::eDispatchIndirect:
            builder.append_key_value( "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DispatchIndirect.m_Buffer ) );
            builder.append_comma();
            builder.append_key_value( "offset", drawcall.m_Payload.m_DispatchIndirect.m_Offset );
            break;

        case DeviceProfilerDrawcallType::eCopyBuffer:
            builder.append_key_value( "srcBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyBuffer.m_SrcBuffer ) );
            builder.append_comma();
            builder.append_key_value( "dstBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyBuffer.m_DstBuffer ) );
            break;

        case DeviceProfilerDrawcallType::eCopyBufferToImage:
            builder.append_key_value( "srcBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyBufferToImage.m_SrcBuffer ) );
            builder.append_comma();
            builder.append_key_value( "dstImage", m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyBufferToImage.m_DstImage ) );
            break;

        case DeviceProfilerDrawcallType::eCopyImage:
            builder.append_key_value( "srcImage", m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyImage.m_SrcImage ) );
            builder.append_comma();
            builder.append_key_value( "dstImage", m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyImage.m_DstImage ) );
            break;

        case DeviceProfilerDrawcallType::eCopyImageToBuffer:
            builder.append_key_value( "srcImage", m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyImageToBuffer.m_SrcImage ) );
            builder.append_comma();
            builder.append_key_value( "dstBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyImageToBuffer.m_DstBuffer ) );
            break;

        case DeviceProfilerDrawcallType::eClearAttachments:
            builder.append_key_value( "attachmentCount", drawcall.m_Payload.m_ClearAttachments.m_Count );
            break;

        case DeviceProfilerDrawcallType::eClearColorImage:
            builder.append_key_value( "image", m_pStringSerializer->GetName( drawcall.m_Payload.m_ClearColorImage.m_Image ) );
            builder.append_comma();
            builder.escape_and_append_with_quotes( "value" );
            builder.append_colon();
            WriteColorClearValue( builder, drawcall.m_Payload.m_ClearColorImage.m_Value );
            break;

        case DeviceProfilerDrawcallType::eClearDepthStencilImage:
            builder.append_key_value( "image", m_pStringSerializer->GetName( drawcall.m_Payload.m_ClearDepthStencilImage.m_Image ) );
            builder.append_comma();
            builder.escape_and_append_with_quotes( "value" );
            builder.append_colon();
            WriteDepthStencilClearValue( builder, drawcall.m_Payload.m_ClearDepthStencilImage.m_Value );
            break;

        case DeviceProfilerDrawcallType::eResolveImage:
            builder.append_key_value( "srcImage", m_pStringSerializer->GetName( drawcall.m_Payload.m_ResolveImage.m_SrcImage ) );
            builder.append_comma();
            builder.append_key_value( "dstImage", m_pStringSerializer->GetName( drawcall.m_Payload.m_ResolveImage.m_DstImage ) );
            break;

        case DeviceProfilerDrawcallType::eBlitImage:
            builder.append_key_value( "srcImage", m_pStringSerializer->GetName( drawcall.m_Payload.m_BlitImage.m_SrcImage ) );
            builder.append_comma();
            builder.append_key_value( "dstImage", m_pStringSerializer->GetName( drawcall.m_Payload.m_BlitImage.m_DstImage ) );
            break;

        case DeviceProfilerDrawcallType::eFillBuffer:
            builder.append_key_value( "dstBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_FillBuffer.m_Buffer ) );
            builder.append_comma();
            builder.append_key_value( "dstOffset", drawcall.m_Payload.m_FillBuffer.m_Offset );
            builder.append_comma();
            builder.append_key_value( "size", drawcall.m_Payload.m_FillBuffer.m_Size );
            builder.append_comma();
            builder.append_key_value( "data", drawcall.m_Payload.m_FillBuffer.m_Data );
            break;

        case DeviceProfilerDrawcallType::eUpdateBuffer:
            builder.append_key_value( "dstBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_UpdateBuffer.m_Buffer ) );
            builder.append_comma();
            builder.append_key_value( "dstOffset", drawcall.m_Payload.m_UpdateBuffer.m_Offset );
            builder.append_comma();
            builder.append_key_value( "dataSize", drawcall.m_Payload.m_UpdateBuffer.m_Size );
            break;

        case DeviceProfilerDrawcallType::eTraceRaysKHR:
            builder.append_key_value( "width", drawcall.m_Payload.m_TraceRays.m_Width );
            builder.append_comma();
            builder.append_key_value( "height", drawcall.m_Payload.m_TraceRays.m_Height );
            builder.append_comma();
            builder.append_key_value( "depth", drawcall.m_Payload.m_TraceRays.m_Depth );
            break;

        case DeviceProfilerDrawcallType::eTraceRaysIndirectKHR:
            builder.append_key_value( "indirectDeviceAddress", drawcall.m_Payload.m_TraceRaysIndirect.m_IndirectAddress );
            break;

        case DeviceProfilerDrawcallType::eTraceRaysIndirect2KHR:
            builder.append_key_value( "indirectDeviceAddress", drawcall.m_Payload.m_TraceRaysIndirect2.m_IndirectAddress );
            break;

        case DeviceProfilerDrawcallType::eBuildAccelerationStructuresKHR:
        case DeviceProfilerDrawcallType::eBuildAccelerationStructuresIndirectKHR:
        {
            const uint32_t infoCount = drawcall.m_Payload.m_BuildAccelerationStructures.m_InfoCount;

            builder.append_key_value( "infoCount", infoCount );
            builder.append_comma();

            builder.escape_and_append_with_quotes( "infos" );
            builder.append_colon();
            builder.start_array();

            for( uint32_t i = 0; i < infoCount; ++i )
            {
                const auto& info = drawcall.m_Payload.m_BuildAccelerationStructures.m_pInfos[i];

                if( i > 0 ) builder.append_comma();
                builder.start_object();
                builder.append_key_value( "type", m_pStringSerializer->GetAccelerationStructureTypeName( info.type ) );
                builder.append_comma();
                builder.append_key_value( "flags", m_pStringSerializer->GetBuildAccelerationStructureFlagNames( info.flags ) );
                builder.append_comma();
                builder.append_key_value( "mode", m_pStringSerializer->GetBuildAccelerationStructureModeName( info.mode ) );
                builder.append_comma();
                builder.append_key_value( "src", m_pStringSerializer->GetName( VkAccelerationStructureKHRHandle( info.srcAccelerationStructure ) ) );
                builder.append_comma();
                builder.append_key_value( "dst", m_pStringSerializer->GetName( VkAccelerationStructureKHRHandle( info.dstAccelerationStructure ) ) );
                builder.append_comma();
                builder.append_key_value( "geometryCount", info.geometryCount );
                builder.append_comma();

                builder.escape_and_append_with_quotes( "geometries" );
                builder.append_colon();

                if( info.pGeometries )
                {
                    builder.start_array();

                    for( uint32_t j = 0; j < info.geometryCount; ++j )
                    {
                        const auto& geometry = info.pGeometries[ j ];

                        if( j > 0 ) builder.append_comma();
                        builder.start_object();

                        builder.append_key_value( "type", m_pStringSerializer->GetGeometryTypeName( geometry.geometryType ) );
                        builder.append_comma();
                        builder.append_key_value( "flags", m_pStringSerializer->GetGeometryFlagNames( geometry.flags ) );
                        builder.append_comma();

                        builder.escape_and_append_with_quotes( "data" );
                        builder.append_colon();
                        builder.start_object();

                        switch( geometry.geometryType )
                        {
                        case VK_GEOMETRY_TYPE_TRIANGLES_KHR:
                            builder.append_key_value( "vertexFormat", m_pStringSerializer->GetFormatName( geometry.geometry.triangles.vertexFormat ) );
                            builder.append_comma();
                            builder.append_key_value( "vertexData", m_pStringSerializer->GetPointer( geometry.geometry.triangles.vertexData.hostAddress ) );
                            builder.append_comma();
                            builder.append_key_value( "vertexStride", geometry.geometry.triangles.vertexStride );
                            builder.append_comma();
                            builder.append_key_value( "maxVertex", geometry.geometry.triangles.maxVertex );
                            builder.append_comma();
                            builder.append_key_value( "indexType", m_pStringSerializer->GetIndexTypeName( geometry.geometry.triangles.indexType ) );
                            builder.append_comma();
                            builder.append_key_value( "indexData", m_pStringSerializer->GetPointer( geometry.geometry.triangles.indexData.hostAddress ) );
                            builder.append_comma();
                            builder.append_key_value( "transformData", m_pStringSerializer->GetPointer( geometry.geometry.triangles.transformData.hostAddress ) );
                            break;

                        case VK_GEOMETRY_TYPE_AABBS_KHR:
                            builder.append_key_value( "data", m_pStringSerializer->GetPointer( geometry.geometry.aabbs.data.hostAddress ) );
                            builder.append_comma();
                            builder.append_key_value( "stride", geometry.geometry.aabbs.stride );
                            break;

                        case VK_GEOMETRY_TYPE_INSTANCES_KHR:
                            builder.append_key_value( "arrayOfPointers", static_cast<bool>( geometry.geometry.instances.arrayOfPointers ) );
                            builder.append_comma();
                            builder.append_key_value( "data", m_pStringSerializer->GetPointer( geometry.geometry.instances.data.hostAddress ) );
                            break;
                        }

                        builder.end_object();
                        builder.append_comma();

                        builder.escape_and_append_with_quotes( "range" );
                        builder.append_colon();
                        builder.start_object();

                        if( drawcall.m_Type == DeviceProfilerDrawcallType::eBuildAccelerationStructuresKHR )
                        {
                            const auto& range = drawcall.m_Payload.m_BuildAccelerationStructures.m_ppRanges[ i ][ j ];
                            builder.append_key_value( "primitiveCount", range.primitiveCount );
                            builder.append_comma();
                            builder.append_key_value( "primitiveOffset", range.primitiveOffset );
                            builder.append_comma();
                            builder.append_key_value( "firstVertex", range.firstVertex );
                            builder.append_comma();
                            builder.append_key_value( "transformOffset", range.transformOffset );
                        }
                        else //( drawcall.m_Type == DeviceProfilerDrawcallType::eBuildAccelerationStructuresIndirectKHR )
                        {
                            builder.append_key_value( "maxPrimitiveCount", drawcall.m_Payload.m_BuildAccelerationStructuresIndirect.m_ppMaxPrimitiveCounts[ i ][ j ] );
                        }

                        builder.end_object();
                    }

                    builder.end_array();
                }
                else
                {
                    builder.append_null();
                }

                builder.end_object();
            }

            builder.end_array();
            break;
        }

        case DeviceProfilerDrawcallType::eBuildMicromapsEXT:
        {
            const uint32_t infoCount = drawcall.m_Payload.m_BuildMicromaps.m_InfoCount;

            builder.append_key_value( "infoCount", infoCount );
            builder.append_comma();

            builder.escape_and_append_with_quotes( "infos" );
            builder.append_colon();
            builder.start_array();

            for( uint32_t i = 0; i < infoCount; ++i )
            {
                const auto& info = drawcall.m_Payload.m_BuildMicromaps.m_pInfos[i];

                if( i > 0 ) builder.append_comma();
                builder.start_object();
                builder.append_key_value( "type", m_pStringSerializer->GetMicromapTypeName( info.type ) );
                builder.append_comma();
                builder.append_key_value( "flags", m_pStringSerializer->GetBuildMicromapFlagNames( info.flags ) );
                builder.append_comma();
                builder.append_key_value( "mode", m_pStringSerializer->GetBuildMicromapModeName( info.mode ) );
                builder.append_comma();
                builder.append_key_value( "dst", m_pStringSerializer->GetName( VkMicromapEXTHandle( info.dstMicromap ) ) );
                builder.append_comma();
                builder.append_key_value( "usageCountsCount", info.usageCountsCount );
                builder.append_comma();

                builder.escape_and_append_with_quotes( "usageCounts" );
                builder.append_colon();
                builder.start_array();

                for( uint32_t j = 0; j < info.usageCountsCount; ++j )
                {
                    const auto& usageCount = info.pUsageCounts[j];

                    if( j > 0 ) builder.append_comma();
                    builder.start_object();
                    builder.append_key_value( "count", usageCount.count );
                    builder.append_comma();
                    builder.append_key_value( "format", usageCount.format );
                    builder.append_comma();
                    builder.append_key_value( "subdivisionLevel", usageCount.subdivisionLevel );
                    builder.end_object();
                }

                builder.end_array();
                builder.append_comma();

                builder.append_key_value( "data", m_pStringSerializer->GetPointer( info.data.hostAddress ) );
                builder.append_comma();
                builder.append_key_value( "scratchData", m_pStringSerializer->GetPointer( info.scratchData.hostAddress ) );
                builder.append_comma();
                builder.append_key_value( "triangleArray", m_pStringSerializer->GetPointer( info.triangleArray.hostAddress ) );
                builder.append_comma();
                builder.append_key_value( "triangleArrayStride", info.triangleArrayStride );
                builder.end_object();
            }

            builder.end_array();
            break;
        }
        }

        builder.end_object();
    }

    /*************************************************************************\

    Function:
        GetPipelineArgs

    Description:
        Serialize pipeline state into JSON object.

    \*************************************************************************/
    void DeviceProfilerJsonSerializer::WritePipelineArgs( simdjson::builder::string_builder& builder, const DeviceProfilerPipeline& pipeline ) const
    {
        bool firstArg = true;

        builder.start_object();

        // Append shader stages info.
        if( !pipeline.m_ShaderTuple.m_Shaders.empty() )
        {
            builder.escape_and_append_with_quotes( "shaders" );
            builder.append_colon();
            builder.start_array();

            bool firstShader = true;
            for( const ProfilerShader& shader : pipeline.m_ShaderTuple.m_Shaders )
            {
                if( !firstShader ) builder.append_comma();
                WriteShaderStageArgs( builder, shader );
                firstShader = false;
            }

            builder.end_array();
            firstArg = false;
        }

        // Append pipeline create info details.
        if( pipeline.m_pCreateInfo )
        {
            switch( pipeline.m_Type )
            {
            case DeviceProfilerPipelineType::eGraphics:
            {
                WriteGraphicsPipelineCreateInfoArgs( builder,
                    pipeline.m_pCreateInfo->m_GraphicsPipelineCreateInfo, firstArg );
                break;
            }
            case DeviceProfilerPipelineType::eCompute:
            {
                WriteComputePipelineCreateInfoArgs( builder,
                    pipeline.m_pCreateInfo->m_ComputePipelineCreateInfo, firstArg );
                break;
            }
            case DeviceProfilerPipelineType::eRayTracingKHR:
            {
                WriteRayTracingPipelineCreateInfoArgs( builder,
                    pipeline.m_pCreateInfo->m_RayTracingPipelineCreateInfoKHR, firstArg );
                break;
            }
            }
        }

        builder.end_object();
    }

    /*************************************************************************\

    Function:
        GetColorClearValue

    Description:
        Serialize VkClearColorValue struct into JSON object.

    \*************************************************************************/
    void DeviceProfilerJsonSerializer::WriteColorClearValue( simdjson::builder::string_builder& builder, const VkClearColorValue& value ) const
    {
        builder.start_object();

        builder.escape_and_append_with_quotes( "VkClearColorValue" );
        builder.append_colon();
        builder.start_object();

        builder.escape_and_append_with_quotes( "float32" );
        builder.append_colon();
        builder.start_array();

        for( int i = 0; i < 4; ++i )
        {
            if( i > 0 ) builder.append_comma();
            builder.append( value.float32[i] );
        }

        builder.end_array();
        builder.append_comma();

        builder.escape_and_append_with_quotes( "int32" );
        builder.append_colon();
        builder.start_array();

        for( int i = 0; i < 4; ++i )
        {
            if( i > 0 ) builder.append_comma();
            builder.append( value.int32[i] );
        }

        builder.end_array();
        builder.append_comma();

        builder.escape_and_append_with_quotes( "uint32" );
        builder.append_colon();
        builder.start_array();

        for( int i = 0; i < 4; ++i )
        {
            if( i > 0 ) builder.append_comma();
            builder.append( value.uint32[i] );
        }

        builder.end_array();
        builder.end_object();

        builder.end_object();
    }

    /*************************************************************************\

    Function:
        GetDepthStencilClearValue

    Description:
        Serialize VkClearDepthStencilValue struct into JSON object.

    \*************************************************************************/
    void DeviceProfilerJsonSerializer::WriteDepthStencilClearValue( simdjson::builder::string_builder& builder, const VkClearDepthStencilValue& value ) const
    {
        builder.start_object();

        builder.escape_and_append_with_quotes( "VkClearDepthStencilValue" );
        builder.append_colon();
        builder.start_object();

        builder.append_key_value( "depth", value.depth );
        builder.append_comma();
        builder.append_key_value( "stencil", value.stencil );

        builder.end_object();

        builder.end_object();
    }

    /*************************************************************************\

    Function:
        GetShaderStageArgs

    Description:
        Serialize shader stage into JSON object.

    \*************************************************************************/
    void DeviceProfilerJsonSerializer::WriteShaderStageArgs( simdjson::builder::string_builder& builder, const ProfilerShader& shader ) const
    {
        builder.start_object();

        builder.append_key_value( "stage", m_pStringSerializer->GetShaderStageName( shader.m_Stage ) );
        builder.append_comma();
        builder.append_key_value( "entryPoint", shader.m_EntryPoint );

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

            builder.append_comma();
            builder.append_key_value( "shaderIdentifier", shaderIdentifier );
        }

        builder.end_object();
    }

    /*************************************************************************\

    Function:
        GetGraphicsPipelineCreateInfoArgs

    Description:
        Serialize graphics pipeline state into JSON object.

    \*************************************************************************/
    void DeviceProfilerJsonSerializer::WriteGraphicsPipelineCreateInfoArgs( simdjson::builder::string_builder& builder, const VkGraphicsPipelineCreateInfo& createInfo, bool firstArg ) const
    {
        // VkPipelineVertexInputStateCreateInfo
        if( !firstArg ) builder.append_comma();
        builder.escape_and_append_with_quotes( "vertexInputState" );
        builder.append_colon();

        if( createInfo.pVertexInputState )
        {
            const auto& state = *createInfo.pVertexInputState;

            builder.start_object();

            builder.append_key_value( "attributeCount", state.vertexAttributeDescriptionCount );
            builder.append_comma();
            builder.append_key_value( "bindingCount", state.vertexBindingDescriptionCount );
            builder.append_comma();

            builder.escape_and_append_with_quotes( "attributes" );
            builder.append_colon();

            if( state.pVertexAttributeDescriptions )
            {
                builder.start_array();

                for( uint32_t i = 0; i < state.vertexAttributeDescriptionCount; ++i )
                {
                    const auto& attribute = state.pVertexAttributeDescriptions[i];

                    if( i > 0 ) builder.append_comma();
                    builder.start_object();
                    builder.append_key_value( "location", attribute.location );
                    builder.append_comma();
                    builder.append_key_value( "binding", attribute.binding );
                    builder.append_comma();
                    builder.append_key_value( "format", m_pStringSerializer->GetFormatName( attribute.format ) );
                    builder.append_comma();
                    builder.append_key_value( "offset", attribute.offset );
                    builder.end_object();
                }

                builder.end_array();
            }
            else
            {
                builder.append_null();
            }

            builder.append_comma();
            builder.escape_and_append_with_quotes( "bindings" );
            builder.append_colon();

            if( state.pVertexBindingDescriptions )
            {
                builder.start_array();

                for( uint32_t i = 0; i < state.vertexBindingDescriptionCount; ++i )
                {
                    const auto& binding = state.pVertexBindingDescriptions[i];

                    if( i > 0 ) builder.append_comma();
                    builder.start_object();
                    builder.append_key_value( "binding", binding.binding );
                    builder.append_comma();
                    builder.append_key_value( "stride", binding.stride );
                    builder.append_comma();
                    builder.append_key_value( "inputRate", m_pStringSerializer->GetVertexInputRateName( binding.inputRate ) );
                    builder.end_object();
                }

                builder.end_array();
            }
            else
            {
                builder.append_null();
            }

            builder.end_object();
        }
        else
        {
            builder.append_null();
        }


        // VkPipelineInputAssemblyStateCreateInfo
        builder.append_comma();
        builder.escape_and_append_with_quotes( "inputAssemblyState" );
        builder.append_colon();

        if( createInfo.pInputAssemblyState )
        {
            const auto& state = *createInfo.pInputAssemblyState;

            builder.start_object();
            builder.append_key_value( "topology", m_pStringSerializer->GetPrimitiveTopologyName( state.topology ) );
            builder.append_comma();
            builder.append_key_value( "primitiveRestartEnable", static_cast<bool>( state.primitiveRestartEnable ) );
            builder.end_object();
        }
        else
        {
            builder.append_null();
        }

        // VkPipelineTessellationStateCreateInfo
        builder.append_comma();
        builder.escape_and_append_with_quotes( "tessellationState" );
        builder.append_colon();

        if( createInfo.pTessellationState )
        {
            const auto& state = *createInfo.pTessellationState;

            builder.start_object();
            builder.append_key_value( "patchControlPoints", state.patchControlPoints );
            builder.end_object();
        }
        else
        {
            builder.append_null();
        }

        // VkPipelineViewportStateCreateInfo
        builder.append_comma();
        builder.escape_and_append_with_quotes( "viewportState" );
        builder.append_colon();

        if( createInfo.pViewportState )
        {
            const auto& state = *createInfo.pViewportState;

            builder.start_object();

            builder.append_key_value( "viewportCount", state.viewportCount );
            builder.append_comma();
            builder.append_key_value( "scissorCount", state.scissorCount );
            builder.append_comma();

            builder.escape_and_append_with_quotes( "viewports" );
            builder.append_colon();

            if( state.pViewports )
            {
                builder.start_array();

                for( uint32_t i = 0; i < state.viewportCount; ++i )
                {
                    const auto& viewport = state.pViewports[i];

                    if( i > 0 ) builder.append_comma();
                    builder.start_object();
                    builder.append_key_value( "x", viewport.x );
                    builder.append_comma();
                    builder.append_key_value( "y", viewport.y );
                    builder.append_comma();
                    builder.append_key_value( "width", viewport.width );
                    builder.append_comma();
                    builder.append_key_value( "height", viewport.height );
                    builder.append_comma();
                    builder.append_key_value( "minDepth", viewport.minDepth );
                    builder.append_comma();
                    builder.append_key_value( "maxDepth", viewport.maxDepth );
                    builder.end_object();
                }

                builder.end_array();
            }
            else
            {
                builder.append_null();
            }

            builder.append_comma();
            builder.escape_and_append_with_quotes( "scissors" );
            builder.append_colon();

            if( state.pScissors )
            {
                builder.start_array();

                for( uint32_t i = 0; i < state.scissorCount; ++i )
                {
                    const auto& scissor = state.pScissors[i];

                    if( i > 0 ) builder.append_comma();
                    builder.start_object();
                    builder.append_key_value( "offsetX", scissor.offset.x );
                    builder.append_comma();
                    builder.append_key_value( "offsetY", scissor.offset.y );
                    builder.append_comma();
                    builder.append_key_value( "extentWidth", scissor.extent.width );
                    builder.append_comma();
                    builder.append_key_value( "extentHeight", scissor.extent.height );
                    builder.end_object();
                }

                builder.end_array();
            }
            else
            {
                builder.append_null();
            }

            builder.end_object();
        }
        else
        {
            builder.append_null();
        }

        // VkPipelineRasterizationStateCreateInfo
        builder.append_comma();
        builder.escape_and_append_with_quotes( "rasterizationState" );
        builder.append_colon();

        if( createInfo.pRasterizationState )
        {
            const auto& state = *createInfo.pRasterizationState;

            builder.start_object();
            builder.append_key_value( "depthClampEnable", static_cast<bool>( state.depthClampEnable ) );
            builder.append_comma();
            builder.append_key_value( "rasterizerDiscardEnable", static_cast<bool>( state.rasterizerDiscardEnable ) );
            builder.append_comma();
            builder.append_key_value( "polygonMode", m_pStringSerializer->GetPolygonModeName( state.polygonMode ) );
            builder.append_comma();
            builder.append_key_value( "cullMode", m_pStringSerializer->GetCullModeName( state.cullMode ) );
            builder.append_comma();
            builder.append_key_value( "frontFace", m_pStringSerializer->GetFrontFaceName( state.frontFace ) );
            builder.append_comma();
            builder.append_key_value( "depthBiasEnable", static_cast<bool>( state.depthBiasEnable ) );
            builder.append_comma();
            builder.append_key_value( "depthBiasConstantFactor", state.depthBiasConstantFactor );
            builder.append_comma();
            builder.append_key_value( "depthBiasClamp", state.depthBiasClamp );
            builder.append_comma();
            builder.append_key_value( "depthBiasSlopeFactor", state.depthBiasSlopeFactor );
            builder.append_comma();
            builder.append_key_value( "lineWidth", state.lineWidth );
            builder.end_object();
        }
        else
        {
            builder.append_null();
        }

        // VkPipelineMultisampleStateCreateInfo
        builder.append_comma();
        builder.escape_and_append_with_quotes( "multisampleState" );
        builder.append_colon();

        if( createInfo.pMultisampleState )
        {
            const auto& state = *createInfo.pMultisampleState;

            builder.start_object();
            builder.append_key_value( "rasterizationSamples", static_cast<uint32_t>( state.rasterizationSamples ) );
            builder.append_comma();
            builder.append_key_value( "sampleShadingEnable", static_cast<bool>( state.sampleShadingEnable ) );
            builder.append_comma();
            builder.append_key_value( "minSampleShading", state.minSampleShading );
            builder.append_comma();
            builder.append_key_value( "sampleMask", fmt::format( "0x{:08X}", state.pSampleMask ? *state.pSampleMask : 0xFFFFFFFF ) );
            builder.append_comma();
            builder.append_key_value( "alphaToCoverateEnable", static_cast<bool>( state.alphaToCoverageEnable ) );
            builder.append_comma();
            builder.append_key_value( "alphaToOneEnable", static_cast<bool>( state.alphaToOneEnable ) );
            builder.end_object();
        }
        else
        {
            builder.append_null();
        }

        // VkPipelineDepthStencilStateCreateInfo
        builder.append_comma();
        builder.escape_and_append_with_quotes( "depthStencilState" );
        builder.append_colon();

        if( createInfo.pDepthStencilState )
        {
            const auto& state = *createInfo.pDepthStencilState;

            builder.start_object();
            builder.append_key_value( "depthTestEnable", static_cast<bool>( state.depthTestEnable ) );
            builder.append_comma();
            builder.append_key_value( "depthWriteEnable", static_cast<bool>( state.depthWriteEnable ) );
            builder.append_comma();
            builder.append_key_value( "depthCompareOp", m_pStringSerializer->GetCompareOpName( state.depthCompareOp ) );
            builder.append_comma();
            builder.append_key_value( "depthBoundsTestEnable", static_cast<bool>( state.depthBoundsTestEnable ) );
            builder.append_comma();
            builder.append_key_value( "minDepthBounds", state.minDepthBounds );
            builder.append_comma();
            builder.append_key_value( "maxDepthBounds", state.maxDepthBounds );
            builder.append_comma();
            builder.append_key_value( "stencilTestEnable", static_cast<bool>( state.stencilTestEnable ) );
            builder.append_comma();

            builder.escape_and_append_with_quotes( "front" );
            builder.append_colon();

            builder.start_object();
            builder.append_key_value( "failOp", static_cast<uint32_t>( state.front.failOp ) );
            builder.append_comma();
            builder.append_key_value( "passOp", static_cast<uint32_t>( state.front.passOp ) );
            builder.append_comma();
            builder.append_key_value( "depthFailOp", static_cast<uint32_t>( state.front.depthFailOp ) );
            builder.append_comma();
            builder.append_key_value( "compareOp", m_pStringSerializer->GetCompareOpName( state.front.compareOp ) );
            builder.append_comma();
            builder.append_key_value( "compareMask", fmt::format( "0x{:02X}", state.front.compareMask ) );
            builder.append_comma();
            builder.append_key_value( "writeMask", fmt::format( "0x{:02X}", state.front.writeMask ) );
            builder.append_comma();
            builder.append_key_value( "reference", fmt::format( "0x{:02X}", state.front.reference ) );
            builder.end_object();
            builder.append_comma();

            builder.escape_and_append_with_quotes( "back" );
            builder.append_colon();

            builder.start_object();
            builder.append_key_value( "failOp", static_cast<uint32_t>( state.back.failOp ) );
            builder.append_comma();
            builder.append_key_value( "passOp", static_cast<uint32_t>( state.back.passOp ) );
            builder.append_comma();
            builder.append_key_value( "depthFailOp", static_cast<uint32_t>( state.back.depthFailOp ) );
            builder.append_comma();
            builder.append_key_value( "compareOp", m_pStringSerializer->GetCompareOpName( state.back.compareOp ) );
            builder.append_comma();
            builder.append_key_value( "compareMask", fmt::format( "0x{:02X}", state.back.compareMask ) );
            builder.append_comma();
            builder.append_key_value( "writeMask", fmt::format( "0x{:02X}", state.back.writeMask ) );
            builder.append_comma();
            builder.append_key_value( "reference", fmt::format( "0x{:02X}", state.back.reference ) );
            builder.end_object();

            builder.end_object();
        }
        else
        {
            builder.append_null();
        }

        // VkPipelineColorBlendStateCreateInfo
        builder.append_comma();
        builder.escape_and_append_with_quotes( "colorBlendState" );
        builder.append_colon();

        if( createInfo.pColorBlendState )
        {
            const auto& state = *createInfo.pColorBlendState;

            builder.start_object();
            builder.append_key_value( "logicOpEnable", static_cast<bool>( state.logicOpEnable ) );
            builder.append_comma();
            builder.append_key_value( "logicOp", m_pStringSerializer->GetLogicOpName( state.logicOp ) );
            builder.append_comma();

            builder.escape_and_append_with_quotes( "blendConstants" );
            builder.append_colon();
            builder.start_array();

            for( int i = 0; i < 4; ++i )
            {
                if( i > 0 ) builder.append_comma();
                builder.append( state.blendConstants[i] );
            }

            builder.end_array();
            builder.append_comma();

            builder.escape_and_append_with_quotes( "attachments" );
            builder.append_colon();

            if( state.pAttachments )
            {
                builder.start_array();

                for( uint32_t i = 0; i < state.attachmentCount; ++i )
                {
                    const auto& attachment = state.pAttachments[i];

                    if( i > 0 ) builder.append_comma();
                    builder.start_object();
                    builder.append_key_value( "blendEnable", static_cast<bool>( attachment.blendEnable ) );
                    builder.append_comma();
                    builder.append_key_value( "srcColorBlendFactor", m_pStringSerializer->GetBlendFactorName( attachment.srcColorBlendFactor ) );
                    builder.append_comma();
                    builder.append_key_value( "dstColorBlendFactor", m_pStringSerializer->GetBlendFactorName( attachment.dstColorBlendFactor ) );
                    builder.append_comma();
                    builder.append_key_value( "colorBlendOp", m_pStringSerializer->GetBlendOpName( attachment.colorBlendOp ) );
                    builder.append_comma();
                    builder.append_key_value( "srcAlphaBlendFactor", m_pStringSerializer->GetBlendFactorName( attachment.srcAlphaBlendFactor ) );
                    builder.append_comma();
                    builder.append_key_value( "dstAlphaBlendFactor", m_pStringSerializer->GetBlendFactorName( attachment.dstAlphaBlendFactor ) );
                    builder.append_comma();
                    builder.append_key_value( "alphaBlendOp", m_pStringSerializer->GetBlendOpName( attachment.alphaBlendOp ) );
                    builder.append_comma();
                    builder.append_key_value( "colorWriteMask", m_pStringSerializer->GetColorComponentFlagNames( attachment.colorWriteMask ) );
                    builder.end_object();
                }

                builder.end_array();
            }
            else
            {
                builder.append_null();
            }

            builder.end_object();
        }
        else
        {
            builder.append_null();
        }

        // VkPipelineDynamicStateCreateInfo
        builder.append_comma();
        builder.escape_and_append_with_quotes( "dynamicStates" );
        builder.append_colon();

        if( createInfo.pDynamicState )
        {
            builder.start_array();

            const auto& state = *createInfo.pDynamicState;
            for( uint32_t i = 0; i < state.dynamicStateCount; ++i )
            {
                if( i > 0 ) builder.append_comma();
                builder.escape_and_append_with_quotes(
                    m_pStringSerializer->GetDynamicStateName( state.pDynamicStates[i] ) );
            }

            builder.end_array();
        }
    }

    /*************************************************************************\

    Function:
        GetComputePipelineCreateInfoArgs

    Description:
        Serialize compute pipeline state into JSON object.

    \*************************************************************************/
    void DeviceProfilerJsonSerializer::WriteComputePipelineCreateInfoArgs( simdjson::builder::string_builder& builder, const VkComputePipelineCreateInfo& createInfo, bool firstArg ) const
    {
        // No additional state to serialize for compute pipelines yet.
    }

    /*************************************************************************\

    Function:
        GetRayTracingPipelineCreateInfoArgs

    Description:
        Serialize ray-tracing pipeline state into JSON object.

    \*************************************************************************/
    void DeviceProfilerJsonSerializer::WriteRayTracingPipelineCreateInfoArgs( simdjson::builder::string_builder& builder, const VkRayTracingPipelineCreateInfoKHR& createInfo, bool firstArg ) const
    {
        if( !firstArg ) builder.append_comma();

        builder.append_key_value( "maxPipelineRayRecursionDepth", createInfo.maxPipelineRayRecursionDepth );
        builder.append_comma();

        // VkRayTracingPipelineInterfaceCreateInfoKHR
        builder.escape_and_append_with_quotes( "libraryInterface" );
        builder.append_colon();

        if( createInfo.pLibraryInterface )
        {
            const auto& state = *createInfo.pLibraryInterface;
            builder.start_object();
            builder.append_key_value( "maxPipelineRayPayloadSize", state.maxPipelineRayPayloadSize );
            builder.append_comma();
            builder.append_key_value( "maxPipelineRayHitAttributeSize", state.maxPipelineRayHitAttributeSize );
            builder.end_object();
        }

        // VkPipelineDynamicStateCreateInfo
        builder.append_comma();
        builder.escape_and_append_with_quotes( "dynamicStates" );
        builder.append_colon();

        if( createInfo.pDynamicState )
        {
            builder.start_array();

            const auto& state = *createInfo.pDynamicState;
            for( uint32_t i = 0; i < state.dynamicStateCount; ++i )
            {
                if( i > 0 ) builder.append_comma();
                builder.escape_and_append_with_quotes(
                    m_pStringSerializer->GetDynamicStateName( state.pDynamicStates[i] ) );
            }

            builder.end_array();
        }
    }
}

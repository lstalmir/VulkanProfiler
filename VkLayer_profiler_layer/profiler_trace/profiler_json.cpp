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
        , m_pDocument( nullptr )
    {
    }

    /*************************************************************************\

    Function:
        SetDocument

    Description:
        Set the JSON document to write to.

    \*************************************************************************/
    void DeviceProfilerJsonSerializer::SetDocument( yyjson_mut_doc* pDocument )
    {
        m_pDocument = pDocument;
    }

    /*************************************************************************\

    Function:
        WriteCommandArgs

    Description:
        Serialize command arguments into JSON object.

    \*************************************************************************/
    void DeviceProfilerJsonSerializer::WriteCommandArgs( yyjson_mut_val* pValue, const DeviceProfilerDrawcall& drawcall ) const
    {
        switch( drawcall.m_Type )
        {
        default:
        case DeviceProfilerDrawcallType::eUnknown:
        case DeviceProfilerDrawcallType::eInsertDebugLabel:
        case DeviceProfilerDrawcallType::eBeginDebugLabel:
        case DeviceProfilerDrawcallType::eEndDebugLabel:
            return;

        case DeviceProfilerDrawcallType::eDraw:
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "vertexCount", drawcall.m_Payload.m_Draw.m_VertexCount );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "instanceCount", drawcall.m_Payload.m_Draw.m_InstanceCount );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "firstVertex", drawcall.m_Payload.m_Draw.m_FirstVertex );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "firstInstance", drawcall.m_Payload.m_Draw.m_FirstInstance );
            return;

        case DeviceProfilerDrawcallType::eDrawIndexed:
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "indexCount", drawcall.m_Payload.m_DrawIndexed.m_IndexCount );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "instanceCount", drawcall.m_Payload.m_DrawIndexed.m_InstanceCount );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "firstIndex", drawcall.m_Payload.m_DrawIndexed.m_FirstIndex );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "vertexOffset", drawcall.m_Payload.m_DrawIndexed.m_VertexOffset );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "firstInstance", drawcall.m_Payload.m_DrawIndexed.m_FirstInstance );
            return;

        case DeviceProfilerDrawcallType::eDrawIndirect:
            yyjson_mut_obj_add_strcpy( m_pDocument, pValue, "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawIndirect.m_Buffer ).c_str() );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "offset", drawcall.m_Payload.m_DrawIndirect.m_Offset );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "drawCount", drawcall.m_Payload.m_DrawIndirect.m_DrawCount );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "stride", drawcall.m_Payload.m_DrawIndirect.m_Stride );
            return;

        case DeviceProfilerDrawcallType::eDrawIndexedIndirect:
            yyjson_mut_obj_add_strcpy( m_pDocument, pValue, "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawIndexedIndirect.m_Buffer ).c_str() );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "offset", drawcall.m_Payload.m_DrawIndexedIndirect.m_Offset );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "drawCount", drawcall.m_Payload.m_DrawIndexedIndirect.m_DrawCount );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "stride", drawcall.m_Payload.m_DrawIndexedIndirect.m_Stride );
            return;

        case DeviceProfilerDrawcallType::eDrawIndirectCount:
            yyjson_mut_obj_add_strcpy( m_pDocument, pValue, "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawIndirectCount.m_Buffer ).c_str() );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "offset", drawcall.m_Payload.m_DrawIndirectCount.m_Offset );
            yyjson_mut_obj_add_strcpy( m_pDocument, pValue, "countBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawIndirectCount.m_CountBuffer ).c_str() );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "countOffset", drawcall.m_Payload.m_DrawIndirectCount.m_CountOffset );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "maxDrawCount", drawcall.m_Payload.m_DrawIndirectCount.m_MaxDrawCount );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "stride", drawcall.m_Payload.m_DrawIndirectCount.m_Stride );
            return;

        case DeviceProfilerDrawcallType::eDrawIndexedIndirectCount:
            yyjson_mut_obj_add_strcpy( m_pDocument, pValue, "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawIndexedIndirectCount.m_Buffer ).c_str() );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "offset", drawcall.m_Payload.m_DrawIndexedIndirectCount.m_Offset );
            yyjson_mut_obj_add_strcpy( m_pDocument, pValue, "countBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawIndexedIndirectCount.m_CountBuffer ).c_str() );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "countOffset", drawcall.m_Payload.m_DrawIndexedIndirectCount.m_CountOffset );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "maxDrawCount", drawcall.m_Payload.m_DrawIndexedIndirectCount.m_MaxDrawCount );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "stride", drawcall.m_Payload.m_DrawIndexedIndirectCount.m_Stride );
            return;

        case DeviceProfilerDrawcallType::eDrawMeshTasks:
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "groupCountX", drawcall.m_Payload.m_DrawMeshTasks.m_GroupCountX );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "groupCountY", drawcall.m_Payload.m_DrawMeshTasks.m_GroupCountY );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "groupCountZ", drawcall.m_Payload.m_DrawMeshTasks.m_GroupCountZ );
            return;

        case DeviceProfilerDrawcallType::eDrawMeshTasksIndirect:
            yyjson_mut_obj_add_strcpy( m_pDocument, pValue, "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawMeshTasksIndirect.m_Buffer ).c_str() );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "offset", drawcall.m_Payload.m_DrawMeshTasksIndirect.m_Offset );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "drawCount", drawcall.m_Payload.m_DrawMeshTasksIndirect.m_DrawCount );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "stride", drawcall.m_Payload.m_DrawMeshTasksIndirect.m_Stride );
            return;

        case DeviceProfilerDrawcallType::eDrawMeshTasksIndirectCount:
            yyjson_mut_obj_add_strcpy( m_pDocument, pValue, "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_Buffer ).c_str() );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "offset", drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_Offset );
            yyjson_mut_obj_add_strcpy( m_pDocument, pValue, "countBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_CountBuffer ).c_str() );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "countOffset", drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_CountOffset );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "maxDrawCount", drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_MaxDrawCount );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "stride", drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_Stride );
            return;

        case DeviceProfilerDrawcallType::eDrawMeshTasksNV:
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "taskCount", drawcall.m_Payload.m_DrawMeshTasksNV.m_TaskCount );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "firstTask", drawcall.m_Payload.m_DrawMeshTasksNV.m_FirstTask );
            return;

        case DeviceProfilerDrawcallType::eDrawMeshTasksIndirectNV:
            yyjson_mut_obj_add_strcpy( m_pDocument, pValue, "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawMeshTasksIndirectNV.m_Buffer ).c_str() );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "offset", drawcall.m_Payload.m_DrawMeshTasksIndirectNV.m_Offset );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "drawCount", drawcall.m_Payload.m_DrawMeshTasksIndirectNV.m_DrawCount );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "stride", drawcall.m_Payload.m_DrawMeshTasksIndirectNV.m_Stride );
            return;

        case DeviceProfilerDrawcallType::eDrawMeshTasksIndirectCountNV:
            yyjson_mut_obj_add_strcpy( m_pDocument, pValue, "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_Buffer ).c_str() );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "offset", drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_Offset );
            yyjson_mut_obj_add_strcpy( m_pDocument, pValue, "countBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_CountBuffer ).c_str() );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "countOffset", drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_CountOffset );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "maxDrawCount", drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_MaxDrawCount );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "stride", drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_Stride );
            return;

        case DeviceProfilerDrawcallType::eDrawMulti:
        {
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "drawCount", drawcall.m_Payload.m_DrawMulti.m_DrawCount );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "instanceCount", drawcall.m_Payload.m_DrawMulti.m_InstanceCount );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "firstInstance", drawcall.m_Payload.m_DrawMulti.m_FirstInstance );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "stride", drawcall.m_Payload.m_DrawMulti.m_Stride );

            yyjson_mut_val* pVertexInfos =
                yyjson_mut_obj_add_arr( m_pDocument, pValue, "vertexInfos" );

            for( uint32_t i = 0; i < drawcall.m_Payload.m_DrawMulti.m_DrawCount; i++ )
            {
                const VkMultiDrawInfoEXT& vertexInfo = drawcall.m_Payload.m_DrawMulti.m_pVertexInfo[i];

                yyjson_mut_val* pVertexInfo = yyjson_mut_arr_add_obj( m_pDocument, pVertexInfos );
                yyjson_mut_obj_add_uint( m_pDocument, pVertexInfo, "firstVertex", vertexInfo.firstVertex );
                yyjson_mut_obj_add_uint( m_pDocument, pVertexInfo, "vertexCount", vertexInfo.vertexCount );
            }

            return;
        }
        
        case DeviceProfilerDrawcallType::eDrawMultiIndexed:
        {
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "drawCount", drawcall.m_Payload.m_DrawMultiIndexed.m_DrawCount );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "instanceCount", drawcall.m_Payload.m_DrawMultiIndexed.m_InstanceCount );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "firstInstance", drawcall.m_Payload.m_DrawMultiIndexed.m_FirstInstance );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "stride", drawcall.m_Payload.m_DrawMultiIndexed.m_Stride );

            yyjson_mut_val* pIndexInfos =
                yyjson_mut_obj_add_arr( m_pDocument, pValue, "indexInfos" );

            for( uint32_t i = 0; i < drawcall.m_Payload.m_DrawMulti.m_DrawCount; i++ )
            {
                const VkMultiDrawIndexedInfoEXT& indexInfo = drawcall.m_Payload.m_DrawMultiIndexed.m_pIndexInfo[i];

                yyjson_mut_val* pIndexInfo = yyjson_mut_arr_add_obj( m_pDocument, pIndexInfos );
                yyjson_mut_obj_add_uint( m_pDocument, pIndexInfo, "firstIndex", indexInfo.firstIndex );
                yyjson_mut_obj_add_uint( m_pDocument, pIndexInfo, "indexCount", indexInfo.indexCount );
                yyjson_mut_obj_add_uint( m_pDocument, pIndexInfo, "vertexOffset", indexInfo.vertexOffset );
            }

            return;
        }

        case DeviceProfilerDrawcallType::eDispatch:
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "groupCountX", drawcall.m_Payload.m_Dispatch.m_GroupCountX );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "groupCountY", drawcall.m_Payload.m_Dispatch.m_GroupCountY );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "groupCountZ", drawcall.m_Payload.m_Dispatch.m_GroupCountZ );
            return;

        case DeviceProfilerDrawcallType::eDispatchIndirect:
            yyjson_mut_obj_add_strcpy( m_pDocument, pValue, "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DispatchIndirect.m_Buffer ).c_str() );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "offset", drawcall.m_Payload.m_DispatchIndirect.m_Offset );
            return;

        case DeviceProfilerDrawcallType::eCopyBuffer:
            yyjson_mut_obj_add_strcpy( m_pDocument, pValue, "srcBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyBuffer.m_SrcBuffer ).c_str() );
            yyjson_mut_obj_add_strcpy( m_pDocument, pValue, "dstBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyBuffer.m_DstBuffer ).c_str() );
            return;

        case DeviceProfilerDrawcallType::eCopyBufferToImage:
            yyjson_mut_obj_add_strcpy( m_pDocument, pValue, "srcBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyBufferToImage.m_SrcBuffer ).c_str() );
            yyjson_mut_obj_add_strcpy( m_pDocument, pValue, "dstImage", m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyBufferToImage.m_DstImage ).c_str() );
            return;

        case DeviceProfilerDrawcallType::eCopyImage:
            yyjson_mut_obj_add_strcpy( m_pDocument, pValue, "srcImage", m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyImage.m_SrcImage ).c_str() );
            yyjson_mut_obj_add_strcpy( m_pDocument, pValue, "dstImage", m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyImage.m_DstImage ).c_str() );
            return;

        case DeviceProfilerDrawcallType::eCopyImageToBuffer:
            yyjson_mut_obj_add_strcpy( m_pDocument, pValue, "srcImage", m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyImageToBuffer.m_SrcImage ).c_str() );
            yyjson_mut_obj_add_strcpy( m_pDocument, pValue, "dstBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyImageToBuffer.m_DstBuffer ).c_str() );
            return;

        case DeviceProfilerDrawcallType::eClearAttachments:
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "attachmentCount", drawcall.m_Payload.m_ClearAttachments.m_Count );
            return;

        case DeviceProfilerDrawcallType::eClearColorImage:
            yyjson_mut_obj_add_strcpy( m_pDocument, pValue, "image", m_pStringSerializer->GetName( drawcall.m_Payload.m_ClearColorImage.m_Image ).c_str() );
            WriteColorClearValue(
                yyjson_mut_obj_add_obj( m_pDocument, pValue, "value" ), drawcall.m_Payload.m_ClearColorImage.m_Value );
            return;

        case DeviceProfilerDrawcallType::eClearDepthStencilImage:
            yyjson_mut_obj_add_strcpy( m_pDocument, pValue, "image", m_pStringSerializer->GetName( drawcall.m_Payload.m_ClearDepthStencilImage.m_Image ).c_str() );
            WriteDepthStencilClearValue( 
                yyjson_mut_obj_add_obj( m_pDocument, pValue, "value" ), drawcall.m_Payload.m_ClearDepthStencilImage.m_Value );
            return;

        case DeviceProfilerDrawcallType::eResolveImage:
            yyjson_mut_obj_add_strcpy( m_pDocument, pValue, "srcImage", m_pStringSerializer->GetName( drawcall.m_Payload.m_ResolveImage.m_SrcImage ).c_str() );
            yyjson_mut_obj_add_strcpy( m_pDocument, pValue, "dstImage", m_pStringSerializer->GetName( drawcall.m_Payload.m_ResolveImage.m_DstImage ).c_str() );
            return;

        case DeviceProfilerDrawcallType::eBlitImage:
            yyjson_mut_obj_add_strcpy( m_pDocument, pValue, "srcImage", m_pStringSerializer->GetName( drawcall.m_Payload.m_BlitImage.m_SrcImage ).c_str() );
            yyjson_mut_obj_add_strcpy( m_pDocument, pValue, "dstImage", m_pStringSerializer->GetName( drawcall.m_Payload.m_BlitImage.m_DstImage ).c_str() );
            return;

        case DeviceProfilerDrawcallType::eFillBuffer:
            yyjson_mut_obj_add_strcpy( m_pDocument, pValue, "dstBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_FillBuffer.m_Buffer ).c_str() );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "dstOffset", drawcall.m_Payload.m_FillBuffer.m_Offset );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "size", drawcall.m_Payload.m_FillBuffer.m_Size );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "data", drawcall.m_Payload.m_FillBuffer.m_Data );
            return;

        case DeviceProfilerDrawcallType::eUpdateBuffer:
            yyjson_mut_obj_add_strcpy( m_pDocument, pValue, "dstBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_UpdateBuffer.m_Buffer ).c_str() );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "dstOffset", drawcall.m_Payload.m_UpdateBuffer.m_Offset );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "dataSize", drawcall.m_Payload.m_UpdateBuffer.m_Size );
            return;

        case DeviceProfilerDrawcallType::eTraceRaysKHR:
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "width", drawcall.m_Payload.m_TraceRays.m_Width );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "height", drawcall.m_Payload.m_TraceRays.m_Height );
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "depth", drawcall.m_Payload.m_TraceRays.m_Depth );
            return;

        case DeviceProfilerDrawcallType::eTraceRaysIndirectKHR:
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "indirectDeviceAddress", drawcall.m_Payload.m_TraceRaysIndirect.m_IndirectAddress );
            return;

        case DeviceProfilerDrawcallType::eTraceRaysIndirect2KHR:
            yyjson_mut_obj_add_uint( m_pDocument, pValue, "indirectDeviceAddress", drawcall.m_Payload.m_TraceRaysIndirect2.m_IndirectAddress );
            return;

        case DeviceProfilerDrawcallType::eBuildAccelerationStructuresKHR:
        case DeviceProfilerDrawcallType::eBuildAccelerationStructuresIndirectKHR:
        {
            const uint32_t infoCount = drawcall.m_Payload.m_BuildAccelerationStructures.m_InfoCount;

            yyjson_mut_obj_add_uint( m_pDocument, pValue, "infoCount", infoCount );
            yyjson_mut_val* pInfos =
                yyjson_mut_obj_add_arr( m_pDocument, pValue, "infos" );

            for( uint32_t i = 0; i < infoCount; ++i )
            {
                const auto& info = drawcall.m_Payload.m_BuildAccelerationStructures.m_pInfos[ i ];

                yyjson_mut_val* pInfo = yyjson_mut_arr_add_obj( m_pDocument, pInfos );
                yyjson_mut_obj_add_strcpy( m_pDocument, pInfo, "type", m_pStringSerializer->GetAccelerationStructureTypeName( info.type ).c_str() );
                yyjson_mut_obj_add_strcpy( m_pDocument, pInfo, "flags", m_pStringSerializer->GetBuildAccelerationStructureFlagNames( info.flags ).c_str() );
                yyjson_mut_obj_add_strcpy( m_pDocument, pInfo, "mode", m_pStringSerializer->GetBuildAccelerationStructureModeName( info.mode ).c_str() );
                yyjson_mut_obj_add_strcpy( m_pDocument, pInfo, "src", m_pStringSerializer->GetName( VkAccelerationStructureKHRHandle( info.srcAccelerationStructure ) ).c_str() );
                yyjson_mut_obj_add_strcpy( m_pDocument, pInfo, "dst", m_pStringSerializer->GetName( VkAccelerationStructureKHRHandle( info.dstAccelerationStructure ) ).c_str() );
                yyjson_mut_obj_add_uint( m_pDocument, pInfo, "geometryCount", info.geometryCount );

                yyjson_mut_val* pGeometries =
                    yyjson_mut_obj_add_arr( m_pDocument, pInfo, "geometries" );

                if( info.pGeometries )
                {
                    for( uint32_t j = 0; j < info.geometryCount; ++j )
                    {
                        const auto& geometry = info.pGeometries[ j ];

                        yyjson_mut_val* pGeometry =
                            yyjson_mut_arr_add_obj( m_pDocument, pGeometries );

                        switch( geometry.geometryType )
                        {
                        case VK_GEOMETRY_TYPE_TRIANGLES_KHR:
                            yyjson_mut_obj_add_strcpy( m_pDocument, pGeometry, "vertexFormat", m_pStringSerializer->GetFormatName( geometry.geometry.triangles.vertexFormat ).c_str() );
                            yyjson_mut_obj_add_strcpy( m_pDocument, pGeometry, "vertexData", m_pStringSerializer->GetPointer( geometry.geometry.triangles.vertexData.hostAddress ).c_str() );
                            yyjson_mut_obj_add_uint( m_pDocument, pGeometry, "vertexStride", geometry.geometry.triangles.vertexStride );
                            yyjson_mut_obj_add_uint( m_pDocument, pGeometry, "maxVertex", geometry.geometry.triangles.maxVertex );
                            yyjson_mut_obj_add_strcpy( m_pDocument, pGeometry, "indexType", m_pStringSerializer->GetIndexTypeName( geometry.geometry.triangles.indexType ).c_str() );
                            yyjson_mut_obj_add_strcpy( m_pDocument, pGeometry, "indexData", m_pStringSerializer->GetPointer( geometry.geometry.triangles.indexData.hostAddress ).c_str() );
                            yyjson_mut_obj_add_strcpy( m_pDocument, pGeometry, "transformData", m_pStringSerializer->GetPointer( geometry.geometry.triangles.transformData.hostAddress ).c_str() );
                            break;

                        case VK_GEOMETRY_TYPE_AABBS_KHR:
                            yyjson_mut_obj_add_strcpy( m_pDocument, pGeometry, "data", m_pStringSerializer->GetPointer( geometry.geometry.aabbs.data.hostAddress ).c_str() );
                            yyjson_mut_obj_add_uint( m_pDocument, pGeometry, "stride", geometry.geometry.aabbs.stride );
                            break;

                        case VK_GEOMETRY_TYPE_INSTANCES_KHR:
                            yyjson_mut_obj_add_bool( m_pDocument, pGeometry, "arrayOfPointers", geometry.geometry.instances.arrayOfPointers );
                            yyjson_mut_obj_add_strcpy( m_pDocument, pGeometry, "data", m_pStringSerializer->GetPointer( geometry.geometry.instances.data.hostAddress ).c_str() );
                            break;
                        }

                        yyjson_mut_val* pGeometryRange =
                            yyjson_mut_obj_add_obj( m_pDocument, pGeometry, "range" );

                        if( drawcall.m_Type == DeviceProfilerDrawcallType::eBuildAccelerationStructuresKHR )
                        {
                            const auto& range = drawcall.m_Payload.m_BuildAccelerationStructures.m_ppRanges[ i ][ j ];
                            yyjson_mut_obj_add_uint( m_pDocument, pGeometryRange, "primitiveCount", range.primitiveCount );
                            yyjson_mut_obj_add_uint( m_pDocument, pGeometryRange, "primitiveOffset", range.primitiveOffset );
                            yyjson_mut_obj_add_uint( m_pDocument, pGeometryRange, "firstVertex", range.firstVertex );
                            yyjson_mut_obj_add_uint( m_pDocument, pGeometryRange, "transformOffset", range.transformOffset );
                        }
                        else //( drawcall.m_Type == DeviceProfilerDrawcallType::eBuildAccelerationStructuresIndirectKHR )
                        {
                            yyjson_mut_obj_add_uint( m_pDocument, pGeometryRange, "maxPrimitiveCount", drawcall.m_Payload.m_BuildAccelerationStructuresIndirect.m_ppMaxPrimitiveCounts[ i ][ j ] );
                        }
                    }
                }
            }

            return;
        }

        case DeviceProfilerDrawcallType::eBuildMicromapsEXT:
        {
            const uint32_t infoCount = drawcall.m_Payload.m_BuildMicromaps.m_InfoCount;

            yyjson_mut_obj_add_uint( m_pDocument, pValue, "infoCount", infoCount );
            yyjson_mut_val* pInfos =
                yyjson_mut_obj_add_arr( m_pDocument, pValue, "infos" );

            for( uint32_t i = 0; i < infoCount; ++i )
            {
                const auto& info = drawcall.m_Payload.m_BuildMicromaps.m_pInfos[i];

                yyjson_mut_val* pInfo = yyjson_mut_arr_add_obj( m_pDocument, pInfos );
                yyjson_mut_obj_add_strcpy( m_pDocument, pInfo, "type", m_pStringSerializer->GetMicromapTypeName( info.type ).c_str() );
                yyjson_mut_obj_add_strcpy( m_pDocument, pInfo, "flags", m_pStringSerializer->GetBuildMicromapFlagNames( info.flags ).c_str() );
                yyjson_mut_obj_add_strcpy( m_pDocument, pInfo, "mode", m_pStringSerializer->GetBuildMicromapModeName( info.mode ).c_str() );
                yyjson_mut_obj_add_strcpy( m_pDocument, pInfo, "dst", m_pStringSerializer->GetName( VkMicromapEXTHandle( info.dstMicromap ) ).c_str() );
                yyjson_mut_obj_add_uint( m_pDocument, pInfo, "usageCountsCount", info.usageCountsCount );

                yyjson_mut_val* pUsageCounts =
                    yyjson_mut_obj_add_arr( m_pDocument, pInfo, "usageCounts" );

                for( uint32_t j = 0; j < info.usageCountsCount; ++j )
                {
                    const auto& usageCount = info.pUsageCounts[j];

                    yyjson_mut_val* pUsageCount = yyjson_mut_arr_add_obj( m_pDocument, pUsageCounts );
                    yyjson_mut_obj_add_uint( m_pDocument, pUsageCount, "count", usageCount.count );
                    yyjson_mut_obj_add_uint( m_pDocument, pUsageCount, "format", usageCount.format );
                    yyjson_mut_obj_add_uint( m_pDocument, pUsageCount, "subdivisionLevel", usageCount.subdivisionLevel );
                }

                yyjson_mut_obj_add_strcpy( m_pDocument, pInfo, "data", m_pStringSerializer->GetPointer( info.data.hostAddress ).c_str() );
                yyjson_mut_obj_add_strcpy( m_pDocument, pInfo, "scratchData", m_pStringSerializer->GetPointer( info.scratchData.hostAddress ).c_str() );
                yyjson_mut_obj_add_strcpy( m_pDocument, pInfo, "triangleArray", m_pStringSerializer->GetPointer( info.triangleArray.hostAddress ).c_str() );
                yyjson_mut_obj_add_uint( m_pDocument, pInfo, "triangleArrayStride", info.triangleArrayStride );
            }

            return;
        }
        }
    }

    /*************************************************************************\

    Function:
        WritePipelineArgs

    Description:
        Serialize pipeline state into JSON object.

    \*************************************************************************/
    void DeviceProfilerJsonSerializer::WritePipelineArgs( yyjson_mut_val* pValue, const DeviceProfilerPipeline& pipeline ) const
    {
        // Append shader stages info.
        if( !pipeline.m_ShaderTuple.m_Shaders.empty() )
        {
            yyjson_mut_val* pShaders =
                yyjson_mut_obj_add_arr( m_pDocument, pValue, "shaders" );

            for( const ProfilerShader& shader : pipeline.m_ShaderTuple.m_Shaders )
            {
                WriteShaderStageArgs( yyjson_mut_arr_add_obj( m_pDocument, pShaders ), shader );
            }
        }

        // Append pipeline create info details.
        if( pipeline.m_pCreateInfo )
        {
            switch( pipeline.m_Type )
            {
            case DeviceProfilerPipelineType::eGraphics:
            {
                WriteGraphicsPipelineCreateInfoArgs( pValue,
                    pipeline.m_pCreateInfo->m_GraphicsPipelineCreateInfo );
                break;
            }
            case DeviceProfilerPipelineType::eCompute:
            {
                WriteComputePipelineCreateInfoArgs( pValue,
                    pipeline.m_pCreateInfo->m_ComputePipelineCreateInfo );
                break;
            }
            case DeviceProfilerPipelineType::eRayTracingKHR:
            {
                WriteRayTracingPipelineCreateInfoArgs( pValue,
                    pipeline.m_pCreateInfo->m_RayTracingPipelineCreateInfoKHR );
                break;
            }
            }
        }
    }

    /*************************************************************************\

    Function:
        WriteColorClearValue

    Description:
        Serialize VkClearColorValue struct into JSON object.

    \*************************************************************************/
    void DeviceProfilerJsonSerializer::WriteColorClearValue( yyjson_mut_val* pValue, const VkClearColorValue& value ) const
    {
        yyjson_mut_val* pFloat32 = yyjson_mut_obj_add_arr( m_pDocument, pValue, "float32" );
        yyjson_mut_arr_add_real( m_pDocument, pFloat32, value.float32[ 0 ] );
        yyjson_mut_arr_add_real( m_pDocument, pFloat32, value.float32[ 1 ] );
        yyjson_mut_arr_add_real( m_pDocument, pFloat32, value.float32[ 2 ] );
        yyjson_mut_arr_add_real( m_pDocument, pFloat32, value.float32[ 3 ] );

        yyjson_mut_val* pInt32 = yyjson_mut_obj_add_arr( m_pDocument, pValue, "int32" );
        yyjson_mut_arr_add_int( m_pDocument, pInt32, value.int32[ 0 ] );
        yyjson_mut_arr_add_int( m_pDocument, pInt32, value.int32[ 1 ] );
        yyjson_mut_arr_add_int( m_pDocument, pInt32, value.int32[ 2 ] );
        yyjson_mut_arr_add_int( m_pDocument, pInt32, value.int32[ 3 ] );

        yyjson_mut_val* pUint32 = yyjson_mut_obj_add_arr( m_pDocument, pValue, "uint32" );
        yyjson_mut_arr_add_uint( m_pDocument, pUint32, value.uint32[ 0 ] );
        yyjson_mut_arr_add_uint( m_pDocument, pUint32, value.uint32[ 1 ] );
        yyjson_mut_arr_add_uint( m_pDocument, pUint32, value.uint32[ 2 ] );
        yyjson_mut_arr_add_uint( m_pDocument, pUint32, value.uint32[ 3 ] );
    }

    /*************************************************************************\

    Function:
        WriteDepthStencilClearValue

    Description:
        Serialize VkClearDepthStencilValue struct into JSON object.

    \*************************************************************************/
    void DeviceProfilerJsonSerializer::WriteDepthStencilClearValue( yyjson_mut_val* pValue, const VkClearDepthStencilValue& value ) const
    {
        yyjson_mut_obj_add_real( m_pDocument, pValue, "depth", value.depth );
        yyjson_mut_obj_add_uint( m_pDocument, pValue, "stencil", value.stencil );
    }

    /*************************************************************************\

    Function:
        WriteShaderStageArgs

    Description:
        Serialize shader stage into JSON object.

    \*************************************************************************/
    void DeviceProfilerJsonSerializer::WriteShaderStageArgs( yyjson_mut_val* pValue, const ProfilerShader& shader ) const
    {
        yyjson_mut_obj_add_strcpy( m_pDocument, pValue, "stage", m_pStringSerializer->GetShaderStageName( shader.m_Stage ).c_str() );
        yyjson_mut_obj_add_strcpy( m_pDocument, pValue, "entryPoint", shader.m_EntryPoint.c_str() );

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

            yyjson_mut_obj_add_strcpy( m_pDocument, pValue, "shaderIdentifier", shaderIdentifier.c_str() );
        }
    }

    /*************************************************************************\

    Function:
        WriteGraphicsPipelineCreateInfoArgs

    Description:
        Serialize graphics pipeline state into JSON object.

    \*************************************************************************/
    void DeviceProfilerJsonSerializer::WriteGraphicsPipelineCreateInfoArgs( yyjson_mut_val* pValue, const VkGraphicsPipelineCreateInfo& createInfo ) const
    {
        // VkPipelineVertexInputStateCreateInfo
        if( createInfo.pVertexInputState )
        {
            const auto& state = *createInfo.pVertexInputState;

            yyjson_mut_val* pVertexInputState =
                yyjson_mut_obj_add_obj( m_pDocument, pValue, "vertexInputState" );
            yyjson_mut_obj_add_uint( m_pDocument, pVertexInputState, "attributeCount", state.vertexAttributeDescriptionCount );
            yyjson_mut_obj_add_uint( m_pDocument, pVertexInputState, "bindingCount", state.vertexBindingDescriptionCount );

            if( state.pVertexAttributeDescriptions )
            {
                yyjson_mut_val* pAttributes =
                    yyjson_mut_obj_add_arr( m_pDocument, pVertexInputState, "attributes" );

                for( uint32_t i = 0; i < state.vertexAttributeDescriptionCount; ++i )
                {
                    const auto& attribute = state.pVertexAttributeDescriptions[i];

                    yyjson_mut_val* pAttribute = yyjson_mut_arr_add_obj( m_pDocument, pAttributes );
                    yyjson_mut_obj_add_uint( m_pDocument, pAttribute, "location", attribute.location );
                    yyjson_mut_obj_add_uint( m_pDocument, pAttribute, "binding", attribute.binding );
                    yyjson_mut_obj_add_strcpy( m_pDocument, pAttribute, "format", m_pStringSerializer->GetFormatName( attribute.format ).c_str() );
                    yyjson_mut_obj_add_uint( m_pDocument, pAttribute, "offset", attribute.offset );
                }
            }
            else
            {
                yyjson_mut_obj_add_null( m_pDocument, pVertexInputState, "attributes" );
            }

            if( state.pVertexBindingDescriptions )
            {
                yyjson_mut_val* pBindings =
                    yyjson_mut_obj_add_arr( m_pDocument, pVertexInputState, "bindings" );

                for( uint32_t i = 0; i < state.vertexBindingDescriptionCount; ++i )
                {
                    const auto& binding = state.pVertexBindingDescriptions[i];

                    yyjson_mut_val* pBinding = yyjson_mut_arr_add_obj( m_pDocument, pBindings );
                    yyjson_mut_obj_add_uint( m_pDocument, pBinding, "binding", binding.binding );
                    yyjson_mut_obj_add_uint( m_pDocument, pBinding, "stride", binding.stride );
                    yyjson_mut_obj_add_strcpy( m_pDocument, pBinding, "inputRate", m_pStringSerializer->GetVertexInputRateName( binding.inputRate ).c_str() );
                }
            }
            else
            {
                yyjson_mut_obj_add_null( m_pDocument, pVertexInputState, "bindings" );
            }
        }
        else
        {
            yyjson_mut_obj_add_null( m_pDocument, pValue, "vertexInputState" );
        }

        // VkPipelineInputAssemblyStateCreateInfo
        if( createInfo.pInputAssemblyState )
        {
            const auto& state = *createInfo.pInputAssemblyState;

            yyjson_mut_val* pInputAssemblyState =
                yyjson_mut_obj_add_obj( m_pDocument, pValue, "inputAssemblyState" );
            yyjson_mut_obj_add_strcpy( m_pDocument, pInputAssemblyState, "topology", m_pStringSerializer->GetPrimitiveTopologyName( state.topology ).c_str() );
            yyjson_mut_obj_add_bool( m_pDocument, pInputAssemblyState, "primitiveRestartEnable", state.primitiveRestartEnable );
        }
        else
        {
            yyjson_mut_obj_add_null( m_pDocument, pValue, "inputAssemblyState" );
        }

        // VkPipelineTessellationStateCreateInfo
        if( createInfo.pTessellationState )
        {
            const auto& state = *createInfo.pTessellationState;

            yyjson_mut_val* pTessellationState =
                yyjson_mut_obj_add_obj( m_pDocument, pValue, "tessellationState" );
            yyjson_mut_obj_add_uint( m_pDocument, pTessellationState, "patchControlPoints", state.patchControlPoints );
        }
        else
        {
            yyjson_mut_obj_add_null( m_pDocument, pValue, "tessellationState" );
        }

        // VkPipelineViewportStateCreateInfo
        if( createInfo.pViewportState )
        {
            const auto& state = *createInfo.pViewportState;

            yyjson_mut_val* pViewportState =
                yyjson_mut_obj_add_obj( m_pDocument, pValue, "viewportState" );
            yyjson_mut_obj_add_uint( m_pDocument, pViewportState, "viewportCount", state.viewportCount );
            yyjson_mut_obj_add_uint( m_pDocument, pViewportState, "scissorCount", state.scissorCount );

            if( state.pViewports )
            {
                yyjson_mut_val* pViewports =
                    yyjson_mut_obj_add_arr( m_pDocument, pViewportState, "viewports" );

                for( uint32_t i = 0; i < state.viewportCount; ++i )
                {
                    const auto& viewport = state.pViewports[i];

                    yyjson_mut_val* pViewport = yyjson_mut_arr_add_obj( m_pDocument, pViewports );
                    yyjson_mut_obj_add_real( m_pDocument, pViewport, "x", viewport.x );
                    yyjson_mut_obj_add_real( m_pDocument, pViewport, "y", viewport.y );
                    yyjson_mut_obj_add_real( m_pDocument, pViewport, "width", viewport.width );
                    yyjson_mut_obj_add_real( m_pDocument, pViewport, "height", viewport.height );
                    yyjson_mut_obj_add_real( m_pDocument, pViewport, "minDepth", viewport.minDepth );
                    yyjson_mut_obj_add_real( m_pDocument, pViewport, "maxDepth", viewport.maxDepth );
                }
            }
            else
            {
                yyjson_mut_obj_add_null( m_pDocument, pViewportState, "viewports" );
            }

            if( state.pScissors )
            {
                yyjson_mut_val* pScissors =
                    yyjson_mut_obj_add_arr( m_pDocument, pViewportState, "scissors" );

                for( uint32_t i = 0; i < state.scissorCount; ++i )
                {
                    const auto& scissor = state.pScissors[i];

                    yyjson_mut_val* pScissor = yyjson_mut_arr_add_obj( m_pDocument, pScissors );
                    yyjson_mut_obj_add_real( m_pDocument, pScissor, "offsetX", scissor.offset.x );
                    yyjson_mut_obj_add_real( m_pDocument, pScissor, "offsetY", scissor.offset.y );
                    yyjson_mut_obj_add_real( m_pDocument, pScissor, "extentWidth", scissor.extent.width );
                    yyjson_mut_obj_add_real( m_pDocument, pScissor, "extentHeight", scissor.extent.height );
                }
            }
            else
            {
                yyjson_mut_obj_add_null( m_pDocument, pViewportState, "scissors" );
            }
        }
        else
        {
            yyjson_mut_obj_add_null( m_pDocument, pValue, "viewportState" );
        }

        // VkPipelineRasterizationStateCreateInfo
        if( createInfo.pRasterizationState )
        {
            const auto& state = *createInfo.pRasterizationState;

            yyjson_mut_val* pRasterizationState =
                yyjson_mut_obj_add_obj( m_pDocument, pValue, "rasterizationState" );
            yyjson_mut_obj_add_bool( m_pDocument, pRasterizationState, "depthClampEnable", state.depthClampEnable );
            yyjson_mut_obj_add_bool( m_pDocument, pRasterizationState, "rasterizerDiscardEnable", state.rasterizerDiscardEnable );
            yyjson_mut_obj_add_strcpy( m_pDocument, pRasterizationState, "polygonMode", m_pStringSerializer->GetPolygonModeName( state.polygonMode ).c_str() );
            yyjson_mut_obj_add_strcpy( m_pDocument, pRasterizationState, "cullMode", m_pStringSerializer->GetCullModeName( state.cullMode ).c_str() );
            yyjson_mut_obj_add_strcpy( m_pDocument, pRasterizationState, "frontFace", m_pStringSerializer->GetFrontFaceName( state.frontFace ).c_str() );
            yyjson_mut_obj_add_bool( m_pDocument, pRasterizationState, "depthBiasEnable", state.depthBiasEnable );
            yyjson_mut_obj_add_real( m_pDocument, pRasterizationState, "depthBiasConstantFactor", state.depthBiasConstantFactor );
            yyjson_mut_obj_add_real( m_pDocument, pRasterizationState, "depthBiasClamp", state.depthBiasClamp );
            yyjson_mut_obj_add_real( m_pDocument, pRasterizationState, "depthBiasSlopeFactor", state.depthBiasSlopeFactor );
            yyjson_mut_obj_add_real( m_pDocument, pRasterizationState, "lineWidth", state.lineWidth );
        }
        else
        {
            yyjson_mut_obj_add_null( m_pDocument, pValue, "rasterizationState" );
        }

        // VkPipelineMultisampleStateCreateInfo
        if( createInfo.pMultisampleState )
        {
            const auto& state = *createInfo.pMultisampleState;

            yyjson_mut_val* pMultisampleState =
                yyjson_mut_obj_add_obj( m_pDocument, pValue, "multisampleState" );
            yyjson_mut_obj_add_uint( m_pDocument, pMultisampleState, "rasterizationSamples", state.rasterizationSamples );
            yyjson_mut_obj_add_bool( m_pDocument, pMultisampleState, "sampleShadingEnable", state.sampleShadingEnable );
            yyjson_mut_obj_add_real( m_pDocument, pMultisampleState, "minSampleShading", state.minSampleShading );
            yyjson_mut_obj_add_strcpy( m_pDocument, pMultisampleState, "sampleMask", fmt::format( "0x{:08X}", state.pSampleMask ? *state.pSampleMask : 0xFFFFFFFF ).c_str() );
            yyjson_mut_obj_add_bool( m_pDocument, pMultisampleState, "alphaToCoverateEnable", state.alphaToCoverageEnable );
            yyjson_mut_obj_add_bool( m_pDocument, pMultisampleState, "alphaToOneEnable", state.alphaToOneEnable );
        }
        else
        {
            yyjson_mut_obj_add_null( m_pDocument, pValue, "multisampleState" );
        }

        // VkPipelineDepthStencilStateCreateInfo
        if( createInfo.pDepthStencilState )
        {
            const auto& state = *createInfo.pDepthStencilState;

            yyjson_mut_val* pDepthStencilState =
                yyjson_mut_obj_add_obj( m_pDocument, pValue, "depthStencilState" );
            yyjson_mut_obj_add_bool( m_pDocument, pDepthStencilState, "depthTestEnable", state.depthTestEnable );
            yyjson_mut_obj_add_bool( m_pDocument, pDepthStencilState, "depthWriteEnable", state.depthWriteEnable );
            yyjson_mut_obj_add_strcpy( m_pDocument, pDepthStencilState, "depthCompareOp", m_pStringSerializer->GetCompareOpName( state.depthCompareOp ).c_str() );
            yyjson_mut_obj_add_bool( m_pDocument, pDepthStencilState, "depthBoundsTestEnable", state.depthBoundsTestEnable );
            yyjson_mut_obj_add_real( m_pDocument, pDepthStencilState, "minDepthBounds", state.minDepthBounds );
            yyjson_mut_obj_add_real( m_pDocument, pDepthStencilState, "maxDepthBounds", state.maxDepthBounds );
            yyjson_mut_obj_add_bool( m_pDocument, pDepthStencilState, "stencilTestEnable", state.stencilTestEnable );

            yyjson_mut_val* pFront =
                yyjson_mut_obj_add_obj( m_pDocument, pDepthStencilState, "front" );
            yyjson_mut_obj_add_uint( m_pDocument, pFront, "failOp", state.front.failOp );
            yyjson_mut_obj_add_uint( m_pDocument, pFront, "passOp", state.front.passOp );
            yyjson_mut_obj_add_uint( m_pDocument, pFront, "depthFailOp", state.front.depthFailOp );
            yyjson_mut_obj_add_strcpy( m_pDocument, pFront, "compareOp", m_pStringSerializer->GetCompareOpName( state.front.compareOp ).c_str() );
            yyjson_mut_obj_add_strcpy( m_pDocument, pFront, "compareMask", fmt::format( "0x{:02X}", state.front.compareMask ).c_str() );
            yyjson_mut_obj_add_strcpy( m_pDocument, pFront, "writeMask", fmt::format( "0x{:02X}", state.front.writeMask ).c_str() );
            yyjson_mut_obj_add_strcpy( m_pDocument, pFront, "reference", fmt::format( "0x{:02X}", state.front.reference ).c_str() );

            yyjson_mut_val* pBack =
                yyjson_mut_obj_add_obj( m_pDocument, pDepthStencilState, "back" );
            yyjson_mut_obj_add_uint( m_pDocument, pBack, "failOp", state.back.failOp );
            yyjson_mut_obj_add_uint( m_pDocument, pBack, "passOp", state.back.passOp );
            yyjson_mut_obj_add_uint( m_pDocument, pBack, "depthFailOp", state.back.depthFailOp );
            yyjson_mut_obj_add_strcpy( m_pDocument, pBack, "compareOp", m_pStringSerializer->GetCompareOpName( state.back.compareOp ).c_str() );
            yyjson_mut_obj_add_strcpy( m_pDocument, pBack, "compareMask", fmt::format( "0x{:02X}", state.back.compareMask ).c_str() );
            yyjson_mut_obj_add_strcpy( m_pDocument, pBack, "writeMask", fmt::format( "0x{:02X}", state.back.writeMask ).c_str() );
            yyjson_mut_obj_add_strcpy( m_pDocument, pBack, "reference", fmt::format( "0x{:02X}", state.back.reference ).c_str() );
        }
        else
        {
            yyjson_mut_obj_add_null( m_pDocument, pValue, "depthStencilState" );
        }

        // VkPipelineColorBlendStateCreateInfo
        if( createInfo.pColorBlendState )
        {
            const auto& state = *createInfo.pColorBlendState;

            yyjson_mut_val* pColorBlendState =
                yyjson_mut_obj_add_obj( m_pDocument, pValue, "colorBlendState" );
            yyjson_mut_obj_add_bool( m_pDocument, pColorBlendState, "logicOpEnable", state.logicOpEnable );
            yyjson_mut_obj_add_strcpy( m_pDocument, pColorBlendState, "logicOp", m_pStringSerializer->GetLogicOpName( state.logicOp ).c_str() );

            yyjson_mut_val* pBlendConstants =
                yyjson_mut_obj_add_arr( m_pDocument, pColorBlendState, "blendConstants" );

            for( uint32_t i = 0; i < 4; ++i )
            {
                yyjson_mut_arr_add_real( m_pDocument, pBlendConstants, state.blendConstants[i] );
            }

            if( state.pAttachments )
            {
                yyjson_mut_val* pAttachments =
                    yyjson_mut_obj_add_arr( m_pDocument, pColorBlendState, "attachments" );

                for( uint32_t i = 0; i < state.attachmentCount; ++i )
                {
                    const auto& attachment = state.pAttachments[i];

                    yyjson_mut_val* pAttachment = yyjson_mut_arr_add_obj( m_pDocument, pAttachments );
                    yyjson_mut_obj_add_bool( m_pDocument, pAttachment, "blendEnable", static_cast<bool>( attachment.blendEnable ) );
                    yyjson_mut_obj_add_strcpy( m_pDocument, pAttachment, "srcColorBlendFactor", m_pStringSerializer->GetBlendFactorName( attachment.srcColorBlendFactor ).c_str() );
                    yyjson_mut_obj_add_strcpy( m_pDocument, pAttachment, "dstColorBlendFactor", m_pStringSerializer->GetBlendFactorName( attachment.dstColorBlendFactor ).c_str() );
                    yyjson_mut_obj_add_strcpy( m_pDocument, pAttachment, "colorBlendOp", m_pStringSerializer->GetBlendOpName( attachment.colorBlendOp ).c_str() );
                    yyjson_mut_obj_add_strcpy( m_pDocument, pAttachment, "srcAlphaBlendFactor", m_pStringSerializer->GetBlendFactorName( attachment.srcAlphaBlendFactor ).c_str() );
                    yyjson_mut_obj_add_strcpy( m_pDocument, pAttachment, "dstAlphaBlendFactor", m_pStringSerializer->GetBlendFactorName( attachment.dstAlphaBlendFactor ).c_str() );
                    yyjson_mut_obj_add_strcpy( m_pDocument, pAttachment, "alphaBlendOp", m_pStringSerializer->GetBlendOpName( attachment.alphaBlendOp ).c_str() );
                    yyjson_mut_obj_add_strcpy( m_pDocument, pAttachment, "colorWriteMask", m_pStringSerializer->GetColorComponentFlagNames( attachment.colorWriteMask ).c_str() );
                }
            }
            else
            {
                yyjson_mut_obj_add_null( m_pDocument, pColorBlendState, "attachments" );
            }
        }
        else
        {
            yyjson_mut_obj_add_null( m_pDocument, pValue, "colorBlendState" );
        }

        // VkPipelineDynamicStateCreateInfo
        if( createInfo.pDynamicState )
        {
            const auto& state = *createInfo.pDynamicState;

            yyjson_mut_val* pDynamicStates =
                yyjson_mut_obj_add_arr( m_pDocument, pValue, "dynamicStates" );

            for( uint32_t i = 0; i < state.dynamicStateCount; ++i )
            {
                yyjson_mut_arr_add_strcpy( m_pDocument, pDynamicStates,
                    m_pStringSerializer->GetDynamicStateName( state.pDynamicStates[i] ).c_str() );
            }
        }
        else
        {
            yyjson_mut_obj_add_null( m_pDocument, pValue, "dynamicStates" );
        }
    }

    /*************************************************************************\

    Function:
        WriteComputePipelineCreateInfoArgs

    Description:
        Serialize compute pipeline state into JSON object.

    \*************************************************************************/
    void DeviceProfilerJsonSerializer::WriteComputePipelineCreateInfoArgs( yyjson_mut_val* pValue, const VkComputePipelineCreateInfo& createInfo ) const
    {
        // No additional state to serialize for compute pipelines yet.
    }

    /*************************************************************************\

    Function:
        WriteRayTracingPipelineCreateInfoArgs

    Description:
        Serialize ray-tracing pipeline state into JSON object.

    \*************************************************************************/
    void DeviceProfilerJsonSerializer::WriteRayTracingPipelineCreateInfoArgs( yyjson_mut_val* pValue, const VkRayTracingPipelineCreateInfoKHR& createInfo ) const
    {
        yyjson_mut_obj_add_uint( m_pDocument, pValue, "maxPipelineRayRecursionDepth", createInfo.maxPipelineRayRecursionDepth );

        // VkRayTracingPipelineInterfaceCreateInfoKHR
        if( createInfo.pLibraryInterface )
        {
            const auto& state = *createInfo.pLibraryInterface;

            yyjson_mut_val* pLibraryInterface =
                yyjson_mut_obj_add_obj( m_pDocument, pValue, "libraryInterface" );
            yyjson_mut_obj_add_uint( m_pDocument, pLibraryInterface, "maxPipelineRayPayloadSize", state.maxPipelineRayPayloadSize );
            yyjson_mut_obj_add_uint( m_pDocument, pLibraryInterface, "maxPipelineRayHitAttributeSize", state.maxPipelineRayHitAttributeSize );
        }
        else
        {
            yyjson_mut_obj_add_null( m_pDocument, pValue, "libraryInterface" );
        }

        // VkPipelineDynamicStateCreateInfo
        if( createInfo.pDynamicState )
        {
            const auto& state = *createInfo.pDynamicState;

            yyjson_mut_val* pDynamicStates =
                yyjson_mut_obj_add_arr( m_pDocument, pValue, "dynamicStates" );

            for( uint32_t i = 0; i < state.dynamicStateCount; ++i )
            {
                yyjson_mut_arr_add_str( m_pDocument, pDynamicStates,
                    m_pStringSerializer->GetDynamicStateName( state.pDynamicStates[i] ).c_str() );
            }
        }
        else
        {
            yyjson_mut_obj_add_null( m_pDocument, pValue, "dynamicStates" );
        }
    }
}

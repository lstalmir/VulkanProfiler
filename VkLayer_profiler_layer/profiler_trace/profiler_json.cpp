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

#include <inttypes.h>

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
    void DeviceProfilerJsonSerializer::WriteCommandArgs( FILE* pFile, const DeviceProfilerDrawcall& drawcall ) const
    {
        fputc( '{', pFile );

        switch( drawcall.m_Type )
        {
        default:
        case DeviceProfilerDrawcallType::eUnknown:
        case DeviceProfilerDrawcallType::eInsertDebugLabel:
        case DeviceProfilerDrawcallType::eBeginDebugLabel:
        case DeviceProfilerDrawcallType::eEndDebugLabel:
            break;

        case DeviceProfilerDrawcallType::eDraw:
            fprintf( pFile,
                "\"vertexCount\":%" PRIu32 ","
                "\"instanceCount\":%" PRIu32 ","
                "\"firstVertex\":%" PRIu32 ","
                "\"firstInstance\":%" PRIu32,
                drawcall.m_Payload.m_Draw.m_VertexCount,
                drawcall.m_Payload.m_Draw.m_InstanceCount,
                drawcall.m_Payload.m_Draw.m_FirstVertex,
                drawcall.m_Payload.m_Draw.m_FirstInstance );
            break;

        case DeviceProfilerDrawcallType::eDrawIndexed:
            fprintf( pFile,
                "\"indexCount\":%" PRIu32 ","
                "\"instanceCount\":%" PRIu32 ","
                "\"firstIndex\":%" PRIu32 ","
                "\"vertexOffset\":%" PRIi32 ","
                "\"firstInstance\":%" PRIu32,
                drawcall.m_Payload.m_DrawIndexed.m_IndexCount,
                drawcall.m_Payload.m_DrawIndexed.m_InstanceCount,
                drawcall.m_Payload.m_DrawIndexed.m_FirstIndex,
                drawcall.m_Payload.m_DrawIndexed.m_VertexOffset,
                drawcall.m_Payload.m_DrawIndexed.m_FirstInstance );
            break;

        case DeviceProfilerDrawcallType::eDrawIndirect:
            fprintf( pFile,
                "\"buffer\":\"%s\","
                "\"offset\":%" PRIu64 ","
                "\"drawCount\":%" PRIu32 ","
                "\"stride\":%" PRIu32,
                m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawIndirect.m_Buffer ).c_str(),
                drawcall.m_Payload.m_DrawIndirect.m_Offset,
                drawcall.m_Payload.m_DrawIndirect.m_DrawCount,
                drawcall.m_Payload.m_DrawIndirect.m_Stride );
            break;

        case DeviceProfilerDrawcallType::eDrawIndexedIndirect:
            fprintf( pFile,
                "\"buffer\":\"%s\","
                "\"offset\":%" PRIu64 ","
                "\"drawCount\":%" PRIu32 ","
                "\"stride\":%" PRIu32,
                m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawIndexedIndirect.m_Buffer ).c_str(),
                drawcall.m_Payload.m_DrawIndexedIndirect.m_Offset,
                drawcall.m_Payload.m_DrawIndexedIndirect.m_DrawCount,
                drawcall.m_Payload.m_DrawIndexedIndirect.m_Stride );
            break;

        case DeviceProfilerDrawcallType::eDrawIndirectCount:
            fprintf( pFile,
                "\"buffer\":\"%s\","
                "\"offset\":%" PRIu64 ","
                "\"countBuffer\":\"%s\","
                "\"countOffset\":%" PRIu64 ","
                "\"maxDrawCount\":%" PRIu32 ","
                "\"stride\":%" PRIu32,
                m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawIndirectCount.m_Buffer ).c_str(),
                drawcall.m_Payload.m_DrawIndirectCount.m_Offset,
                m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawIndirectCount.m_CountBuffer ).c_str(),
                drawcall.m_Payload.m_DrawIndirectCount.m_CountOffset,
                drawcall.m_Payload.m_DrawIndirectCount.m_MaxDrawCount,
                drawcall.m_Payload.m_DrawIndirectCount.m_Stride );
            break;

        case DeviceProfilerDrawcallType::eDrawIndexedIndirectCount:
            fprintf( pFile,
                "\"buffer\":\"%s\","
                "\"offset\":%" PRIu64 ","
                "\"countBuffer\":\"%s\","
                "\"countOffset\":%" PRIu64 ","
                "\"maxDrawCount\":%" PRIu32 ","
                "\"stride\":%" PRIu32,
                m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawIndexedIndirectCount.m_Buffer ).c_str(),
                drawcall.m_Payload.m_DrawIndexedIndirectCount.m_Offset,
                m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawIndexedIndirectCount.m_CountBuffer ).c_str(),
                drawcall.m_Payload.m_DrawIndexedIndirectCount.m_CountOffset,
                drawcall.m_Payload.m_DrawIndexedIndirectCount.m_MaxDrawCount,
                drawcall.m_Payload.m_DrawIndexedIndirectCount.m_Stride );
            break;

        case DeviceProfilerDrawcallType::eDrawMeshTasks:
            fprintf( pFile,
                "\"groupCountX\":%" PRIu32 ","
                "\"groupCountY\":%" PRIu32 ","
                "\"groupCountZ\":%" PRIu32,
                drawcall.m_Payload.m_DrawMeshTasks.m_GroupCountX,
                drawcall.m_Payload.m_DrawMeshTasks.m_GroupCountY,
                drawcall.m_Payload.m_DrawMeshTasks.m_GroupCountZ );
            break;

        case DeviceProfilerDrawcallType::eDrawMeshTasksIndirect:
            fprintf( pFile,
                "\"buffer\":\"%s\","
                "\"offset\":%" PRIu64 ","
                "\"drawCount\":%" PRIu32 ","
                "\"stride\":%" PRIu32,
                m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawMeshTasksIndirect.m_Buffer ).c_str(),
                drawcall.m_Payload.m_DrawMeshTasksIndirect.m_Offset,
                drawcall.m_Payload.m_DrawMeshTasksIndirect.m_DrawCount,
                drawcall.m_Payload.m_DrawMeshTasksIndirect.m_Stride );
            break;

        case DeviceProfilerDrawcallType::eDrawMeshTasksIndirectCount:
            fprintf( pFile,
                "\"buffer\":\"%s\","
                "\"offset\":%" PRIu64 ","
                "\"countBuffer\":\"%s\","
                "\"countOffset\":%" PRIu64 ","
                "\"maxDrawCount\":%" PRIu32 ","
                "\"stride\":%" PRIu32,
                m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_Buffer ).c_str(),
                drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_Offset,
                m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_CountBuffer ).c_str(),
                drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_CountOffset,
                drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_MaxDrawCount,
                drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_Stride );
            break;

        case DeviceProfilerDrawcallType::eDrawMeshTasksNV:
            fprintf( pFile,
                "\"taskCount\":%" PRIu32 ","
                "\"firstTask\":%" PRIu32,
                drawcall.m_Payload.m_DrawMeshTasksNV.m_TaskCount,
                drawcall.m_Payload.m_DrawMeshTasksNV.m_FirstTask );
            break;

        case DeviceProfilerDrawcallType::eDrawMeshTasksIndirectNV:
            fprintf( pFile,
                "\"buffer\":\"%s\","
                "\"offset\":%" PRIu64 ","
                "\"drawCount\":%" PRIu32 ","
                "\"stride\":%" PRIu32,
                m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawMeshTasksIndirectNV.m_Buffer ).c_str(),
                drawcall.m_Payload.m_DrawMeshTasksIndirectNV.m_Offset,
                drawcall.m_Payload.m_DrawMeshTasksIndirectNV.m_DrawCount,
                drawcall.m_Payload.m_DrawMeshTasksIndirectNV.m_Stride );
            break;

        case DeviceProfilerDrawcallType::eDrawMeshTasksIndirectCountNV:
            fprintf( pFile,
                "\"buffer\":\"%s\","
                "\"offset\":%" PRIu64 ","
                "\"countBuffer\":\"%s\","
                "\"countOffset\":%" PRIu64 ","
                "\"maxDrawCount\":%" PRIu32 ","
                "\"stride\":%" PRIu32,
                m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_Buffer ).c_str(),
                drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_Offset,
                m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_CountBuffer ).c_str(),
                drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_CountOffset,
                drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_MaxDrawCount,
                drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_Stride );
            break;

        case DeviceProfilerDrawcallType::eDrawMulti:
            fprintf( pFile,
                "\"drawCount\":%" PRIu32 ","
                "\"instanceCount\":%" PRIu32 ","
                "\"firstInstance\":%" PRIu32 ","
                "\"stride\":%" PRIu32 ","
                "\"vertexInfos\":[",
                drawcall.m_Payload.m_DrawMulti.m_DrawCount,
                drawcall.m_Payload.m_DrawMulti.m_InstanceCount,
                drawcall.m_Payload.m_DrawMulti.m_FirstInstance,
                drawcall.m_Payload.m_DrawMulti.m_Stride );

            for( uint32_t i = 0; i < drawcall.m_Payload.m_DrawMulti.m_DrawCount; i++ )
            {
                const VkMultiDrawInfoEXT& vertexInfo = drawcall.m_Payload.m_DrawMulti.m_pVertexInfo[i];
                if( i > 0 ) fputc( ',', pFile );
                fprintf( pFile,
                    "{\"firstVertex\":%" PRIu32 ","
                    "\"vertexCount\":%" PRIu32 "}",
                    vertexInfo.firstVertex,
                    vertexInfo.vertexCount );
            }

            fputc( ']', pFile );
            break;
        
        case DeviceProfilerDrawcallType::eDrawMultiIndexed:
            fprintf( pFile,
                "\"drawCount\":%" PRIu32 ","
                "\"instanceCount\":%" PRIu32 ","
                "\"firstInstance\":%" PRIu32 ","
                "\"stride\":%" PRIu32 ","
                "\"indexInfos\":[",
                drawcall.m_Payload.m_DrawMultiIndexed.m_DrawCount,
                drawcall.m_Payload.m_DrawMultiIndexed.m_InstanceCount,
                drawcall.m_Payload.m_DrawMultiIndexed.m_FirstInstance,
                drawcall.m_Payload.m_DrawMultiIndexed.m_Stride );

            for( uint32_t i = 0; i < drawcall.m_Payload.m_DrawMulti.m_DrawCount; i++ )
            {
                const VkMultiDrawIndexedInfoEXT& indexInfo = drawcall.m_Payload.m_DrawMultiIndexed.m_pIndexInfo[i];
                if( i > 0 ) fputc( ',', pFile );
                fprintf( pFile,
                    "{\"firstIndex\":%" PRIu32 ","
                    "\"indexCount\":%" PRIu32 ","
                    "\"vertexOffset\":%" PRIu32 "}",
                    indexInfo.firstIndex,
                    indexInfo.indexCount,
                    indexInfo.vertexOffset );
            }

            fputc( ']', pFile );
            break;

        case DeviceProfilerDrawcallType::eDispatch:
            fprintf( pFile,
                "\"groupCountX\":%" PRIu32 ","
                "\"groupCountY\":%" PRIu32 ","
                "\"groupCountZ\":%" PRIu32,
                drawcall.m_Payload.m_Dispatch.m_GroupCountX,
                drawcall.m_Payload.m_Dispatch.m_GroupCountY,
                drawcall.m_Payload.m_Dispatch.m_GroupCountZ );
            break;

        case DeviceProfilerDrawcallType::eDispatchIndirect:
            fprintf( pFile,
                "\"buffer\":\"%s\","
                "\"offset\":%" PRIu64,
                m_pStringSerializer->GetName( drawcall.m_Payload.m_DispatchIndirect.m_Buffer ).c_str(),
                drawcall.m_Payload.m_DispatchIndirect.m_Offset );
            break;

        case DeviceProfilerDrawcallType::eCopyBuffer:
            fprintf( pFile,
                "\"srcBuffer\":\"%s\","
                "\"dstBuffer\":\"%s\"",
                m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyBuffer.m_SrcBuffer ).c_str(),
                m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyBuffer.m_DstBuffer ).c_str() );
            break;

        case DeviceProfilerDrawcallType::eCopyBufferToImage:
            fprintf( pFile,
                "\"srcBuffer\":\"%s\","
                "\"dstImage\":\"%s\"",
                m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyBufferToImage.m_SrcBuffer ).c_str(),
                m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyBufferToImage.m_DstImage ).c_str() );
            break;

        case DeviceProfilerDrawcallType::eCopyImage:
            fprintf( pFile,
                "\"srcImage\":\"%s\","
                "\"dstImage\":\"%s\"",
                m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyImage.m_SrcImage ).c_str(),
                m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyImage.m_DstImage ).c_str() );
            break;

        case DeviceProfilerDrawcallType::eCopyImageToBuffer:
            fprintf( pFile,
                "\"srcImage\":\"%s\","
                "\"dstBuffer\":\"%s\"",
                m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyImageToBuffer.m_SrcImage ).c_str(),
                m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyImageToBuffer.m_DstBuffer ).c_str() );
            break;

        case DeviceProfilerDrawcallType::eClearAttachments:
            fprintf( pFile,
                "\"attachmentCount\":%" PRIu32,
                drawcall.m_Payload.m_ClearAttachments.m_Count );
            break;

        case DeviceProfilerDrawcallType::eClearColorImage:
            fprintf( pFile,
                "\"image\":\"%s\","
                "\"value\":",
                m_pStringSerializer->GetName( drawcall.m_Payload.m_ClearColorImage.m_Image ).c_str() );
            WriteColorClearValue( pFile, drawcall.m_Payload.m_ClearColorImage.m_Value );
            break;

        case DeviceProfilerDrawcallType::eClearDepthStencilImage:
            fprintf( pFile,
                "\"image\":\"%s\","
                "\"value\":",
                m_pStringSerializer->GetName( drawcall.m_Payload.m_ClearDepthStencilImage.m_Image ).c_str() );
            WriteDepthStencilClearValue( pFile, drawcall.m_Payload.m_ClearDepthStencilImage.m_Value );
            break;

        case DeviceProfilerDrawcallType::eResolveImage:
            fprintf( pFile,
                "\"srcImage\":\"%s\","
                "\"dstImage\":\"%s\"",
                m_pStringSerializer->GetName( drawcall.m_Payload.m_ResolveImage.m_SrcImage ).c_str(),
                m_pStringSerializer->GetName( drawcall.m_Payload.m_ResolveImage.m_DstImage ).c_str() );
            break;

        case DeviceProfilerDrawcallType::eBlitImage:
            fprintf( pFile,
                "\"srcImage\":\"%s\","
                "\"dstImage\":\"%s\"",
                m_pStringSerializer->GetName( drawcall.m_Payload.m_BlitImage.m_SrcImage ).c_str(),
                m_pStringSerializer->GetName( drawcall.m_Payload.m_BlitImage.m_DstImage ).c_str() );
            break;

        case DeviceProfilerDrawcallType::eFillBuffer:
            fprintf( pFile,
                "\"dstBuffer\":\"%s\","
                "\"dstOffset\":%" PRIu64 ","
                "\"size\":%" PRIu64 ","
                "\"data\":%" PRIu32,
                m_pStringSerializer->GetName( drawcall.m_Payload.m_FillBuffer.m_Buffer ).c_str(),
                drawcall.m_Payload.m_FillBuffer.m_Offset,
                drawcall.m_Payload.m_FillBuffer.m_Size,
                drawcall.m_Payload.m_FillBuffer.m_Data );
            break;

        case DeviceProfilerDrawcallType::eUpdateBuffer:
            fprintf( pFile,
                "\"dstBuffer\":\"%s\","
                "\"dstOffset\":%" PRIu64 ","
                "\"dataSize\":%" PRIu64,
                m_pStringSerializer->GetName( drawcall.m_Payload.m_UpdateBuffer.m_Buffer ).c_str(),
                drawcall.m_Payload.m_UpdateBuffer.m_Offset,
                drawcall.m_Payload.m_UpdateBuffer.m_Size );
            break;

        case DeviceProfilerDrawcallType::eTraceRaysKHR:
            fprintf( pFile,
                "\"width\":%" PRIu32 ","
                "\"height\":%" PRIu32 ","
                "\"depth\":%" PRIu32,
                drawcall.m_Payload.m_TraceRays.m_Width,
                drawcall.m_Payload.m_TraceRays.m_Height,
                drawcall.m_Payload.m_TraceRays.m_Depth );
            break;

        case DeviceProfilerDrawcallType::eTraceRaysIndirectKHR:
            fprintf( pFile,
                "\"indirectDeviceAddress\":%" PRIu64,
                drawcall.m_Payload.m_TraceRaysIndirect.m_IndirectAddress );
            break;

        case DeviceProfilerDrawcallType::eTraceRaysIndirect2KHR:
            fprintf( pFile,
                "\"indirectDeviceAddress\":%" PRIu64,
                drawcall.m_Payload.m_TraceRaysIndirect2.m_IndirectAddress );
            break;

        case DeviceProfilerDrawcallType::eBuildAccelerationStructuresKHR:
        case DeviceProfilerDrawcallType::eBuildAccelerationStructuresIndirectKHR:
        {
            const uint32_t infoCount = drawcall.m_Payload.m_BuildAccelerationStructures.m_InfoCount;

            fprintf( pFile,
                "\"infoCount\":%" PRIu32 ","
                "\"infos\":[",
                infoCount );

            for( uint32_t i = 0; i < infoCount; ++i )
            {
                const auto& info = drawcall.m_Payload.m_BuildAccelerationStructures.m_pInfos[ i ];

                if( i > 0 ) fputc( ',', pFile );
                fprintf( pFile,
                    "\"type\":\"%s\","
                    "\"flags\":\"%s\","
                    "\"mode\":\"%s\","
                    "\"src\":\"%s\","
                    "\"dst\":\"%s\","
                    "\"geometryCount\":%" PRIu32 ","
                    "\"geometries\":[",
                    m_pStringSerializer->GetAccelerationStructureTypeName( info.type ).c_str(),
                    m_pStringSerializer->GetBuildAccelerationStructureFlagNames( info.flags ).c_str(),
                    m_pStringSerializer->GetBuildAccelerationStructureModeName( info.mode ).c_str(),
                    m_pStringSerializer->GetName( VkAccelerationStructureKHRHandle( info.srcAccelerationStructure ) ).c_str(),
                    m_pStringSerializer->GetName( VkAccelerationStructureKHRHandle( info.dstAccelerationStructure ) ).c_str(),
                    info.geometryCount );

                if( info.pGeometries )
                {
                    for( uint32_t j = 0; j < info.geometryCount; ++j )
                    {
                        const auto& geometry = info.pGeometries[ j ];

                        if( j > 0 ) fputc( ',', pFile );
                        fprintf( pFile,
                            "{\"type\":\"%s\","
                            "\"flags\":\"%s\","
                            "\"data\":{",
                            m_pStringSerializer->GetGeometryTypeName( geometry.geometryType ).c_str(),
                            m_pStringSerializer->GetGeometryFlagNames( geometry.flags ).c_str() );

                        switch( geometry.geometryType )
                        {
                        case VK_GEOMETRY_TYPE_TRIANGLES_KHR:
                            fprintf( pFile,
                                "\"vertexFormat\":\"%s\","
                                "\"vertexData\":\"%s\","
                                "\"vertexStride\":%" PRIu64 ","
                                "\"maxVertex\":%" PRIu32 ","
                                "\"indexType\":\"%s\","
                                "\"indexData\":\"%s\","
                                "\"transformData\":\"%s\"",
                                m_pStringSerializer->GetFormatName( geometry.geometry.triangles.vertexFormat ).c_str(),
                                m_pStringSerializer->GetPointer( geometry.geometry.triangles.vertexData.hostAddress ).c_str(),
                                geometry.geometry.triangles.vertexStride,
                                geometry.geometry.triangles.maxVertex,
                                m_pStringSerializer->GetIndexTypeName( geometry.geometry.triangles.indexType ).c_str(),
                                m_pStringSerializer->GetPointer( geometry.geometry.triangles.indexData.hostAddress ).c_str(),
                                m_pStringSerializer->GetPointer( geometry.geometry.triangles.transformData.hostAddress ).c_str() );
                            break;

                        case VK_GEOMETRY_TYPE_AABBS_KHR:
                            fprintf( pFile,
                                "\"data\":\"%s\","
                                "\"stride\":%" PRIu64 "",
                                m_pStringSerializer->GetPointer( geometry.geometry.aabbs.data.hostAddress ).c_str(),
                                geometry.geometry.aabbs.stride );
                            break;

                        case VK_GEOMETRY_TYPE_INSTANCES_KHR:
                            fprintf( pFile,
                                "\"arrayOfPointers\":%s,"
                                "\"data\":\"%s\"",
                                geometry.geometry.instances.arrayOfPointers ? "true" : "false",
                                m_pStringSerializer->GetPointer( geometry.geometry.instances.data.hostAddress ).c_str() );
                            break;
                        }

                        fputs( "},\"range\":{", pFile );

                        if( drawcall.m_Type == DeviceProfilerDrawcallType::eBuildAccelerationStructuresKHR )
                        {
                            const auto& range = drawcall.m_Payload.m_BuildAccelerationStructures.m_ppRanges[ i ][ j ];
                            fprintf( pFile,
                                "\"primitiveCount\":%" PRIu32 ","
                                "\"primitiveOffset\":%" PRIu32 ","
                                "\"firstVertex\":%" PRIu32 ","
                                "\"transformOffset\":%" PRIu32 "",
                                range.primitiveCount,
                                range.primitiveOffset,
                                range.firstVertex,
                                range.transformOffset );
                        }
                        else //( drawcall.m_Type == DeviceProfilerDrawcallType::eBuildAccelerationStructuresIndirectKHR )
                        {
                            fprintf( pFile,
                                "\"maxPrimitiveCount\":%" PRIu32 "",
                                drawcall.m_Payload.m_BuildAccelerationStructuresIndirect.m_ppMaxPrimitiveCounts[ i ][ j ] );
                        }

                        fputs( "}}", pFile );
                    }
                }

                fputc( ']', pFile );
            }

            fputc( ']', pFile );
            break;
        }

        case DeviceProfilerDrawcallType::eBuildMicromapsEXT:
        {
            const uint32_t infoCount = drawcall.m_Payload.m_BuildMicromaps.m_InfoCount;

            fprintf( pFile,
                "\"infoCount\":%" PRIu32 ","
                "\"infos\":[",
                infoCount );

            for( uint32_t i = 0; i < infoCount; ++i )
            {
                const auto& info = drawcall.m_Payload.m_BuildMicromaps.m_pInfos[i];

                if( i > 0 ) fputc( ',', pFile );
                fprintf( pFile,
                    "\"type\":\"%s\","
                    "\"flags\":\"%s\","
                    "\"mode\":\"%s\","
                    "\"dst\":\"%s\","
                    "\"usageCountsCount\":%" PRIu32 ","
                    "\"usageCounts\":[",
                    m_pStringSerializer->GetMicromapTypeName( info.type ).c_str(),
                    m_pStringSerializer->GetBuildMicromapFlagNames( info.flags ).c_str(),
                    m_pStringSerializer->GetBuildMicromapModeName( info.mode ).c_str(),
                    m_pStringSerializer->GetName( VkMicromapEXTHandle( info.dstMicromap ) ).c_str(),
                    info.usageCountsCount );

                for( uint32_t j = 0; j < info.usageCountsCount; ++j )
                {
                    const auto& usageCount = info.pUsageCounts[j];
                    if( j > 0 ) fputc( ',', pFile );
                    fprintf( pFile,
                        "{\"count\":%" PRIu32 ","
                        "\"format\":%" PRIu32 ","
                        "\"subdivisionLevel\":%" PRIu32 "}",
                        usageCount.count,
                        usageCount.format,
                        usageCount.subdivisionLevel );
                }

                fprintf( pFile,
                    "],"
                    "\"data\":\"%s\","
                    "\"scratchData\":\"%s\","
                    "\"triangleArray\":\"%s\","
                    "\"triangleArrayStride\":%" PRIu64 "",
                    m_pStringSerializer->GetPointer( info.data.hostAddress ).c_str(),
                    m_pStringSerializer->GetPointer( info.scratchData.hostAddress ).c_str(),
                    m_pStringSerializer->GetPointer( info.triangleArray.hostAddress ).c_str(),
                    info.triangleArrayStride );
            }

            fputc( ']', pFile );
            break;
        }
        }

        fputc( '}', pFile );
    }

    /*************************************************************************\

    Function:
        GetPipelineArgs

    Description:
        Serialize pipeline state into JSON object.

    \*************************************************************************/
    void DeviceProfilerJsonSerializer::WritePipelineArgs( FILE* pFile, const DeviceProfilerPipeline& pipeline ) const
    {
        // Append shader stages info.
        if( !pipeline.m_ShaderTuple.m_Shaders.empty() )
        {
            fputs( "\"shaders\":[", pFile );
            bool firstShader = true;
            for( const ProfilerShader& shader : pipeline.m_ShaderTuple.m_Shaders )
            {
                if( !firstShader ) fputc( ',', pFile );
                firstShader = false;
                WriteShaderStageArgs( pFile, shader );
            }
            fputc( ']', pFile );
        }

        // Append pipeline create info details.
        if( pipeline.m_pCreateInfo )
        {
            switch( pipeline.m_Type )
            {
            case DeviceProfilerPipelineType::eGraphics:
            {
                WriteGraphicsPipelineCreateInfoArgs( pFile,
                    pipeline.m_pCreateInfo->m_GraphicsPipelineCreateInfo );
                break;
            }
            case DeviceProfilerPipelineType::eCompute:
            {
                WriteComputePipelineCreateInfoArgs( pFile,
                    pipeline.m_pCreateInfo->m_ComputePipelineCreateInfo );
                break;
            }
            case DeviceProfilerPipelineType::eRayTracingKHR:
            {
                WriteRayTracingPipelineCreateInfoArgs( pFile,
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
    void DeviceProfilerJsonSerializer::WriteColorClearValue( FILE* pFile, const VkClearColorValue& value ) const
    {
        fputs( "{\"VkClearColorValue\":{", pFile );
        fputs( "\"float32\":[", pFile );
        fprintf( pFile, "%f,%f,%f,%f", value.float32[0], value.float32[1], value.float32[2], value.float32[3] );
        fputs( "],", pFile );
        fputs( "\"int32\":[", pFile );
        fprintf( pFile, "%d,%d,%d,%d", value.int32[0], value.int32[1], value.int32[2], value.int32[3] );
        fputs( "],", pFile );
        fputs( "\"uint32\":[", pFile );
        fprintf( pFile, "%u,%u,%u,%u", value.uint32[0], value.uint32[1], value.uint32[2], value.uint32[3] );
        fputs( "]}}", pFile );
    }

    /*************************************************************************\

    Function:
        GetDepthStencilClearValue

    Description:
        Serialize VkClearDepthStencilValue struct into JSON object.

    \*************************************************************************/
    void DeviceProfilerJsonSerializer::WriteDepthStencilClearValue( FILE* pFile, const VkClearDepthStencilValue& value ) const
    {
        fputs( "{\"VkClearDepthStencilValue\":{", pFile );
        fprintf( pFile, "\"depth\":%f,\"stencil\":%u", value.depth, value.stencil );
        fputs( "}}", pFile );
    }

    /*************************************************************************\

    Function:
        GetShaderStageArgs

    Description:
        Serialize shader stage into JSON object.

    \*************************************************************************/
    void DeviceProfilerJsonSerializer::WriteShaderStageArgs( FILE* pFile, const ProfilerShader& shader ) const
    {
        fprintf( pFile,
            "{\"stage\":\"%s\","
            "\"entryPoint\":\"%s\"",
            m_pStringSerializer->GetShaderStageName( shader.m_Stage ).c_str(),
            shader.m_EntryPoint.c_str() );

        if( shader.m_pShaderModule )
        {
            static constexpr char hexDigits[] = "0123456789abcdef";

            const ProfilerShaderModule& shaderModule = *shader.m_pShaderModule;
            const uint32_t identifierSize = shaderModule.m_IdentifierSize;
            const uint8_t* pIdentifier = shaderModule.m_Identifier;

            if( identifierSize > 0 && pIdentifier )
            {
                fputs( ",\"shaderIdentifier\":\"", pFile );

                // Convert from the end to keep the little-endian order.
                for( uint32_t i = identifierSize; i > 0; --i )
                {
                    fputc( hexDigits[pIdentifier[i - 1] >> 4], pFile );
                    fputc( hexDigits[pIdentifier[i - 1] & 0xF], pFile );

                    if( ( i != 1 ) && ( ( i - 1 ) % 8 ) == 0 )
                    {
                        // Insert a dash separator every 8 bytes for better readability.
                        fputc( '-', pFile );
                    }
                }

                fputc( '"', pFile );
            }
        }

        fputc( '}', pFile );
    }

    /*************************************************************************\

    Function:
        GetGraphicsPipelineCreateInfoArgs

    Description:
        Serialize graphics pipeline state into JSON object.

    \*************************************************************************/
    void DeviceProfilerJsonSerializer::WriteGraphicsPipelineCreateInfoArgs( FILE* pFile, const VkGraphicsPipelineCreateInfo& createInfo ) const
    {
        // VkPipelineVertexInputStateCreateInfo
        fputs( "\"vertexInputState\":", pFile );
        if( createInfo.pVertexInputState )
        {
            const auto& state = *createInfo.pVertexInputState;

            fprintf( pFile,
                "{\"attributeCount\":%" PRIu32 ","
                "\"bindingCount\":%" PRIu32 ","
                "\"attributes\":",
                state.vertexAttributeDescriptionCount,
                state.vertexBindingDescriptionCount );

            if( state.pVertexAttributeDescriptions )
            {
                fputc( '[', pFile );
                for( uint32_t i = 0; i < state.vertexAttributeDescriptionCount; ++i )
                {
                    const auto& attribute = state.pVertexAttributeDescriptions[i];
                    if( i > 0 ) fputc( ',', pFile );
                    fprintf( pFile,
                        "{\"location\":%" PRIu32 ","
                        "\"binding\":%" PRIu32 ","
                        "\"format\":\"%s\","
                        "\"offset\":%" PRIu32 "}",
                        attribute.location,
                        attribute.binding,
                        m_pStringSerializer->GetFormatName( attribute.format ).c_str(),
                        attribute.offset );
                }
                fputc( ']', pFile );
            }
            else
            {
                fputs( "null", pFile );
            }

            fputs( ",\"bindings\":", pFile );

            if( state.pVertexBindingDescriptions )
            {
                fputc( '[', pFile );
                for( uint32_t i = 0; i < state.vertexBindingDescriptionCount; ++i )
                {
                    const auto& binding = state.pVertexBindingDescriptions[i];
                    if( i > 0 ) fputc( ',', pFile );
                    fprintf( pFile,
                        "{\"binding\":%" PRIu32 ","
                        "\"stride\":%" PRIu32 ","
                        "\"inputRate\":\"%s\"}",
                        binding.binding,
                        binding.stride,
                        m_pStringSerializer->GetVertexInputRateName( binding.inputRate ).c_str() );
                }
                fputc( ']', pFile );
            }
            else
            {
                fputs( "null", pFile );
            }

            fputs( "},", pFile );
        }
        else
        {
            fputs( "null,", pFile );
        }

        // VkPipelineInputAssemblyStateCreateInfo
        fputs( "\"inputAssemblyState\":", pFile );
        if( createInfo.pInputAssemblyState )
        {
            const auto& state = *createInfo.pInputAssemblyState;
            fprintf( pFile,
                "{\"topology\":\"%s\","
                "\"primitiveRestartEnable\":%s},",
                m_pStringSerializer->GetPrimitiveTopologyName( state.topology ).c_str(),
                state.primitiveRestartEnable ? "true" : "false" );
        }
        else
        {
            fputs( "null,", pFile );
        }

        // VkPipelineTessellationStateCreateInfo
        fputs( "\"tessellationState\":", pFile );
        if( createInfo.pTessellationState )
        {
            const auto& state = *createInfo.pTessellationState;
            fprintf( pFile,
                "{\"patchControlPoints\":%" PRIu32 "},",
                state.patchControlPoints );
        }
        else
        {
            fputs( "null,", pFile );
        }

        // VkPipelineViewportStateCreateInfo
        fputs( "\"viewportState\":", pFile );
        if( createInfo.pViewportState )
        {
            const auto& state = *createInfo.pViewportState;
            fprintf( pFile,
                "{\"viewportCount\":%" PRIu32 ","
                "\"scissorCount\":%" PRIu32 ","
                "\"viewports\":",
                state.viewportCount,
                state.scissorCount );

            if( state.pViewports )
            {
                fputc( '[', pFile );
                for( uint32_t i = 0; i < state.viewportCount; ++i )
                {
                    const auto& viewport = state.pViewports[i];
                    if( i > 0 ) fputc( ',', pFile );
                    fprintf( pFile,
                        "{\"x\":%f,"
                        "\"y\":%f,"
                        "\"width\":%f,"
                        "\"height\":%f,"
                        "\"minDepth\":%f,"
                        "\"maxDepth\":%f}",
                        viewport.x,
                        viewport.y,
                        viewport.width,
                        viewport.height,
                        viewport.minDepth,
                        viewport.maxDepth );
                }
                fputc( ']', pFile );
            }
            else
            {
                fputs( "null", pFile );
            }

            fputs( ",\"scissors\":", pFile );

            if( state.pScissors )
            {
                fputc( '[', pFile );
                for( uint32_t i = 0; i < state.scissorCount; ++i )
                {
                    const auto& scissor = state.pScissors[i];
                    if( i > 0 ) fputc( ',', pFile );
                    fprintf( pFile,
                        "{\"offsetX\":%" PRIi32 ","
                        "\"offsetY\":%" PRIi32 ","
                        "\"extentWidth\":%" PRIu32 ","
                        "\"extentHeight\":%" PRIu32 "}",
                        scissor.offset.x,
                        scissor.offset.y,
                        scissor.extent.width,
                        scissor.extent.height );
                }
                fputc( ']', pFile );
            }
            else
            {
                fputs( "null", pFile );
            }

            fputs( "},", pFile );
        }
        else
        {
            fputs( "null,", pFile );
        }

        // VkPipelineRasterizationStateCreateInfo
        fputs( "\"rasterizationState\":", pFile );
        if( createInfo.pRasterizationState )
        {
            const auto& state = *createInfo.pRasterizationState;
            fprintf( pFile,
                "{\"depthClampEnable\":%s,"
                "\"rasterizerDiscardEnable\":%s,"
                "\"polygonMode\":\"%s\","
                "\"cullMode\":\"%s\","
                "\"frontFace\":\"%s\","
                "\"depthBiasEnable\":%s,"
                "\"depthBiasConstantFactor\":%f,"
                "\"depthBiasClamp\":%f,"
                "\"depthBiasSlopeFactor\":%f,"
                "\"lineWidth\":%f},",
                state.depthClampEnable ? "true" : "false",
                state.rasterizerDiscardEnable ? "true" : "false",
                m_pStringSerializer->GetPolygonModeName( state.polygonMode ).c_str(),
                m_pStringSerializer->GetCullModeName( state.cullMode ).c_str(),
                m_pStringSerializer->GetFrontFaceName( state.frontFace ).c_str(),
                state.depthBiasEnable ? "true" : "false",
                state.depthBiasConstantFactor,
                state.depthBiasClamp,
                state.depthBiasSlopeFactor,
                state.lineWidth );
        }
        else
        {
            fputs( "null,", pFile );
        }

        // VkPipelineMultisampleStateCreateInfo
        fputs( "\"multisampleState\":", pFile );
        if( createInfo.pMultisampleState )
        {
            const auto& state = *createInfo.pMultisampleState;
            fprintf( pFile,
                "{\"rasterizationSamples\":%" PRIu32 ","
                "\"sampleShadingEnable\":%s,"
                "\"minSampleShading\":%f,"
                "\"sampleMask\":\"0x%08X\","
                "\"alphaToCoverageEnable\":%s,"
                "\"alphaToOneEnable\":%s}",
                state.rasterizationSamples,
                state.sampleShadingEnable ? "true" : "false",
                state.minSampleShading,
                state.pSampleMask ? *state.pSampleMask : 0xFFFFFFFF,
                state.alphaToCoverageEnable ? "true" : "false",
                state.alphaToOneEnable ? "true" : "false" );
        }
        else
        {
            fputs( "null,", pFile );
        }

        // VkPipelineDepthStencilStateCreateInfo
        fputs( "\"depthStencilState\":", pFile );
        if( createInfo.pDepthStencilState )
        {
            const auto& state = *createInfo.pDepthStencilState;
            fprintf( pFile,
                "{\"depthTestEnable\":%s,"
                "\"depthWriteEnable\":%s,"
                "\"depthCompareOp\":\"%s\","
                "\"depthBoundsTestEnable\":%s,"
                "\"minDepthBounds\":%f,"
                "\"maxDepthBounds\":%f,"
                "\"stencilTestEnable\":%s,",
                state.depthTestEnable ? "true" : "false",
                state.depthWriteEnable ? "true" : "false",
                m_pStringSerializer->GetCompareOpName( state.depthCompareOp ).c_str(),
                state.depthBoundsTestEnable ? "true" : "false",
                state.minDepthBounds,
                state.maxDepthBounds,
                state.stencilTestEnable ? "true" : "false" );

            fprintf( pFile,
                "\"front\":{"
                "\"failOp\":%" PRIu32 ","
                "\"passOp\":%" PRIu32 ","
                "\"depthFailOp\":%" PRIu32 ","
                "\"compareOp\":\"%s\","
                "\"compareMask\":\"0x%02X\","
                "\"writeMask\":\"0x%02X\","
                "\"reference\":\"0x%02X\"},",
                state.front.failOp,
                state.front.passOp,
                state.front.depthFailOp,
                m_pStringSerializer->GetCompareOpName( state.front.compareOp ).c_str(),
                state.front.compareMask,
                state.front.writeMask,
                state.front.reference );

            fprintf( pFile,
                "\"back\":{"
                "\"failOp\":%" PRIu32 ","
                "\"passOp\":%" PRIu32 ","
                "\"depthFailOp\":%" PRIu32 ","
                "\"compareOp\":\"%s\","
                "\"compareMask\":\"0x%02X\","
                "\"writeMask\":\"0x%02X\","
                "\"reference\":\"0x%02X\"}",
                state.back.failOp,
                state.back.passOp,
                state.back.depthFailOp,
                m_pStringSerializer->GetCompareOpName( state.back.compareOp ).c_str(),
                state.back.compareMask,
                state.back.writeMask,
                state.back.reference );

            fputs( "},", pFile );
        }
        else
        {
            fputs( "null,", pFile );
        }

        // VkPipelineColorBlendStateCreateInfo
        fputs( "\"colorBlendState\":", pFile );
        if( createInfo.pColorBlendState )
        {
            const auto& state = *createInfo.pColorBlendState;
            fprintf( pFile,
                "{\"logicOpEnable\":%s,"
                "\"logicOp\":\"%s\","
                "\"blendConstants\":[%f,%f,%f,%f],"
                "\"attachments\":",
                state.logicOpEnable ? "true" : "false",
                m_pStringSerializer->GetLogicOpName( state.logicOp ).c_str(),
                state.blendConstants[0],
                state.blendConstants[1],
                state.blendConstants[2],
                state.blendConstants[3] );

            if( state.pAttachments )
            {
                fputc( '[', pFile );
                for( uint32_t i = 0; i < state.attachmentCount; ++i )
                {
                    const auto& attachment = state.pAttachments[i];
                    if( i > 0 ) fputc( ',', pFile );
                    fprintf( pFile,
                        "{\"blendEnable\":%s,"
                        "\"srcColorBlendFactor\":\"%s\","
                        "\"dstColorBlendFactor\":\"%s\","
                        "\"colorBlendOp\":\"%s\","
                        "\"srcAlphaBlendFactor\":\"%s\","
                        "\"dstAlphaBlendFactor\":\"%s\","
                        "\"alphaBlendOp\":\"%s\","
                        "\"colorWriteMask\":\"%s\"}",
                        attachment.blendEnable ? "true" : "false",
                        m_pStringSerializer->GetBlendFactorName( attachment.srcColorBlendFactor ).c_str(),
                        m_pStringSerializer->GetBlendFactorName( attachment.dstColorBlendFactor ).c_str(),
                        m_pStringSerializer->GetBlendOpName( attachment.colorBlendOp ).c_str(),
                        m_pStringSerializer->GetBlendFactorName( attachment.srcAlphaBlendFactor ).c_str(),
                        m_pStringSerializer->GetBlendFactorName( attachment.dstAlphaBlendFactor ).c_str(),
                        m_pStringSerializer->GetBlendOpName( attachment.alphaBlendOp ).c_str(),
                        m_pStringSerializer->GetColorComponentFlagNames( attachment.colorWriteMask ).c_str() );
                }
                fputc( ']', pFile );
            }
            else
            {
                fputs( "null", pFile );
            }

            fputs( "},", pFile );
        }
        else
        {
            fputs( "null,", pFile );
        }

        // VkPipelineDynamicStateCreateInfo
        fputs( "\"dynamicStates\":", pFile );
        if( createInfo.pDynamicState )
        {
            fputc( '[', pFile );
            const auto& state = *createInfo.pDynamicState;
            for( uint32_t i = 0; i < state.dynamicStateCount; ++i )
            {
                if( i > 0 ) fputc( ',', pFile );
                fprintf( pFile,
                    "\"%s\"",
                    m_pStringSerializer->GetDynamicStateName( state.pDynamicStates[i] ).c_str() );
            }
            fputc( ']', pFile );
        }
        else
        {
            fputs( "null", pFile );
        }
    }

    /*************************************************************************\

    Function:
        GetComputePipelineCreateInfoArgs

    Description:
        Serialize compute pipeline state into JSON object.

    \*************************************************************************/
    void DeviceProfilerJsonSerializer::WriteComputePipelineCreateInfoArgs( FILE* pFile, const VkComputePipelineCreateInfo& createInfo ) const
    {
        // No additional state to serialize for compute pipelines yet.
    }

    /*************************************************************************\

    Function:
        GetRayTracingPipelineCreateInfoArgs

    Description:
        Serialize ray-tracing pipeline state into JSON object.

    \*************************************************************************/
    void DeviceProfilerJsonSerializer::WriteRayTracingPipelineCreateInfoArgs( FILE* pFile, const VkRayTracingPipelineCreateInfoKHR& createInfo ) const
    {
        fprintf( pFile,
            "\"maxPipelineRayRecursionDepth\":%" PRIu32 ","
            "\"libraryInterface\":",
            createInfo.maxPipelineRayRecursionDepth );

        // VkRayTracingPipelineInterfaceCreateInfoKHR
        if( createInfo.pLibraryInterface )
        {
            const auto& state = *createInfo.pLibraryInterface;
            fprintf( pFile,
                "{\"maxPipelineRayPayloadSize\":%" PRIu32 ","
                "\"maxPipelineRayHitAttributeSize\":%" PRIu32 "},",
                state.maxPipelineRayPayloadSize,
                state.maxPipelineRayHitAttributeSize );
        }
        else
        {
            fputs( "null,", pFile );
        }

        fputs( "\"dynamicStates\":", pFile );

        // VkPipelineDynamicStateCreateInfo
        if( createInfo.pDynamicState )
        {
            fputc( '[', pFile );
            const auto& state = *createInfo.pDynamicState;
            for( uint32_t i = 0; i < state.dynamicStateCount; ++i )
            {
                if( i > 0 ) fputc( ',', pFile );
                fprintf( pFile, "\"%s\"", m_pStringSerializer->GetDynamicStateName( state.pDynamicStates[i] ).c_str() );
            }
            fputc( ']', pFile );
        }
        else
        {
            fputs( "null", pFile );
        }
    }
}

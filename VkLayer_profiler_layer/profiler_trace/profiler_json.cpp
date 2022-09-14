// Copyright (c) 2019-2021 Lukasz Stalmirski
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
#include "profiler_helpers/profiler_data_helpers.h"
#include "profiler_layer_objects/VkObject.h"

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

        case DeviceProfilerDrawcallType::eBuildAccelerationStructuresKHR:
        case DeviceProfilerDrawcallType::eBuildAccelerationStructuresIndirectKHR:
        {
            const uint32_t infoCount = drawcall.m_Payload.m_BuildAccelerationStructures.m_InfoCount;

            std::vector<nlohmann::json> infos;
            infos.reserve(infoCount);

            for( uint32_t i = 0; i < infoCount; ++i )
            {
                const auto& info = drawcall.m_Payload.m_BuildAccelerationStructures.m_pInfos[ i ];
                infos.push_back( {
                    { "type", m_pStringSerializer->GetAccelerationStructureTypeName( info.type ) },
                    { "flags", m_pStringSerializer->GetBuildAccelerationStructureFlagNames( info.flags ) },
                    { "mode", m_pStringSerializer->GetBuildAccelerationStructureModeName( info.mode ) },
                    { "src", m_pStringSerializer->GetName( info.srcAccelerationStructure ) },
                    { "dst", m_pStringSerializer->GetName( info.dstAccelerationStructure ) },
                    { "geometryCount", info.geometryCount } } );
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

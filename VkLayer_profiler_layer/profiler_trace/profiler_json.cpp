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
        DeviceProfilerJsonBuilder

    Description:
        Constructor.

    \*************************************************************************/
    DeviceProfilerJsonBuilder::DeviceProfilerJsonBuilder()
        : m_Builder()
        , m_FirstElementInScope( true )
    {
    }

    /*************************************************************************\

    Function:
        Reset

    \*************************************************************************/
    void DeviceProfilerJsonBuilder::Reset()
    {
        m_Builder.clear();
        m_FirstElementInScope = true;
    }

    /*************************************************************************\

    Function:
        AddValue

    \*************************************************************************/
    void DeviceProfilerJsonBuilder::AddValue( std::string_view value )
    {
        if( !m_FirstElementInScope )
        {
            m_Builder.append_comma();
        }

        m_Builder.escape_and_append_with_quotes( value );
        m_FirstElementInScope = false;
    }

    /*************************************************************************\

    Function:
        AddValue

    \*************************************************************************/
    void DeviceProfilerJsonBuilder::AddValue( uint64_t value )
    {
        if( !m_FirstElementInScope )
        {
            m_Builder.append_comma();
        }

        m_Builder.append( value );
        m_FirstElementInScope = false;
    }

    /*************************************************************************\

    Function:
        AddValue

    \*************************************************************************/
    void DeviceProfilerJsonBuilder::AddValue( int64_t value )
    {
        if( !m_FirstElementInScope )
        {
            m_Builder.append_comma();
        }

        m_Builder.append( value );
        m_FirstElementInScope = false;
    }

    /*************************************************************************\

    Function:
        AddValue

    \*************************************************************************/
    void DeviceProfilerJsonBuilder::AddValue( uint32_t value )
    {
        if( !m_FirstElementInScope )
        {
            m_Builder.append_comma();
        }

        m_Builder.append( value );
        m_FirstElementInScope = false;
    }

    /*************************************************************************\

    Function:
        AddValue

    \*************************************************************************/
    void DeviceProfilerJsonBuilder::AddValue( int32_t value )
    {
        if( !m_FirstElementInScope )
        {
            m_Builder.append_comma();
        }

        m_Builder.append( value );
        m_FirstElementInScope = false;
    }

    /*************************************************************************\

    Function:
        AddValue

    \*************************************************************************/
    void DeviceProfilerJsonBuilder::AddValue( double value )
    {
        if( !m_FirstElementInScope )
        {
            m_Builder.append_comma();
        }

        m_Builder.append( value );
        m_FirstElementInScope = false;
    }

    /*************************************************************************\

    Function:
        AddValue

    \*************************************************************************/
    void DeviceProfilerJsonBuilder::AddValue( bool value )
    {
        if( !m_FirstElementInScope )
        {
            m_Builder.append_comma();
        }

        m_Builder.append( value );
        m_FirstElementInScope = false;
    }

    /*************************************************************************\

    Function:
        AddKeyValue

    \*************************************************************************/
    void DeviceProfilerJsonBuilder::AddKeyValue( std::string_view key, std::string_view value )
    {
        if( !m_FirstElementInScope )
        {
            m_Builder.append_comma();
        }

        m_Builder.append_key_value( key, value );
        m_FirstElementInScope = false;
    }

    /*************************************************************************\

    Function:
        AddKeyValue

    \*************************************************************************/
    void DeviceProfilerJsonBuilder::AddKeyValue( std::string_view key, uint64_t value )
    {
        if( !m_FirstElementInScope )
        {
            m_Builder.append_comma();
        }

        m_Builder.append_key_value( key, value );
        m_FirstElementInScope = false;
    }

    /*************************************************************************\

    Function:
        AddKeyValue

    \*************************************************************************/
    void DeviceProfilerJsonBuilder::AddKeyValue( std::string_view key, int64_t value )
    {
        if( !m_FirstElementInScope )
        {
            m_Builder.append_comma();
        }

        m_Builder.append_key_value( key, value );
        m_FirstElementInScope = false;
    }

    /*************************************************************************\

    Function:
        AddKeyValue

    \*************************************************************************/
    void DeviceProfilerJsonBuilder::AddKeyValue( std::string_view key, uint32_t value )
    {
        if( !m_FirstElementInScope )
        {
            m_Builder.append_comma();
        }

        m_Builder.append_key_value( key, value );
        m_FirstElementInScope = false;
    }

    /*************************************************************************\

    Function:
        AddKeyValue

    \*************************************************************************/
    void DeviceProfilerJsonBuilder::AddKeyValue( std::string_view key, int32_t value )
    {
        if( !m_FirstElementInScope )
        {
            m_Builder.append_comma();
        }

        m_Builder.append_key_value( key, value );
        m_FirstElementInScope = false;
    }

    /*************************************************************************\

    Function:
        AddKeyValue

    \*************************************************************************/
    void DeviceProfilerJsonBuilder::AddKeyValue( std::string_view key, double value )
    {
        if( !m_FirstElementInScope )
        {
            m_Builder.append_comma();
        }

        m_Builder.append_key_value( key, value );
        m_FirstElementInScope = false;
    }

    /*************************************************************************\

    Function:
        AddKeyValue

    \*************************************************************************/
    void DeviceProfilerJsonBuilder::AddKeyValue( std::string_view key, bool value )
    {
        if( !m_FirstElementInScope )
        {
            m_Builder.append_comma();
        }

        m_Builder.append_key_value( key, value );
        m_FirstElementInScope = false;
    }

    /*************************************************************************\

    Function:
        BeginKeyValue

    \*************************************************************************/
    void DeviceProfilerJsonBuilder::BeginKeyValue( std::string_view key )
    {
        if( !m_FirstElementInScope )
        {
            m_Builder.append_comma();
        }

        m_Builder.escape_and_append_with_quotes( key );
        m_Builder.append_colon();
        m_FirstElementInScope = true;
    }

    /*************************************************************************\

    Function:
        EndKeyValue

    \*************************************************************************/
    void DeviceProfilerJsonBuilder::EndKeyValue()
    {
        m_FirstElementInScope = false;
    }

    /*************************************************************************\

    Function:
        BeginArray

    \*************************************************************************/
    void DeviceProfilerJsonBuilder::BeginArray( std::string_view name )
    {
        if( !m_FirstElementInScope )
        {
            m_Builder.append_comma();
        }

        if( !name.empty() )
        {
            m_Builder.escape_and_append_with_quotes( name );
            m_Builder.append_colon();
        }

        m_Builder.start_array();
        m_FirstElementInScope = true;
    }

    /*************************************************************************\

    Function:
        BeginArray

    \*************************************************************************/
    bool DeviceProfilerJsonBuilder::BeginArray( std::string_view name, const void* pSourceArray )
    {
        if( !m_FirstElementInScope )
        {
            m_Builder.append_comma();
        }

        if( !name.empty() )
        {
            m_Builder.escape_and_append_with_quotes( name );
            m_Builder.append_colon();
        }

        if( pSourceArray )
        {
            m_Builder.start_array();
            m_FirstElementInScope = true;
        }
        else
        {
            m_Builder.append_null();
            m_FirstElementInScope = false;
        }

        return ( pSourceArray != nullptr );
    }

    /*************************************************************************\

    Function:
        EndArray

    \*************************************************************************/
    void DeviceProfilerJsonBuilder::EndArray()
    {
        m_Builder.end_array();
        m_FirstElementInScope = false;
    }

    /*************************************************************************\

    Function:
        BeginObject

    \*************************************************************************/
    void DeviceProfilerJsonBuilder::BeginObject( std::string_view name )
    {
        if( !m_FirstElementInScope )
        {
            m_Builder.append_comma();
        }

        if( !name.empty() )
        {
            m_Builder.escape_and_append_with_quotes( name );
            m_Builder.append_colon();
        }

        m_Builder.start_object();
        m_FirstElementInScope = true;
    }

    /*************************************************************************\

    Function:
        BeginObject

    \*************************************************************************/
    bool DeviceProfilerJsonBuilder::BeginObject( std::string_view name, const void* pSourceObject )
    {
        if( !m_FirstElementInScope )
        {
            m_Builder.append_comma();
        }

        if( !name.empty() )
        {
            m_Builder.escape_and_append_with_quotes( name );
            m_Builder.append_colon();
        }

        if( pSourceObject )
        {
            m_Builder.start_object();
            m_FirstElementInScope = true;
        }
        else
        {
            m_Builder.append_null();
            m_FirstElementInScope = false;
        }

        return ( pSourceObject != nullptr );
    }

    /*************************************************************************\

    Function:
        EndObject

    \*************************************************************************/
    void DeviceProfilerJsonBuilder::EndObject()
    {
        m_Builder.end_object();
        m_FirstElementInScope = false;
    }

    /*************************************************************************\

    Function:
        AddValue

    \*************************************************************************/
    void DeviceProfilerJsonBuilder::EndScope()
    {
        m_FirstElementInScope = true;
    }

    /*************************************************************************\

    Function:
        GetStringBuilder

    \*************************************************************************/
    simdjson::builder::string_builder& DeviceProfilerJsonBuilder::GetStringBuilder()
    {
        return m_Builder;
    }

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
    void DeviceProfilerJsonSerializer::WriteCommandArgs( DeviceProfilerJsonBuilder& builder, const DeviceProfilerDrawcall& drawcall ) const
    {
        builder.BeginObject();

        switch( drawcall.m_Type )
        {
        default:
        case DeviceProfilerDrawcallType::eUnknown:
        case DeviceProfilerDrawcallType::eInsertDebugLabel:
        case DeviceProfilerDrawcallType::eBeginDebugLabel:
        case DeviceProfilerDrawcallType::eEndDebugLabel:
            break;

        case DeviceProfilerDrawcallType::eDraw:
            builder.AddKeyValue( "vertexCount", drawcall.m_Payload.m_Draw.m_VertexCount );
            builder.AddKeyValue( "instanceCount", drawcall.m_Payload.m_Draw.m_InstanceCount );
            builder.AddKeyValue( "firstVertex", drawcall.m_Payload.m_Draw.m_FirstVertex );
            builder.AddKeyValue( "firstInstance", drawcall.m_Payload.m_Draw.m_FirstInstance );
            break;

        case DeviceProfilerDrawcallType::eDrawIndexed:
            builder.AddKeyValue( "indexCount", drawcall.m_Payload.m_DrawIndexed.m_IndexCount );
            builder.AddKeyValue( "instanceCount", drawcall.m_Payload.m_DrawIndexed.m_InstanceCount );
            builder.AddKeyValue( "firstIndex", drawcall.m_Payload.m_DrawIndexed.m_FirstIndex );
            builder.AddKeyValue( "vertexOffset", drawcall.m_Payload.m_DrawIndexed.m_VertexOffset );
            builder.AddKeyValue( "firstInstance", drawcall.m_Payload.m_DrawIndexed.m_FirstInstance );
            break;

        case DeviceProfilerDrawcallType::eDrawIndirect:
            builder.AddKeyValue( "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawIndirect.m_Buffer ) );
            builder.AddKeyValue( "offset", drawcall.m_Payload.m_DrawIndirect.m_Offset );
            builder.AddKeyValue( "drawCount", drawcall.m_Payload.m_DrawIndirect.m_DrawCount );
            builder.AddKeyValue( "stride", drawcall.m_Payload.m_DrawIndirect.m_Stride );
            break;

        case DeviceProfilerDrawcallType::eDrawIndexedIndirect:
            builder.AddKeyValue( "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawIndexedIndirect.m_Buffer ) );
            builder.AddKeyValue( "offset", drawcall.m_Payload.m_DrawIndexedIndirect.m_Offset );
            builder.AddKeyValue( "drawCount", drawcall.m_Payload.m_DrawIndexedIndirect.m_DrawCount );
            builder.AddKeyValue( "stride", drawcall.m_Payload.m_DrawIndexedIndirect.m_Stride );
            break;

        case DeviceProfilerDrawcallType::eDrawIndirectCount:
            builder.AddKeyValue( "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawIndirectCount.m_Buffer ) );
            builder.AddKeyValue( "offset", drawcall.m_Payload.m_DrawIndirectCount.m_Offset );
            builder.AddKeyValue( "countBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawIndirectCount.m_CountBuffer ) );
            builder.AddKeyValue( "countOffset", drawcall.m_Payload.m_DrawIndirectCount.m_CountOffset );
            builder.AddKeyValue( "maxDrawCount", drawcall.m_Payload.m_DrawIndirectCount.m_MaxDrawCount );
            builder.AddKeyValue( "stride", drawcall.m_Payload.m_DrawIndirectCount.m_Stride );
            break;

        case DeviceProfilerDrawcallType::eDrawIndexedIndirectCount:
            builder.AddKeyValue( "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawIndexedIndirectCount.m_Buffer ) );
            builder.AddKeyValue( "offset", drawcall.m_Payload.m_DrawIndexedIndirectCount.m_Offset );
            builder.AddKeyValue( "countBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawIndexedIndirectCount.m_CountBuffer ) );
            builder.AddKeyValue( "countOffset", drawcall.m_Payload.m_DrawIndexedIndirectCount.m_CountOffset );
            builder.AddKeyValue( "maxDrawCount", drawcall.m_Payload.m_DrawIndexedIndirectCount.m_MaxDrawCount );
            builder.AddKeyValue( "stride", drawcall.m_Payload.m_DrawIndexedIndirectCount.m_Stride );
            break;

        case DeviceProfilerDrawcallType::eDrawMeshTasks:
            builder.AddKeyValue( "groupCountX", drawcall.m_Payload.m_DrawMeshTasks.m_GroupCountX );
            builder.AddKeyValue( "groupCountY", drawcall.m_Payload.m_DrawMeshTasks.m_GroupCountY );
            builder.AddKeyValue( "groupCountZ", drawcall.m_Payload.m_DrawMeshTasks.m_GroupCountZ );
            break;

        case DeviceProfilerDrawcallType::eDrawMeshTasksIndirect:
            builder.AddKeyValue( "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawMeshTasksIndirect.m_Buffer ) );
            builder.AddKeyValue( "offset", drawcall.m_Payload.m_DrawMeshTasksIndirect.m_Offset );
            builder.AddKeyValue( "drawCount", drawcall.m_Payload.m_DrawMeshTasksIndirect.m_DrawCount );
            builder.AddKeyValue( "stride", drawcall.m_Payload.m_DrawMeshTasksIndirect.m_Stride );
            break;

        case DeviceProfilerDrawcallType::eDrawMeshTasksIndirectCount:
            builder.AddKeyValue( "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_Buffer ) );
            builder.AddKeyValue( "offset", drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_Offset );
            builder.AddKeyValue( "countBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_CountBuffer ) );
            builder.AddKeyValue( "countOffset", drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_CountOffset );
            builder.AddKeyValue( "maxDrawCount", drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_MaxDrawCount );
            builder.AddKeyValue( "stride", drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_Stride );
            break;

        case DeviceProfilerDrawcallType::eDrawMeshTasksNV:
            builder.AddKeyValue( "taskCount", drawcall.m_Payload.m_DrawMeshTasksNV.m_TaskCount );
            builder.AddKeyValue( "firstTask", drawcall.m_Payload.m_DrawMeshTasksNV.m_FirstTask );
            break;

        case DeviceProfilerDrawcallType::eDrawMeshTasksIndirectNV:
            builder.AddKeyValue( "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawMeshTasksIndirectNV.m_Buffer ) );
            builder.AddKeyValue( "offset", drawcall.m_Payload.m_DrawMeshTasksIndirectNV.m_Offset );
            builder.AddKeyValue( "drawCount", drawcall.m_Payload.m_DrawMeshTasksIndirectNV.m_DrawCount );
            builder.AddKeyValue( "stride", drawcall.m_Payload.m_DrawMeshTasksIndirectNV.m_Stride );
            break;

        case DeviceProfilerDrawcallType::eDrawMeshTasksIndirectCountNV:
            builder.AddKeyValue( "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_Buffer ) );
            builder.AddKeyValue( "offset", drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_Offset );
            builder.AddKeyValue( "countBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_CountBuffer ) );
            builder.AddKeyValue( "countOffset", drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_CountOffset );
            builder.AddKeyValue( "maxDrawCount", drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_MaxDrawCount );
            builder.AddKeyValue( "stride", drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_Stride );
            break;

        case DeviceProfilerDrawcallType::eDrawMulti:
            builder.AddKeyValue( "drawCount", drawcall.m_Payload.m_DrawMulti.m_DrawCount );
            builder.AddKeyValue( "instanceCount", drawcall.m_Payload.m_DrawMulti.m_InstanceCount );
            builder.AddKeyValue( "firstInstance", drawcall.m_Payload.m_DrawMulti.m_FirstInstance );
            builder.AddKeyValue( "stride", drawcall.m_Payload.m_DrawMulti.m_Stride );

            builder.BeginArray( "vertexInfos" );

            for( uint32_t i = 0; i < drawcall.m_Payload.m_DrawMulti.m_DrawCount; i++ )
            {
                const VkMultiDrawInfoEXT& vertexInfo = drawcall.m_Payload.m_DrawMulti.m_pVertexInfo[i];
                builder.BeginObject();
                builder.AddKeyValue( "firstVertex", vertexInfo.firstVertex );
                builder.AddKeyValue( "vertexCount", vertexInfo.vertexCount );
                builder.EndObject();
            }

            builder.EndArray();
            break;
        
        case DeviceProfilerDrawcallType::eDrawMultiIndexed:
            builder.AddKeyValue( "drawCount", drawcall.m_Payload.m_DrawMultiIndexed.m_DrawCount );
            builder.AddKeyValue( "instanceCount", drawcall.m_Payload.m_DrawMultiIndexed.m_InstanceCount );
            builder.AddKeyValue( "firstInstance", drawcall.m_Payload.m_DrawMultiIndexed.m_FirstInstance );
            builder.AddKeyValue( "stride", drawcall.m_Payload.m_DrawMultiIndexed.m_Stride );

            builder.BeginArray( "indexInfos" );

            for( uint32_t i = 0; i < drawcall.m_Payload.m_DrawMultiIndexed.m_DrawCount; i++ )
            {
                const VkMultiDrawIndexedInfoEXT& indexInfo = drawcall.m_Payload.m_DrawMultiIndexed.m_pIndexInfo[i];
                builder.BeginObject();
                builder.AddKeyValue( "firstIndex", indexInfo.firstIndex );
                builder.AddKeyValue( "indexCount", indexInfo.indexCount );
                builder.AddKeyValue( "vertexOffset", indexInfo.vertexOffset );
                builder.EndObject();
            }

            builder.EndArray();
            break;

        case DeviceProfilerDrawcallType::eDispatch:
            builder.AddKeyValue( "groupCountX", drawcall.m_Payload.m_Dispatch.m_GroupCountX );
            builder.AddKeyValue( "groupCountY", drawcall.m_Payload.m_Dispatch.m_GroupCountY );
            builder.AddKeyValue( "groupCountZ", drawcall.m_Payload.m_Dispatch.m_GroupCountZ );
            break;

        case DeviceProfilerDrawcallType::eDispatchIndirect:
            builder.AddKeyValue( "buffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_DispatchIndirect.m_Buffer ) );
            builder.AddKeyValue( "offset", drawcall.m_Payload.m_DispatchIndirect.m_Offset );
            break;

        case DeviceProfilerDrawcallType::eCopyBuffer:
            builder.AddKeyValue( "srcBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyBuffer.m_SrcBuffer ) );
            builder.AddKeyValue( "dstBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyBuffer.m_DstBuffer ) );
            break;

        case DeviceProfilerDrawcallType::eCopyBufferToImage:
            builder.AddKeyValue( "srcBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyBufferToImage.m_SrcBuffer ) );
            builder.AddKeyValue( "dstImage", m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyBufferToImage.m_DstImage ) );
            break;

        case DeviceProfilerDrawcallType::eCopyImage:
            builder.AddKeyValue( "srcImage", m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyImage.m_SrcImage ) );
            builder.AddKeyValue( "dstImage", m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyImage.m_DstImage ) );
            break;

        case DeviceProfilerDrawcallType::eCopyImageToBuffer:
            builder.AddKeyValue( "srcImage", m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyImageToBuffer.m_SrcImage ) );
            builder.AddKeyValue( "dstBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_CopyImageToBuffer.m_DstBuffer ) );
            break;

        case DeviceProfilerDrawcallType::eClearAttachments:
            builder.AddKeyValue( "attachmentCount", drawcall.m_Payload.m_ClearAttachments.m_Count );
            break;

        case DeviceProfilerDrawcallType::eClearColorImage:
            builder.AddKeyValue( "image", m_pStringSerializer->GetName( drawcall.m_Payload.m_ClearColorImage.m_Image ) );
            builder.BeginKeyValue( "value" );
            WriteColorClearValue( builder, drawcall.m_Payload.m_ClearColorImage.m_Value );
            builder.EndKeyValue();
            break;

        case DeviceProfilerDrawcallType::eClearDepthStencilImage:
            builder.AddKeyValue( "image", m_pStringSerializer->GetName( drawcall.m_Payload.m_ClearDepthStencilImage.m_Image ) );
            builder.BeginKeyValue( "value" );
            WriteDepthStencilClearValue( builder, drawcall.m_Payload.m_ClearDepthStencilImage.m_Value );
            builder.EndKeyValue();
            break;

        case DeviceProfilerDrawcallType::eResolveImage:
            builder.AddKeyValue( "srcImage", m_pStringSerializer->GetName( drawcall.m_Payload.m_ResolveImage.m_SrcImage ) );
            builder.AddKeyValue( "dstImage", m_pStringSerializer->GetName( drawcall.m_Payload.m_ResolveImage.m_DstImage ) );
            break;

        case DeviceProfilerDrawcallType::eBlitImage:
            builder.AddKeyValue( "srcImage", m_pStringSerializer->GetName( drawcall.m_Payload.m_BlitImage.m_SrcImage ) );
            builder.AddKeyValue( "dstImage", m_pStringSerializer->GetName( drawcall.m_Payload.m_BlitImage.m_DstImage ) );
            break;

        case DeviceProfilerDrawcallType::eFillBuffer:
            builder.AddKeyValue( "dstBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_FillBuffer.m_Buffer ) );
            builder.AddKeyValue( "dstOffset", drawcall.m_Payload.m_FillBuffer.m_Offset );
            builder.AddKeyValue( "size", drawcall.m_Payload.m_FillBuffer.m_Size );
            builder.AddKeyValue( "data", drawcall.m_Payload.m_FillBuffer.m_Data );
            break;

        case DeviceProfilerDrawcallType::eUpdateBuffer:
            builder.AddKeyValue( "dstBuffer", m_pStringSerializer->GetName( drawcall.m_Payload.m_UpdateBuffer.m_Buffer ) );
            builder.AddKeyValue( "dstOffset", drawcall.m_Payload.m_UpdateBuffer.m_Offset );
            builder.AddKeyValue( "dataSize", drawcall.m_Payload.m_UpdateBuffer.m_Size );
            break;

        case DeviceProfilerDrawcallType::eTraceRaysKHR:
            builder.AddKeyValue( "width", drawcall.m_Payload.m_TraceRays.m_Width );
            builder.AddKeyValue( "height", drawcall.m_Payload.m_TraceRays.m_Height );
            builder.AddKeyValue( "depth", drawcall.m_Payload.m_TraceRays.m_Depth );
            break;

        case DeviceProfilerDrawcallType::eTraceRaysIndirectKHR:
            builder.AddKeyValue( "indirectDeviceAddress", drawcall.m_Payload.m_TraceRaysIndirect.m_IndirectAddress );
            break;

        case DeviceProfilerDrawcallType::eTraceRaysIndirect2KHR:
            builder.AddKeyValue( "indirectDeviceAddress", drawcall.m_Payload.m_TraceRaysIndirect2.m_IndirectAddress );
            break;

        case DeviceProfilerDrawcallType::eBuildAccelerationStructuresKHR:
        case DeviceProfilerDrawcallType::eBuildAccelerationStructuresIndirectKHR:
        {
            const uint32_t infoCount = drawcall.m_Payload.m_BuildAccelerationStructures.m_InfoCount;

            builder.AddKeyValue( "infoCount", infoCount );
            builder.BeginArray( "infos" );

            for( uint32_t i = 0; i < infoCount; ++i )
            {
                const auto& info = drawcall.m_Payload.m_BuildAccelerationStructures.m_pInfos[i];

                builder.BeginObject();
                builder.AddKeyValue( "type", m_pStringSerializer->GetAccelerationStructureTypeName( info.type ) );
                builder.AddKeyValue( "flags", m_pStringSerializer->GetBuildAccelerationStructureFlagNames( info.flags ) );
                builder.AddKeyValue( "mode", m_pStringSerializer->GetBuildAccelerationStructureModeName( info.mode ) );
                builder.AddKeyValue( "src", m_pStringSerializer->GetName( VkAccelerationStructureKHRHandle( info.srcAccelerationStructure ) ) );
                builder.AddKeyValue( "dst", m_pStringSerializer->GetName( VkAccelerationStructureKHRHandle( info.dstAccelerationStructure ) ) );
                builder.AddKeyValue( "geometryCount", info.geometryCount );

                if( builder.BeginArray( "geometries", info.pGeometries ) )
                {
                    for( uint32_t j = 0; j < info.geometryCount; ++j )
                    {
                        const auto& geometry = info.pGeometries[ j ];

                        builder.BeginObject();
                        builder.AddKeyValue( "type", m_pStringSerializer->GetGeometryTypeName( geometry.geometryType ) );
                        builder.AddKeyValue( "flags", m_pStringSerializer->GetGeometryFlagNames( geometry.flags ) );

                        builder.BeginObject( "data" );

                        switch( geometry.geometryType )
                        {
                        case VK_GEOMETRY_TYPE_TRIANGLES_KHR:
                            builder.AddKeyValue( "vertexFormat", m_pStringSerializer->GetFormatName( geometry.geometry.triangles.vertexFormat ) );
                            builder.AddKeyValue( "vertexData", m_pStringSerializer->GetPointer( geometry.geometry.triangles.vertexData.hostAddress ) );
                            builder.AddKeyValue( "vertexStride", geometry.geometry.triangles.vertexStride );
                            builder.AddKeyValue( "maxVertex", geometry.geometry.triangles.maxVertex );
                            builder.AddKeyValue( "indexType", m_pStringSerializer->GetIndexTypeName( geometry.geometry.triangles.indexType ) );
                            builder.AddKeyValue( "indexData", m_pStringSerializer->GetPointer( geometry.geometry.triangles.indexData.hostAddress ) );
                            builder.AddKeyValue( "transformData", m_pStringSerializer->GetPointer( geometry.geometry.triangles.transformData.hostAddress ) );
                            break;

                        case VK_GEOMETRY_TYPE_AABBS_KHR:
                            builder.AddKeyValue( "data", m_pStringSerializer->GetPointer( geometry.geometry.aabbs.data.hostAddress ) );
                            builder.AddKeyValue( "stride", geometry.geometry.aabbs.stride );
                            break;

                        case VK_GEOMETRY_TYPE_INSTANCES_KHR:
                            builder.AddKeyValue( "arrayOfPointers", static_cast<bool>( geometry.geometry.instances.arrayOfPointers ) );
                            builder.AddKeyValue( "data", m_pStringSerializer->GetPointer( geometry.geometry.instances.data.hostAddress ) );
                            break;
                        }

                        builder.EndObject();
                        builder.BeginObject( "range" );

                        if( drawcall.m_Type == DeviceProfilerDrawcallType::eBuildAccelerationStructuresKHR )
                        {
                            const auto& range = drawcall.m_Payload.m_BuildAccelerationStructures.m_ppRanges[ i ][ j ];
                            builder.AddKeyValue( "primitiveCount", range.primitiveCount );
                            builder.AddKeyValue( "primitiveOffset", range.primitiveOffset );
                            builder.AddKeyValue( "firstVertex", range.firstVertex );
                            builder.AddKeyValue( "transformOffset", range.transformOffset );
                        }
                        else //( drawcall.m_Type == DeviceProfilerDrawcallType::eBuildAccelerationStructuresIndirectKHR )
                        {
                            builder.AddKeyValue( "maxPrimitiveCount", drawcall.m_Payload.m_BuildAccelerationStructuresIndirect.m_ppMaxPrimitiveCounts[ i ][ j ] );
                        }

                        builder.EndObject();
                    }

                    builder.EndArray();
                }

                builder.EndObject();
            }

            builder.EndArray();
            break;
        }

        case DeviceProfilerDrawcallType::eBuildMicromapsEXT:
        {
            const uint32_t infoCount = drawcall.m_Payload.m_BuildMicromaps.m_InfoCount;

            builder.AddKeyValue( "infoCount", infoCount );
            builder.BeginArray( "infos" );

            for( uint32_t i = 0; i < infoCount; ++i )
            {
                const auto& info = drawcall.m_Payload.m_BuildMicromaps.m_pInfos[i];

                builder.BeginObject();
                builder.AddKeyValue( "type", m_pStringSerializer->GetMicromapTypeName( info.type ) );
                builder.AddKeyValue( "flags", m_pStringSerializer->GetBuildMicromapFlagNames( info.flags ) );
                builder.AddKeyValue( "mode", m_pStringSerializer->GetBuildMicromapModeName( info.mode ) );
                builder.AddKeyValue( "dst", m_pStringSerializer->GetName( VkMicromapEXTHandle( info.dstMicromap ) ) );
                builder.AddKeyValue( "usageCountsCount", info.usageCountsCount );

                builder.BeginArray( "usageCounts" );

                for( uint32_t j = 0; j < info.usageCountsCount; ++j )
                {
                    const auto& usageCount = info.pUsageCounts[j];

                    builder.BeginObject();
                    builder.AddKeyValue( "count", usageCount.count );
                    builder.AddKeyValue( "format", usageCount.format );
                    builder.AddKeyValue( "subdivisionLevel", usageCount.subdivisionLevel );
                    builder.EndObject();
                }

                builder.EndArray();

                builder.AddKeyValue( "data", m_pStringSerializer->GetPointer( info.data.hostAddress ) );
                builder.AddKeyValue( "scratchData", m_pStringSerializer->GetPointer( info.scratchData.hostAddress ) );
                builder.AddKeyValue( "triangleArray", m_pStringSerializer->GetPointer( info.triangleArray.hostAddress ) );
                builder.AddKeyValue( "triangleArrayStride", info.triangleArrayStride );
                builder.EndObject();
            }

            builder.EndArray();
            break;
        }
        }

        builder.EndObject();
    }

    /*************************************************************************\

    Function:
        GetPipelineArgs

    Description:
        Serialize pipeline state into JSON object.

    \*************************************************************************/
    void DeviceProfilerJsonSerializer::WritePipelineArgs( DeviceProfilerJsonBuilder& builder, const DeviceProfilerPipeline& pipeline ) const
    {
        builder.BeginObject();

        // Append shader stages info.
        if( !pipeline.m_ShaderTuple.m_Shaders.empty() )
        {
            builder.BeginArray( "shaders" );

            for( const ProfilerShader& shader : pipeline.m_ShaderTuple.m_Shaders )
            {
                WriteShaderStageArgs( builder, shader );
            }

            builder.EndArray();
        }

        // Append pipeline create info details.
        if( pipeline.m_pCreateInfo )
        {
            switch( pipeline.m_Type )
            {
            case DeviceProfilerPipelineType::eGraphics:
            {
                WriteGraphicsPipelineCreateInfoArgs( builder,
                    pipeline.m_pCreateInfo->m_GraphicsPipelineCreateInfo );
                break;
            }
            case DeviceProfilerPipelineType::eCompute:
            {
                WriteComputePipelineCreateInfoArgs( builder,
                    pipeline.m_pCreateInfo->m_ComputePipelineCreateInfo );
                break;
            }
            case DeviceProfilerPipelineType::eRayTracingKHR:
            {
                WriteRayTracingPipelineCreateInfoArgs( builder,
                    pipeline.m_pCreateInfo->m_RayTracingPipelineCreateInfoKHR );
                break;
            }
            }
        }

        builder.EndObject();
    }

    /*************************************************************************\

    Function:
        GetColorClearValue

    Description:
        Serialize VkClearColorValue struct into JSON object.

    \*************************************************************************/
    void DeviceProfilerJsonSerializer::WriteColorClearValue( DeviceProfilerJsonBuilder& builder, const VkClearColorValue& value ) const
    {
        builder.BeginObject();
        builder.BeginObject( "VkClearColorValue" );

        builder.BeginArray( "float32" );
        for( int i = 0; i < 4; ++i )
        {
            builder.AddValue( value.float32[i] );
        }
        builder.EndArray();

        builder.BeginArray( "int32" );
        for( int i = 0; i < 4; ++i )
        {
            builder.AddValue( value.int32[i] );
        }
        builder.EndArray();

        builder.BeginArray( "uint32" );
        for( int i = 0; i < 4; ++i )
        {
            builder.AddValue( value.uint32[i] );
        }
        builder.EndArray();

        builder.EndObject();
        builder.EndObject();
    }

    /*************************************************************************\

    Function:
        GetDepthStencilClearValue

    Description:
        Serialize VkClearDepthStencilValue struct into JSON object.

    \*************************************************************************/
    void DeviceProfilerJsonSerializer::WriteDepthStencilClearValue( DeviceProfilerJsonBuilder& builder, const VkClearDepthStencilValue& value ) const
    {
        builder.BeginObject();
        builder.BeginObject( "VkClearDepthStencilValue" );
        builder.AddKeyValue( "depth", value.depth );
        builder.AddKeyValue( "stencil", value.stencil );
        builder.EndObject();
        builder.EndObject();
    }

    /*************************************************************************\

    Function:
        GetShaderStageArgs

    Description:
        Serialize shader stage into JSON object.

    \*************************************************************************/
    void DeviceProfilerJsonSerializer::WriteShaderStageArgs( DeviceProfilerJsonBuilder& builder, const ProfilerShader& shader ) const
    {
        builder.BeginObject();
        builder.AddKeyValue( "stage", m_pStringSerializer->GetShaderStageName( shader.m_Stage ) );
        builder.AddKeyValue( "entryPoint", shader.m_EntryPoint );

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

            builder.AddKeyValue( "shaderIdentifier", shaderIdentifier );
        }

        builder.EndObject();
    }

    /*************************************************************************\

    Function:
        GetGraphicsPipelineCreateInfoArgs

    Description:
        Serialize graphics pipeline state into JSON object.

    \*************************************************************************/
    void DeviceProfilerJsonSerializer::WriteGraphicsPipelineCreateInfoArgs( DeviceProfilerJsonBuilder& builder, const VkGraphicsPipelineCreateInfo& createInfo ) const
    {
        // VkPipelineVertexInputStateCreateInfo
        if( builder.BeginObject( "vertexInputState", createInfo.pVertexInputState ) )
        {
            const auto& state = *createInfo.pVertexInputState;

            builder.AddKeyValue( "attributeCount", state.vertexAttributeDescriptionCount );
            builder.AddKeyValue( "bindingCount", state.vertexBindingDescriptionCount );

            if( builder.BeginArray( "attributes", state.pVertexAttributeDescriptions ) )
            {
                for( uint32_t i = 0; i < state.vertexAttributeDescriptionCount; ++i )
                {
                    const auto& attribute = state.pVertexAttributeDescriptions[i];

                    builder.BeginObject();
                    builder.AddKeyValue( "location", attribute.location );
                    builder.AddKeyValue( "binding", attribute.binding );
                    builder.AddKeyValue( "format", m_pStringSerializer->GetFormatName( attribute.format ) );
                    builder.AddKeyValue( "offset", attribute.offset );
                    builder.EndObject();
                }

                builder.EndArray();
            }

            if( builder.BeginArray( "bindings", state.pVertexBindingDescriptions ) )
            {
                for( uint32_t i = 0; i < state.vertexBindingDescriptionCount; ++i )
                {
                    const auto& binding = state.pVertexBindingDescriptions[i];

                    builder.BeginObject();
                    builder.AddKeyValue( "binding", binding.binding );
                    builder.AddKeyValue( "stride", binding.stride );
                    builder.AddKeyValue( "inputRate", m_pStringSerializer->GetVertexInputRateName( binding.inputRate ) );
                    builder.EndObject();
                }

                builder.EndArray();
            }

            builder.EndObject();
        }

        // VkPipelineInputAssemblyStateCreateInfo
        if( builder.BeginObject( "inputAssemblyState", createInfo.pInputAssemblyState ) )
        {
            const auto& state = *createInfo.pInputAssemblyState;

            builder.AddKeyValue( "topology", m_pStringSerializer->GetPrimitiveTopologyName( state.topology ) );
            builder.AddKeyValue( "primitiveRestartEnable", static_cast<bool>( state.primitiveRestartEnable ) );
            builder.EndObject();
        }

        // VkPipelineTessellationStateCreateInfo
        if( builder.BeginObject( "tessellationState", createInfo.pTessellationState ) )
        {
            const auto& state = *createInfo.pTessellationState;

            builder.AddKeyValue( "patchControlPoints", state.patchControlPoints );
            builder.EndObject();
        }

        // VkPipelineViewportStateCreateInfo
        if( builder.BeginObject( "viewportState", createInfo.pViewportState ) )
        {
            const auto& state = *createInfo.pViewportState;

            builder.AddKeyValue( "viewportCount", state.viewportCount );
            builder.AddKeyValue( "scissorCount", state.scissorCount );

            if( builder.BeginArray( "viewports", state.pViewports ) )
            {
                for( uint32_t i = 0; i < state.viewportCount; ++i )
                {
                    const auto& viewport = state.pViewports[i];

                    builder.BeginObject();
                    builder.AddKeyValue( "x", viewport.x );
                    builder.AddKeyValue( "y", viewport.y );
                    builder.AddKeyValue( "width", viewport.width );
                    builder.AddKeyValue( "height", viewport.height );
                    builder.AddKeyValue( "minDepth", viewport.minDepth );
                    builder.AddKeyValue( "maxDepth", viewport.maxDepth );
                    builder.EndObject();
                }

                builder.EndArray();
            }

            if( builder.BeginArray( "scissors", state.pScissors ) )
            {
                for( uint32_t i = 0; i < state.scissorCount; ++i )
                {
                    const auto& scissor = state.pScissors[i];

                    builder.BeginObject();
                    builder.AddKeyValue( "offsetX", scissor.offset.x );
                    builder.AddKeyValue( "offsetY", scissor.offset.y );
                    builder.AddKeyValue( "extentWidth", scissor.extent.width );
                    builder.AddKeyValue( "extentHeight", scissor.extent.height );
                    builder.EndObject();
                }

                builder.EndArray();
            }

            builder.EndObject();
        }

        // VkPipelineRasterizationStateCreateInfo
        if( builder.BeginObject( "rasterizationState", createInfo.pRasterizationState ) )
        {
            const auto& state = *createInfo.pRasterizationState;

            builder.AddKeyValue( "depthClampEnable", static_cast<bool>( state.depthClampEnable ) );
            builder.AddKeyValue( "rasterizerDiscardEnable", static_cast<bool>( state.rasterizerDiscardEnable ) );
            builder.AddKeyValue( "polygonMode", m_pStringSerializer->GetPolygonModeName( state.polygonMode ) );
            builder.AddKeyValue( "cullMode", m_pStringSerializer->GetCullModeName( state.cullMode ) );
            builder.AddKeyValue( "frontFace", m_pStringSerializer->GetFrontFaceName( state.frontFace ) );
            builder.AddKeyValue( "depthBiasEnable", static_cast<bool>( state.depthBiasEnable ) );
            builder.AddKeyValue( "depthBiasConstantFactor", state.depthBiasConstantFactor );
            builder.AddKeyValue( "depthBiasClamp", state.depthBiasClamp );
            builder.AddKeyValue( "depthBiasSlopeFactor", state.depthBiasSlopeFactor );
            builder.AddKeyValue( "lineWidth", state.lineWidth );
            builder.EndObject();
        }

        // VkPipelineMultisampleStateCreateInfo
        if( builder.BeginObject( "multisampleState", createInfo.pMultisampleState ) )
        {
            const auto& state = *createInfo.pMultisampleState;

            builder.AddKeyValue( "rasterizationSamples", static_cast<uint32_t>( state.rasterizationSamples ) );
            builder.AddKeyValue( "sampleShadingEnable", static_cast<bool>( state.sampleShadingEnable ) );
            builder.AddKeyValue( "minSampleShading", state.minSampleShading );
            builder.AddKeyValue( "sampleMask", fmt::format( "0x{:08X}", state.pSampleMask ? *state.pSampleMask : 0xFFFFFFFF ) );
            builder.AddKeyValue( "alphaToCoverateEnable", static_cast<bool>( state.alphaToCoverageEnable ) );
            builder.AddKeyValue( "alphaToOneEnable", static_cast<bool>( state.alphaToOneEnable ) );
            builder.EndObject();
        }

        // VkPipelineDepthStencilStateCreateInfo
        if( builder.BeginObject( "depthStencilState", createInfo.pDepthStencilState ) )
        {
            const auto& state = *createInfo.pDepthStencilState;

            builder.AddKeyValue( "depthTestEnable", static_cast<bool>( state.depthTestEnable ) );
            builder.AddKeyValue( "depthWriteEnable", static_cast<bool>( state.depthWriteEnable ) );
            builder.AddKeyValue( "depthCompareOp", m_pStringSerializer->GetCompareOpName( state.depthCompareOp ) );
            builder.AddKeyValue( "depthBoundsTestEnable", static_cast<bool>( state.depthBoundsTestEnable ) );
            builder.AddKeyValue( "minDepthBounds", state.minDepthBounds );
            builder.AddKeyValue( "maxDepthBounds", state.maxDepthBounds );
            builder.AddKeyValue( "stencilTestEnable", static_cast<bool>( state.stencilTestEnable ) );

            builder.BeginObject( "front" );
            builder.AddKeyValue( "failOp", static_cast<uint32_t>( state.front.failOp ) );
            builder.AddKeyValue( "passOp", static_cast<uint32_t>( state.front.passOp ) );
            builder.AddKeyValue( "depthFailOp", static_cast<uint32_t>( state.front.depthFailOp ) );
            builder.AddKeyValue( "compareOp", m_pStringSerializer->GetCompareOpName( state.front.compareOp ) );
            builder.AddKeyValue( "compareMask", fmt::format( "0x{:02X}", state.front.compareMask ) );
            builder.AddKeyValue( "writeMask", fmt::format( "0x{:02X}", state.front.writeMask ) );
            builder.AddKeyValue( "reference", fmt::format( "0x{:02X}", state.front.reference ) );
            builder.EndObject();

            builder.BeginObject( "back" );
            builder.AddKeyValue( "failOp", static_cast<uint32_t>( state.back.failOp ) );
            builder.AddKeyValue( "passOp", static_cast<uint32_t>( state.back.passOp ) );
            builder.AddKeyValue( "depthFailOp", static_cast<uint32_t>( state.back.depthFailOp ) );
            builder.AddKeyValue( "compareOp", m_pStringSerializer->GetCompareOpName( state.back.compareOp ) );
            builder.AddKeyValue( "compareMask", fmt::format( "0x{:02X}", state.back.compareMask ) );
            builder.AddKeyValue( "writeMask", fmt::format( "0x{:02X}", state.back.writeMask ) );
            builder.AddKeyValue( "reference", fmt::format( "0x{:02X}", state.back.reference ) );
            builder.EndObject();

            builder.EndObject();
        }

        // VkPipelineColorBlendStateCreateInfo
        if( builder.BeginObject( "colorBlendState", createInfo.pColorBlendState ) )
        {
            const auto& state = *createInfo.pColorBlendState;

            builder.AddKeyValue( "logicOpEnable", static_cast<bool>( state.logicOpEnable ) );
            builder.AddKeyValue( "logicOp", m_pStringSerializer->GetLogicOpName( state.logicOp ) );

            builder.BeginArray( "blendConstants" );
            for( int i = 0; i < 4; ++i )
            {
                builder.AddValue( state.blendConstants[i] );
            }
            builder.EndArray();

            if( builder.BeginArray( "attachments", state.pAttachments ) )
            {
                for( uint32_t i = 0; i < state.attachmentCount; ++i )
                {
                    const auto& attachment = state.pAttachments[i];

                    builder.BeginObject();
                    builder.AddKeyValue( "blendEnable", static_cast<bool>( attachment.blendEnable ) );
                    builder.AddKeyValue( "srcColorBlendFactor", m_pStringSerializer->GetBlendFactorName( attachment.srcColorBlendFactor ) );
                    builder.AddKeyValue( "dstColorBlendFactor", m_pStringSerializer->GetBlendFactorName( attachment.dstColorBlendFactor ) );
                    builder.AddKeyValue( "colorBlendOp", m_pStringSerializer->GetBlendOpName( attachment.colorBlendOp ) );
                    builder.AddKeyValue( "srcAlphaBlendFactor", m_pStringSerializer->GetBlendFactorName( attachment.srcAlphaBlendFactor ) );
                    builder.AddKeyValue( "dstAlphaBlendFactor", m_pStringSerializer->GetBlendFactorName( attachment.dstAlphaBlendFactor ) );
                    builder.AddKeyValue( "alphaBlendOp", m_pStringSerializer->GetBlendOpName( attachment.alphaBlendOp ) );
                    builder.AddKeyValue( "colorWriteMask", m_pStringSerializer->GetColorComponentFlagNames( attachment.colorWriteMask ) );
                    builder.EndObject();
                }

                builder.EndArray();
            }

            builder.EndObject();
        }

        // VkPipelineDynamicStateCreateInfo
        if( builder.BeginArray( "dynamicStates", createInfo.pDynamicState ) )
        {
            const auto& state = *createInfo.pDynamicState;
            for( uint32_t i = 0; i < state.dynamicStateCount; ++i )
            {
                builder.AddValue(
                    m_pStringSerializer->GetDynamicStateName( state.pDynamicStates[i] ) );
            }

            builder.EndArray();
        }
    }

    /*************************************************************************\

    Function:
        GetComputePipelineCreateInfoArgs

    Description:
        Serialize compute pipeline state into JSON object.

    \*************************************************************************/
    void DeviceProfilerJsonSerializer::WriteComputePipelineCreateInfoArgs( DeviceProfilerJsonBuilder& builder, const VkComputePipelineCreateInfo& createInfo ) const
    {
        // No additional state to serialize for compute pipelines yet.
    }

    /*************************************************************************\

    Function:
        GetRayTracingPipelineCreateInfoArgs

    Description:
        Serialize ray-tracing pipeline state into JSON object.

    \*************************************************************************/
    void DeviceProfilerJsonSerializer::WriteRayTracingPipelineCreateInfoArgs( DeviceProfilerJsonBuilder& builder, const VkRayTracingPipelineCreateInfoKHR& createInfo ) const
    {
        builder.AddKeyValue( "maxPipelineRayRecursionDepth", createInfo.maxPipelineRayRecursionDepth );
        builder.AddKeyValue( "libraryInterface", createInfo.pLibraryInterface );

        // VkRayTracingPipelineInterfaceCreateInfoKHR
        if( builder.BeginObject( "libraryInterface", createInfo.pLibraryInterface ) )
        {
            const auto& state = *createInfo.pLibraryInterface;

            builder.AddKeyValue( "maxPipelineRayPayloadSize", state.maxPipelineRayPayloadSize );
            builder.AddKeyValue( "maxPipelineRayHitAttributeSize", state.maxPipelineRayHitAttributeSize );
            builder.EndObject();
        }

        // VkPipelineDynamicStateCreateInfo
        if( builder.BeginArray( "dynamicStates", createInfo.pDynamicState ) )
        {
            const auto& state = *createInfo.pDynamicState;
            for( uint32_t i = 0; i < state.dynamicStateCount; ++i )
            {
                builder.AddValue(
                    m_pStringSerializer->GetDynamicStateName( state.pDynamicStates[i] ) );
            }

            builder.EndArray();
        }
    }
}

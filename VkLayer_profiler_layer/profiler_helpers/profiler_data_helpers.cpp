// Copyright (c) 2020 Lukasz Stalmirski
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

#include "profiler_data_helpers.h"
#include "profiler/profiler_data.h"
#include "profiler/profiler_helpers.h"
#include "profiler_layer_objects/VkDevice_object.h"
#include <fmt/format.h>

namespace Profiler
{
    /***********************************************************************************\

    Function:
        DeviceProfilerStringSerializer

    Description:
        Constructor.

    \***********************************************************************************/
    DeviceProfilerStringSerializer::DeviceProfilerStringSerializer( const VkDevice_Object& device )
        : m_Device( device )
    {
    }

    /***********************************************************************************\

    Function:
        GetName

    Description:
        Returns name of the drawcall.

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetName( const DeviceProfilerDrawcall& drawcall ) const
    {
        switch( drawcall.m_Type )
        {
        default:
        case DeviceProfilerDrawcallType::eUnknown:
            return fmt::format( "Unknown command ({})", drawcall.m_Type );

        case DeviceProfilerDrawcallType::eInsertDebugLabel:
        case DeviceProfilerDrawcallType::eBeginDebugLabel:
            return drawcall.m_Payload.m_DebugLabel.m_pName;

        case DeviceProfilerDrawcallType::eEndDebugLabel:
            return "";

        case DeviceProfilerDrawcallType::eDraw:
            return fmt::format( "vkCmdDraw ({}, {}, {}, {})",
                drawcall.m_Payload.m_Draw.m_VertexCount,
                drawcall.m_Payload.m_Draw.m_InstanceCount,
                drawcall.m_Payload.m_Draw.m_FirstVertex,
                drawcall.m_Payload.m_Draw.m_FirstInstance );

        case DeviceProfilerDrawcallType::eDrawIndexed:
            return fmt::format( "vkCmdDrawIndexed ({}, {}, {}, {}, {})",
                drawcall.m_Payload.m_DrawIndexed.m_IndexCount,
                drawcall.m_Payload.m_DrawIndexed.m_InstanceCount,
                drawcall.m_Payload.m_DrawIndexed.m_FirstIndex,
                drawcall.m_Payload.m_DrawIndexed.m_VertexOffset,
                drawcall.m_Payload.m_DrawIndexed.m_FirstInstance );

        case DeviceProfilerDrawcallType::eDrawIndirect:
            return fmt::format( "vkCmdDrawIndirect ({}, {}, {}, {})",
                GetName( drawcall.m_Payload.m_DrawIndirect.m_Buffer ),
                drawcall.m_Payload.m_DrawIndirect.m_Offset,
                drawcall.m_Payload.m_DrawIndirect.m_DrawCount,
                drawcall.m_Payload.m_DrawIndirect.m_Stride );

        case DeviceProfilerDrawcallType::eDrawIndexedIndirect:
            return fmt::format( "vkCmdDrawIndexedIndirect ({}, {}, {}, {})",
                GetName( drawcall.m_Payload.m_DrawIndexedIndirect.m_Buffer ),
                drawcall.m_Payload.m_DrawIndexedIndirect.m_Offset,
                drawcall.m_Payload.m_DrawIndexedIndirect.m_DrawCount,
                drawcall.m_Payload.m_DrawIndexedIndirect.m_Stride );

        case DeviceProfilerDrawcallType::eDrawIndirectCount:
            return fmt::format( "vkCmdDrawIndirectCount ({}, {}, {}, {}, {}, {})",
                GetName( drawcall.m_Payload.m_DrawIndirectCount.m_Buffer ),
                drawcall.m_Payload.m_DrawIndirectCount.m_Offset,
                GetName( drawcall.m_Payload.m_DrawIndirectCount.m_CountBuffer ),
                drawcall.m_Payload.m_DrawIndirectCount.m_CountOffset,
                drawcall.m_Payload.m_DrawIndirectCount.m_MaxDrawCount,
                drawcall.m_Payload.m_DrawIndirectCount.m_Stride );

        case DeviceProfilerDrawcallType::eDrawIndexedIndirectCount:
            return fmt::format( "vkCmdDrawIndexedIndirectCount ({}, {}, {}, {}, {}, {})",
                GetName( drawcall.m_Payload.m_DrawIndexedIndirectCount.m_Buffer ),
                drawcall.m_Payload.m_DrawIndexedIndirectCount.m_Offset,
                GetName( drawcall.m_Payload.m_DrawIndexedIndirectCount.m_CountBuffer ),
                drawcall.m_Payload.m_DrawIndexedIndirectCount.m_CountOffset,
                drawcall.m_Payload.m_DrawIndexedIndirectCount.m_MaxDrawCount,
                drawcall.m_Payload.m_DrawIndexedIndirectCount.m_Stride );

        case DeviceProfilerDrawcallType::eDispatch:
            return fmt::format( "vkCmdDispatch ({}, {}, {})",
                drawcall.m_Payload.m_Dispatch.m_GroupCountX,
                drawcall.m_Payload.m_Dispatch.m_GroupCountY,
                drawcall.m_Payload.m_Dispatch.m_GroupCountZ );

        case DeviceProfilerDrawcallType::eDispatchIndirect:
            return fmt::format( "vkCmdDispatchIndirect ({}, {})",
                GetName( drawcall.m_Payload.m_DispatchIndirect.m_Buffer ),
                drawcall.m_Payload.m_DispatchIndirect.m_Offset );
            
        case DeviceProfilerDrawcallType::eCopyBuffer:
            return fmt::format( "vkCmdCopyBuffer ({}, {})",
                GetName( drawcall.m_Payload.m_CopyBuffer.m_SrcBuffer ),
                GetName( drawcall.m_Payload.m_CopyBuffer.m_DstBuffer ) );

        case DeviceProfilerDrawcallType::eCopyBufferToImage:
            return fmt::format( "vkCmdCopyBufferToImage ({}, {})",
                GetName( drawcall.m_Payload.m_CopyBufferToImage.m_SrcBuffer ),
                GetName( drawcall.m_Payload.m_CopyBufferToImage.m_DstImage ) );

        case DeviceProfilerDrawcallType::eCopyImage:
            return fmt::format( "vkCmdCopyImage ({}, {})",
                GetName( drawcall.m_Payload.m_CopyImage.m_SrcImage ),
                GetName( drawcall.m_Payload.m_CopyImage.m_DstImage ) );

        case DeviceProfilerDrawcallType::eCopyImageToBuffer:
            return fmt::format( "vkCmdCopyImageToBuffer ({}, {})",
                GetName( drawcall.m_Payload.m_CopyImageToBuffer.m_SrcImage ),
                GetName( drawcall.m_Payload.m_CopyImageToBuffer.m_DstBuffer ) );

        case DeviceProfilerDrawcallType::eClearAttachments:
            return fmt::format( "vkCmdClearAttachments ({})",
                GetName( drawcall.m_Payload.m_ClearAttachments.m_Count ) );

        case DeviceProfilerDrawcallType::eClearColorImage:
            return fmt::format( "vkCmdClearColorImage ({}, C=[{}, {}, {}, {}])",
                GetName( drawcall.m_Payload.m_ClearColorImage.m_Image ),
                drawcall.m_Payload.m_ClearColorImage.m_Value.float32[ 0 ],
                drawcall.m_Payload.m_ClearColorImage.m_Value.float32[ 1 ],
                drawcall.m_Payload.m_ClearColorImage.m_Value.float32[ 2 ],
                drawcall.m_Payload.m_ClearColorImage.m_Value.float32[ 3 ] );

        case DeviceProfilerDrawcallType::eClearDepthStencilImage:
            return fmt::format( "vkCmdClearDepthStencilImage ({}, D={}, S={})",
                GetName( drawcall.m_Payload.m_ClearDepthStencilImage.m_Image ),
                drawcall.m_Payload.m_ClearDepthStencilImage.m_Value.depth,
                drawcall.m_Payload.m_ClearDepthStencilImage.m_Value.stencil );

        case DeviceProfilerDrawcallType::eResolveImage:
            return fmt::format( "vkCmdResolveImage ({}, {})",
                GetName( drawcall.m_Payload.m_ResolveImage.m_SrcImage ),
                GetName( drawcall.m_Payload.m_ResolveImage.m_DstImage ) );

        case DeviceProfilerDrawcallType::eBlitImage:
            return fmt::format( "vkCmdBlitImage ({}, {})",
                GetName( drawcall.m_Payload.m_BlitImage.m_SrcImage ),
                GetName( drawcall.m_Payload.m_BlitImage.m_DstImage ) );

        case DeviceProfilerDrawcallType::eFillBuffer:
            return fmt::format( "vkCmdFillBuffer ({}, {}, {}, {})",
                GetName( drawcall.m_Payload.m_FillBuffer.m_Buffer ),
                drawcall.m_Payload.m_FillBuffer.m_Offset,
                drawcall.m_Payload.m_FillBuffer.m_Size,
                drawcall.m_Payload.m_FillBuffer.m_Data );

        case DeviceProfilerDrawcallType::eUpdateBuffer:
            return fmt::format( "vkCmdUpdateBuffer ({}, {}, {}, {})",
                GetName( drawcall.m_Payload.m_UpdateBuffer.m_Buffer ),
                drawcall.m_Payload.m_UpdateBuffer.m_Offset,
                drawcall.m_Payload.m_UpdateBuffer.m_Size );
        }
    }

    /***********************************************************************************\

    Function:
        GetName

    Description:
        Returns name of the pipeline.

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetName( const DeviceProfilerPipelineData& pipeline ) const
    {
        return GetName( pipeline.m_Handle );
    }

    /***********************************************************************************\

    Function:
        GetName

    Description:
        Returns name of the subpass.

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetName( const DeviceProfilerSubpassData& subpass ) const
    {
        return fmt::format( "Subpass {}", subpass.m_Index );
    }

    /***********************************************************************************\

    Function:
        GetName

    Description:
        Returns name of the render pass.

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetName( const DeviceProfilerRenderPassData& renderPass ) const
    {
        return GetName( renderPass.m_Handle );
    }

    /***********************************************************************************\

    Function:
        GetName

    Description:
        Returns name of the command buffer.

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetName( const DeviceProfilerCommandBufferData& commandBuffer ) const
    {
        return GetName( commandBuffer.m_Handle );
    }

    /***********************************************************************************\

    Function:
        GetName

    Description:
        Returns name of the Vulkan API object.

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetName( const VkObject& object ) const
    {
        auto it = m_Device.Debug.ObjectNames.find( object );
        if( it != m_Device.Debug.ObjectNames.end() )
        {
            return it->second;
        }

        return fmt::format( "{} {:#018x}", object.m_pTypeName, object.m_Handle );
    }

    /***********************************************************************************\

    Function:
        GetCommandName

    Description:
        Returns name of the Vulkan API function.

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetCommandName( const DeviceProfilerDrawcall& drawcall ) const
    {
        switch( drawcall.m_Type )
        {
        default:
        case DeviceProfilerDrawcallType::eUnknown:
            return fmt::format( "Unknown command ({})", drawcall.m_Type );

        case DeviceProfilerDrawcallType::eInsertDebugLabel:
            return "vkCmdInsertDebugLabelEXT";

        case DeviceProfilerDrawcallType::eBeginDebugLabel:
            return "vkCmdBeginDebugLabelEXT";

        case DeviceProfilerDrawcallType::eEndDebugLabel:
            return "vkCmdEndDebugLabelEXT";

        case DeviceProfilerDrawcallType::eDraw:
            return "vkCmdDraw";

        case DeviceProfilerDrawcallType::eDrawIndexed:
            return "vkCmdDrawIndexed";

        case DeviceProfilerDrawcallType::eDrawIndirect:
            return "vkCmdDrawIndirect";

        case DeviceProfilerDrawcallType::eDrawIndexedIndirect:
            return "vkCmdDrawIndexedIndirect";

        case DeviceProfilerDrawcallType::eDrawIndirectCount:
            return "vkCmdDrawIndirectCount";

        case DeviceProfilerDrawcallType::eDrawIndexedIndirectCount:
            return "vkCmdDrawIndexedIndirectCount";

        case DeviceProfilerDrawcallType::eDispatch:
            return "vkCmdDispatch";

        case DeviceProfilerDrawcallType::eDispatchIndirect:
            return "vkCmdDispatchIndirect";

        case DeviceProfilerDrawcallType::eCopyBuffer:
            return "vkCmdCopyBuffer";

        case DeviceProfilerDrawcallType::eCopyBufferToImage:
            return "vkCmdCopyBufferToImage";

        case DeviceProfilerDrawcallType::eCopyImage:
            return "vkCmdCopyImage";

        case DeviceProfilerDrawcallType::eCopyImageToBuffer:
            return "vkCmdCopyImageToBuffer";

        case DeviceProfilerDrawcallType::eClearAttachments:
            return "vkCmdClearAttachments";

        case DeviceProfilerDrawcallType::eClearColorImage:
            return "vkCmdClearColorImage";

        case DeviceProfilerDrawcallType::eClearDepthStencilImage:
            return "vkCmdClearDepthStencilImage";

        case DeviceProfilerDrawcallType::eResolveImage:
            return "vkCmdResolveImage";

        case DeviceProfilerDrawcallType::eBlitImage:
            return "vkCmdBlitImage";

        case DeviceProfilerDrawcallType::eFillBuffer:
            return "vkCmdFillBuffer";

        case DeviceProfilerDrawcallType::eUpdateBuffer:
            return "vkCmdUpdateBuffer";
        }
    }

    /***********************************************************************************\

    Function:
        GetColorHex

    Description:
        Returns hexadecimal 24-bit color representation (in #RRGGBB format).

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetColorHex( const float* pColor ) const
    {
        const uint8_t R = static_cast<uint8_t>(pColor[ 0 ] * 255.f);
        const uint8_t G = static_cast<uint8_t>(pColor[ 1 ] * 255.f);
        const uint8_t B = static_cast<uint8_t>(pColor[ 2 ] * 255.f);

        char color[ 8 ] = "#XXXXXX";
        u8tohex( color + 1, R );
        u8tohex( color + 3, G );
        u8tohex( color + 5, B );

        return color;
    }
}

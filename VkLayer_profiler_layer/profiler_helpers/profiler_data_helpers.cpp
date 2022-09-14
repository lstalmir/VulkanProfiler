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

#include "profiler_data_helpers.h"
#include "profiler/profiler_data.h"
#include "profiler/profiler_helpers.h"
#include "profiler_layer_objects/VkDevice_object.h"
#include <fmt/format.h>
#include <sstream>

namespace Profiler
{
    struct FlagsStringBuilder
    {
        std::stringstream m_Stream;
        bool m_Empty = true;

        void AddFlag(std::string&& flag)
        {
            if (!m_Empty)
                m_Stream << " | ";
            m_Stream << flag;
            m_Empty = false;
        }

        std::string BuildString()
        {
            return m_Stream.str();
        }
    };

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
            return fmt::format( "vkCmdUpdateBuffer ({}, {}, {})",
                GetName( drawcall.m_Payload.m_UpdateBuffer.m_Buffer ),
                drawcall.m_Payload.m_UpdateBuffer.m_Offset,
                drawcall.m_Payload.m_UpdateBuffer.m_Size );

        case DeviceProfilerDrawcallType::eTraceRaysKHR:
            return fmt::format( "vkCmdTraceRaysKHR ({}, {}, {})",
                drawcall.m_Payload.m_TraceRays.m_Width,
                drawcall.m_Payload.m_TraceRays.m_Height,
                drawcall.m_Payload.m_TraceRays.m_Depth );

        case DeviceProfilerDrawcallType::eTraceRaysIndirectKHR:
            return fmt::format( "vkCmdTraceRaysIndirectKHR ({})",
                drawcall.m_Payload.m_TraceRaysIndirect.m_IndirectAddress );

        case DeviceProfilerDrawcallType::eBuildAccelerationStructuresKHR:
            return fmt::format("vkCmdBuildAccelerationStructuresKHR ({})",
                drawcall.m_Payload.m_BuildAccelerationStructures.m_InfoCount);

        case DeviceProfilerDrawcallType::eBuildAccelerationStructuresIndirectKHR:
            return fmt::format("vkCmdBuildAccelerationStructuresIndirectKHR ({})",
                drawcall.m_Payload.m_BuildAccelerationStructures.m_InfoCount);

        case DeviceProfilerDrawcallType::eCopyAccelerationStructureKHR:
            return fmt::format("vkCmdCopyAccelerationStructureKHR ({}, {}, {})",
                GetName(drawcall.m_Payload.m_CopyAccelerationStructure.m_Src),
                GetName(drawcall.m_Payload.m_CopyAccelerationStructure.m_Dst),
                GetCopyAccelerationStructureModeName(drawcall.m_Payload.m_CopyAccelerationStructure.m_Mode));

        case DeviceProfilerDrawcallType::eCopyAccelerationStructureToMemoryKHR:
            return fmt::format("vkCmdCopyAccelerationStructureToMemoryKHR ({}, {}, {})",
                GetName(drawcall.m_Payload.m_CopyAccelerationStructureToMemory.m_Src),
                drawcall.m_Payload.m_CopyAccelerationStructureToMemory.m_Dst.hostAddress,
                GetCopyAccelerationStructureModeName(drawcall.m_Payload.m_CopyAccelerationStructure.m_Mode));

        case DeviceProfilerDrawcallType::eCopyMemoryToAccelerationStructureKHR:
            return fmt::format("vkCmdCopyMemoryToAccelerationStructureKHR ({}, {}, {})",
                drawcall.m_Payload.m_CopyMemoryToAccelerationStructure.m_Src.hostAddress,
                GetName(drawcall.m_Payload.m_CopyMemoryToAccelerationStructure.m_Dst),
                GetCopyAccelerationStructureModeName(drawcall.m_Payload.m_CopyAccelerationStructure.m_Mode));
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
        if( renderPass.m_Handle != VK_NULL_HANDLE )
        {
            return GetName( renderPass.m_Handle );
        }

        switch( renderPass.m_Type )
        {
        case DeviceProfilerRenderPassType::eGraphics:
            return "Graphics Pass";
        case DeviceProfilerRenderPassType::eCompute:
            return "Compute Pass";
        case DeviceProfilerRenderPassType::eRayTracing:
            return "Ray Tracing Pass";
        case DeviceProfilerRenderPassType::eCopy:
            return "Copy Pass";
        }

        return "Unknown Pass";
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
        std::string objectName;

        if( m_Device.Debug.ObjectNames.find( object, &objectName ) )
        {
            return objectName;
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

        case DeviceProfilerDrawcallType::eTraceRaysKHR:
            return "vkCmdTraceRaysKHR";

        case DeviceProfilerDrawcallType::eTraceRaysIndirectKHR:
            return "vkCmdTraceRaysIndirectKHR";

        case DeviceProfilerDrawcallType::eBuildAccelerationStructuresKHR:
            return "vkCmdBuildAccelerationStructuresKHR";

        case DeviceProfilerDrawcallType::eBuildAccelerationStructuresIndirectKHR:
            return "vkCmdBuildAccelerationStructuresIndirectKHR";

        case DeviceProfilerDrawcallType::eCopyAccelerationStructureKHR:
            return "vkCmdCopyAccelerationStructureKHR";

        case DeviceProfilerDrawcallType::eCopyAccelerationStructureToMemoryKHR:
            return "vkCmdCopyAccelerationStructureToMemoryKHR";

        case DeviceProfilerDrawcallType::eCopyMemoryToAccelerationStructureKHR:
            return "vkCmdCopyMemoryToAccelerationStructureKHR";
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
        const uint8_t R = static_cast<uint8_t>( pColor[ 0 ] * 255.f );
        const uint8_t G = static_cast<uint8_t>( pColor[ 1 ] * 255.f );
        const uint8_t B = static_cast<uint8_t>( pColor[ 2 ] * 255.f );

        char color[ 8 ] = "#XXXXXX";
        u8tohex( color + 1, R );
        u8tohex( color + 3, G );
        u8tohex( color + 5, B );

        return color;
    }

    /***********************************************************************************\

    Function:
        GetCopyAccelerationStructureModeName

    Description:

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetCopyAccelerationStructureModeName(
        VkCopyAccelerationStructureModeKHR mode) const
    {
        switch (mode)
        {
        case VK_COPY_ACCELERATION_STRUCTURE_MODE_CLONE_KHR:
            return "Clone";
        case VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR:
            return "Compact";
        case VK_COPY_ACCELERATION_STRUCTURE_MODE_SERIALIZE_KHR:
            return "Serialize";
        case VK_COPY_ACCELERATION_STRUCTURE_MODE_DESERIALIZE_KHR:
            return "Deserialize";
        }
        return fmt::format("Unknown mode ({})", mode);
    }

    /***********************************************************************************\

    Function:
        GetAccelerationStructureTypeName

    Description:

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetAccelerationStructureTypeName(
        VkAccelerationStructureTypeKHR type) const
    {
        switch (type)
        {
        case VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR:
            return "Top-level";
        case VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR:
            return "Bottom-level";
        case VK_ACCELERATION_STRUCTURE_TYPE_GENERIC_KHR:
            return "Generic";
        }
        return fmt::format("Unknown type ({})", type);
    }

    /***********************************************************************************\

    Function:
        GetBuildAccelerationStructureFlagNames

    Description:

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetBuildAccelerationStructureFlagNames(
        VkBuildAccelerationStructureFlagsKHR flags) const
    {
        FlagsStringBuilder builder;

        if (flags & VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR)
            builder.AddFlag("Allow update (1)");
        if (flags & VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR)
            builder.AddFlag("Allow compaction (2)");
        if (flags & VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR)
            builder.AddFlag("Prefer fast trace (4)");
        if (flags & VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR)
            builder.AddFlag("Prefer fast build (8)");
        if (flags & VK_BUILD_ACCELERATION_STRUCTURE_LOW_MEMORY_BIT_KHR)
            builder.AddFlag("Low memory (16)");
        if (flags & VK_BUILD_ACCELERATION_STRUCTURE_MOTION_BIT_NV)
            builder.AddFlag("Motion (32)");

        for (uint32_t i = 6; i < 8 * sizeof(flags); ++i)
        {
            uint32_t unkownFlag = 1U << i;
            if (flags & unkownFlag)
                builder.AddFlag(fmt::format("Unknown flag ({})", unkownFlag));
        }

        return builder.BuildString();
    }

    /***********************************************************************************\

    Function:
        GetBuildAccelerationStructureModeName

    Description:

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetBuildAccelerationStructureModeName(
        VkBuildAccelerationStructureModeKHR mode) const
    {
        switch (mode)
        {
        case VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR:
            return "Build";
        case VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR:
            return "Update";
        }
        return fmt::format("Unknown mode ({})", mode);
    }
}

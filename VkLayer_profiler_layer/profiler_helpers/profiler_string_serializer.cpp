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

#include "profiler_string_serializer.h"
#include "profiler/profiler_data.h"
#include "profiler/profiler_frontend.h"
#include "profiler/profiler_helpers.h"
#include "profiler_layer_objects/VkDevice_object.h"
#include <fmt/format.h>
#include <sstream>

namespace Profiler
{
    struct FlagsStringBuilder
    {
        std::stringstream m_Stream;
        bool              m_Empty = true;
        const char*       m_pSeparator = DeviceProfilerStringSerializer::DefaultFlagsSeparator;

        void AddFlag( const std::string_view& flag )
        {
            if( !m_Empty )
                m_Stream << m_pSeparator;
            m_Stream << flag;
            m_Empty = false;
        }

        void AddUnknownFlags( uint64_t flags, uint32_t startIndex )
        {
            for( uint32_t i = startIndex; i < sizeof( uint64_t ) * 8; ++i )
            {
                uint64_t unkownFlag = ( 1ULL << i );
                if( flags & unkownFlag )
                {
                    if( !m_Empty )
                        m_Stream << m_pSeparator;
                    m_Stream << "Unknown flag (" << unkownFlag << ")";
                    m_Empty = false;
                }
            }
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
    DeviceProfilerStringSerializer::DeviceProfilerStringSerializer( DeviceProfilerFrontend& frontend )
        : m_Frontend( frontend )
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
            return fmt::format( "Unknown command ({})", static_cast<uint32_t>( drawcall.m_Type ) );

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

        case DeviceProfilerDrawcallType::eDrawMeshTasks:
            return fmt::format( "vkCmdDrawMeshTasksEXT ({}, {}, {})",
                drawcall.m_Payload.m_DrawMeshTasks.m_GroupCountX,
                drawcall.m_Payload.m_DrawMeshTasks.m_GroupCountY,
                drawcall.m_Payload.m_DrawMeshTasks.m_GroupCountZ );

        case DeviceProfilerDrawcallType::eDrawMeshTasksIndirect:
            return fmt::format( "vkCmdDrawMeshTasksIndirectEXT ({}, {}, {}, {})",
                GetName( drawcall.m_Payload.m_DrawMeshTasksIndirect.m_Buffer ),
                drawcall.m_Payload.m_DrawMeshTasksIndirect.m_Offset,
                drawcall.m_Payload.m_DrawMeshTasksIndirect.m_DrawCount,
                drawcall.m_Payload.m_DrawMeshTasksIndirect.m_Stride );

        case DeviceProfilerDrawcallType::eDrawMeshTasksIndirectCount:
            return fmt::format( "vkCmdDrawMeshTasksIndirectCountEXT ({}, {}, {}, {}, {}, {})",
                GetName( drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_Buffer ),
                drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_Offset,
                GetName( drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_CountBuffer ),
                drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_CountOffset,
                drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_MaxDrawCount,
                drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_Stride );

        case DeviceProfilerDrawcallType::eDrawMeshTasksNV:
            return fmt::format( "vkCmdDrawMeshTasksNV ({}, {})",
                drawcall.m_Payload.m_DrawMeshTasksNV.m_TaskCount,
                drawcall.m_Payload.m_DrawMeshTasksNV.m_FirstTask );

        case DeviceProfilerDrawcallType::eDrawMeshTasksIndirectNV:
            return fmt::format( "vkCmdDrawMeshTasksIndirectNV ({}, {}, {}, {})",
                GetName( drawcall.m_Payload.m_DrawMeshTasksIndirectNV.m_Buffer ),
                drawcall.m_Payload.m_DrawMeshTasksIndirectNV.m_Offset,
                drawcall.m_Payload.m_DrawMeshTasksIndirectNV.m_DrawCount,
                drawcall.m_Payload.m_DrawMeshTasksIndirectNV.m_Stride );

        case DeviceProfilerDrawcallType::eDrawMeshTasksIndirectCountNV:
            return fmt::format( "vkCmdDrawMeshTasksIndirectCountNV ({}, {}, {}, {}, {}, {})",
                GetName( drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_Buffer ),
                drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_Offset,
                GetName( drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_CountBuffer ),
                drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_CountOffset,
                drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_MaxDrawCount,
                drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_Stride );

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
            return fmt::format( "vkCmdTraceRaysIndirectKHR (0x{:16x})",
                drawcall.m_Payload.m_TraceRaysIndirect.m_IndirectAddress );

        case DeviceProfilerDrawcallType::eTraceRaysIndirect2KHR:
            return fmt::format( "vkCmdTraceRaysIndirect2KHR (0x{:16x})",
                drawcall.m_Payload.m_TraceRaysIndirect2.m_IndirectAddress );

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

        case DeviceProfilerDrawcallType::eBuildMicromapsEXT:
            return fmt::format( "vkCmdBuildMicromapsEXT ({})",
                drawcall.m_Payload.m_BuildMicromaps.m_InfoCount );

        case DeviceProfilerDrawcallType::eCopyMicromapEXT:
            return fmt::format( "vkCmdCopyMicromapEXT ({}, {}, {})",
                GetName( drawcall.m_Payload.m_CopyMicromap.m_Src ),
                GetName( drawcall.m_Payload.m_CopyMicromap.m_Dst ),
                GetCopyMicromapModeName( drawcall.m_Payload.m_CopyMicromap.m_Mode ) );

        case DeviceProfilerDrawcallType::eCopyMemoryToMicromapEXT:
            return fmt::format( "vkCmdCopyMemoryToMicromapEXT ({}, {}, {})",
                drawcall.m_Payload.m_CopyMemoryToMicromap.m_Src.hostAddress,
                GetName( drawcall.m_Payload.m_CopyMemoryToMicromap.m_Dst ),
                GetCopyMicromapModeName( drawcall.m_Payload.m_CopyMemoryToMicromap.m_Mode ) );

        case DeviceProfilerDrawcallType::eCopyMicromapToMemoryEXT:
            return fmt::format( "vkCmdCopyMicromapToMemoryEXT ({}, {}, {})",
                GetName( drawcall.m_Payload.m_CopyMicromapToMemory.m_Src ),
                drawcall.m_Payload.m_CopyMicromapToMemory.m_Dst.hostAddress,
                GetCopyMicromapModeName( drawcall.m_Payload.m_CopyMicromapToMemory.m_Mode ) );
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
        // Construct the pipeline's name dynamically from the shaders.
        if( pipeline.m_UsesShaderObjects )
        {
            if( pipeline.m_BindPoint == VK_PIPELINE_BIND_POINT_GRAPHICS )
            {
                return pipeline.m_ShaderTuple.GetShaderStageHashesString(
                    VK_SHADER_STAGE_VERTEX_BIT |
                    VK_SHADER_STAGE_FRAGMENT_BIT );
            }

            if( pipeline.m_BindPoint == VK_PIPELINE_BIND_POINT_COMPUTE )
            {
                return pipeline.m_ShaderTuple.GetShaderStageHashesString(
                    VK_SHADER_STAGE_COMPUTE_BIT );
            }
        }

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

        std::string renderPassName = "Unknown Pass";

        switch( renderPass.m_Type )
        {
        case DeviceProfilerRenderPassType::eGraphics:
            renderPassName = "Graphics Pass"; break;
        case DeviceProfilerRenderPassType::eCompute:
            renderPassName = "Compute Pass"; break;
        case DeviceProfilerRenderPassType::eRayTracing:
            renderPassName = "Ray Tracing Pass"; break;
        case DeviceProfilerRenderPassType::eCopy:
            renderPassName = "Copy Pass"; break;
        }

        if( renderPass.m_Dynamic )
        {
            renderPassName = "Dynamic " + renderPassName;
        }

        return renderPassName;
    }

    /***********************************************************************************\

    Function:
        GetName

    Description:
        Returns name of the render pass command.

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetName( const DeviceProfilerRenderPassBeginData&, bool dynamic ) const
    {
        return (!dynamic) ? "vkCmdBeginRenderPass" : "vkCmdBeginRendering";
    }

    /***********************************************************************************\

    Function:
        GetName

    Description:
        Returns name of the render pass command.

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetName(const DeviceProfilerRenderPassEndData&, bool dynamic) const
    {
        return (!dynamic) ? "vkCmdEndRenderPass" : "vkCmdEndRendering";
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
        std::string objectName = m_Frontend.GetObjectName( object );

        if( !objectName.empty() )
        {
            return objectName;
        }

        return fmt::format( "{} {:#018x}",
            VkObject_Runtime_Traits::FromObjectType( object.m_Type ).ObjectTypeName,
            object.m_Handle );
    }

    /***********************************************************************************\

    Function:
        GetObjectID

    Description:
        Returns unique identifier for the Vulkan API object.

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetObjectID( const VkObject& object ) const
    {
        return fmt::format( "{}:{}:{}",
            static_cast<uint32_t>( object.m_Type ),
            object.m_Handle,
            object.m_CreateTime );
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
            return fmt::format( "Unknown command ({})", static_cast<uint32_t>( drawcall.m_Type ) );

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

        case DeviceProfilerDrawcallType::eDrawMeshTasks:
            return "vkCmdDrawMeshTasksEXT";

        case DeviceProfilerDrawcallType::eDrawMeshTasksIndirect:
            return "vkCmdDrawMeshTasksIndirectEXT";

        case DeviceProfilerDrawcallType::eDrawMeshTasksIndirectCount:
            return "vkCmdDrawMeshTasksIndirectCountEXT";

        case DeviceProfilerDrawcallType::eDrawMeshTasksNV:
            return "vkCmdDrawMeshTasksNV";

        case DeviceProfilerDrawcallType::eDrawMeshTasksIndirectNV:
            return "vkCmdDrawMeshTasksIndirectNV";

        case DeviceProfilerDrawcallType::eDrawMeshTasksIndirectCountNV:
            return "vkCmdDrawMeshTasksIndirectCountNV";

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

        case DeviceProfilerDrawcallType::eTraceRaysIndirect2KHR:
            return "vkCmdTraceRaysIndirect2KHR";

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

        case DeviceProfilerDrawcallType::eBuildMicromapsEXT:
            return "vkCmdBuildMicromapsEXT";

        case DeviceProfilerDrawcallType::eCopyMicromapEXT:
            return "vkCmdCopyMicromapEXT";

        case DeviceProfilerDrawcallType::eCopyMemoryToMicromapEXT:
            return "vkCmdCopyMemoryToMicromapEXT";

        case DeviceProfilerDrawcallType::eCopyMicromapToMemoryEXT:
            return "vkCmdCopyMicromapToMemoryEXT";
        }
    }

    /***********************************************************************************\

    Function:
        GetPointer

    Description:
        Returns string representation of a pointer.

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetPointer( const void* ptr ) const
    {
        if( ptr == nullptr )
        {
            return "null";
        }

        char pointer[ 19 ] = "0x0000000000000000";
        char* pPointerStr = pointer + 2;
        ProfilerStringFunctions::Hex( pPointerStr, static_cast<uint64_t>( reinterpret_cast<uintptr_t>( ptr ) ) );

        return pointer;
    }

    /***********************************************************************************\

    Function:
        GetBool

    Description:
        Returns string representation of a boolean value.

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetBool( VkBool32 value ) const
    {
        switch( value )
        {
        case VK_TRUE:
            return "True";
        case VK_FALSE:
            return "False";
        default:
            return std::to_string( value );
        }
    }

    /***********************************************************************************\

    Function:
        GetVec4

    Description:
        Returns string representation of a 4-component vector.

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetVec4( const float* pValue ) const
    {
        return fmt::format( "{:.2f}, {:.2f}, {:.2f}, {:.2f}",
            pValue[ 0 ],
            pValue[ 1 ],
            pValue[ 2 ],
            pValue[ 3 ] );
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
        char* pColorStr = color + 1;
        pColorStr = ProfilerStringFunctions::Hex( pColorStr, R );
        pColorStr = ProfilerStringFunctions::Hex( pColorStr, G );
        pColorStr = ProfilerStringFunctions::Hex( pColorStr, B );

        return color;
    }

    /***********************************************************************************\

    Function:
        GetByteSize

    Description:
        Returns short representation of a byte size.

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetByteSize( VkDeviceSize size ) const
    {
        typedef uint8_t Kilobyte[1024];
        typedef Kilobyte Megabyte[1024];
        typedef Megabyte Gigabyte[1024];

        if( size < sizeof( Kilobyte ) )
        {
            return fmt::format( "{} B", size );
        }

        if( size < sizeof( Megabyte ) )
        {
            return fmt::format( "{:.1f} kB", size / static_cast<float>( sizeof( Kilobyte ) ) );
        }

        if( size < sizeof( Gigabyte ) )
        {
            return fmt::format( "{:.1f} MB", size / static_cast<float>( sizeof( Megabyte ) ) );
        }

        return fmt::format( "{:.1f} GB", size / static_cast<float>( sizeof( Gigabyte ) ) );
    }

    /***********************************************************************************\

    Function:
        GetQueueFlagNames

    Description:
        Returns string representation of a VkQueueFlags.

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetQueueTypeName( VkQueueFlags flags ) const
    {
        if( flags & VK_QUEUE_GRAPHICS_BIT )
            return "Graphics";
        if( flags & VK_QUEUE_COMPUTE_BIT )
            return "Compute";
        if( ( flags & VK_QUEUE_VIDEO_DECODE_BIT_KHR ) || ( flags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR ) )
            return "Video";
        if( flags & VK_QUEUE_TRANSFER_BIT )
            return "Transfer";
        return "";
    }

    /***********************************************************************************\

    Function:
        GetQueueFlagNames

    Description:
        Returns string representation of a VkQueueFlags.

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetQueueFlagNames( VkQueueFlags flags ) const
    {
        FlagsStringBuilder builder;

        if( flags & VK_QUEUE_GRAPHICS_BIT )
            builder.AddFlag( "Graphics" );
        if( flags & VK_QUEUE_COMPUTE_BIT )
            builder.AddFlag( "Compute" );
        if( flags & VK_QUEUE_TRANSFER_BIT )
            builder.AddFlag( "Transfer" );
        if( flags & VK_QUEUE_SPARSE_BINDING_BIT )
            builder.AddFlag( "Sparse binding" );
        if( flags & VK_QUEUE_PROTECTED_BIT )
            builder.AddFlag( "Protected" );
        if( flags & VK_QUEUE_VIDEO_DECODE_BIT_KHR )
            builder.AddFlag( "Video decode" );
        if( flags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR )
            builder.AddFlag( "Video encode" );
        if( flags & VK_QUEUE_OPTICAL_FLOW_BIT_NV )
            builder.AddFlag( "Optical flow" );

        builder.AddUnknownFlags( flags, 8 );

        return builder.BuildString();
    }

    /***********************************************************************************\

    Function:
        GetShaderName

    Description:
        Returns string representation of the shader.

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetShaderName( const ProfilerShader& shader ) const
    {
        if( shader.m_pShaderModule && shader.m_pShaderModule->m_pFileName )
        {
            return fmt::format( "{} {:08X} ({} > {})",
                GetShaderStageName( shader.m_Stage ),
                shader.m_Hash,
                shader.m_pShaderModule->m_pFileName,
                shader.m_EntryPoint );
        }

        return fmt::format( "{} {:08X} ({})",
            GetShaderStageName( shader.m_Stage ),
            shader.m_Hash,
            shader.m_EntryPoint );
    }

    /***********************************************************************************\

    Function:
        GetShortShaderName

    Description:
        Returns short string representation of the shader.

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetShortShaderName( const ProfilerShader& shader ) const
    {
        return fmt::format( "{} {:08X} {}",
            GetShortShaderStageName( shader.m_Stage ),
            shader.m_Hash,
            shader.m_EntryPoint );
    }

    /***********************************************************************************\

    Function:
        GetShaderStageName

    Description:
        Returns string representation of the shader stage.

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetShaderStageName( VkShaderStageFlagBits stage ) const
    {
        switch( stage )
        {
        case VK_SHADER_STAGE_VERTEX_BIT:
            return "Vertex shader";
        case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
            return "Tessellation control shader";
        case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
            return "Tessellation evaluation shader";
        case VK_SHADER_STAGE_GEOMETRY_BIT:
            return "Geometry shader";
        case VK_SHADER_STAGE_FRAGMENT_BIT:
            return "Fragment shader";
        case VK_SHADER_STAGE_COMPUTE_BIT:
            return "Compute shader";
        case VK_SHADER_STAGE_TASK_BIT_EXT:
            return "Task shader";
        case VK_SHADER_STAGE_MESH_BIT_EXT:
            return "Mesh shader";
        case VK_SHADER_STAGE_RAYGEN_BIT_KHR:
            return "Ray generation shader";
        case VK_SHADER_STAGE_ANY_HIT_BIT_KHR:
            return "Ray any-hit shader";
        case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:
            return "Ray closest-hit shader";
        case VK_SHADER_STAGE_MISS_BIT_KHR:
            return "Ray miss shader";
        case VK_SHADER_STAGE_INTERSECTION_BIT_KHR:
            return "Ray intersection shader";
        case VK_SHADER_STAGE_CALLABLE_BIT_KHR:
            return "Callable shader";
        default:
            return fmt::format( "Unknown shader stage ({})", static_cast<uint32_t>( stage ) );
        }
    }

    /***********************************************************************************\

    Function:
        GetShortShaderStageName

    Description:
        Returns short string representation of the shader stage.

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetShortShaderStageName( VkShaderStageFlagBits stage ) const
    {
        switch( stage )
        {
        case VK_SHADER_STAGE_VERTEX_BIT:
            return "vs";
        case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
            return "tcs";
        case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
            return "tes";
        case VK_SHADER_STAGE_GEOMETRY_BIT:
            return "gs";
        case VK_SHADER_STAGE_FRAGMENT_BIT:
            return "ps";
        case VK_SHADER_STAGE_COMPUTE_BIT:
            return "cs";
        case VK_SHADER_STAGE_TASK_BIT_EXT:
            return "task";
        case VK_SHADER_STAGE_MESH_BIT_EXT:
            return "mesh";
        case VK_SHADER_STAGE_RAYGEN_BIT_KHR:
            return "raygen";
        case VK_SHADER_STAGE_ANY_HIT_BIT_KHR:
            return "anyhit";
        case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:
            return "closesthit";
        case VK_SHADER_STAGE_MISS_BIT_KHR:
            return "miss";
        case VK_SHADER_STAGE_INTERSECTION_BIT_KHR:
            return "intersection";
        case VK_SHADER_STAGE_CALLABLE_BIT_KHR:
            return "callable";
        default:
            return fmt::to_string( static_cast<uint32_t>(stage) );
        }
    }

    /***********************************************************************************\

    Function:
        GetShaderGroupTypeName

    Description:
        Returns string representation of VkRayTracingShaderGroupTypeKHR.

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetShaderGroupTypeName( VkRayTracingShaderGroupTypeKHR groupType ) const
    {
        switch( groupType )
        {
        case VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR:
            return "General";
        case VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR:
            return "Triangles";
        case VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR:
            return "Procedural";
        default:
            return fmt::format( "Unknown ({})", static_cast<uint32_t>( groupType ) );
        }
    }

    /***********************************************************************************\

    Function:
        GetGeneralShaderGroupTypeName

    Description:
        Returns string representation of VkShaderStageFlagBits for a general shader group.

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetGeneralShaderGroupTypeName( VkShaderStageFlagBits stage ) const
    {
        switch( stage )
        {
        case VK_SHADER_STAGE_RAYGEN_BIT_KHR:
            return "Raygen";
        case VK_SHADER_STAGE_MISS_BIT_KHR:
            return "Miss";
        case VK_SHADER_STAGE_CALLABLE_BIT_KHR:
            return "Callable";
        default:
            return std::string();
        }
    }

    /***********************************************************************************\

    Function:
        GetFormatName

    Description:
        Returns string representation of a VkFormat.

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetFormatName( VkFormat format ) const
    {
        switch( format )
        {
        case VK_FORMAT_UNDEFINED:
            return "Undefined";
        case VK_FORMAT_R4G4_UNORM_PACK8:
            return "R4G4 Unorm";
        case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
            return "R4G4B4A4 Unorm";
        case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
            return "B4G4R4A4 Unorm";
        case VK_FORMAT_R5G6B5_UNORM_PACK16:
            return "R5G6B5 Unorm";
        case VK_FORMAT_B5G6R5_UNORM_PACK16:
            return "B5G6R5 Unorm";
        case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
            return "R5G5B5A1 Unorm";
        case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
            return "B5G5R5A1 Unorm";
        case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
            return "A1R5G5B5 Unorm";
        case VK_FORMAT_R8_UNORM:
            return "R8 Unorm";
        case VK_FORMAT_R8_SNORM:
            return "R8 Snorm";
        case VK_FORMAT_R8_USCALED:
            return "R8 Uscaled";
        case VK_FORMAT_R8_SSCALED:
            return "R8 Sscaled";
        case VK_FORMAT_R8_UINT:
            return "R8 Uint";
        case VK_FORMAT_R8_SINT:
            return "R8 Sint";
        case VK_FORMAT_R8_SRGB:
            return "R8 Srgb";
        case VK_FORMAT_R8G8_UNORM:
            return "R8G8 Unorm";
        case VK_FORMAT_R8G8_SNORM:
            return "R8G8 Snorm";
        case VK_FORMAT_R8G8_USCALED:
            return "R8G8 Uscaled";
        case VK_FORMAT_R8G8_SSCALED:
            return "R8G8 Sscaled";
        case VK_FORMAT_R8G8_UINT:
            return "R8G8 Uint";
        case VK_FORMAT_R8G8_SINT:
            return "R8G8 Sint";
        case VK_FORMAT_R8G8_SRGB:
            return "R8G8 Srgb";
        case VK_FORMAT_R8G8B8_UNORM:
            return "R8G8B8 Unorm";
        case VK_FORMAT_R8G8B8_SNORM:
            return "R8G8B8 Snorm";
        case VK_FORMAT_R8G8B8_USCALED:
            return "R8G8B8 Uscaled";
        case VK_FORMAT_R8G8B8_SSCALED:
            return "R8G8B8 Sscaled";
        case VK_FORMAT_R8G8B8_UINT:
            return "R8G8B8 Uint";
        case VK_FORMAT_R8G8B8_SINT:
            return "R8G8B8 Sint";
        case VK_FORMAT_R8G8B8_SRGB:
            return "R8G8B8 Srgb";
        case VK_FORMAT_B8G8R8_UNORM:
            return "B8G8R8 Unorm";
        case VK_FORMAT_B8G8R8_SNORM:
            return "B8G8R8 Snorm";
        case VK_FORMAT_B8G8R8_USCALED:
            return "B8G8R8 Uscaled";
        case VK_FORMAT_B8G8R8_SSCALED:
            return "B8G8R8 Sscaled";
        case VK_FORMAT_B8G8R8_UINT:
            return "B8G8R8 Uint";
        case VK_FORMAT_B8G8R8_SINT:
            return "B8G8R8 Sint";
        case VK_FORMAT_B8G8R8_SRGB:
            return "B8G8R8 Srgb";
        case VK_FORMAT_R8G8B8A8_UNORM:
            return "R8G8B8A8 Unorm";
        case VK_FORMAT_R8G8B8A8_SNORM:
            return "R8G8B8A8 Snorm";
        case VK_FORMAT_R8G8B8A8_USCALED:
            return "R8G8B8A8 Uscaled";
        case VK_FORMAT_R8G8B8A8_SSCALED:
            return "R8G8B8A8 Sscaled";
        case VK_FORMAT_R8G8B8A8_UINT:
            return "R8G8B8A8 Uint";
        case VK_FORMAT_R8G8B8A8_SINT:
            return "R8G8B8A8 Sint";
        case VK_FORMAT_R8G8B8A8_SRGB:
            return "R8G8B8A8 Srgb";
        case VK_FORMAT_B8G8R8A8_UNORM:
            return "B8G8R8A8 Unorm";
        case VK_FORMAT_B8G8R8A8_SNORM:
            return "B8G8R8A8 Snorm";
        case VK_FORMAT_B8G8R8A8_USCALED:
            return "B8G8R8A8 Uscaled";
        case VK_FORMAT_B8G8R8A8_SSCALED:
            return "B8G8R8A8 Sscaled";
        case VK_FORMAT_B8G8R8A8_UINT:
            return "B8G8R8A8 Uint";
        case VK_FORMAT_B8G8R8A8_SINT:
            return "B8G8R8A8 Sint";
        case VK_FORMAT_B8G8R8A8_SRGB:
            return "B8G8R8A8 Srgb";
        case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
            return "A8B8G8R8 Unorm";
        case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
            return "A8B8G8R8 Snorm";
        case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
            return "A8B8G8R8 Uscaled";
        case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
            return "A8B8G8R8 Sscaled";
        case VK_FORMAT_A8B8G8R8_UINT_PACK32:
            return "A8B8G8R8 Uint";
        case VK_FORMAT_A8B8G8R8_SINT_PACK32:
            return "A8B8G8R8 Sint";
        case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
            return "A8B8G8R8 Srgb";
        case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
            return "A2R10G10B10 Unorm";
        case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
            return "A2R10G10B10 Snorm";
        case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
            return "A2R10G10B10 Uscaled";
        case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
            return "A2R10G10B10 Sscaled";
        case VK_FORMAT_A2R10G10B10_UINT_PACK32:
            return "A2R10G10B10 Uint";
        case VK_FORMAT_A2R10G10B10_SINT_PACK32:
            return "A2R10G10B10 Sint";
        case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
            return "A2B10G10R10 Unorm";
        case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
            return "A2B10G10R10 Snorm";
        case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
            return "A2B10G10R10 Uscaled";
        case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
            return "A2B10G10R10 Sscaled";
        case VK_FORMAT_A2B10G10R10_UINT_PACK32:
            return "A2B10G10R10 Uint";
        case VK_FORMAT_A2B10G10R10_SINT_PACK32:
            return "A2B10G10R10 Sint";
        case VK_FORMAT_R16_UNORM:
            return "R16 Unorm";
        case VK_FORMAT_R16_SNORM:
            return "R16 Snorm";
        case VK_FORMAT_R16_USCALED:
            return "R16 Uscaled";
        case VK_FORMAT_R16_SSCALED:
            return "R16 Sscaled";
        case VK_FORMAT_R16_UINT:
            return "R16 Uint";
        case VK_FORMAT_R16_SINT:
            return "R16 Sint";
        case VK_FORMAT_R16_SFLOAT:
            return "R16 Sfloat";
        case VK_FORMAT_R16G16_UNORM:
            return "R16G16 Unorm";
        case VK_FORMAT_R16G16_SNORM:
            return "R16G16 Snorm";
        case VK_FORMAT_R16G16_USCALED:
            return "R16G16 Uscaled";
        case VK_FORMAT_R16G16_SSCALED:
            return "R16G16 Sscaled";
        case VK_FORMAT_R16G16_UINT:
            return "R16G16 Uint";
        case VK_FORMAT_R16G16_SINT:
            return "R16G16 Sint";
        case VK_FORMAT_R16G16_SFLOAT:
            return "R16G16 Sfloat";
        case VK_FORMAT_R16G16B16_UNORM:
            return "R16G16B16 Unorm";
        case VK_FORMAT_R16G16B16_SNORM:
            return "R16G16B16 Snorm";
        case VK_FORMAT_R16G16B16_USCALED:
            return "R16G16B16 Uscaled";
        case VK_FORMAT_R16G16B16_SSCALED:
            return "R16G16B16 Sscaled";
        case VK_FORMAT_R16G16B16_UINT:
            return "R16G16B16 Uint";
        case VK_FORMAT_R16G16B16_SINT:
            return "R16G16B16 Sint";
        case VK_FORMAT_R16G16B16_SFLOAT:
            return "R16G16B16 Sfloat";
        case VK_FORMAT_R16G16B16A16_UNORM:
            return "R16G16B16A16 Unorm";
        case VK_FORMAT_R16G16B16A16_SNORM:
            return "R16G16B16A16 Snorm";
        case VK_FORMAT_R16G16B16A16_USCALED:
            return "R16G16B16A16 Uscaled";
        case VK_FORMAT_R16G16B16A16_SSCALED:
            return "R16G16B16A16 Sscaled";
        case VK_FORMAT_R16G16B16A16_UINT:
            return "R16G16B16A16 Uint";
        case VK_FORMAT_R16G16B16A16_SINT:
            return "R16G16B16A16 Sint";
        case VK_FORMAT_R16G16B16A16_SFLOAT:
            return "R16G16B16A16 Sfloat";
        case VK_FORMAT_R32_UINT:
            return "R32 Uint";
        case VK_FORMAT_R32_SINT:
            return "R32 Sint";
        case VK_FORMAT_R32_SFLOAT:
            return "R32 Sfloat";
        case VK_FORMAT_R32G32_UINT:
            return "R32G32 Uint";
        case VK_FORMAT_R32G32_SINT:
            return "R32G32 Sint";
        case VK_FORMAT_R32G32_SFLOAT:
            return "R32G32 Sfloat";
        case VK_FORMAT_R32G32B32_UINT:
            return "R32G32B32 Uint";
        case VK_FORMAT_R32G32B32_SINT:
            return "R32G32B32 Sint";
        case VK_FORMAT_R32G32B32_SFLOAT:
            return "R32G32B32 Sfloat";
        case VK_FORMAT_R32G32B32A32_UINT:
            return "R32G32B32A32 Uint";
        case VK_FORMAT_R32G32B32A32_SINT:
            return "R32G32B32A32 Sint";
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return "R32G32B32A32 Sfloat";
        case VK_FORMAT_R64_UINT:
            return "R64 Uint";
        case VK_FORMAT_R64_SINT:
            return "R64 Sint";
        case VK_FORMAT_R64_SFLOAT:
            return "R64 Sfloat";
        case VK_FORMAT_R64G64_UINT:
            return "R64G64 Uint";
        case VK_FORMAT_R64G64_SINT:
            return "R64G64 Sint";
        case VK_FORMAT_R64G64_SFLOAT:
            return "R64G64 Sfloat";
        case VK_FORMAT_R64G64B64_UINT:
            return "R64G64B64 Uint";
        case VK_FORMAT_R64G64B64_SINT:
            return "R64G64B64 Sint";
        case VK_FORMAT_R64G64B64_SFLOAT:
            return "R64G64B64 Sfloat";
        case VK_FORMAT_R64G64B64A64_UINT:
            return "R64G64B64A64 Uint";
        case VK_FORMAT_R64G64B64A64_SINT:
            return "R64G64B64A64 Sint";
        case VK_FORMAT_R64G64B64A64_SFLOAT:
            return "R64G64B64A64 Sfloat";
        case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
            return "B10G11R11 Ufloat";
        case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
            return "E5B9G9R9 Ufloat";
        case VK_FORMAT_D16_UNORM:
            return "D16 Unorm";
        case VK_FORMAT_X8_D24_UNORM_PACK32:
            return "D24 Unorm";
        case VK_FORMAT_D32_SFLOAT:
            return "D32 Sfloat";
        case VK_FORMAT_S8_UINT:
            return "S8 Uint";
        case VK_FORMAT_D16_UNORM_S8_UINT:
            return "D16 Unorm S8 Uint";
        case VK_FORMAT_D24_UNORM_S8_UINT:
            return "D24 Unorm S8 Uint";
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            return "D32 Sfloat S8 Uint";
        case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
            return "BC1 RGB Unorm";
        case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
            return "BC1 RGB Srgb";
        case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
            return "BC1 RGBA Unorm";
        case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
            return "BC1 RGBA Srgb";
        case VK_FORMAT_BC2_UNORM_BLOCK:
            return "BC2 Unorm";
        case VK_FORMAT_BC2_SRGB_BLOCK:
            return "BC2 Srgb";
        case VK_FORMAT_BC3_UNORM_BLOCK:
            return "BC3 Unorm";
        case VK_FORMAT_BC3_SRGB_BLOCK:
            return "BC3 Srgb";
        case VK_FORMAT_BC4_UNORM_BLOCK:
            return "BC4 Unorm";
        case VK_FORMAT_BC4_SNORM_BLOCK:
            return "BC4 Snorm";
        case VK_FORMAT_BC5_UNORM_BLOCK:
            return "BC5 Unorm";
        case VK_FORMAT_BC5_SNORM_BLOCK:
            return "BC5 Snorm";
        case VK_FORMAT_BC6H_UFLOAT_BLOCK:
            return "BC6H Ufloat";
        case VK_FORMAT_BC6H_SFLOAT_BLOCK:
            return "BC6H Sfloat";
        case VK_FORMAT_BC7_UNORM_BLOCK:
            return "BC7 Unorm";
        case VK_FORMAT_BC7_SRGB_BLOCK:
            return "BC7 Srgb";
        case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
            return "ETC2 R8G8B8 Unorm";
        case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
            return "ETC2 R8G8B8 Srgb";
        case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
            return "ETC2 R8G8B8A1 Unorm";
        case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
            return "ETC2 R8G8B8A1 Srgb";
        case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
            return "ETC2 R8G8B8A8 Unorm";
        case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
            return "ETC2 R8G8B8A8 Srgb";
        case VK_FORMAT_EAC_R11_UNORM_BLOCK:
            return "EAC R11 Unorm";
        case VK_FORMAT_EAC_R11_SNORM_BLOCK:
            return "EAC R11 Snorm";
        case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
            return "EAC R11G11 Unorm";
        case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
            return "EAC R11G11 Snorm";
        case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
            return "ASTC 4x4 Unorm";
        case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
            return "ASTC 4x4 Srgb";
        case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
            return "ASTC 5x4 Unorm";
        case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
            return "ASTC 5x4 Srgb";
        case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
            return "ASTC 5x5 Unorm";
        case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
            return "ASTC 5x5 Srgb";
        case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
            return "ASTC 6x5 Unorm";
        case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
            return "ASTC 6x5 Srgb";
        case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
            return "ASTC 6x6 Unorm";
        case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
            return "ASTC 6x6 Srgb";
        case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
            return "ASTC 8x5 Unorm";
        case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
            return "ASTC 8x5 Srgb";
        case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
            return "ASTC 8x6 Unorm";
        case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
            return "ASTC 8x6 Srgb";
        case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
            return "ASTC 8x8 Unorm";
        case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
            return "ASTC 8x8 Srgb";
        case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
            return "ASTC 10x5 Unorm";
        case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
            return "ASTC 10x5 Srgb";
        case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
            return "ASTC 10x6 Unorm";
        case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
            return "ASTC 10x6 Srgb";
        case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
            return "ASTC 10x8 Unorm";
        case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
            return "ASTC 10x8 Srgb";
        case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
            return "ASTC 10x10 Unorm";
        case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
            return "ASTC 10x10 Srgb";
        case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
            return "ASTC 12x10 Unorm";
        case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
            return "ASTC 12x10 Srgb";
        case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
            return "ASTC 12x12 Unorm";
        case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
            return "ASTC 12x12 Srgb";
        case VK_FORMAT_G8B8G8R8_422_UNORM:
            return "G8B8G8R8 422 Unorm";
        case VK_FORMAT_B8G8R8G8_422_UNORM:
            return "B8G8R8G8 422 Unorm";
        case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM:
            return "G8 B8 R8 3-Plane 420 Unorm";
        case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM:
            return "G8 B8R8 2-Plane 420 Unorm";
        case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM:
            return "G8 B8 R8 3-Plane 422 Unorm";
        case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM:
            return "G8 B8R8 2-Plane 422 Unorm";
        case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM:
            return "G8 B8 R8 3-Plane 444 Unorm";
        case VK_FORMAT_R10X6_UNORM_PACK16:
            return "R10 Unorm";
        case VK_FORMAT_R10X6G10X6_UNORM_2PACK16:
            return "R10G10 Unorm";
        case VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16:
            return "R10G10B10A10 Unorm";
        case VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16:
            return "G10B10G10R10 422 Unorm";
        case VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16:
            return "B10G10R10G10 422 Unorm";
        case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
            return "G10 B10 R10 3-Plane 420 Unorm";
        case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
            return "G10 B10R10 2-Plane 420 Unorm";
        case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
            return "G10 B10 R10 3-Plane 422 Unorm";
        case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
            return "G10 B10R10 2-Plane 422 Unorm";
        case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
            return "G10 B10 R10 3-Plane 444 Unorm";
        case VK_FORMAT_R12X4_UNORM_PACK16:
            return "R12 Unorm";
        case VK_FORMAT_R12X4G12X4_UNORM_2PACK16:
            return "R12G12 Unorm";
        case VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16:
            return "R12G12B12A12 Unorm";
        case VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16:
            return "G12B12G12R12 422 Unorm";
        case VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16:
            return "B12G12R12G12 422 Unorm";
        case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
            return "G12 B12 R12 3-Plane 420 Unorm";
        case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
            return "G12 B12R12 2-Plane 420 Unorm";
        case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
            return "G12 B12 R12 3-Plane 422 Unorm";
        case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
            return "G12 B12R12 2-Plane 422 Unorm";
        case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
            return "G12 B12 R12 3-Plane 444 Unorm";
        case VK_FORMAT_G16B16G16R16_422_UNORM:
            return "G16B16G16R16 422 Unorm";
        case VK_FORMAT_B16G16R16G16_422_UNORM:
            return "B16G16R16G16 422 Unorm";
        case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM:
            return "G16 B16 R16 3-Plane 420 Unorm";
        case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM:
            return "G16 B16R16 2-Plane 420 Unorm";
        case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM:
            return "G16 B16 R16 3-Plane 422 Unorm";
        case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM:
            return "G16 B16R16 2-Plane 422 Unorm";
        case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM:
            return "G16 B16 R16 3-Plane 444 Unorm";
        case VK_FORMAT_G8_B8R8_2PLANE_444_UNORM:
            return "G8 B8R8 2-Plane 444 Unorm";
        case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16:
            return "G16 B10R10 2-Plane 444 Unorm";
        case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16:
            return "G12 B12R12 2-Plane 444 Unorm";
        case VK_FORMAT_G16_B16R16_2PLANE_444_UNORM:
            return "G16 B16R16 2-Plane 444 Unorm";
        case VK_FORMAT_A4R4G4B4_UNORM_PACK16:
            return "A4R4G4B4 Unorm";
        case VK_FORMAT_A4B4G4R4_UNORM_PACK16:
            return "A4B4G4R4 Unorm";
        case VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK:
            return "ASTC 4x4 Sfloat";
        case VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK:
            return "ASTC 5x4 Sfloat";
        case VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK:
            return "ASTC 5x5 Sfloat";
        case VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK:
            return "ASTC 6x5 Sfloat";
        case VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK:
            return "ASTC 6x6 Sfloat";
        case VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK:
            return "ASTC 8x5 Sfloat";
        case VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK:
            return "ASTC 8x6 Sfloat";
        case VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK:
            return "ASTC 8x8 Sfloat";
        case VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK:
            return "ASTC 10x5 Sfloat";
        case VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK:
            return "ASTC 10x6 Sfloat";
        case VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK:
            return "ASTC 10x8 Sfloat";
        case VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK:
            return "ASTC 10x10 Sfloat";
        case VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK:
            return "ASTC 12x10 Sfloat";
        case VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK:
            return "ASTC 12x12 Sfloat";
        case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG:
            return "PVRTC1 2BPP Unorm";
        case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG:
            return "PVRTC1 4BPP Unorm";
        case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG:
            return "PVRTC2 2BPP Unorm";
        case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG:
            return "PVRTC2 4BPP Unorm";
        case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG:
            return "PVRTC1 2BPP Srgb";
        case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG:
            return "PVRTC1 4BPP Srgb";
        case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG:
            return "PVRTC2 2BPP Srgb";
        case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG:
            return "PVRTC2 4BPP Srgb";
        }
        return fmt::format( "Unknown format ({})", static_cast<uint32_t>( format ) );
    }

    /***********************************************************************************\

    Function:
        GetIndexTypeName

    Description:

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetIndexTypeName( VkIndexType type ) const
    {
        switch( type )
        {
        case VK_INDEX_TYPE_UINT16:
            return "Uint16";
        case VK_INDEX_TYPE_UINT32:
            return "Uint32";
        case VK_INDEX_TYPE_UINT8_EXT:
            return "Uint8";
        case VK_INDEX_TYPE_NONE_KHR:
            return "None";
        }
        return fmt::format( "Unknown type ({})", static_cast<uint32_t>( type ) );
    }

    /***********************************************************************************\

    Function:
        GetVertexInputRateName

    Description:

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetVertexInputRateName( VkVertexInputRate rate ) const
    {
        switch( rate )
        {
        case VK_VERTEX_INPUT_RATE_VERTEX:
            return "Per vertex";
        case VK_VERTEX_INPUT_RATE_INSTANCE:
            return "Per instance";
        default:
            return fmt::format( "Unknown rate ({})", static_cast<uint32_t>(rate) );
        }
    }

    /***********************************************************************************\

    Function:
        GetPrimitiveTopologyName

    Description:

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetPrimitiveTopologyName( VkPrimitiveTopology topology ) const
    {
        switch( topology )
        {
        case VK_PRIMITIVE_TOPOLOGY_POINT_LIST:
            return "Point list";
        case VK_PRIMITIVE_TOPOLOGY_LINE_LIST:
            return "Line list";
        case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP:
            return "Line strip";
        case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
            return "Triangle list";
        case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
            return "Triangle strip";
        case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
            return "Triangle fan";
        case VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY:
            return "Line list with adjacency";
        case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY:
            return "Line strip with adjacency";
        case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY:
            return "Triangle list with adjacency";
        case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY:
            return "Triangle strip with adjacency";
        case VK_PRIMITIVE_TOPOLOGY_PATCH_LIST:
            return "Patch list";
        default:
            return fmt::format( "Unknown ({})", static_cast<uint32_t>(topology) );
        }
    }

    /***********************************************************************************\

    Function:
        GetPolygonModeName

    Description:

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetPolygonModeName( VkPolygonMode mode ) const
    {
        switch( mode )
        {
        case VK_POLYGON_MODE_FILL:
            return "Fill";
        case VK_POLYGON_MODE_LINE:
            return "Line";
        case VK_POLYGON_MODE_POINT:
            return "Point";
        case VK_POLYGON_MODE_FILL_RECTANGLE_NV:
            return "Fill rectangle";
        default:
            return fmt::format( "Unknown ({})", static_cast<uint32_t>(mode) );
        }
    }

    /***********************************************************************************\

    Function:
        GetCullModeName

    Description:

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetCullModeName( VkCullModeFlags mode ) const
    {
        switch( mode )
        {
        case VK_CULL_MODE_NONE:
            return "None";
        case VK_CULL_MODE_FRONT_BIT:
            return "Front";
        case VK_CULL_MODE_BACK_BIT:
            return "Back";
        case VK_CULL_MODE_FRONT_AND_BACK:
            return "All";
        default:
            return fmt::format( "Unknown ({})", static_cast<uint32_t>(mode) );
        }
    }

    /***********************************************************************************\

    Function:
        GetFrontFaceName

    Description:

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetFrontFaceName( VkFrontFace mode ) const
    {
        switch( mode )
        {
        case VK_FRONT_FACE_COUNTER_CLOCKWISE:
            return "Counter-clockwise";
        case VK_FRONT_FACE_CLOCKWISE:
            return "Clockwise";
        default:
            return fmt::format( "Unknown ({})", static_cast<uint32_t>(mode) );
        }
    }

    /***********************************************************************************\

    Function:
        GetBlendFactorName

    Description:

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetBlendFactorName( VkBlendFactor factor ) const
    {
        switch( factor )
        {
        case VK_BLEND_FACTOR_ZERO:
            return "Zero";
        case VK_BLEND_FACTOR_ONE:
            return "One";
        case VK_BLEND_FACTOR_SRC_COLOR:
            return "Src color";
        case VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR:
            return "1 - Src color";
        case VK_BLEND_FACTOR_DST_COLOR:
            return "Dst color";
        case VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR:
            return "1 - Dst color";
        case VK_BLEND_FACTOR_SRC_ALPHA:
            return "Src alpha";
        case VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA:
            return "1 - Src alpha";
        case VK_BLEND_FACTOR_DST_ALPHA:
            return "Dst alpha";
        case VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA:
            return "1 - Dst alpha";
        case VK_BLEND_FACTOR_CONSTANT_COLOR:
            return "Constant";
        case VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR:
            return "1 - Constant";
        case VK_BLEND_FACTOR_SRC_ALPHA_SATURATE:
            return "Src alpha (sat)";
        case VK_BLEND_FACTOR_SRC1_COLOR:
            return "Src1 color";
        case VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR:
            return "1 - Src1 color";
        case VK_BLEND_FACTOR_SRC1_ALPHA:
            return "Src1 alpha";
        case VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA:
            return "1 - Src1 alpha";
        default:
            return fmt::format( "Unknown ({})", static_cast<uint32_t>(factor) );
        }
    }

    /***********************************************************************************\

    Function:
        GetBlendOpName

    Description:

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetBlendOpName( VkBlendOp op ) const
    {
        switch( op )
        {
        case VK_BLEND_OP_ADD:
            return "Add";
        case VK_BLEND_OP_SUBTRACT:
            return "Sub";
        case VK_BLEND_OP_REVERSE_SUBTRACT:
            return "Rev sub";
        case VK_BLEND_OP_MIN:
            return "Min";
        case VK_BLEND_OP_MAX:
            return "Max";
        case VK_BLEND_OP_ZERO_EXT:
            return "Zero";
        case VK_BLEND_OP_SRC_EXT:
            return "Src";
        case VK_BLEND_OP_DST_EXT:
            return "Dst";
        case VK_BLEND_OP_SRC_OVER_EXT:
            return "Src over";
        case VK_BLEND_OP_DST_OVER_EXT:
            return "Dst over";
        case VK_BLEND_OP_SRC_IN_EXT:
            return "Src in";
        case VK_BLEND_OP_DST_IN_EXT:
            return "Dst in";
        case VK_BLEND_OP_SRC_OUT_EXT:
            return "Src out";
        case VK_BLEND_OP_DST_OUT_EXT:
            return "Dst out";
        case VK_BLEND_OP_SRC_ATOP_EXT:
            return "Src atop";
        case VK_BLEND_OP_DST_ATOP_EXT:
            return "Dst atop";
        case VK_BLEND_OP_XOR_EXT:
            return "Xor";
        case VK_BLEND_OP_MULTIPLY_EXT:
            return "Mul";
        case VK_BLEND_OP_SCREEN_EXT:
            return "Screen";
        case VK_BLEND_OP_OVERLAY_EXT:
            return "Overlay";
        case VK_BLEND_OP_DARKEN_EXT:
            return "Darken";
        case VK_BLEND_OP_LIGHTEN_EXT:
            return "Lighten";
        case VK_BLEND_OP_COLORDODGE_EXT:
            return "Color dodge";
        case VK_BLEND_OP_COLORBURN_EXT:
            return "Color burn";
        case VK_BLEND_OP_HARDLIGHT_EXT:
            return "Hard light";
        case VK_BLEND_OP_SOFTLIGHT_EXT:
            return "Soft light";
        case VK_BLEND_OP_DIFFERENCE_EXT:
            return "Difference";
        case VK_BLEND_OP_EXCLUSION_EXT:
            return "Exclusion";
        case VK_BLEND_OP_INVERT_EXT:
            return "Invert";
        case VK_BLEND_OP_INVERT_RGB_EXT:
            return "Invert RGB";
        case VK_BLEND_OP_LINEARDODGE_EXT:
            return "Linear dodge";
        case VK_BLEND_OP_LINEARBURN_EXT:
            return "Linear burn";
        case VK_BLEND_OP_VIVIDLIGHT_EXT:
            return "Vivid light";
        case VK_BLEND_OP_LINEARLIGHT_EXT:
            return "Linear light";
        case VK_BLEND_OP_PINLIGHT_EXT:
            return "Pin light";
        case VK_BLEND_OP_HARDMIX_EXT:
            return "Hard mix";
        case VK_BLEND_OP_HSL_HUE_EXT:
            return "HSL hue";
        case VK_BLEND_OP_HSL_SATURATION_EXT:
            return "HSL saturation";
        case VK_BLEND_OP_HSL_COLOR_EXT:
            return "HSL color";
        case VK_BLEND_OP_HSL_LUMINOSITY_EXT:
            return "HSL luminosity";
        case VK_BLEND_OP_PLUS_EXT:
            return "Plus";
        case VK_BLEND_OP_PLUS_CLAMPED_EXT:
            return "Plus clamped";
        case VK_BLEND_OP_PLUS_CLAMPED_ALPHA_EXT:
            return "Plus clamped alpha";
        case VK_BLEND_OP_PLUS_DARKER_EXT:
            return "Plus darker";
        case VK_BLEND_OP_MINUS_EXT:
            return "Minus";
        case VK_BLEND_OP_MINUS_CLAMPED_EXT:
            return "Minus clamped";
        case VK_BLEND_OP_CONTRAST_EXT:
            return "Contrast";
        case VK_BLEND_OP_INVERT_OVG_EXT:
            return "Invert OVG";
        case VK_BLEND_OP_RED_EXT:
            return "Red";
        case VK_BLEND_OP_GREEN_EXT:
            return "Green";
        case VK_BLEND_OP_BLUE_EXT:
            return "Blue";
        default:
            return fmt::format( "Unknown ({})", static_cast<uint32_t>(op) );
        }
    }

    /***********************************************************************************\

    Function:
        GetCompareOpName

    Description:

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetCompareOpName( VkCompareOp op ) const
    {
        switch( op )
        {
        case VK_COMPARE_OP_NEVER:
            return "Never";
        case VK_COMPARE_OP_LESS:
            return "Less";
        case VK_COMPARE_OP_EQUAL:
            return "Equal";
        case VK_COMPARE_OP_LESS_OR_EQUAL:
            return "Less or equal";
        case VK_COMPARE_OP_GREATER:
            return "Greater";
        case VK_COMPARE_OP_NOT_EQUAL:
            return "Not equal";
        case VK_COMPARE_OP_GREATER_OR_EQUAL:
            return "Greater or equal";
        case VK_COMPARE_OP_ALWAYS:
            return "Always";
        default:
            return fmt::format( "Unknown ({})", static_cast<uint32_t>(op) );
        }
    }

    /***********************************************************************************\

    Function:
        GetLogicOpName

    Description:

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetLogicOpName( VkLogicOp op ) const
    {
        switch( op )
        {
        case VK_LOGIC_OP_CLEAR:
            return "Clear";
        case VK_LOGIC_OP_AND:
            return "AND";
        case VK_LOGIC_OP_AND_REVERSE:
            return "AND (reverse)";
        case VK_LOGIC_OP_COPY:
            return "Copy";
        case VK_LOGIC_OP_AND_INVERTED:
            return "AND (inverted)";
        case VK_LOGIC_OP_NO_OP:
            return "No-op";
        case VK_LOGIC_OP_XOR:
            return "XOR";
        case VK_LOGIC_OP_OR:
            return "OR";
        case VK_LOGIC_OP_NOR:
            return "NOR";
        case VK_LOGIC_OP_EQUIVALENT:
            return "Equivalent";
        case VK_LOGIC_OP_INVERT:
            return "Invert";
        case VK_LOGIC_OP_OR_REVERSE:
            return "OR (reverse)";
        case VK_LOGIC_OP_COPY_INVERTED:
            return "Copy (inverted)";
        case VK_LOGIC_OP_OR_INVERTED:
            return "OR (inverted)";
        case VK_LOGIC_OP_NAND:
            return u8"NAND";
        case VK_LOGIC_OP_SET:
            return u8"Set";
        default:
            return fmt::format( "Unknown ({})", static_cast<uint32_t>(op) );
        }
    }

    /***********************************************************************************\

    Function:
        GetColorComponentFlagNames

    Description:

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetColorComponentFlagNames( VkColorComponentFlags flags ) const
    {
        char mask[ 5 ] = "____";
        if( flags & VK_COLOR_COMPONENT_R_BIT )
            mask[ 0 ] = 'R';
        if( flags & VK_COLOR_COMPONENT_G_BIT )
            mask[ 1 ] = 'G';
        if( flags & VK_COLOR_COMPONENT_B_BIT )
            mask[ 2 ] = 'B';
        if( flags & VK_COLOR_COMPONENT_A_BIT )
            mask[ 3 ] = 'A';
        return mask;
    }

    /***********************************************************************************\

    Function:
        GetMemoryPropertyFlagNames

    Description:

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetMemoryPropertyFlagNames(
        VkMemoryPropertyFlags flags,
        const char* separator ) const
    {
        FlagsStringBuilder builder;
        builder.m_pSeparator = separator;

        if( flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT )
            builder.AddFlag( "Device local" );
        if( flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT )
            builder.AddFlag( "Host visible" );
        if( flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT )
            builder.AddFlag( "Host coherent" );
        if( flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT )
            builder.AddFlag( "Host cached" );
        if( flags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT )
            builder.AddFlag( "Lazily allocated" );
        if( flags & VK_MEMORY_PROPERTY_PROTECTED_BIT )
            builder.AddFlag( "Protected" );
        if( flags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD )
            builder.AddFlag( "Device coherent" );
        if( flags & VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD )
            builder.AddFlag( "Device uncached" );
        if( flags & VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV )
            builder.AddFlag( "RDMA capable" );

        builder.AddUnknownFlags( flags, 9 );

        return builder.BuildString();
    }

    /***********************************************************************************\

    Function:
        GetBufferUsageFlagNames

    Description:

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetBufferUsageFlagNames(
        VkBufferUsageFlags flags,
        const char* separator ) const
    {
        FlagsStringBuilder builder;
        builder.m_pSeparator = separator;

        if( flags & VK_BUFFER_USAGE_TRANSFER_SRC_BIT )
            builder.AddFlag( "Transfer source" );
        if( flags & VK_BUFFER_USAGE_TRANSFER_DST_BIT )
            builder.AddFlag( "Transfer destination" );
        if( flags & VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT )
            builder.AddFlag( "Uniform texel buffer" );
        if( flags & VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT )
            builder.AddFlag( "Storage texel buffer" );
        if( flags & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT )
            builder.AddFlag( "Uniform buffer" );
        if( flags & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT )
            builder.AddFlag( "Storage buffer" );
        if( flags & VK_BUFFER_USAGE_INDEX_BUFFER_BIT )
            builder.AddFlag( "Index buffer" );
        if( flags & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT )
            builder.AddFlag( "Vertex buffer" );
        if( flags & VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT )
            builder.AddFlag( "Indirect buffer" );
        if( flags & VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT )
            builder.AddFlag( "Conditional rendering" );
        if( flags & VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR )
            builder.AddFlag( "Shader binding table" );
        if( flags & VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT )
            builder.AddFlag( "Transform feedback buffer" );
        if( flags & VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_COUNTER_BUFFER_BIT_EXT )
            builder.AddFlag( "Transform feedback counter buffer" );
        if( flags & VK_BUFFER_USAGE_VIDEO_DECODE_SRC_BIT_KHR )
            builder.AddFlag( "Video decode source" );
        if( flags & VK_BUFFER_USAGE_VIDEO_DECODE_DST_BIT_KHR )
            builder.AddFlag( "Video decode destination" );
        if( flags & VK_BUFFER_USAGE_VIDEO_ENCODE_DST_BIT_KHR )
            builder.AddFlag( "Video encode destination" );
        if( flags & VK_BUFFER_USAGE_VIDEO_ENCODE_SRC_BIT_KHR )
            builder.AddFlag( "Video encode source" );
        if( flags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT )
            builder.AddFlag( "Shader device address" );
        if( flags & VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR )
            builder.AddFlag( "Acceleration structure build input read-only" );
        if( flags & VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR )
            builder.AddFlag( "Acceleration structure storage" );
        if( flags & VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT )
            builder.AddFlag( "Sampler descriptor buffer" );
        if( flags & VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT )
            builder.AddFlag( "Resource descriptor buffer" );
        if( flags & VK_BUFFER_USAGE_MICROMAP_BUILD_INPUT_READ_ONLY_BIT_EXT )
            builder.AddFlag( "Micromap build input read-only" );
        if( flags & VK_BUFFER_USAGE_MICROMAP_STORAGE_BIT_EXT )
            builder.AddFlag( "Micromap storage" );
        if( flags & VK_BUFFER_USAGE_PUSH_DESCRIPTORS_DESCRIPTOR_BUFFER_BIT_EXT )
            builder.AddFlag( "Push descriptors descriptor buffer" );

        builder.AddUnknownFlags( flags, 27 );

        return builder.BuildString();
    }

    /***********************************************************************************\

    Function:
        GetImageUsageFlagNames

    Description:

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetImageUsageFlagNames(
        VkImageUsageFlags flags,
        const char* separator ) const
    {
        FlagsStringBuilder builder;
        builder.m_pSeparator = separator;

        if( flags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT )
            builder.AddFlag( "Transfer source" );
        if( flags & VK_IMAGE_USAGE_TRANSFER_DST_BIT )
            builder.AddFlag( "Transfer destination" );
        if( flags & VK_IMAGE_USAGE_SAMPLED_BIT )
            builder.AddFlag( "Sampled image" );
        if( flags & VK_IMAGE_USAGE_STORAGE_BIT )
            builder.AddFlag( "Storage image" );
        if( flags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT )
            builder.AddFlag( "Color attachment" );
        if( flags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT )
            builder.AddFlag( "Depth-stencil attachment" );
        if( flags & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT )
            builder.AddFlag( "Transient attachment" );
        if( flags & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT )
            builder.AddFlag( "Input attachment" );
        if( flags & VK_IMAGE_USAGE_HOST_TRANSFER_BIT )
            builder.AddFlag( "Host transfer" );
        if( flags & VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR )
            builder.AddFlag( "Video decode destination" );
        if( flags & VK_IMAGE_USAGE_VIDEO_DECODE_SRC_BIT_KHR )
            builder.AddFlag( "Video decode source" );
        if( flags & VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR )
            builder.AddFlag( "Video decoded picture buffer" );
        if( flags & VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT )
            builder.AddFlag( "Fragment density map" );
        if( flags & VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR )
            builder.AddFlag( "Fragment shading rate attachment" );
        if( flags & VK_IMAGE_USAGE_VIDEO_ENCODE_DST_BIT_KHR )
            builder.AddFlag( "Video encode destination" );
        if( flags & VK_IMAGE_USAGE_VIDEO_ENCODE_SRC_BIT_KHR )
            builder.AddFlag( "Video encode source" );
        if( flags & VK_IMAGE_USAGE_VIDEO_ENCODE_DPB_BIT_KHR )
            builder.AddFlag( "Video encode decoded picture buffer" );
        if( flags & VK_IMAGE_USAGE_ATTACHMENT_FEEDBACK_LOOP_BIT_EXT )
            builder.AddFlag( "Attachment feedback loop" );
        if( flags & VK_IMAGE_USAGE_INVOCATION_MASK_BIT_HUAWEI )
            builder.AddFlag( "Invocation mask" );
        if( flags & VK_IMAGE_USAGE_SAMPLE_WEIGHT_BIT_QCOM )
            builder.AddFlag( "Sample weight image" );
        if( flags & VK_IMAGE_USAGE_SAMPLE_BLOCK_MATCH_BIT_QCOM )
            builder.AddFlag( "Sample block match image" );
        if( flags & VK_IMAGE_USAGE_VIDEO_ENCODE_QUANTIZATION_DELTA_MAP_BIT_KHR )
            builder.AddFlag( "Video encode quantization delta map" );
        if( flags & VK_IMAGE_USAGE_VIDEO_ENCODE_EMPHASIS_MAP_BIT_KHR )
            builder.AddFlag( "Video encode emphasis map" );

        builder.AddUnknownFlags( flags, 27 );

        return builder.BuildString();
    }

    /***********************************************************************************\

    Function:
        GetImageTypeName

    Description:

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetImageTypeName(
        VkImageType type,
        VkImageCreateFlags flags,
        uint32_t arrayLayers ) const
    {
        std::string typeName;
        switch( type )
        {
        case VK_IMAGE_TYPE_1D:
            typeName = "Image 1D";
            break;
        case VK_IMAGE_TYPE_2D:
            typeName = "Image 2D";
            break;
        case VK_IMAGE_TYPE_3D:
            typeName = "Image 3D";
            break;
        default:
            return fmt::format( "Unknown ({})", static_cast<uint32_t>( type ) );
        }

        if( flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT )
        {
            typeName += " Cube";
            arrayLayers /= 6;
        }

        if( arrayLayers > 1 )
        {
            typeName += " Array";
        }

        return typeName;
    }

    /***********************************************************************************\

    Function:
        GetImageTilingName

    Description:

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetImageTilingName(
        VkImageTiling tiling ) const
    {
        switch( tiling )
        {
        case VK_IMAGE_TILING_OPTIMAL:
            return "Optimal";
        case VK_IMAGE_TILING_LINEAR:
            return "Linear";
        case VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT:
            return "DRM format modifier";
        }
        return fmt::format( "Unknown tiling ({})", static_cast<uint32_t>( tiling ) );
    }

    /***********************************************************************************\

    Function:
        GetImageTypeName

    Description:

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetImageAspectFlagNames(
        VkImageAspectFlags flags,
        const char* separator ) const
    {
        FlagsStringBuilder builder;
        builder.m_pSeparator = separator;

        if( flags & VK_IMAGE_ASPECT_COLOR_BIT )
            builder.AddFlag( "Color" );
        if( flags & VK_IMAGE_ASPECT_DEPTH_BIT )
            builder.AddFlag( "Depth" );
        if( flags & VK_IMAGE_ASPECT_STENCIL_BIT )
            builder.AddFlag( "Stencil" );
        if( flags & VK_IMAGE_ASPECT_METADATA_BIT )
            builder.AddFlag( "Metadata" );
        if( flags & VK_IMAGE_ASPECT_PLANE_0_BIT )
            builder.AddFlag( "Plane 0" );
        if( flags & VK_IMAGE_ASPECT_PLANE_1_BIT )
            builder.AddFlag( "Plane 1" );
        if( flags & VK_IMAGE_ASPECT_PLANE_2_BIT )
            builder.AddFlag( "Plane 2" );
        if( flags & VK_IMAGE_ASPECT_MEMORY_PLANE_0_BIT_EXT )
            builder.AddFlag( "Memory plane 0" );
        if( flags & VK_IMAGE_ASPECT_MEMORY_PLANE_1_BIT_EXT )
            builder.AddFlag( "Memory plane 1" );
        if( flags & VK_IMAGE_ASPECT_MEMORY_PLANE_2_BIT_EXT )
            builder.AddFlag( "Memory plane 2" );
        if( flags & VK_IMAGE_ASPECT_MEMORY_PLANE_3_BIT_EXT )
            builder.AddFlag( "Memory plane 3" );

        builder.AddUnknownFlags( flags, 11 );

        return builder.BuildString();
    }

    /***********************************************************************************\

    Function:
        GetCopyAccelerationStructureModeName

    Description:

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetCopyAccelerationStructureModeName(
        VkCopyAccelerationStructureModeKHR mode ) const
    {
        switch( mode )
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
        return fmt::format( "Unknown mode ({})", static_cast<uint32_t>( mode ) );
    }

    /***********************************************************************************\

    Function:
        GetAccelerationStructureTypeName

    Description:

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetAccelerationStructureTypeName(
        VkAccelerationStructureTypeKHR type ) const
    {
        switch( type )
        {
        case VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR:
            return "Top-level";
        case VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR:
            return "Bottom-level";
        case VK_ACCELERATION_STRUCTURE_TYPE_GENERIC_KHR:
            return "Generic";
        }
        return fmt::format( "Unknown type ({})", static_cast<uint32_t>( type ) );
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

        if( flags & VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR )
            builder.AddFlag( "Allow update (1)" );
        if( flags & VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR )
            builder.AddFlag( "Allow compaction (2)" );
        if( flags & VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR )
            builder.AddFlag( "Prefer fast trace (4)" );
        if( flags & VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR )
            builder.AddFlag( "Prefer fast build (8)" );
        if( flags & VK_BUILD_ACCELERATION_STRUCTURE_LOW_MEMORY_BIT_KHR )
            builder.AddFlag( "Low memory (16)" );
        if( flags & VK_BUILD_ACCELERATION_STRUCTURE_MOTION_BIT_NV )
            builder.AddFlag( "Motion (32)" );

        builder.AddUnknownFlags( flags, 6 );

        return builder.BuildString();
    }

    /***********************************************************************************\

    Function:
        GetBuildAccelerationStructureModeName

    Description:

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetBuildAccelerationStructureModeName(
        VkBuildAccelerationStructureModeKHR mode ) const
    {
        switch( mode )
        {
        case VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR:
            return "Build";
        case VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR:
            return "Update";
        }
        return fmt::format( "Unknown mode ({})", static_cast<uint32_t>( mode ) );
    }

    /***********************************************************************************\

    Function:
        GetCopyMicromapModeName

    Description:

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetCopyMicromapModeName(
        VkCopyMicromapModeEXT mode ) const
    {
        switch( mode )
        {
        case VK_COPY_MICROMAP_MODE_CLONE_EXT:
            return "Clone";
        case VK_COPY_MICROMAP_MODE_SERIALIZE_EXT:
            return "Serialize";
        case VK_COPY_MICROMAP_MODE_DESERIALIZE_EXT:
            return "Deserialize";
        case VK_COPY_MICROMAP_MODE_COMPACT_EXT:
            return "Compact";
        }
        return fmt::format( "Unknown mode ({})", static_cast<uint32_t>( mode ) );
    }

    /***********************************************************************************\

    Function:
        GetMicromapTypeName

    Description:

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetMicromapTypeName(
        VkMicromapTypeEXT type ) const
    {
        switch( type )
        {
        case VK_MICROMAP_TYPE_OPACITY_MICROMAP_EXT:
            return "Opacity Micromap";
        }
        return fmt::format( "Unknown type ({})", static_cast<uint32_t>( type ) );
    }

    /***********************************************************************************\

    Function:
        GetBuildMicromapModeName

    Description:

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetBuildMicromapModeName(
        VkBuildMicromapModeEXT mode ) const
    {
        switch( mode )
        {
        case VK_BUILD_MICROMAP_MODE_BUILD_EXT:
            return "Build";
        }
        return fmt::format( "Unknown mode ({})", static_cast<uint32_t>( mode ) );
    }

    /***********************************************************************************\

    Function:
        GetBuildMicromapFlagNames

    Description:

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetBuildMicromapFlagNames(
        VkBuildMicromapFlagsEXT flags ) const
    {
        FlagsStringBuilder builder;

        if( flags & VK_BUILD_MICROMAP_PREFER_FAST_TRACE_BIT_EXT )
            builder.AddFlag( "Prefer fast trace (1)" );
        if( flags & VK_BUILD_MICROMAP_PREFER_FAST_BUILD_BIT_EXT )
            builder.AddFlag( "Prefer fast build (2)" );
        if( flags & VK_BUILD_MICROMAP_ALLOW_COMPACTION_BIT_EXT )
            builder.AddFlag( "Allow compaction (4)" );

        for( uint32_t i = 3; i < 8 * sizeof( flags ); ++i )
        {
            uint32_t unkownFlag = 1U << i;
            if( flags & unkownFlag )
                builder.AddFlag( fmt::format( "Unknown flag ({})", unkownFlag ) );
        }

        return builder.BuildString();
    }

    /***********************************************************************************\

    Function:
        GetGeometryTypeName

    Description:

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetGeometryTypeName( VkGeometryTypeKHR type ) const
    {
        switch( type )
        {
        case VK_GEOMETRY_TYPE_TRIANGLES_KHR:
            return "Triangles";
        case VK_GEOMETRY_TYPE_AABBS_KHR:
            return "AABBs";
        case VK_GEOMETRY_TYPE_INSTANCES_KHR:
            return "Instances";
        }
        return fmt::format( "Unknown type ({})", static_cast<uint32_t>( type ) );
    }

    /***********************************************************************************\

    Function:
        GetGeometryFlagNames

    Description:

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetGeometryFlagNames( VkGeometryFlagsKHR flags ) const
    {
        FlagsStringBuilder builder;

        if( flags & VK_GEOMETRY_OPAQUE_BIT_KHR )
            builder.AddFlag( "Opaque (1)" );
        if( flags & VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR )
            builder.AddFlag( "No duplicate any-hit invocation (2)" );

        builder.AddUnknownFlags( flags, 2 );

        return builder.BuildString();
    }
}

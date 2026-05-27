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

#include "profiler_string_serializer.h"
#include "profiler/profiler_data.h"
#include "profiler/profiler_frontend.h"
#include "profiler/profiler_helpers.h"
#include "profiler_layer_objects/VkDevice_object.h"
#include <fmt/format.h>
#include <sstream>

namespace Profiler
{
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

        case DeviceProfilerDrawcallType::eDrawMulti:
            return fmt::format( "vkCmdDrawMultiEXT ({}, {}, {}, {})",
                drawcall.m_Payload.m_DrawMulti.m_DrawCount,
                drawcall.m_Payload.m_DrawMulti.m_InstanceCount,
                drawcall.m_Payload.m_DrawMulti.m_FirstInstance,
                drawcall.m_Payload.m_DrawMulti.m_Stride );

        case DeviceProfilerDrawcallType::eDrawMultiIndexed:
            return fmt::format( "vkCmdDrawMultiIndexedEXT ({}, {}, {}, {})",
                drawcall.m_Payload.m_DrawMultiIndexed.m_DrawCount,
                drawcall.m_Payload.m_DrawMultiIndexed.m_InstanceCount,
                drawcall.m_Payload.m_DrawMultiIndexed.m_FirstInstance,
                drawcall.m_Payload.m_DrawMultiIndexed.m_Stride );

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
                drawcall.m_Payload.m_ClearAttachments.m_Count );

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
    std::string DeviceProfilerStringSerializer::GetName( const DeviceProfilerPipelineData& pipeline, bool showEntryPoints ) const
    {
        // Use assigned name if available.
        if( pipeline.m_Handle != VK_NULL_HANDLE )
        {
            auto name = m_Frontend.GetObjectName( pipeline.m_Handle );
            if( !name.empty() )
            {
                return name;
            }
        }

        // Construct the pipeline's name dynamically from the shaders.
        switch( pipeline.m_BindPoint )
        {
        case VK_PIPELINE_BIND_POINT_GRAPHICS:
        {
            return pipeline.m_ShaderTuple.GetShaderStageHashesString(
                VK_SHADER_STAGE_VERTEX_BIT |
                VK_SHADER_STAGE_TASK_BIT_EXT |
                VK_SHADER_STAGE_MESH_BIT_EXT |
                VK_SHADER_STAGE_FRAGMENT_BIT,
                showEntryPoints,
                true /*skipEmptyStages*/ );
        }

        case VK_PIPELINE_BIND_POINT_COMPUTE:
        {
            return pipeline.m_ShaderTuple.GetShaderStageHashesString(
                VK_SHADER_STAGE_COMPUTE_BIT,
                showEntryPoints );
        }

        case VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR:
        {
            return pipeline.m_ShaderTuple.GetShaderStageHashesString(
                VK_SHADER_STAGE_RAYGEN_BIT_KHR |
                VK_SHADER_STAGE_ANY_HIT_BIT_KHR |
                VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
                showEntryPoints );
        }
        }

        // Unknown pipeline bind point.
        return fmt::format( "VkPipeline {:#018x}", pipeline.m_Handle.GetHandleAsUint64() );
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
            VkObjectRuntimeTraits::FromObjectType( object.m_Type ).ObjectTypeName,
            object.GetHandleAsUint64() );
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
            object.GetHandleAsUint64(),
            object.m_CreateTime );
    }

    /***********************************************************************************\

    Function:
        GetObjectTypeName

    Description:
        Returns pretty Vulkan object type name.

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetObjectTypeName( VkObjectType objectType ) const
    {
        switch( objectType )
        {
        default:
        {
            auto traits = VkObjectRuntimeTraits::FromObjectType( objectType );
            if( traits.ObjectType == objectType )
            {
                std::string typeName = traits.ObjectTypeName + 2; // Skip "Vk" prefix.

                // Strip extension suffix.
                while( typeName.length() > 1 && isupper( typeName.back() ) )
                {
                    typeName.pop_back();
                }

                // Insert spaces before uppercase letters to make it more readable.
                for( auto it = typeName.begin() + 1; it != typeName.end(); ++it )
                {
                    if( isupper( *it ) )
                    {
                        it = typeName.insert( it, ' ' ) + 1;
                    }
                }

                return typeName;
            }

            return fmt::format( "Unknown Object Type ({})", static_cast<uint32_t>( objectType ) );
        }

        case VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR:
            return "Ray-Tracing Acceleration Structure";
        }
    }

    /***********************************************************************************\

    Function:
        GetShortObjectTypeName

    Description:
        Returns short Vulkan object type name.

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetShortObjectTypeName( VkObjectType objectType ) const
    {
        switch( objectType )
        {
        default:
        {
            auto traits = VkObjectRuntimeTraits::FromObjectType( objectType );
            if( traits.ObjectType == objectType )
            {
                return GetObjectTypeName( objectType );
            }
            return "Unknown";
        }

        case VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR:
            return "RTAS";
        }
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

        case DeviceProfilerDrawcallType::eDrawMulti:
            return "vkCmdDrawMultiEXT";

        case DeviceProfilerDrawcallType::eDrawMultiIndexed:
            return "vkCmdDrawMultiIndexedEXT";

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
        GetVersionString

    Description:
        Returns string representation of a version encoded using Vulkan macros.

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetVersionString( uint32_t version ) const
    {
        return fmt::format( "{}.{}.{}",
            VK_API_VERSION_MAJOR( version ),
            VK_API_VERSION_MINOR( version ),
            VK_API_VERSION_PATCH( version ) );
    }

    /***********************************************************************************\

    Function:
        GetVersion

    Description:
        Read version encoded in a string and returns it as a uint32_t encoded using
        Vulkan macros.

    \***********************************************************************************/
    uint32_t DeviceProfilerStringSerializer::GetVersion( const std::string_view& str ) const
    {
        uint32_t variant = 0, major = 0, minor = 0, patch = 0;

        int result = sscanf( str.data(), "%u.%u.%u.%u", &variant, &major, &minor, &patch );
        if( result == 3 )
        {
            return VK_MAKE_API_VERSION( 0, variant, major, minor );
        }

        return VK_MAKE_API_VERSION( variant, major, minor, patch );
    }

    /***********************************************************************************\

    Function:
        GetQueueTypeName

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
        GetColorComponentFlags

    Description:

    \***********************************************************************************/
    VkColorComponentFlags DeviceProfilerStringSerializer::GetColorComponentFlags( const std::string_view& str ) const
    {
        VkColorComponentFlags flags = 0;

        for( char c : str )
        {
            switch( c )
            {
            case 'R': flags |= VK_COLOR_COMPONENT_R_BIT; break;
            case 'G': flags |= VK_COLOR_COMPONENT_G_BIT; break;
            case 'B': flags |= VK_COLOR_COMPONENT_B_BIT; break;
            case 'A': flags |= VK_COLOR_COMPONENT_A_BIT; break;
            }
        }

        return flags;
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
        GetImageType

    Description:

    \***********************************************************************************/
    VkImageType DeviceProfilerStringSerializer::GetImageType( const std::string_view& str ) const
    {
        switch( FNV( str.substr( 0, 8 ) ) )
        {
        case FNV( "Image 1D" ):
            return VK_IMAGE_TYPE_1D;
        case FNV( "Image 2D" ):
            return VK_IMAGE_TYPE_2D;
        case FNV( "Image 3D" ):
            return VK_IMAGE_TYPE_3D;
        }

        return static_cast<VkImageType>( 0 );
    }
}

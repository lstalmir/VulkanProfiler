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

#include "profiler_string_mappings.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        MapValueToString

    Description:
        Return name of the provided key from the mapping.
        If the key is not found, return a formatted string with the unknown value.

    \***********************************************************************************/
    template<typename T, typename U>
    static std::string MapValueToString( const T& mapping, U key, const fmt::format_string<char>& unknownValueFormat = "Unknown ({})" )
    {
        std::string_view name = mapping[key];
        if( !name.empty() )
        {
            return std::string( name );
        }

        return fmt::format( unknownValueFormat, static_cast<uint64_t>( key ) );
    }

    /***********************************************************************************\

    Function:
        MapFlagsToString

    Description:
        Build a string with names of all flags set in the provided bitmask.
        Unknown flags are also reported.

    \***********************************************************************************/
    template<typename T>
    static std::string MapFlagsToString( const T& mapping, uint64_t flags, const std::string_view& separator )
    {
        std::stringstream stream;
        bool streamEmpty = true;

        uint64_t knownFlags = 0;
        mapping.Apply(
            [&]( uint64_t bit, std::string_view name )
            {
                knownFlags |= bit;
                if( flags & bit )
                {
                    if( !streamEmpty )
                    {
                        stream << separator;
                    }

                    stream << name;
                    streamEmpty = false;
                }
            } );

        for( uint32_t i = 0; i < sizeof( uint64_t ) * 8; ++i )
        {
            uint64_t bit = ( 1ULL << i );
            if( ( flags & bit ) && !( knownFlags & bit ) )
            {
                if( !streamEmpty )
                {
                    stream << separator;
                }

                stream << "Unknown flag (" << bit << ")";
                streamEmpty = false;
            }
        }

        return stream.str();
    }

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
            return fmt::format( "{} ({}, {}, {}, {})",
                g_scProfilerDrawcallTypeNames[DeviceProfilerDrawcallType::eDraw],
                drawcall.m_Payload.m_Draw.m_VertexCount,
                drawcall.m_Payload.m_Draw.m_InstanceCount,
                drawcall.m_Payload.m_Draw.m_FirstVertex,
                drawcall.m_Payload.m_Draw.m_FirstInstance );

        case DeviceProfilerDrawcallType::eDrawIndexed:
            return fmt::format( "{} ({}, {}, {}, {}, {})",
                g_scProfilerDrawcallTypeNames[DeviceProfilerDrawcallType::eDrawIndexed],
                drawcall.m_Payload.m_DrawIndexed.m_IndexCount,
                drawcall.m_Payload.m_DrawIndexed.m_InstanceCount,
                drawcall.m_Payload.m_DrawIndexed.m_FirstIndex,
                drawcall.m_Payload.m_DrawIndexed.m_VertexOffset,
                drawcall.m_Payload.m_DrawIndexed.m_FirstInstance );

        case DeviceProfilerDrawcallType::eDrawIndirect:
            return fmt::format( "{} ({}, {}, {}, {})",
                g_scProfilerDrawcallTypeNames[DeviceProfilerDrawcallType::eDrawIndirect],
                GetName( drawcall.m_Payload.m_DrawIndirect.m_Buffer ),
                drawcall.m_Payload.m_DrawIndirect.m_Offset,
                drawcall.m_Payload.m_DrawIndirect.m_DrawCount,
                drawcall.m_Payload.m_DrawIndirect.m_Stride );

        case DeviceProfilerDrawcallType::eDrawIndexedIndirect:
            return fmt::format( "{} ({}, {}, {}, {})",
                g_scProfilerDrawcallTypeNames[DeviceProfilerDrawcallType::eDrawIndexedIndirect],
                GetName( drawcall.m_Payload.m_DrawIndexedIndirect.m_Buffer ),
                drawcall.m_Payload.m_DrawIndexedIndirect.m_Offset,
                drawcall.m_Payload.m_DrawIndexedIndirect.m_DrawCount,
                drawcall.m_Payload.m_DrawIndexedIndirect.m_Stride );

        case DeviceProfilerDrawcallType::eDrawIndirectCount:
            return fmt::format( "{} ({}, {}, {}, {}, {}, {})",
                g_scProfilerDrawcallTypeNames[DeviceProfilerDrawcallType::eDrawIndirectCount],
                GetName( drawcall.m_Payload.m_DrawIndirectCount.m_Buffer ),
                drawcall.m_Payload.m_DrawIndirectCount.m_Offset,
                GetName( drawcall.m_Payload.m_DrawIndirectCount.m_CountBuffer ),
                drawcall.m_Payload.m_DrawIndirectCount.m_CountOffset,
                drawcall.m_Payload.m_DrawIndirectCount.m_MaxDrawCount,
                drawcall.m_Payload.m_DrawIndirectCount.m_Stride );

        case DeviceProfilerDrawcallType::eDrawIndexedIndirectCount:
            return fmt::format( "{} ({}, {}, {}, {}, {}, {})",
                g_scProfilerDrawcallTypeNames[DeviceProfilerDrawcallType::eDrawIndexedIndirectCount],
                GetName( drawcall.m_Payload.m_DrawIndexedIndirectCount.m_Buffer ),
                drawcall.m_Payload.m_DrawIndexedIndirectCount.m_Offset,
                GetName( drawcall.m_Payload.m_DrawIndexedIndirectCount.m_CountBuffer ),
                drawcall.m_Payload.m_DrawIndexedIndirectCount.m_CountOffset,
                drawcall.m_Payload.m_DrawIndexedIndirectCount.m_MaxDrawCount,
                drawcall.m_Payload.m_DrawIndexedIndirectCount.m_Stride );

        case DeviceProfilerDrawcallType::eDrawMeshTasks:
            return fmt::format( "{} ({}, {}, {})",
                g_scProfilerDrawcallTypeNames[DeviceProfilerDrawcallType::eDrawMeshTasks],
                drawcall.m_Payload.m_DrawMeshTasks.m_GroupCountX,
                drawcall.m_Payload.m_DrawMeshTasks.m_GroupCountY,
                drawcall.m_Payload.m_DrawMeshTasks.m_GroupCountZ );

        case DeviceProfilerDrawcallType::eDrawMeshTasksIndirect:
            return fmt::format( "{} ({}, {}, {}, {})",
                g_scProfilerDrawcallTypeNames[DeviceProfilerDrawcallType::eDrawMeshTasksIndirect],
                GetName( drawcall.m_Payload.m_DrawMeshTasksIndirect.m_Buffer ),
                drawcall.m_Payload.m_DrawMeshTasksIndirect.m_Offset,
                drawcall.m_Payload.m_DrawMeshTasksIndirect.m_DrawCount,
                drawcall.m_Payload.m_DrawMeshTasksIndirect.m_Stride );

        case DeviceProfilerDrawcallType::eDrawMeshTasksIndirectCount:
            return fmt::format( "{} ({}, {}, {}, {}, {}, {})",
                g_scProfilerDrawcallTypeNames[DeviceProfilerDrawcallType::eDrawMeshTasksIndirectCount],
                GetName( drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_Buffer ),
                drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_Offset,
                GetName( drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_CountBuffer ),
                drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_CountOffset,
                drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_MaxDrawCount,
                drawcall.m_Payload.m_DrawMeshTasksIndirectCount.m_Stride );

        case DeviceProfilerDrawcallType::eDrawMeshTasksNV:
            return fmt::format( "{} ({}, {})",
                g_scProfilerDrawcallTypeNames[DeviceProfilerDrawcallType::eDrawMeshTasksNV],
                drawcall.m_Payload.m_DrawMeshTasksNV.m_TaskCount,
                drawcall.m_Payload.m_DrawMeshTasksNV.m_FirstTask );

        case DeviceProfilerDrawcallType::eDrawMeshTasksIndirectNV:
            return fmt::format( "{} ({}, {}, {}, {})",
                g_scProfilerDrawcallTypeNames[DeviceProfilerDrawcallType::eDrawMeshTasksIndirectNV],
                GetName( drawcall.m_Payload.m_DrawMeshTasksIndirectNV.m_Buffer ),
                drawcall.m_Payload.m_DrawMeshTasksIndirectNV.m_Offset,
                drawcall.m_Payload.m_DrawMeshTasksIndirectNV.m_DrawCount,
                drawcall.m_Payload.m_DrawMeshTasksIndirectNV.m_Stride );

        case DeviceProfilerDrawcallType::eDrawMeshTasksIndirectCountNV:
            return fmt::format( "{} ({}, {}, {}, {}, {}, {})",
                g_scProfilerDrawcallTypeNames[DeviceProfilerDrawcallType::eDrawMeshTasksIndirectCountNV],
                GetName( drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_Buffer ),
                drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_Offset,
                GetName( drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_CountBuffer ),
                drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_CountOffset,
                drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_MaxDrawCount,
                drawcall.m_Payload.m_DrawMeshTasksIndirectCountNV.m_Stride );

        case DeviceProfilerDrawcallType::eDrawMulti:
            return fmt::format( "{} ({}, {}, {}, {})",
                g_scProfilerDrawcallTypeNames[DeviceProfilerDrawcallType::eDrawMulti],
                drawcall.m_Payload.m_DrawMulti.m_DrawCount,
                drawcall.m_Payload.m_DrawMulti.m_InstanceCount,
                drawcall.m_Payload.m_DrawMulti.m_FirstInstance,
                drawcall.m_Payload.m_DrawMulti.m_Stride );

        case DeviceProfilerDrawcallType::eDrawMultiIndexed:
            return fmt::format( "{} ({}, {}, {}, {})",
                g_scProfilerDrawcallTypeNames[DeviceProfilerDrawcallType::eDrawMultiIndexed],
                drawcall.m_Payload.m_DrawMultiIndexed.m_DrawCount,
                drawcall.m_Payload.m_DrawMultiIndexed.m_InstanceCount,
                drawcall.m_Payload.m_DrawMultiIndexed.m_FirstInstance,
                drawcall.m_Payload.m_DrawMultiIndexed.m_Stride );

        case DeviceProfilerDrawcallType::eDispatch:
            return fmt::format( "{} ({}, {}, {})",
                g_scProfilerDrawcallTypeNames[DeviceProfilerDrawcallType::eDispatch],
                drawcall.m_Payload.m_Dispatch.m_GroupCountX,
                drawcall.m_Payload.m_Dispatch.m_GroupCountY,
                drawcall.m_Payload.m_Dispatch.m_GroupCountZ );

        case DeviceProfilerDrawcallType::eDispatchIndirect:
            return fmt::format( "{} ({}, {})",
                g_scProfilerDrawcallTypeNames[DeviceProfilerDrawcallType::eDispatchIndirect],
                GetName( drawcall.m_Payload.m_DispatchIndirect.m_Buffer ),
                drawcall.m_Payload.m_DispatchIndirect.m_Offset );

        case DeviceProfilerDrawcallType::eCopyBuffer:
            return fmt::format( "{} ({}, {})",
                g_scProfilerDrawcallTypeNames[DeviceProfilerDrawcallType::eCopyBuffer],
                GetName( drawcall.m_Payload.m_CopyBuffer.m_SrcBuffer ),
                GetName( drawcall.m_Payload.m_CopyBuffer.m_DstBuffer ) );

        case DeviceProfilerDrawcallType::eCopyBufferToImage:
            return fmt::format( "{} ({}, {})",
                g_scProfilerDrawcallTypeNames[DeviceProfilerDrawcallType::eCopyBufferToImage],
                GetName( drawcall.m_Payload.m_CopyBufferToImage.m_SrcBuffer ),
                GetName( drawcall.m_Payload.m_CopyBufferToImage.m_DstImage ) );

        case DeviceProfilerDrawcallType::eCopyImage:
            return fmt::format( "{} ({}, {})",
                g_scProfilerDrawcallTypeNames[DeviceProfilerDrawcallType::eCopyImage],
                GetName( drawcall.m_Payload.m_CopyImage.m_SrcImage ),
                GetName( drawcall.m_Payload.m_CopyImage.m_DstImage ) );

        case DeviceProfilerDrawcallType::eCopyImageToBuffer:
            return fmt::format( "{} ({}, {})",
                g_scProfilerDrawcallTypeNames[DeviceProfilerDrawcallType::eCopyImageToBuffer],
                GetName( drawcall.m_Payload.m_CopyImageToBuffer.m_SrcImage ),
                GetName( drawcall.m_Payload.m_CopyImageToBuffer.m_DstBuffer ) );

        case DeviceProfilerDrawcallType::eClearAttachments:
            return fmt::format( "{} ({})",
                g_scProfilerDrawcallTypeNames[DeviceProfilerDrawcallType::eClearAttachments],
                drawcall.m_Payload.m_ClearAttachments.m_Count );

        case DeviceProfilerDrawcallType::eClearColorImage:
            return fmt::format( "{} ({}, C=[{}, {}, {}, {}])",
                g_scProfilerDrawcallTypeNames[DeviceProfilerDrawcallType::eClearColorImage],
                GetName( drawcall.m_Payload.m_ClearColorImage.m_Image ),
                drawcall.m_Payload.m_ClearColorImage.m_Value.float32[0],
                drawcall.m_Payload.m_ClearColorImage.m_Value.float32[1],
                drawcall.m_Payload.m_ClearColorImage.m_Value.float32[2],
                drawcall.m_Payload.m_ClearColorImage.m_Value.float32[3] );

        case DeviceProfilerDrawcallType::eClearDepthStencilImage:
            return fmt::format( "{} ({}, D={}, S={})",
                g_scProfilerDrawcallTypeNames[DeviceProfilerDrawcallType::eClearDepthStencilImage],
                GetName( drawcall.m_Payload.m_ClearDepthStencilImage.m_Image ),
                drawcall.m_Payload.m_ClearDepthStencilImage.m_Value.depth,
                drawcall.m_Payload.m_ClearDepthStencilImage.m_Value.stencil );

        case DeviceProfilerDrawcallType::eResolveImage:
            return fmt::format( "{} ({}, {})",
                g_scProfilerDrawcallTypeNames[DeviceProfilerDrawcallType::eResolveImage],
                GetName( drawcall.m_Payload.m_ResolveImage.m_SrcImage ),
                GetName( drawcall.m_Payload.m_ResolveImage.m_DstImage ) );

        case DeviceProfilerDrawcallType::eBlitImage:
            return fmt::format( "{} ({}, {})",
                g_scProfilerDrawcallTypeNames[DeviceProfilerDrawcallType::eBlitImage],
                GetName( drawcall.m_Payload.m_BlitImage.m_SrcImage ),
                GetName( drawcall.m_Payload.m_BlitImage.m_DstImage ) );

        case DeviceProfilerDrawcallType::eFillBuffer:
            return fmt::format( "{} ({}, {}, {}, {})",
                g_scProfilerDrawcallTypeNames[DeviceProfilerDrawcallType::eFillBuffer],
                GetName( drawcall.m_Payload.m_FillBuffer.m_Buffer ),
                drawcall.m_Payload.m_FillBuffer.m_Offset,
                drawcall.m_Payload.m_FillBuffer.m_Size,
                drawcall.m_Payload.m_FillBuffer.m_Data );

        case DeviceProfilerDrawcallType::eUpdateBuffer:
            return fmt::format( "{} ({}, {}, {})",
                g_scProfilerDrawcallTypeNames[DeviceProfilerDrawcallType::eUpdateBuffer],
                GetName( drawcall.m_Payload.m_UpdateBuffer.m_Buffer ),
                drawcall.m_Payload.m_UpdateBuffer.m_Offset,
                drawcall.m_Payload.m_UpdateBuffer.m_Size );

        case DeviceProfilerDrawcallType::eTraceRaysKHR:
            return fmt::format( "{} ({}, {}, {})",
                g_scProfilerDrawcallTypeNames[DeviceProfilerDrawcallType::eTraceRaysKHR],
                drawcall.m_Payload.m_TraceRays.m_Width,
                drawcall.m_Payload.m_TraceRays.m_Height,
                drawcall.m_Payload.m_TraceRays.m_Depth );

        case DeviceProfilerDrawcallType::eTraceRaysIndirectKHR:
            return fmt::format( "{} (0x{:16x})",
                g_scProfilerDrawcallTypeNames[DeviceProfilerDrawcallType::eTraceRaysIndirectKHR],
                drawcall.m_Payload.m_TraceRaysIndirect.m_IndirectAddress );

        case DeviceProfilerDrawcallType::eTraceRaysIndirect2KHR:
            return fmt::format( "{} (0x{:16x})",
                g_scProfilerDrawcallTypeNames[DeviceProfilerDrawcallType::eTraceRaysIndirect2KHR],
                drawcall.m_Payload.m_TraceRaysIndirect2.m_IndirectAddress );

        case DeviceProfilerDrawcallType::eBuildAccelerationStructuresKHR:
            return fmt::format( "{} ({})",
                g_scProfilerDrawcallTypeNames[DeviceProfilerDrawcallType::eBuildAccelerationStructuresKHR],
                drawcall.m_Payload.m_BuildAccelerationStructures.m_InfoCount );

        case DeviceProfilerDrawcallType::eBuildAccelerationStructuresIndirectKHR:
            return fmt::format( "{} ({})",
                g_scProfilerDrawcallTypeNames[DeviceProfilerDrawcallType::eBuildAccelerationStructuresIndirectKHR],
                drawcall.m_Payload.m_BuildAccelerationStructures.m_InfoCount );

        case DeviceProfilerDrawcallType::eCopyAccelerationStructureKHR:
            return fmt::format( "{} ({}, {}, {})",
                g_scProfilerDrawcallTypeNames[DeviceProfilerDrawcallType::eCopyAccelerationStructureKHR],
                GetName( drawcall.m_Payload.m_CopyAccelerationStructure.m_Src ),
                GetName( drawcall.m_Payload.m_CopyAccelerationStructure.m_Dst ),
                GetCopyAccelerationStructureModeName( drawcall.m_Payload.m_CopyAccelerationStructure.m_Mode ) );

        case DeviceProfilerDrawcallType::eCopyAccelerationStructureToMemoryKHR:
            return fmt::format( "{} ({}, {}, {})",
                g_scProfilerDrawcallTypeNames[DeviceProfilerDrawcallType::eCopyAccelerationStructureToMemoryKHR],
                GetName( drawcall.m_Payload.m_CopyAccelerationStructureToMemory.m_Src ),
                drawcall.m_Payload.m_CopyAccelerationStructureToMemory.m_Dst.hostAddress,
                GetCopyAccelerationStructureModeName( drawcall.m_Payload.m_CopyAccelerationStructure.m_Mode ) );

        case DeviceProfilerDrawcallType::eCopyMemoryToAccelerationStructureKHR:
            return fmt::format( "{} ({}, {}, {})",
                g_scProfilerDrawcallTypeNames[DeviceProfilerDrawcallType::eCopyMemoryToAccelerationStructureKHR],
                drawcall.m_Payload.m_CopyMemoryToAccelerationStructure.m_Src.hostAddress,
                GetName( drawcall.m_Payload.m_CopyMemoryToAccelerationStructure.m_Dst ),
                GetCopyAccelerationStructureModeName( drawcall.m_Payload.m_CopyAccelerationStructure.m_Mode ) );

        case DeviceProfilerDrawcallType::eBuildMicromapsEXT:
            return fmt::format( "{} ({})",
                g_scProfilerDrawcallTypeNames[DeviceProfilerDrawcallType::eBuildMicromapsEXT],
                drawcall.m_Payload.m_BuildMicromaps.m_InfoCount );

        case DeviceProfilerDrawcallType::eCopyMicromapEXT:
            return fmt::format( "{} ({}, {}, {})",
                g_scProfilerDrawcallTypeNames[DeviceProfilerDrawcallType::eCopyMicromapEXT],
                GetName( drawcall.m_Payload.m_CopyMicromap.m_Src ),
                GetName( drawcall.m_Payload.m_CopyMicromap.m_Dst ),
                GetCopyMicromapModeName( drawcall.m_Payload.m_CopyMicromap.m_Mode ) );

        case DeviceProfilerDrawcallType::eCopyMemoryToMicromapEXT:
            return fmt::format( "{} ({}, {}, {})",
                g_scProfilerDrawcallTypeNames[DeviceProfilerDrawcallType::eCopyMemoryToMicromapEXT],
                drawcall.m_Payload.m_CopyMemoryToMicromap.m_Src.hostAddress,
                GetName( drawcall.m_Payload.m_CopyMemoryToMicromap.m_Dst ),
                GetCopyMicromapModeName( drawcall.m_Payload.m_CopyMemoryToMicromap.m_Mode ) );

        case DeviceProfilerDrawcallType::eCopyMicromapToMemoryEXT:
            return fmt::format( "{} ({}, {}, {})",
                g_scProfilerDrawcallTypeNames[DeviceProfilerDrawcallType::eCopyMicromapToMemoryEXT],
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

        auto name = g_scProfilerRenderPassTypeNames[renderPass.m_Type];
        if( !name.empty() )
        {
            renderPassName = name;
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
        return ( !dynamic ) ? "vkCmdBeginRenderPass" : "vkCmdBeginRendering";
    }

    /***********************************************************************************\

    Function:
        GetName

    Description:
        Returns name of the render pass command.

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetName( const DeviceProfilerRenderPassEndData&, bool dynamic ) const
    {
        return ( !dynamic ) ? "vkCmdEndRenderPass" : "vkCmdEndRendering";
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
        return MapValueToString( g_scProfilerDrawcallTypeNames, drawcall.m_Type, "Unknown command ({})" );
    }

    /***********************************************************************************\

    Function:
        GetPointer

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetPointer( const void* ptr ) const
    {
        if( ptr == nullptr )
        {
            return "null";
        }

        char pointer[19] = "0x0000000000000000";
        char* pPointerStr = pointer + 2;
        ProfilerStringFunctions::Hex( pPointerStr, static_cast<uint64_t>( reinterpret_cast<uintptr_t>( ptr ) ) );

        return pointer;
    }

    /***********************************************************************************\

    Function:
        GetBool

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

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetVec4( const float* pValue ) const
    {
        return fmt::format( "{:.2f}, {:.2f}, {:.2f}, {:.2f}",
            pValue[0],
            pValue[1],
            pValue[2],
            pValue[3] );
    }

    /***********************************************************************************\

    Function:
        GetVersion

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetVersion( uint32_t version ) const
    {
        return fmt::format( "{}.{}.{}",
            VK_VERSION_MAJOR( version ),
            VK_VERSION_MINOR( version ),
            VK_VERSION_PATCH( version ) );
    }

    /***********************************************************************************\

    Function:
        GetColorHex

    Description:
        Returns hexadecimal 24-bit color representation (in #RRGGBB format).

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetColorHex( const float* pColor ) const
    {
        const uint8_t R = static_cast<uint8_t>( pColor[0] * 255.f );
        const uint8_t G = static_cast<uint8_t>( pColor[1] * 255.f );
        const uint8_t B = static_cast<uint8_t>( pColor[2] * 255.f );

        char color[8] = "#XXXXXX";
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
        Returns a human-readable string representation of the given byte size,
        using the appropriate unit (B, kB, MB, GB).

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
        GetDeviceTypeName

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetDeviceTypeName( VkPhysicalDeviceType type ) const
    {
        return MapValueToString( g_scVulkanDeviceTypeNames, type );
    }

    /***********************************************************************************\

    Function:
        GetQueueFlagNames

    Description:
        Returns the most suitable queue type name based on the given queue flags.

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetQueueTypeName( VkQueueFlags flags ) const
    {
        if( flags & VK_QUEUE_GRAPHICS_BIT )
            return std::string( g_scVulkanQueueTypeNames[VK_QUEUE_GRAPHICS_BIT] );
        if( flags & VK_QUEUE_COMPUTE_BIT )
            return std::string( g_scVulkanQueueTypeNames[VK_QUEUE_COMPUTE_BIT] );
        if( ( flags & VK_QUEUE_VIDEO_DECODE_BIT_KHR ) || ( flags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR ) )
            return std::string( g_scVulkanQueueTypeNames[VK_QUEUE_VIDEO_DECODE_BIT_KHR | VK_QUEUE_VIDEO_ENCODE_BIT_KHR] );
        if( flags & VK_QUEUE_TRANSFER_BIT )
            return std::string( g_scVulkanQueueTypeNames[VK_QUEUE_TRANSFER_BIT] );
        return "";
    }

    /***********************************************************************************\

    Function:
        GetQueueFlagNames

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetQueueFlagNames( VkQueueFlags flags, const char* separator ) const
    {
        return MapFlagsToString( g_scVulkanQueueFlagNames, flags, separator );
    }

    /***********************************************************************************\

    Function:
        GetShaderName

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

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetShaderStageName( VkShaderStageFlagBits stage ) const
    {
        return MapValueToString( g_scVulkanShaderStageNames, stage, "Unknown shader stage ({})" );
    }

    /***********************************************************************************\

    Function:
        GetShortShaderStageName

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetShortShaderStageName( VkShaderStageFlagBits stage ) const
    {
        return MapValueToString( g_scVulkanShortShaderStageNames, stage, "{}" );
    }

    /***********************************************************************************\

    Function:
        GetShaderGroupTypeName

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetShaderGroupTypeName( VkRayTracingShaderGroupTypeKHR groupType ) const
    {
        return MapValueToString( g_scVulkanRayTracingShaderGroupTypeNames, groupType );
    }

    /***********************************************************************************\

    Function:
        GetGeneralShaderGroupTypeName

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetGeneralShaderGroupTypeName( VkShaderStageFlagBits stage ) const
    {
        return MapValueToString( g_scVulkanRayTracingGeneralShaderGroupTypeNames, stage );
    }

    /***********************************************************************************\

    Function:
        GetFormatName

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetFormatName( VkFormat format ) const
    {
        return MapValueToString( g_scVulkanFormatNames, format );
    }

    /***********************************************************************************\

    Function:
        GetIndexTypeName

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetIndexTypeName( VkIndexType type ) const
    {
        return MapValueToString( g_scVulkanIndexTypeNames, type );
    }

    /***********************************************************************************\

    Function:
        GetVertexInputRateName

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetVertexInputRateName( VkVertexInputRate rate ) const
    {
        return MapValueToString( g_scVulkanVertexInputRateNames, rate );
    }

    /***********************************************************************************\

    Function:
        GetPrimitiveTopologyName

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetPrimitiveTopologyName( VkPrimitiveTopology topology ) const
    {
        return MapValueToString( g_scVulkanPrimitiveTopologyNames, topology );
    }

    /***********************************************************************************\

    Function:
        GetPolygonModeName

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetPolygonModeName( VkPolygonMode mode ) const
    {
        return MapValueToString( g_scVulkanPolygonModeNames, mode );
    }

    /***********************************************************************************\

    Function:
        GetCullModeName

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetCullModeName( VkCullModeFlags mode ) const
    {
        return MapValueToString( g_scVulkanCullModeNames, mode );
    }

    /***********************************************************************************\

    Function:
        GetFrontFaceName

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetFrontFaceName( VkFrontFace mode ) const
    {
        return MapValueToString( g_scVulkanFrontFaceNames, mode );
    }

    /***********************************************************************************\

    Function:
        GetBlendFactorName

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetBlendFactorName( VkBlendFactor factor ) const
    {
        return MapValueToString( g_scVulkanBlendFactorNames, factor );
    }

    /***********************************************************************************\

    Function:
        GetBlendOpName

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetBlendOpName( VkBlendOp op ) const
    {
        return MapValueToString( g_scVulkanBlendOpNames, op );
    }

    /***********************************************************************************\

    Function:
        GetCompareOpName

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetCompareOpName( VkCompareOp op ) const
    {
        return MapValueToString( g_scVulkanCompareOpNames, op );
    }

    /***********************************************************************************\

    Function:
        GetLogicOpName

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetLogicOpName( VkLogicOp op ) const
    {
        return MapValueToString( g_scVulkanLogicOpNames, op );
    }

    /***********************************************************************************\

    Function:
        GetColorComponentFlagNames

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetColorComponentFlagNames( VkColorComponentFlags flags ) const
    {
        char mask[5] = "____";
        if( flags & VK_COLOR_COMPONENT_R_BIT )
            mask[0] = 'R';
        if( flags & VK_COLOR_COMPONENT_G_BIT )
            mask[1] = 'G';
        if( flags & VK_COLOR_COMPONENT_B_BIT )
            mask[2] = 'B';
        if( flags & VK_COLOR_COMPONENT_A_BIT )
            mask[3] = 'A';
        return mask;
    }

    /***********************************************************************************\

    Function:
        GetDynamicStateName

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetDynamicStateName( VkDynamicState state ) const
    {
        return MapValueToString( g_scVulkanDynamicStateNames, state );
    }

    /***********************************************************************************\

    Function:
        GetMemoryPropertyFlagNames

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetMemoryPropertyFlagNames( VkMemoryPropertyFlags flags, const char* separator ) const
    {
        return MapFlagsToString( g_scVulkanMemoryPropertyFlagNames, flags, separator );
    }

    /***********************************************************************************\

    Function:
        GetBufferUsageFlagNames

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetBufferUsageFlagNames( VkBufferUsageFlags flags, const char* separator ) const
    {
        return MapFlagsToString( g_scVulkanBufferUsageFlagNames, flags, separator );
    }

    /***********************************************************************************\

    Function:
        GetImageUsageFlagNames

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetImageUsageFlagNames( VkImageUsageFlags flags, const char* separator ) const
    {
        return MapFlagsToString( g_scVulkanImageUsageFlagNames, flags, separator );
    }

    /***********************************************************************************\

    Function:
        GetImageTypeName

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetImageTypeName( VkImageType type, VkImageCreateFlags flags, uint32_t arrayLayers ) const
    {
        auto imageTypeName = g_scVulkanImageTypeNames[type];
        if( imageTypeName.empty() )
            return fmt::format( "Unknown ({})", static_cast<uint32_t>( type ) );

        std::string typeName( imageTypeName );

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

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetImageTilingName( VkImageTiling tiling ) const
    {
        return MapValueToString( g_scVulkanImageTilingNames, tiling, "Unknown tiling ({})" );
    }

    /***********************************************************************************\

    Function:
        GetImageTypeName

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetImageAspectFlagNames( VkImageAspectFlags flags, const char* separator ) const
    {
        return MapFlagsToString( g_scVulkanImageAspectFlagNames, flags, separator );
    }

    /***********************************************************************************\

    Function:
        GetCopyAccelerationStructureModeName

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetCopyAccelerationStructureModeName( VkCopyAccelerationStructureModeKHR mode ) const
    {
        return MapValueToString( g_scVulkanCopyAccelerationStructureModeNames, mode, "Unknown mode ({})" );
    }

    /***********************************************************************************\

    Function:
        GetAccelerationStructureTypeName

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetAccelerationStructureTypeName( VkAccelerationStructureTypeKHR type ) const
    {
        return MapValueToString( g_scVulkanAccelerationStructureTypeNames, type, "Unknown type ({})" );
    }

    /***********************************************************************************\

    Function:
        GetAccelerationStructureTypeFlagNames

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetAccelerationStructureTypeFlagNames( VkProfilerAccelerationStructureTypeFlagsEXT flags, const char* separator ) const
    {
        return MapFlagsToString( g_scProfilerAccelerationStructureTypeFlagNames, flags, separator );
    }

    /***********************************************************************************\

    Function:
        GetBuildAccelerationStructureFlagNames

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetBuildAccelerationStructureFlagNames( VkBuildAccelerationStructureFlagsKHR flags, const char* separator ) const
    {
        return MapFlagsToString( g_scVulkanBuildAccelerationStructureFlagNames, flags, separator );
    }

    /***********************************************************************************\

    Function:
        GetBuildAccelerationStructureModeName

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetBuildAccelerationStructureModeName( VkBuildAccelerationStructureModeKHR mode ) const
    {
        return MapValueToString( g_scVulkanBuildAccelerationStructureModeNames, mode, "Unknown mode ({})" );
    }

    /***********************************************************************************\

    Function:
        GetCopyMicromapModeName

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetCopyMicromapModeName( VkCopyMicromapModeEXT mode ) const
    {
        return MapValueToString( g_scVulkanCopyMicromapModeNames, mode, "Unknown mode ({})" );
    }

    /***********************************************************************************\

    Function:
        GetMicromapTypeName

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetMicromapTypeName( VkMicromapTypeEXT type ) const
    {
        return MapValueToString( g_scVulkanMicromapTypeNames, type, "Unknown type ({})" );
    }

    /***********************************************************************************\

    Function:
        GetMicromapTypeFlagNames

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetMicromapTypeFlagNames( VkFlags flags, const char* separator ) const
    {
        return MapFlagsToString( g_scProfilerMicromapTypeFlagNames, flags, separator );
    }

    /***********************************************************************************\

    Function:
        GetBuildMicromapModeName

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetBuildMicromapModeName( VkBuildMicromapModeEXT mode ) const
    {
        return MapValueToString( g_scVulkanBuildMicromapModeNames, mode, "Unknown mode ({})" );
    }

    /***********************************************************************************\

    Function:
        GetBuildMicromapFlagNames

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetBuildMicromapFlagNames( VkBuildMicromapFlagsEXT flags, const char* separator ) const
    {
        return MapFlagsToString( g_scVulkanBuildMicromapFlagNames, flags, separator );
    }

    /***********************************************************************************\

    Function:
        GetGeometryTypeName

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetGeometryTypeName( VkGeometryTypeKHR type ) const
    {
        return MapValueToString( g_scVulkanGeometryTypeNames, type, "Unknown type ({})" );
    }

    /***********************************************************************************\

    Function:
        GetGeometryFlagNames

    \***********************************************************************************/
    std::string DeviceProfilerStringSerializer::GetGeometryFlagNames( VkGeometryFlagsKHR flags, const char* separator ) const
    {
        return MapFlagsToString( g_scVulkanGeometryFlagNames, flags, separator );
    }
}

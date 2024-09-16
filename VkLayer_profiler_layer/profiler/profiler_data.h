// Copyright (c) 2019-2024 Lukasz Stalmirski
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

#pragma once
#include "profiler_counters.h"
#include "profiler_shader.h"
#include <assert.h>
#include <chrono>
#include <vector>
#include <list>
#include <deque>
#include <variant>
#include <unordered_map>
#include <cstring>
#include <vulkan/vulkan.h>
// Import extension structures
#include "profiler_ext/VkProfilerEXT.h"

namespace Profiler
{
    template<typename T> using ContainerType = std::deque<T>;

    /***********************************************************************************\

    Enumeration:
        ProfilerDrawcallType

    Description:
        Profiled drawcall types. Pipeline type associated with the drawcall is stored
        in the high 16 bits of the value.

    \***********************************************************************************/
    enum class DeviceProfilerDrawcallType : uint32_t
    {
        eUnknown = 0x00000000,
        eInsertDebugLabel = 0xFFFF0001,
        eBeginDebugLabel = 0xFFFF0002,
        eEndDebugLabel = 0xFFFF0003,
        eDraw = 0x00010000,
        eDrawIndexed = 0x00010001,
        eDrawIndirect = 0x00010002,
        eDrawIndexedIndirect = 0x00010003,
        eDrawIndirectCount = 0x00010004,
        eDrawIndexedIndirectCount = 0x00010005,
        eDrawMeshTasks = 0x00010006,
        eDrawMeshTasksIndirect = 0x00010007,
        eDrawMeshTasksIndirectCount = 0x00010008,
        eDrawMeshTasksNV = 0x00010009,
        eDrawMeshTasksIndirectNV = 0x0001000A,
        eDrawMeshTasksIndirectCountNV = 0x0001000B,
        eDispatch = 0x00020000,
        eDispatchIndirect = 0x00020001,
        eCopyBuffer = 0x00030000,
        eCopyBufferToImage = 0x00040000,
        eCopyImage = 0x00050000,
        eCopyImageToBuffer = 0x00060000,
        eClearAttachments = 0x00070000,
        eClearColorImage = 0x00080000,
        eClearDepthStencilImage = 0x00090000,
        eResolveImage = 0x000A0000,
        eBlitImage = 0x000B0000,
        eFillBuffer = 0x000C0000,
        eUpdateBuffer = 0x000D0000,
        eTraceRaysKHR = 0x000E0000,
        eTraceRaysIndirectKHR = 0x000E0001,
        eBuildAccelerationStructuresKHR = 0x000F0000,
        eBuildAccelerationStructuresIndirectKHR = 0x000F0001,
        eCopyAccelerationStructureKHR = 0x00100000,
        eCopyAccelerationStructureToMemoryKHR = 0x00200000,
        eCopyMemoryToAccelerationStructureKHR = 0x00300000,
    };

    /***********************************************************************************\

    Enumeration:
        ProfilerPipelineType

    Description:

    \***********************************************************************************/
    enum class DeviceProfilerPipelineType : uint32_t
    {
        eNone = 0x00000000,
        eDebug = 0xFFFF0000,
        eGraphics = 0x00010000,
        eCompute = 0x00020000,
        eCopyBuffer = 0x00030000,
        eCopyBufferToImage = 0x00040000,
        eCopyImage = 0x00050000,
        eCopyImageToBuffer = 0x00060000,
        eClearAttachments = 0x00070000,
        eClearColorImage = 0x00080000,
        eClearDepthStencilImage = 0x00090000,
        eResolveImage = 0x000A0000,
        eBlitImage = 0x000B0000,
        eFillBuffer = 0x000C0000,
        eUpdateBuffer = 0x000D0000,
        eBeginRenderPass = 0x000BFFFF,
        eEndRenderPass = 0x000EFFFF,
        eRayTracingKHR = 0x000E0000,
        eBuildAccelerationStructuresKHR = 0x000F0000,
        eCopyAccelerationStructureKHR = 0x00100000,
        eCopyAccelerationStructureToMemoryKHR = 0x00200000,
        eCopyMemoryToAccelerationStructureKHR = 0x00300000,
    };

    /***********************************************************************************\

    Enumeration:
        DeviceProfilerRenderPassType

    Description:

    \***********************************************************************************/
    enum class DeviceProfilerRenderPassType : uint32_t
    {
        eNone,
        eGraphics,
        eCompute,
        eRayTracing,
        eCopy
    };
    /***********************************************************************************\

    Structure:
        DeviceProfilerSubpassDataType

    Description:
        Supported types of subpass data.

        In core Vulkan, only either inline pipelines, or only secondary command buffers
        are allowed in a subpass. With VK_EXT_nested_command_buffers subpass may contain
        both pipelines and command buffers.

        This enum must be kept in sync with order of structures in the std::variant that
        holds the data.

    \***********************************************************************************/
    enum class DeviceProfilerSubpassDataType : uint32_t
    {
        ePipeline,
        eCommandBuffer
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerTimestamp

    Description:
        Timestamp allocation info.
        Contains the timestamp index and its last recorded value.

    \***********************************************************************************/
    struct DeviceProfilerTimestamp
    {
        uint64_t m_Index = UINT64_MAX;
        uint64_t m_Value = UINT64_MAX;
    };

    /***********************************************************************************\

    Drawcall-specific playloads

    \***********************************************************************************/
    struct DeviceProfilerDrawcallDebugLabelPayload
    {
        char* m_pName;
        float m_Color[ 4 ];
    };

    struct DeviceProfilerDrawcallDrawPayload
    {
        uint32_t m_VertexCount;
        uint32_t m_InstanceCount;
        uint32_t m_FirstVertex;
        uint32_t m_FirstInstance;
    };

    struct DeviceProfilerDrawcallDrawIndexedPayload
    {
        uint32_t m_IndexCount;
        uint32_t m_InstanceCount;
        uint32_t m_FirstIndex;
        int32_t  m_VertexOffset;
        uint32_t m_FirstInstance;
    };

    struct DeviceProfilerDrawcallDrawIndirectPayload
    {
        VkBuffer m_Buffer;
        VkDeviceSize m_Offset;
        uint32_t m_DrawCount;
        uint32_t m_Stride;
    };

    struct DeviceProfilerDrawcallDrawIndexedIndirectPayload
        : DeviceProfilerDrawcallDrawIndirectPayload
    {
    };

    struct DeviceProfilerDrawcallDrawIndirectCountPayload
    {
        VkBuffer m_Buffer;
        VkDeviceSize m_Offset;
        VkBuffer m_CountBuffer;
        VkDeviceSize m_CountOffset;
        uint32_t m_MaxDrawCount;
        uint32_t m_Stride;
    };

    struct DeviceProfilerDrawcallDrawIndexedIndirectCountPayload
        : DeviceProfilerDrawcallDrawIndirectCountPayload
    {
    };

    struct DeviceProfilerDrawcallDrawMeshTasksPayload
    {
        uint32_t m_GroupCountX;
        uint32_t m_GroupCountY;
        uint32_t m_GroupCountZ;
    };

    struct DeviceProfilerDrawcallDrawMeshTasksIndirectPayload
    {
        VkBuffer m_Buffer;
        VkDeviceSize m_Offset;
        uint32_t m_DrawCount;
        uint32_t m_Stride;
    };

    struct DeviceProfilerDrawcallDrawMeshTasksIndirectCountPayload
    {
        VkBuffer m_Buffer;
        VkDeviceSize m_Offset;
        VkBuffer m_CountBuffer;
        VkDeviceSize m_CountOffset;
        uint32_t m_MaxDrawCount;
        uint32_t m_Stride;
    };

    struct DeviceProfilerDrawcallDrawMeshTasksNvPayload
    {
        uint32_t m_TaskCount;
        uint32_t m_FirstTask;
    };

    struct DeviceProfilerDrawcallDrawMeshTasksIndirectNvPayload
        : DeviceProfilerDrawcallDrawMeshTasksIndirectPayload
    {
    };

    struct DeviceProfilerDrawcallDrawMeshTasksIndirectCountNvPayload
        : DeviceProfilerDrawcallDrawMeshTasksIndirectCountPayload
    {
    };

    struct DeviceProfilerDrawcallDispatchPayload
    {
        uint32_t m_GroupCountX;
        uint32_t m_GroupCountY;
        uint32_t m_GroupCountZ;
    };

    struct DeviceProfilerDrawcallDispatchIndirectPayload
    {
        VkBuffer m_Buffer;
        VkDeviceSize m_Offset;
    };

    struct DeviceProfilerDrawcallCopyBufferPayload
    {
        VkBuffer m_SrcBuffer;
        VkBuffer m_DstBuffer;
    };

    struct DeviceProfilerDrawcallCopyBufferToImagePayload
    {
        VkBuffer m_SrcBuffer;
        VkImage  m_DstImage;
    };

    struct DeviceProfilerDrawcallCopyImagePayload
    {
        VkImage  m_SrcImage;
        VkImage  m_DstImage;
    };

    struct DeviceProfilerDrawcallCopyImageToBufferPayload
    {
        VkImage  m_SrcImage;
        VkBuffer m_DstBuffer;
    };

    struct DeviceProfilerDrawcallClearAttachmentsPayload
    {
        uint32_t m_Count;
    };

    struct DeviceProfilerDrawcallClearColorImagePayload
    {
        VkImage m_Image;
        VkClearColorValue m_Value;
    };

    struct DeviceProfilerDrawcallClearDepthStencilImagePayload
    {
        VkImage m_Image;
        VkClearDepthStencilValue m_Value;
    };

    struct DeviceProfilerDrawcallResolveImagePayload
    {
        VkImage  m_SrcImage;
        VkImage  m_DstImage;
    };

    struct DeviceProfilerDrawcallBlitImagePayload
    {
        VkImage  m_SrcImage;
        VkImage  m_DstImage;
    };

    struct DeviceProfilerDrawcallFillBufferPayload
    {
        VkBuffer m_Buffer;
        VkDeviceSize m_Offset;
        VkDeviceSize m_Size;
        uint32_t m_Data;
    };

    struct DeviceProfilerDrawcallUpdateBufferPayload
    {
        VkBuffer m_Buffer;
        VkDeviceSize m_Offset;
        VkDeviceSize m_Size;
    };

    struct DeviceProfilerDrawcallTraceRaysPayload
    {
        uint32_t m_Width;
        uint32_t m_Height;
        uint32_t m_Depth;
    };

    struct DeviceProfilerDrawcallTraceRaysIndirectPayload
    {
        VkDeviceAddress m_IndirectAddress;
    };

    struct DeviceProfilerDrawcallBuildAccelerationStructuresPayloadBase
    {
        uint32_t m_InfoCount;
        VkAccelerationStructureBuildGeometryInfoKHR* m_pInfos;


        DeviceProfilerDrawcallBuildAccelerationStructuresPayloadBase() = default;
        DeviceProfilerDrawcallBuildAccelerationStructuresPayloadBase( uint32_t infoCount, const VkAccelerationStructureBuildGeometryInfoKHR* pInfos )
            : m_InfoCount( infoCount )
            , m_pInfos( CopyAccelerationStructureBuildGeometryInfos( infoCount, pInfos ) )
        {
        }

        static VkAccelerationStructureBuildGeometryInfoKHR* CopyAccelerationStructureBuildGeometryInfos(
            uint32_t infoCount,
            const VkAccelerationStructureBuildGeometryInfoKHR* pInfos )
        {
            // Create copy of build infos
            VkAccelerationStructureBuildGeometryInfoKHR* pDuplicatedInfos = CopyElements( infoCount, pInfos );

            // Create copy of geometry infos of each build info
            for( uint32_t i = 0; i < infoCount; ++i )
            {
                VkAccelerationStructureBuildGeometryInfoKHR& buildInfo = pDuplicatedInfos[ i ];
                const uint32_t geometryCount = buildInfo.geometryCount;

                if( buildInfo.pGeometries != nullptr )
                {
                    buildInfo.pGeometries = CopyElements( geometryCount, buildInfo.pGeometries );
                }

                else if( buildInfo.ppGeometries != nullptr )
                {
                    VkAccelerationStructureGeometryKHR* pGeometries =
                        reinterpret_cast<VkAccelerationStructureGeometryKHR*>(malloc(geometryCount * sizeof(VkAccelerationStructureGeometryKHR)));

                    if( pGeometries != nullptr )
                    {
                        for( uint32_t j = 0; j < geometryCount; ++j )
                        {
                            pGeometries[ j ] = *buildInfo.ppGeometries[ j ];
                        }
                    }

                    buildInfo.pGeometries = pGeometries;
                }

                buildInfo.ppGeometries = nullptr;
            }

            return pDuplicatedInfos;
        }
    };

    struct DeviceProfilerDrawcallBuildAccelerationStructuresPayload
        : DeviceProfilerDrawcallBuildAccelerationStructuresPayloadBase
    {
        VkAccelerationStructureBuildRangeInfoKHR** m_ppRanges;


        DeviceProfilerDrawcallBuildAccelerationStructuresPayload() = default;
        DeviceProfilerDrawcallBuildAccelerationStructuresPayload( uint32_t infoCount, const VkAccelerationStructureBuildGeometryInfoKHR* pInfos, const VkAccelerationStructureBuildRangeInfoKHR* const* ppRanges )
            : DeviceProfilerDrawcallBuildAccelerationStructuresPayloadBase( infoCount, pInfos )
            , m_ppRanges( CopyAccelerationStructureBuildRangeInfos( infoCount, pInfos, ppRanges ) )
        {
        }

        static VkAccelerationStructureBuildRangeInfoKHR** CopyAccelerationStructureBuildRangeInfos(
            uint32_t infoCount,
            const VkAccelerationStructureBuildGeometryInfoKHR* pInfos,
            const VkAccelerationStructureBuildRangeInfoKHR* const* ppRanges )
        {
            // Allocate an array of range pointers
            VkAccelerationStructureBuildRangeInfoKHR** ppDuplicatedRanges =
                reinterpret_cast<VkAccelerationStructureBuildRangeInfoKHR**>( malloc( infoCount * sizeof( VkAccelerationStructureBuildRangeInfoKHR* ) ) );

            if( ppDuplicatedRanges != nullptr )
            {
                for( uint32_t i = 0; i < infoCount; ++i )
                {
                    // Copy ranges for geometries of the current build info
                    ppDuplicatedRanges[ i ] = CopyElements( pInfos[ i ].geometryCount, ppRanges[ i ] );
                }
            }

            return ppDuplicatedRanges;
        }
    };

    struct DeviceProfilerDrawcallBuildAccelerationStructuresIndirectPayload
        : DeviceProfilerDrawcallBuildAccelerationStructuresPayloadBase
    {
        uint32_t** m_ppMaxPrimitiveCounts;


        DeviceProfilerDrawcallBuildAccelerationStructuresIndirectPayload() = default;
        DeviceProfilerDrawcallBuildAccelerationStructuresIndirectPayload( uint32_t infoCount, const VkAccelerationStructureBuildGeometryInfoKHR* pInfos, const uint32_t* const* ppMaxPrimitiveCounts )
            : DeviceProfilerDrawcallBuildAccelerationStructuresPayloadBase( infoCount, pInfos )
            , m_ppMaxPrimitiveCounts( CopyMaxPrimitiveCounts(infoCount, pInfos, ppMaxPrimitiveCounts ) )
        {
        }

        static uint32_t** CopyMaxPrimitiveCounts(
            uint32_t infoCount,
            const VkAccelerationStructureBuildGeometryInfoKHR* pInfos,
            const uint32_t* const* ppMaxPrimitiveCounts )
        {
            // Allocate an array of range pointers
            uint32_t** ppDuplicatedPrimitiveCounts =
                reinterpret_cast<uint32_t**>( malloc( infoCount * sizeof( uint32_t* ) ) );

            if( ppDuplicatedPrimitiveCounts != nullptr )
            {
                for( uint32_t i = 0; i < infoCount; ++i )
                {
                    // Copy max primitive counts for geometries of the current build info
                    ppDuplicatedPrimitiveCounts[ i ] = CopyElements( pInfos[ i ].geometryCount, ppMaxPrimitiveCounts[ i ] );
                }
            }

            return ppDuplicatedPrimitiveCounts;
        }
    };

    struct DeviceProfilerDrawcallCopyAccelerationStructurePayload
    {
        VkAccelerationStructureKHR m_Src;
        VkAccelerationStructureKHR m_Dst;
        VkCopyAccelerationStructureModeKHR m_Mode;
    };

    struct DeviceProfilerDrawcallCopyAccelerationStructureToMemoryPayload
    {
        VkAccelerationStructureKHR m_Src;
        VkDeviceOrHostAddressKHR m_Dst;
        VkCopyAccelerationStructureModeKHR m_Mode;
    };

    struct DeviceProfilerDrawcallCopyMemoryToAccelerationStructurePayload
    {
        VkDeviceOrHostAddressConstKHR m_Src;
        VkAccelerationStructureKHR m_Dst;
        VkCopyAccelerationStructureModeKHR m_Mode;
    };

#define PROFILER_DECL_DRAWCALL_PAYLOAD( type, name )                   \
    type name;                                                         \
    inline void operator=( const type& value ) { this->name = value; }

    /***********************************************************************************\

    Structure:
        ProfilerDrawcallPayload

    Description:
        Contains data associated with the drawcall.

    \***********************************************************************************/
    union DeviceProfilerDrawcallPayload
    {
        PROFILER_DECL_DRAWCALL_PAYLOAD( DeviceProfilerDrawcallDebugLabelPayload, m_DebugLabel );
        PROFILER_DECL_DRAWCALL_PAYLOAD( DeviceProfilerDrawcallDrawPayload, m_Draw );
        PROFILER_DECL_DRAWCALL_PAYLOAD( DeviceProfilerDrawcallDrawIndexedPayload, m_DrawIndexed );
        PROFILER_DECL_DRAWCALL_PAYLOAD( DeviceProfilerDrawcallDrawIndirectPayload, m_DrawIndirect );
        PROFILER_DECL_DRAWCALL_PAYLOAD( DeviceProfilerDrawcallDrawIndexedIndirectPayload, m_DrawIndexedIndirect );
        PROFILER_DECL_DRAWCALL_PAYLOAD( DeviceProfilerDrawcallDrawIndirectCountPayload, m_DrawIndirectCount );
        PROFILER_DECL_DRAWCALL_PAYLOAD( DeviceProfilerDrawcallDrawIndexedIndirectCountPayload, m_DrawIndexedIndirectCount );
        PROFILER_DECL_DRAWCALL_PAYLOAD( DeviceProfilerDrawcallDrawMeshTasksPayload, m_DrawMeshTasks );
        PROFILER_DECL_DRAWCALL_PAYLOAD( DeviceProfilerDrawcallDrawMeshTasksIndirectPayload, m_DrawMeshTasksIndirect );
        PROFILER_DECL_DRAWCALL_PAYLOAD( DeviceProfilerDrawcallDrawMeshTasksIndirectCountPayload, m_DrawMeshTasksIndirectCount );
        PROFILER_DECL_DRAWCALL_PAYLOAD( DeviceProfilerDrawcallDrawMeshTasksNvPayload, m_DrawMeshTasksNV );
        PROFILER_DECL_DRAWCALL_PAYLOAD( DeviceProfilerDrawcallDrawMeshTasksIndirectNvPayload, m_DrawMeshTasksIndirectNV );
        PROFILER_DECL_DRAWCALL_PAYLOAD( DeviceProfilerDrawcallDrawMeshTasksIndirectCountNvPayload, m_DrawMeshTasksIndirectCountNV );
        PROFILER_DECL_DRAWCALL_PAYLOAD( DeviceProfilerDrawcallDispatchPayload, m_Dispatch );
        PROFILER_DECL_DRAWCALL_PAYLOAD( DeviceProfilerDrawcallDispatchIndirectPayload, m_DispatchIndirect );
        PROFILER_DECL_DRAWCALL_PAYLOAD( DeviceProfilerDrawcallCopyBufferPayload, m_CopyBuffer );
        PROFILER_DECL_DRAWCALL_PAYLOAD( DeviceProfilerDrawcallCopyBufferToImagePayload, m_CopyBufferToImage );
        PROFILER_DECL_DRAWCALL_PAYLOAD( DeviceProfilerDrawcallCopyImagePayload, m_CopyImage );
        PROFILER_DECL_DRAWCALL_PAYLOAD( DeviceProfilerDrawcallCopyImageToBufferPayload, m_CopyImageToBuffer );
        PROFILER_DECL_DRAWCALL_PAYLOAD( DeviceProfilerDrawcallClearAttachmentsPayload, m_ClearAttachments );
        PROFILER_DECL_DRAWCALL_PAYLOAD( DeviceProfilerDrawcallClearColorImagePayload, m_ClearColorImage );
        PROFILER_DECL_DRAWCALL_PAYLOAD( DeviceProfilerDrawcallClearDepthStencilImagePayload, m_ClearDepthStencilImage );
        PROFILER_DECL_DRAWCALL_PAYLOAD( DeviceProfilerDrawcallResolveImagePayload, m_ResolveImage );
        PROFILER_DECL_DRAWCALL_PAYLOAD( DeviceProfilerDrawcallBlitImagePayload, m_BlitImage );
        PROFILER_DECL_DRAWCALL_PAYLOAD( DeviceProfilerDrawcallFillBufferPayload, m_FillBuffer );
        PROFILER_DECL_DRAWCALL_PAYLOAD( DeviceProfilerDrawcallUpdateBufferPayload, m_UpdateBuffer );
        PROFILER_DECL_DRAWCALL_PAYLOAD( DeviceProfilerDrawcallTraceRaysPayload, m_TraceRays );
        PROFILER_DECL_DRAWCALL_PAYLOAD( DeviceProfilerDrawcallTraceRaysIndirectPayload, m_TraceRaysIndirect );
        PROFILER_DECL_DRAWCALL_PAYLOAD( DeviceProfilerDrawcallBuildAccelerationStructuresPayload, m_BuildAccelerationStructures );
        PROFILER_DECL_DRAWCALL_PAYLOAD( DeviceProfilerDrawcallBuildAccelerationStructuresIndirectPayload, m_BuildAccelerationStructuresIndirect );
        PROFILER_DECL_DRAWCALL_PAYLOAD( DeviceProfilerDrawcallCopyAccelerationStructurePayload, m_CopyAccelerationStructure );
        PROFILER_DECL_DRAWCALL_PAYLOAD( DeviceProfilerDrawcallCopyAccelerationStructureToMemoryPayload, m_CopyAccelerationStructureToMemory );
        PROFILER_DECL_DRAWCALL_PAYLOAD( DeviceProfilerDrawcallCopyMemoryToAccelerationStructurePayload, m_CopyMemoryToAccelerationStructure );
    };

    /***********************************************************************************\

    Structure:
        ProfilerDrawcall

    Description:
        Contains data collected per-drawcall.

    \***********************************************************************************/
    struct DeviceProfilerDrawcall
    {
        DeviceProfilerDrawcallType                          m_Type = {};
        DeviceProfilerDrawcallPayload                       m_Payload = {};
        DeviceProfilerTimestamp                             m_BeginTimestamp;
        DeviceProfilerTimestamp                             m_EndTimestamp;

        inline DeviceProfilerTimestamp GetBeginTimestamp() const { return m_BeginTimestamp; }
        inline DeviceProfilerTimestamp GetEndTimestamp() const { return m_EndTimestamp; }

        inline DeviceProfilerPipelineType GetPipelineType() const
        {
            // Pipeline type is encoded in DeviceProfilerDrawcallType
            return (DeviceProfilerPipelineType)((uint32_t)m_Type & 0xFFFF0000);
        }

        inline DeviceProfilerDrawcall() = default;

        // Debug labels must be handled here - library needs to extend lifetime of the
        // string passed by the application to be able to print it later.
        inline DeviceProfilerDrawcall( const DeviceProfilerDrawcall& dc )
            : m_Type( dc.m_Type )
            , m_Payload( dc.m_Payload )
            , m_BeginTimestamp( dc.m_BeginTimestamp )
            , m_EndTimestamp( dc.m_EndTimestamp )
        {
            if( dc.GetPipelineType() == DeviceProfilerPipelineType::eDebug )
            {
                // Create copy of already stored string
                m_Payload.m_DebugLabel.m_pName = ProfilerStringFunctions::DuplicateString( dc.m_Payload.m_DebugLabel.m_pName );
            }

            if( dc.m_Type == DeviceProfilerDrawcallType::eBuildAccelerationStructuresKHR )
            {
                // Create copy of build infos
                m_Payload.m_BuildAccelerationStructures.m_pInfos =
                    DeviceProfilerDrawcallBuildAccelerationStructuresPayload::CopyAccelerationStructureBuildGeometryInfos(
                        dc.m_Payload.m_BuildAccelerationStructures.m_InfoCount,
                        dc.m_Payload.m_BuildAccelerationStructures.m_pInfos );

                m_Payload.m_BuildAccelerationStructures.m_ppRanges =
                    DeviceProfilerDrawcallBuildAccelerationStructuresPayload::CopyAccelerationStructureBuildRangeInfos(
                        dc.m_Payload.m_BuildAccelerationStructures.m_InfoCount,
                        dc.m_Payload.m_BuildAccelerationStructures.m_pInfos,
                        dc.m_Payload.m_BuildAccelerationStructures.m_ppRanges );
            }

            if( dc.m_Type == DeviceProfilerDrawcallType::eBuildAccelerationStructuresIndirectKHR )
            {
                // Create copy of build infos
                m_Payload.m_BuildAccelerationStructuresIndirect.m_pInfos =
                    DeviceProfilerDrawcallBuildAccelerationStructuresIndirectPayload::CopyAccelerationStructureBuildGeometryInfos(
                        dc.m_Payload.m_BuildAccelerationStructuresIndirect.m_InfoCount,
                        dc.m_Payload.m_BuildAccelerationStructuresIndirect.m_pInfos );

                m_Payload.m_BuildAccelerationStructuresIndirect.m_ppMaxPrimitiveCounts =
                    DeviceProfilerDrawcallBuildAccelerationStructuresIndirectPayload::CopyMaxPrimitiveCounts(
                        dc.m_Payload.m_BuildAccelerationStructuresIndirect.m_InfoCount,
                        dc.m_Payload.m_BuildAccelerationStructuresIndirect.m_pInfos,
                        dc.m_Payload.m_BuildAccelerationStructuresIndirect.m_ppMaxPrimitiveCounts );
            }
        }

        inline DeviceProfilerDrawcall( DeviceProfilerDrawcall&& dc )
            : DeviceProfilerDrawcall()
        {
            // No need to copy debug label string if we're moving from other drawcall
            // Just make sure the original one is invalidated and won't be freed
            Swap( dc );
        }

        // Destroy drawcall - in case of debug labels free its name
        inline ~DeviceProfilerDrawcall()
        {
            if( GetPipelineType() == DeviceProfilerPipelineType::eDebug )
            {
                free( m_Payload.m_DebugLabel.m_pName );
            }

            if( m_Type == DeviceProfilerDrawcallType::eBuildAccelerationStructuresKHR )
            {
                for( uint32_t i = 0; i < m_Payload.m_BuildAccelerationStructures.m_InfoCount; ++i )
                {
                    free( const_cast<VkAccelerationStructureGeometryKHR*>( m_Payload.m_BuildAccelerationStructures.m_pInfos[ i ].pGeometries ) );
                    free( m_Payload.m_BuildAccelerationStructures.m_ppRanges[ i ] );
                }

                free( m_Payload.m_BuildAccelerationStructures.m_pInfos );
                free( m_Payload.m_BuildAccelerationStructures.m_ppRanges );
            }

            if( m_Type == DeviceProfilerDrawcallType::eBuildAccelerationStructuresIndirectKHR )
            {
                for( uint32_t i = 0; i < m_Payload.m_BuildAccelerationStructuresIndirect.m_InfoCount; ++i )
                {
                    free( const_cast<VkAccelerationStructureGeometryKHR*>( m_Payload.m_BuildAccelerationStructuresIndirect.m_pInfos[ i ].pGeometries ) );
                    free( m_Payload.m_BuildAccelerationStructuresIndirect.m_ppMaxPrimitiveCounts[ i ] );
                }

                free( m_Payload.m_BuildAccelerationStructuresIndirect.m_pInfos );
                free( m_Payload.m_BuildAccelerationStructuresIndirect.m_ppMaxPrimitiveCounts );
            }
        }

        // Swap data of 2 drawcalls
        inline void Swap( DeviceProfilerDrawcall& dc )
        {
            std::swap( m_Type, dc.m_Type );
            std::swap( m_Payload, dc.m_Payload );
            std::swap( m_BeginTimestamp, dc.m_BeginTimestamp );
            std::swap( m_EndTimestamp, dc.m_EndTimestamp );
        }

        // Assignment operators
        inline DeviceProfilerDrawcall& operator=( const DeviceProfilerDrawcall& dc )
        {
            DeviceProfilerDrawcall( dc ).Swap( *this );
            return *this;
        }

        inline DeviceProfilerDrawcall& operator=( DeviceProfilerDrawcall&& dc )
        {
            DeviceProfilerDrawcall( std::move( dc ) ).Swap( *this );
            return *this;
        }
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerDrawcallStats

    Description:
        Stores number and summarized times (total, min and max) of each drawcall type.

    \***********************************************************************************/
    struct DeviceProfilerDrawcallStats
    {
        struct Stats
        {
            uint64_t m_Count = 0;
            uint64_t m_TicksSum = 0;
            uint64_t m_TicksMax = 0;
            uint64_t m_TicksMin = 0;

            inline uint64_t GetTicksAvg() const
            {
                return m_Count ? (m_TicksSum / m_Count) : 0;
            }

            inline void AddTicks( uint64_t ticks )
            {
                if( m_TicksSum == 0 )
                {
                    m_TicksMax = ticks;
                    m_TicksMin = ticks;
                }
                else
                {
                    m_TicksMax = std::max( m_TicksMax, ticks );
                    m_TicksMin = std::min( m_TicksMin, ticks );
                }

                m_TicksSum += ticks;
            }

            inline void AddStats( const Stats& stats )
            {
                m_Count += stats.m_Count;

                if( m_TicksSum == 0 )
                {
                    m_TicksMax = stats.m_TicksMax;
                    m_TicksMin = stats.m_TicksMin;
                }
                else
                {
                    m_TicksMax = std::max( m_TicksMax, stats.m_TicksMax );
                    m_TicksMin = std::min( m_TicksMin, stats.m_TicksMin );
                }

                m_TicksSum += stats.m_TicksSum;
            }
        };

        Stats m_DrawStats = {};
        Stats m_DrawIndirectStats = {};
        Stats m_DrawMeshTasksStats = {};
        Stats m_DrawMeshTasksIndirectStats = {};
        Stats m_DispatchStats = {};
        Stats m_DispatchIndirectStats = {};
        Stats m_CopyBufferStats = {};
        Stats m_CopyBufferToImageStats = {};
        Stats m_CopyImageStats = {};
        Stats m_CopyImageToBufferStats = {};
        Stats m_ClearColorStats = {};
        Stats m_ClearDepthStencilStats = {};
        Stats m_ResolveStats = {};
        Stats m_BlitImageStats = {};
        Stats m_FillBufferStats = {};
        Stats m_UpdateBufferStats = {};
        Stats m_TraceRaysStats = {};
        Stats m_TraceRaysIndirectStats = {};
        Stats m_BuildAccelerationStructuresStats = {};
        Stats m_BuildAccelerationStructuresIndirectStats = {};
        Stats m_CopyAccelerationStructureStats = {};
        Stats m_CopyAccelerationStructureToMemoryStats = {};
        Stats m_CopyMemoryToAccelerationStructureStats = {};
        Stats m_PipelineBarrierStats = {};

        // Stat iteration helpers.
        inline Stats* begin() { return reinterpret_cast<Stats*>(this); }
        inline Stats* end() { return reinterpret_cast<Stats*>(this + 1); }

        inline const Stats* begin() const { return reinterpret_cast<const Stats*>(this); }
        inline const Stats* end() const { return reinterpret_cast<const Stats*>(this + 1); }

        inline constexpr size_t size() const { return sizeof( *this ) / sizeof( Stats ); }

        inline Stats* data() { return begin(); }
        inline const Stats* data() const { return begin(); }

        // Return stats for the given drawcall type.
        inline Stats* GetStats( DeviceProfilerDrawcallType type )
        {
            switch( type )
            {
            case DeviceProfilerDrawcallType::eDraw:
            case DeviceProfilerDrawcallType::eDrawIndexed:
                return &m_DrawStats;
            case DeviceProfilerDrawcallType::eDrawIndirect:
            case DeviceProfilerDrawcallType::eDrawIndexedIndirect:
            case DeviceProfilerDrawcallType::eDrawIndirectCount:
            case DeviceProfilerDrawcallType::eDrawIndexedIndirectCount:
                return &m_DrawIndirectStats;
            case DeviceProfilerDrawcallType::eDrawMeshTasks:
            case DeviceProfilerDrawcallType::eDrawMeshTasksNV:
                return &m_DrawMeshTasksStats;
            case DeviceProfilerDrawcallType::eDrawMeshTasksIndirect:
            case DeviceProfilerDrawcallType::eDrawMeshTasksIndirectNV:
            case DeviceProfilerDrawcallType::eDrawMeshTasksIndirectCount:
            case DeviceProfilerDrawcallType::eDrawMeshTasksIndirectCountNV:
                return &m_DrawMeshTasksIndirectStats;
            case DeviceProfilerDrawcallType::eDispatch:
                return &m_DispatchStats;
            case DeviceProfilerDrawcallType::eDispatchIndirect:
                return &m_DispatchIndirectStats;
            case DeviceProfilerDrawcallType::eCopyBuffer:
                return &m_CopyBufferStats;
            case DeviceProfilerDrawcallType::eCopyBufferToImage:
                return &m_CopyBufferToImageStats;
            case DeviceProfilerDrawcallType::eCopyImage:
                return &m_CopyImageStats;
            case DeviceProfilerDrawcallType::eCopyImageToBuffer:
                return &m_CopyImageToBufferStats;
            case DeviceProfilerDrawcallType::eClearColorImage:
            case DeviceProfilerDrawcallType::eClearAttachments:
                return &m_ClearColorStats;
            case DeviceProfilerDrawcallType::eClearDepthStencilImage:
                return &m_ClearDepthStencilStats;
            case DeviceProfilerDrawcallType::eResolveImage:
                return &m_ResolveStats;
            case DeviceProfilerDrawcallType::eBlitImage:
                return &m_BlitImageStats;
            case DeviceProfilerDrawcallType::eFillBuffer:
                return &m_FillBufferStats;
            case DeviceProfilerDrawcallType::eUpdateBuffer:
                return &m_UpdateBufferStats;
            case DeviceProfilerDrawcallType::eTraceRaysKHR:
                return &m_TraceRaysStats;
            case DeviceProfilerDrawcallType::eTraceRaysIndirectKHR:
                return &m_TraceRaysIndirectStats;
            case DeviceProfilerDrawcallType::eBuildAccelerationStructuresKHR:
                return &m_BuildAccelerationStructuresStats;
            case DeviceProfilerDrawcallType::eBuildAccelerationStructuresIndirectKHR:
                return &m_BuildAccelerationStructuresIndirectStats;
            case DeviceProfilerDrawcallType::eCopyAccelerationStructureKHR:
                return &m_CopyAccelerationStructureStats;
            case DeviceProfilerDrawcallType::eCopyAccelerationStructureToMemoryKHR:
                return &m_CopyAccelerationStructureToMemoryStats;
            case DeviceProfilerDrawcallType::eCopyMemoryToAccelerationStructureKHR:
                return &m_CopyMemoryToAccelerationStructureStats;
            default:
                return nullptr;
            }
        }

        // Increment count of specific drawcall type.
        void AddCount( const DeviceProfilerDrawcall& drawcall )
        {
            uint32_t count = 1;
            switch( drawcall.m_Type )
            {
            case DeviceProfilerDrawcallType::eClearAttachments:
                count = drawcall.m_Payload.m_ClearAttachments.m_Count;
                break;
            case DeviceProfilerDrawcallType::eBuildAccelerationStructuresKHR:
            case DeviceProfilerDrawcallType::eBuildAccelerationStructuresIndirectKHR:
                count = drawcall.m_Payload.m_BuildAccelerationStructures.m_InfoCount;
                break;
            }

            AddCount( drawcall.m_Type, count );
        }

        // Increment count of specific drawcall type.
        void AddCount( DeviceProfilerDrawcallType type, uint64_t count )
        {
            Stats* pStats = GetStats( type );
            if( pStats )
            {
                pStats->m_Count += count;
            }
        }

        // Increment total, min and max ticks of the specific drawcall type.
        void AddTicks( DeviceProfilerDrawcallType type, uint64_t ticks )
        {
            Stats* pStats = GetStats( type );
            if( pStats )
            {
                pStats->AddTicks( ticks );
            }
        }

        // Increment all stats with stats from the other structure.
        void AddStats( const DeviceProfilerDrawcallStats& stats )
        {
            Stats* pStat = begin();
            Stats const* pOtherStat = stats.begin();

            Stats* pEnd = end();
            while( pStat != pEnd )
            {
                pStat->AddStats( *pOtherStat );
                pStat++;
                pOtherStat++;
            }
        }

        DeviceProfilerDrawcallStats& operator+=( const DeviceProfilerDrawcallStats& stats )
        {
            AddStats( stats );
            return *this;
        }
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerPipeline

    Description:
        Represents VkPipeline object.

    \***********************************************************************************/
    struct DeviceProfilerPipeline
    {
        union CreateInfo
        {
            VkGraphicsPipelineCreateInfo                    m_GraphicsPipelineCreateInfo;
            VkComputePipelineCreateInfo                     m_ComputePipelineCreateInfo;
            VkRayTracingPipelineCreateInfoKHR               m_RayTracingPipelineCreateInfoKHR;
        };

        VkPipeline                                          m_Handle = {};
        VkPipelineBindPoint                                 m_BindPoint = {};
        ProfilerShaderTuple                                 m_ShaderTuple = {};
        DeviceProfilerPipelineType                          m_Type = {};

        bool                                                m_UsesRayQuery = false;
        bool                                                m_UsesRayTracing = false;

        bool                                                m_UsesShaderObjects = false;

        std::shared_ptr<CreateInfo>                         m_pCreateInfo = nullptr;

        inline void Finalize()
        {
            // Prefetch shader capabilities.
            m_UsesRayQuery = m_ShaderTuple.UsesRayQuery();
            m_UsesRayTracing = m_ShaderTuple.UsesRayTracing();

            // Calculate pipeline hash.
            m_ShaderTuple.UpdateHash();
        }

        static std::shared_ptr<CreateInfo> CopyPipelineCreateInfo( const VkGraphicsPipelineCreateInfo* pCreateInfo );
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerPipelineData

    Description:
        Contains data collected per-pipeline.

    \***********************************************************************************/
    struct DeviceProfilerPipelineData : DeviceProfilerPipeline
    {
        DeviceProfilerTimestamp                             m_BeginTimestamp;
        DeviceProfilerTimestamp                             m_EndTimestamp;
        ContainerType<struct DeviceProfilerDrawcall>        m_Drawcalls = {};

        inline DeviceProfilerPipelineData() = default;

        inline DeviceProfilerPipelineData( const DeviceProfilerPipeline& pipeline )
            : DeviceProfilerPipeline( pipeline )
        {
        }

        inline bool operator==( const DeviceProfilerPipelineData& rh ) const
        {
            return m_ShaderTuple == rh.m_ShaderTuple;
        }

        inline DeviceProfilerTimestamp GetBeginTimestamp() const { return m_BeginTimestamp; }
        inline DeviceProfilerTimestamp GetEndTimestamp() const { return m_EndTimestamp; }
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerSubpass

    Description:
        Contains captured GPU timestamp data for render pass subpass.

    \***********************************************************************************/
    struct DeviceProfilerSubpass
    {
        uint32_t                                            m_Index = {};
        uint32_t                                            m_ResolveCount = {};
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerSubpassData

    Description:
        Contains captured GPU timestamp data for render pass subpass.

    \***********************************************************************************/
    struct DeviceProfilerSubpassData
    {
        uint32_t                                            m_Index = {};
        VkSubpassContents                                   m_Contents = {};
        DeviceProfilerTimestamp                             m_BeginTimestamp;
        DeviceProfilerTimestamp                             m_EndTimestamp;

        struct Data;
        std::vector<Data>                                   m_Data = {};

        inline DeviceProfilerTimestamp GetBeginTimestamp() const { return m_BeginTimestamp; }
        inline DeviceProfilerTimestamp GetEndTimestamp() const { return m_EndTimestamp; }
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerRenderPass

    Description:
        Represents VkRenderPass object.

    \***********************************************************************************/
    struct DeviceProfilerRenderPass
    {
        VkRenderPass                                        m_Handle = {};
        DeviceProfilerRenderPassType                        m_Type = {};
        std::vector<struct DeviceProfilerSubpass>           m_Subpasses = {};
        uint32_t                                            m_ClearColorAttachmentCount = {};
        uint32_t                                            m_ClearDepthStencilAttachmentCount = {};
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerRenderPassBeginData

    Description:
        Contains captured GPU timestamp data for vkCmdBeginRenderPass... command.

    \***********************************************************************************/
    struct DeviceProfilerRenderPassBeginData
    {
        VkAttachmentLoadOp                                  m_ColorAttachmentLoadOp = {};
        VkAttachmentLoadOp                                  m_DepthAttachmentLoadOp = {};
        VkAttachmentLoadOp                                  m_StencilAttachmentLoadOp = {};
        DeviceProfilerTimestamp                             m_BeginTimestamp;
        DeviceProfilerTimestamp                             m_EndTimestamp;

        inline DeviceProfilerTimestamp GetBeginTimestamp() const { return m_BeginTimestamp; }
        inline DeviceProfilerTimestamp GetEndTimestamp() const { return m_EndTimestamp; }
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerRenderPassEndData

    Description:
        Contains captured GPU timestamp data for vkCmdEndRenderPass... command.

    \***********************************************************************************/
    struct DeviceProfilerRenderPassEndData
    {
        VkAttachmentStoreOp                                 m_ColorAttachmentStoreOp = {};
        VkAttachmentStoreOp                                 m_DepthAttachmentStoreOp = {};
        VkAttachmentStoreOp                                 m_StencilAttachmentStoreOp = {};
        DeviceProfilerTimestamp                             m_BeginTimestamp;
        DeviceProfilerTimestamp                             m_EndTimestamp;

        inline DeviceProfilerTimestamp GetBeginTimestamp() const { return m_BeginTimestamp; }
        inline DeviceProfilerTimestamp GetEndTimestamp() const { return m_EndTimestamp; }
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerRenderPassData

    Description:
        Contains captured GPU timestamp data for single render pass.

    \***********************************************************************************/
    struct DeviceProfilerRenderPassData
    {
        VkRenderPass                                        m_Handle = {};
        DeviceProfilerTimestamp                             m_BeginTimestamp;
        DeviceProfilerTimestamp                             m_EndTimestamp;

        DeviceProfilerRenderPassType                        m_Type = {};
        bool                                                m_Dynamic = {};
        bool                                                m_ClearsColorAttachments = {};
        bool                                                m_ClearsDepthStencilAttachments = {};
        bool                                                m_ResolvesAttachments = {};

        DeviceProfilerRenderPassBeginData                   m_Begin = {};
        DeviceProfilerRenderPassEndData                     m_End = {};

        ContainerType<struct DeviceProfilerSubpassData>     m_Subpasses = {};

        bool HasBeginCommand() const { return m_Handle != VK_NULL_HANDLE || m_Dynamic; }
        bool HasEndCommand() const { return m_Handle != VK_NULL_HANDLE || m_Dynamic; }

        inline DeviceProfilerTimestamp GetBeginTimestamp() const { return m_BeginTimestamp; }
        inline DeviceProfilerTimestamp GetEndTimestamp() const { return m_EndTimestamp; }
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerCommandBufferData

    Description:
        Contains captured GPU timestamp data for single command buffer.

    \***********************************************************************************/
    struct DeviceProfilerCommandBufferData
    {
        VkCommandBuffer                                     m_Handle = {};
        VkCommandBufferLevel                                m_Level = {};
        DeviceProfilerDrawcallStats                         m_Stats = {};
        DeviceProfilerTimestamp                             m_BeginTimestamp;
        DeviceProfilerTimestamp                             m_EndTimestamp;

        bool                                                m_DataValid = false;

        ContainerType<struct DeviceProfilerRenderPassData>  m_RenderPasses = {};

        std::vector<VkProfilerPerformanceCounterResultEXT>  m_PerformanceQueryResults = {};
        uint32_t                                            m_PerformanceQueryMetricsSetIndex = UINT32_MAX;

        uint64_t                                            m_ProfilerCpuOverheadNs = {};

        inline DeviceProfilerTimestamp GetBeginTimestamp() const { return m_BeginTimestamp; }
        inline DeviceProfilerTimestamp GetEndTimestamp() const { return m_EndTimestamp; }
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerSubmitData

    Description:
        Contains captured command buffers data for single submit.

    \***********************************************************************************/
    struct DeviceProfilerSubmitData
    {
        ContainerType<struct DeviceProfilerCommandBufferData> m_CommandBuffers = {};
        std::vector<VkSemaphore>                            m_SignalSemaphores = {};
        std::vector<VkSemaphore>                            m_WaitSemaphores = {};

        DeviceProfilerTimestamp                             m_BeginTimestamp;
        DeviceProfilerTimestamp                             m_EndTimestamp;

        inline DeviceProfilerTimestamp GetBeginTimestamp() const { return m_BeginTimestamp; }
        inline DeviceProfilerTimestamp GetEndTimestamp() const { return m_EndTimestamp; }
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerSubmitBatchData

    Description:
        Stores data for whole vkQueueSubmit.

    \***********************************************************************************/
    struct DeviceProfilerSubmitBatchData
    {
        VkQueue                                             m_Handle = {};
        ContainerType<struct DeviceProfilerSubmitData>      m_Submits = {};
        uint64_t                                            m_Timestamp = {};
        uint32_t                                            m_ThreadId = {};
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerHeapMemoryData

    Description:

    \***********************************************************************************/
    struct DeviceProfilerMemoryHeapData
    {
        uint64_t m_AllocationSize = {};
        uint64_t m_AllocationCount = {};
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerHeapMemoryData

    Description:

    \***********************************************************************************/
    struct DeviceProfilerMemoryTypeData
    {
        uint64_t m_AllocationSize = {};
        uint64_t m_AllocationCount = {};
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerMemoryData

    Description:

    \***********************************************************************************/
    struct DeviceProfilerMemoryData
    {
        uint64_t m_TotalAllocationSize = {};
        uint64_t m_TotalAllocationCount = {};

        std::vector<struct DeviceProfilerMemoryHeapData> m_Heaps = {};
        std::vector<struct DeviceProfilerMemoryTypeData> m_Types = {};
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerCPUData

    Description:

    \***********************************************************************************/
    struct DeviceProfilerCPUData
    {
        uint64_t                                            m_BeginTimestamp = {};
        uint64_t                                            m_EndTimestamp = {};
        float                                               m_FramesPerSec = {};
        uint32_t                                            m_FrameIndex = {};
        uint32_t                                            m_ThreadId = {};
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerSynchronizationTimestamps

    Description:

    \***********************************************************************************/
    struct DeviceProfilerSynchronizationTimestamps
    {
        VkTimeDomainEXT                                     m_HostTimeDomain = {};
        uint64_t                                            m_HostCalibratedTimestamp = {};
        uint64_t                                            m_DeviceCalibratedTimestamp = {};
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerFrameData

    Description:

    \***********************************************************************************/
    struct DeviceProfilerFrameData
    {
        ContainerType<struct DeviceProfilerSubmitBatchData> m_Submits = {};
        ContainerType<struct DeviceProfilerPipelineData>    m_TopPipelines = {};

        DeviceProfilerDrawcallStats                         m_Stats = {};

        uint64_t                                            m_Ticks = {};

        DeviceProfilerMemoryData                            m_Memory = {};
        DeviceProfilerCPUData                               m_CPU = {};
        std::vector<struct TipRange>                        m_TIP = {};

        std::vector<VkProfilerPerformanceCounterResultEXT>  m_VendorMetrics = {};

        DeviceProfilerSynchronizationTimestamps             m_SyncTimestamps = {};
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerSubpassData::Data

    Description:

    \***********************************************************************************/
    struct DeviceProfilerSubpassData::Data : public std::variant<
        DeviceProfilerPipelineData,
        DeviceProfilerCommandBufferData>
    {
        // Use std::variant's constructors.
        using std::variant<DeviceProfilerPipelineData, DeviceProfilerCommandBufferData>::variant;

        constexpr DeviceProfilerSubpassDataType GetType() const
        {
            return static_cast<DeviceProfilerSubpassDataType>(index());
        }

        inline DeviceProfilerTimestamp GetBeginTimestamp() const
        {
            switch( GetType() )
            {
            case DeviceProfilerSubpassDataType::ePipeline:
                return std::get<DeviceProfilerPipelineData>( *this ).GetBeginTimestamp();
            case DeviceProfilerSubpassDataType::eCommandBuffer:
                return std::get<DeviceProfilerCommandBufferData>( *this ).GetBeginTimestamp();
            default:
                return DeviceProfilerTimestamp();
            }
        }

        inline DeviceProfilerTimestamp GetEndTimestamp() const
        {
            switch( GetType() )
            {
            case DeviceProfilerSubpassDataType::ePipeline:
                return std::get<DeviceProfilerPipelineData>( *this ).GetEndTimestamp();
            case DeviceProfilerSubpassDataType::eCommandBuffer:
                return std::get<DeviceProfilerCommandBufferData>( *this ).GetEndTimestamp();
            default:
                return DeviceProfilerTimestamp();
            }
        }
    };
}

namespace std
{
    template<>
    struct hash<Profiler::DeviceProfilerPipeline>
    {
        inline size_t operator()( const Profiler::DeviceProfilerPipeline& pipeline ) const
        {
            return pipeline.m_ShaderTuple.m_Hash;
        }
    };

    template<>
    struct hash<Profiler::DeviceProfilerPipelineData>
    {
        inline size_t operator()( const Profiler::DeviceProfilerPipelineData& pipeline ) const
        {
            return pipeline.m_ShaderTuple.m_Hash;
        }
    };
}

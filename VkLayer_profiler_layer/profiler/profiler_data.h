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

#include "profiler_layer_objects/VkObject.h"

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
        eTraceRaysIndirect2KHR = 0x000E0002,
        eBuildAccelerationStructuresKHR = 0x000F0000,
        eBuildAccelerationStructuresIndirectKHR = 0x000F0001,
        eCopyAccelerationStructureKHR = 0x00100000,
        eCopyAccelerationStructureToMemoryKHR = 0x00110000,
        eCopyMemoryToAccelerationStructureKHR = 0x00120000,
        eBuildMicromapsEXT = 0x00130000,
        eCopyMicromapEXT = 0x00140000,
        eCopyMemoryToMicromapEXT = 0x00150000,
        eCopyMicromapToMemoryEXT = 0x00160000,
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
        eCopyAccelerationStructureToMemoryKHR = 0x00110000,
        eCopyMemoryToAccelerationStructureKHR = 0x00120000,
        eBuildMicromapsEXT = 0x00130000,
        eCopyMicromapEXT = 0x00140000,
        eCopyMemoryToMicromapEXT = 0x00150000,
        eCopyMicromapToMemoryEXT = 0x00160000,
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
    template<DeviceProfilerDrawcallType Type, bool HasIndirectPayload = false>
    struct DeviceProfilerDrawcallBasePayload
    {
        static constexpr bool m_scHasIndirectPayload = HasIndirectPayload;
        static constexpr auto m_scDrawcallType = Type;

        template<typename ProfilerT>
        inline void ResolveObjectHandles( const ProfilerT& )
        {
        }

        template<typename PayloadT>
        inline void CopyDynamicAllocations( const PayloadT& )
        {
        }

        inline void FreeDynamicAllocations()
        {
        }

        template<typename T>
        inline void FreeConst( T pObject )
        {
            using cv_value_type = typename std::remove_pointer<T>::type;
            using value_type = typename std::remove_cv<cv_value_type>::type;
            free( const_cast<value_type*>( pObject ) );
        }
    };

    template<DeviceProfilerDrawcallType Type>
    struct DeviceProfilerDrawcallDebugLabelBasePayload
        : DeviceProfilerDrawcallBasePayload<Type>
    {
        const char* m_pName;
        float m_Color[ 4 ];
        bool m_OwnsDynamicAllocations;

        inline void CopyDynamicAllocations( const DeviceProfilerDrawcallDebugLabelBasePayload& other )
        {
            m_pName = ProfilerStringFunctions::DuplicateString( other.m_pName );
            m_OwnsDynamicAllocations = true;
        }

        inline void FreeDynamicAllocations()
        {
            if( m_OwnsDynamicAllocations )
            {
                this->FreeConst( m_pName );
            }
        }
    };

    struct DeviceProfilerDrawcallInsertDebugLabelPayload
        : DeviceProfilerDrawcallDebugLabelBasePayload<
              DeviceProfilerDrawcallType::eInsertDebugLabel>
    {
    };

    struct DeviceProfilerDrawcallBeginDebugLabelPayload
        : DeviceProfilerDrawcallDebugLabelBasePayload<
              DeviceProfilerDrawcallType::eBeginDebugLabel>
    {
    };

    struct DeviceProfilerDrawcallEndDebugLabelPayload
        : DeviceProfilerDrawcallDebugLabelBasePayload<
              DeviceProfilerDrawcallType::eEndDebugLabel>
    {
    };

    struct DeviceProfilerDrawcallDrawPayload
        : DeviceProfilerDrawcallBasePayload<
              DeviceProfilerDrawcallType::eDraw>
    {
        uint32_t m_VertexCount;
        uint32_t m_InstanceCount;
        uint32_t m_FirstVertex;
        uint32_t m_FirstInstance;
    };

    struct DeviceProfilerDrawcallDrawIndexedPayload
        : DeviceProfilerDrawcallBasePayload<
              DeviceProfilerDrawcallType::eDrawIndexed>
    {
        uint32_t m_IndexCount;
        uint32_t m_InstanceCount;
        uint32_t m_FirstIndex;
        int32_t  m_VertexOffset;
        uint32_t m_FirstInstance;
    };

    template<DeviceProfilerDrawcallType Type>
    struct DeviceProfilerDrawcallDrawIndirectBasePayload
        : DeviceProfilerDrawcallBasePayload<Type, true /*HasIndirectPayload*/>
    {
        VkBufferHandle m_Buffer;
        VkDeviceSize m_Offset;
        uint32_t m_DrawCount;
        uint32_t m_Stride;
        size_t m_IndirectArgsOffset;

        template<typename ProfilerT>
        inline void ResolveObjectHandles( const ProfilerT& profiler )
        {
            profiler.ResolveObjectHandle( m_Buffer );
        }
    };

    struct DeviceProfilerDrawcallDrawIndirectPayload
        : DeviceProfilerDrawcallDrawIndirectBasePayload<
              DeviceProfilerDrawcallType::eDrawIndirect>
    {
    };

    struct DeviceProfilerDrawcallDrawIndexedIndirectPayload
        : DeviceProfilerDrawcallDrawIndirectBasePayload<
              DeviceProfilerDrawcallType::eDrawIndexedIndirect>
    {
    };

    template<DeviceProfilerDrawcallType Type>
    struct DeviceProfilerDrawcallDrawIndirectCountBasePayload
        : DeviceProfilerDrawcallBasePayload<Type, true /*HasIndirectPayload*/>
    {
        VkBufferHandle m_Buffer;
        VkDeviceSize m_Offset;
        VkBufferHandle m_CountBuffer;
        VkDeviceSize m_CountOffset;
        uint32_t m_MaxDrawCount;
        uint32_t m_Stride;
        size_t m_IndirectArgsOffset;
        size_t m_IndirectCountOffset;

        template<typename ProfilerT>
        inline void ResolveObjectHandles( const ProfilerT& profiler )
        {
            profiler.ResolveObjectHandle( m_Buffer );
            profiler.ResolveObjectHandle( m_CountBuffer );
        }
    };

    struct DeviceProfilerDrawcallDrawIndirectCountPayload
        : DeviceProfilerDrawcallDrawIndirectCountBasePayload<
              DeviceProfilerDrawcallType::eDrawIndirectCount>
    {
    };

    struct DeviceProfilerDrawcallDrawIndexedIndirectCountPayload
        : DeviceProfilerDrawcallDrawIndirectCountBasePayload<
              DeviceProfilerDrawcallType::eDrawIndexedIndirectCount>
    {
    };

    struct DeviceProfilerDrawcallDrawMeshTasksPayload
        : DeviceProfilerDrawcallBasePayload<
              DeviceProfilerDrawcallType::eDrawMeshTasks>
    {
        uint32_t m_GroupCountX;
        uint32_t m_GroupCountY;
        uint32_t m_GroupCountZ;
    };

    template<DeviceProfilerDrawcallType Type>
    struct DeviceProfilerDrawcallDrawMeshTasksIndirectBasePayload
        : DeviceProfilerDrawcallBasePayload<Type, true /*HasIndirectPayload*/>
    {
        VkBufferHandle m_Buffer;
        VkDeviceSize m_Offset;
        uint32_t m_DrawCount;
        uint32_t m_Stride;
        size_t m_IndirectArgsOffset;

        template<typename ProfilerT>
        inline void ResolveObjectHandles( const ProfilerT& profiler )
        {
            profiler.ResolveObjectHandle( m_Buffer );
        }
    };

    struct DeviceProfilerDrawcallDrawMeshTasksIndirectPayload
        : DeviceProfilerDrawcallDrawMeshTasksIndirectBasePayload<
              DeviceProfilerDrawcallType::eDrawMeshTasksIndirect>
    {
    };

    template<DeviceProfilerDrawcallType Type>
    struct DeviceProfilerDrawcallDrawMeshTasksIndirectCountBasePayload
        : DeviceProfilerDrawcallBasePayload<Type, true /*HasIndirectPayload*/>
    {
        VkBufferHandle m_Buffer;
        VkDeviceSize m_Offset;
        VkBufferHandle m_CountBuffer;
        VkDeviceSize m_CountOffset;
        uint32_t m_MaxDrawCount;
        uint32_t m_Stride;
        size_t m_IndirectArgsOffset;
        size_t m_IndirectCountOffset;

        template<typename ProfilerT>
        inline void ResolveObjectHandles( const ProfilerT& profiler )
        {
            profiler.ResolveObjectHandle( m_Buffer );
            profiler.ResolveObjectHandle( m_CountBuffer );
        }
    };

    struct DeviceProfilerDrawcallDrawMeshTasksIndirectCountPayload
        : DeviceProfilerDrawcallDrawMeshTasksIndirectCountBasePayload<
              DeviceProfilerDrawcallType::eDrawMeshTasksIndirectCount>
    {
    };

    struct DeviceProfilerDrawcallDrawMeshTasksNvPayload
        : DeviceProfilerDrawcallBasePayload<
              DeviceProfilerDrawcallType::eDrawMeshTasksNV>
    {
        uint32_t m_TaskCount;
        uint32_t m_FirstTask;
    };

    struct DeviceProfilerDrawcallDrawMeshTasksIndirectNvPayload
        : DeviceProfilerDrawcallDrawMeshTasksIndirectBasePayload<
              DeviceProfilerDrawcallType::eDrawMeshTasksIndirectNV>
    {
    };

    struct DeviceProfilerDrawcallDrawMeshTasksIndirectCountNvPayload
        : DeviceProfilerDrawcallDrawMeshTasksIndirectCountBasePayload<
              DeviceProfilerDrawcallType::eDrawMeshTasksIndirectCountNV>
    {
    };

    struct DeviceProfilerDrawcallDispatchPayload
        : DeviceProfilerDrawcallBasePayload<
              DeviceProfilerDrawcallType::eDispatch>
    {
        uint32_t m_GroupCountX;
        uint32_t m_GroupCountY;
        uint32_t m_GroupCountZ;
    };

    struct DeviceProfilerDrawcallDispatchIndirectPayload
        : DeviceProfilerDrawcallBasePayload<
              DeviceProfilerDrawcallType::eDispatchIndirect,
              true /*HasIndirectPayload*/>
    {
        VkBufferHandle m_Buffer;
        VkDeviceSize m_Offset;
        size_t m_IndirectArgsOffset;

        template<typename ProfilerT>
        inline void ResolveObjectHandles( const ProfilerT& profiler )
        {
            profiler.ResolveObjectHandle( m_Buffer );
        }
    };

    struct DeviceProfilerDrawcallCopyBufferPayload
        : DeviceProfilerDrawcallBasePayload<
              DeviceProfilerDrawcallType::eCopyBuffer>
    {
        VkBufferHandle m_SrcBuffer;
        VkBufferHandle m_DstBuffer;

        template<typename ProfilerT>
        inline void ResolveObjectHandles( const ProfilerT& profiler )
        {
            profiler.ResolveObjectHandle( m_SrcBuffer );
            profiler.ResolveObjectHandle( m_DstBuffer );
        }
    };

    struct DeviceProfilerDrawcallCopyBufferToImagePayload
        : DeviceProfilerDrawcallBasePayload<
              DeviceProfilerDrawcallType::eCopyBufferToImage>
    {
        VkBufferHandle m_SrcBuffer;
        VkImageHandle m_DstImage;

        template<typename ProfilerT>
        inline void ResolveObjectHandles( const ProfilerT& profiler )
        {
            profiler.ResolveObjectHandle( m_SrcBuffer );
            profiler.ResolveObjectHandle( m_DstImage );
        }
    };

    struct DeviceProfilerDrawcallCopyImagePayload
        : DeviceProfilerDrawcallBasePayload<
              DeviceProfilerDrawcallType::eCopyImage>
    {
        VkImageHandle m_SrcImage;
        VkImageHandle m_DstImage;

        template<typename ProfilerT>
        inline void ResolveObjectHandles( const ProfilerT& profiler )
        {
            profiler.ResolveObjectHandle( m_SrcImage );
            profiler.ResolveObjectHandle( m_DstImage );
        }
    };

    struct DeviceProfilerDrawcallCopyImageToBufferPayload
        : DeviceProfilerDrawcallBasePayload<
              DeviceProfilerDrawcallType::eCopyImageToBuffer>
    {
        VkImageHandle m_SrcImage;
        VkBufferHandle m_DstBuffer;

        template<typename ProfilerT>
        inline void ResolveObjectHandles( const ProfilerT& profiler )
        {
            profiler.ResolveObjectHandle( m_SrcImage );
            profiler.ResolveObjectHandle( m_DstBuffer );
        }
    };

    struct DeviceProfilerDrawcallClearAttachmentsPayload
        : DeviceProfilerDrawcallBasePayload<
              DeviceProfilerDrawcallType::eClearAttachments>
    {
        uint32_t m_Count;
    };

    struct DeviceProfilerDrawcallClearColorImagePayload
        : DeviceProfilerDrawcallBasePayload<
              DeviceProfilerDrawcallType::eClearColorImage>
    {
        VkImageHandle m_Image;
        VkClearColorValue m_Value;

        template<typename ProfilerT>
        inline void ResolveObjectHandles( const ProfilerT& profiler )
        {
            profiler.ResolveObjectHandle( m_Image );
        }
    };

    struct DeviceProfilerDrawcallClearDepthStencilImagePayload
        : DeviceProfilerDrawcallBasePayload<
              DeviceProfilerDrawcallType::eClearDepthStencilImage>
    {
        VkImageHandle m_Image;
        VkClearDepthStencilValue m_Value;

        template<typename ProfilerT>
        inline void ResolveObjectHandles( const ProfilerT& profiler )
        {
            profiler.ResolveObjectHandle( m_Image );
        }
    };

    struct DeviceProfilerDrawcallResolveImagePayload
        : DeviceProfilerDrawcallBasePayload<
              DeviceProfilerDrawcallType::eResolveImage>
    {
        VkImageHandle m_SrcImage;
        VkImageHandle m_DstImage;

        template<typename ProfilerT>
        inline void ResolveObjectHandles( const ProfilerT& profiler )
        {
            profiler.ResolveObjectHandle( m_SrcImage );
            profiler.ResolveObjectHandle( m_DstImage );
        }
    };

    struct DeviceProfilerDrawcallBlitImagePayload
        : DeviceProfilerDrawcallBasePayload<
              DeviceProfilerDrawcallType::eBlitImage>
    {
        VkImageHandle m_SrcImage;
        VkImageHandle m_DstImage;

        template<typename ProfilerT>
        inline void ResolveObjectHandles( const ProfilerT& profiler )
        {
            profiler.ResolveObjectHandle( m_SrcImage );
            profiler.ResolveObjectHandle( m_DstImage );
        }
    };

    struct DeviceProfilerDrawcallFillBufferPayload
        : DeviceProfilerDrawcallBasePayload<
            DeviceProfilerDrawcallType::eFillBuffer>
    {
        VkBufferHandle m_Buffer;
        VkDeviceSize m_Offset;
        VkDeviceSize m_Size;
        uint32_t m_Data;

        template<typename ProfilerT>
        inline void ResolveObjectHandles( const ProfilerT& profiler )
        {
            profiler.ResolveObjectHandle( m_Buffer );
        }
    };

    struct DeviceProfilerDrawcallUpdateBufferPayload
        : DeviceProfilerDrawcallBasePayload<
              DeviceProfilerDrawcallType::eUpdateBuffer>
    {
        VkBufferHandle m_Buffer;
        VkDeviceSize m_Offset;
        VkDeviceSize m_Size;

        template<typename ProfilerT>
        inline void ResolveObjectHandles( const ProfilerT& profiler )
        {
            profiler.ResolveObjectHandle( m_Buffer );
        }
    };

    struct DeviceProfilerDrawcallTraceRaysPayload
        : DeviceProfilerDrawcallBasePayload<
              DeviceProfilerDrawcallType::eTraceRaysKHR>
    {
        uint32_t m_Width;
        uint32_t m_Height;
        uint32_t m_Depth;
    };

    struct DeviceProfilerDrawcallTraceRaysIndirectPayload
        : DeviceProfilerDrawcallBasePayload<
            DeviceProfilerDrawcallType::eTraceRaysIndirectKHR,
            true /*HasIndirectPayload*/>
    {
        VkDeviceAddress m_IndirectAddress;
    };

    struct DeviceProfilerDrawcallTraceRaysIndirect2Payload
        : DeviceProfilerDrawcallBasePayload<
              DeviceProfilerDrawcallType::eTraceRaysIndirect2KHR,
              true /*HasIndirectPayload*/>
    {
        VkDeviceAddress m_IndirectAddress;
    };

    template<DeviceProfilerDrawcallType Type>
    struct DeviceProfilerDrawcallBuildAccelerationStructuresBasePayload
        : DeviceProfilerDrawcallBasePayload<Type>
    {
        uint32_t m_InfoCount;
        bool m_OwnsDynamicAllocations;
        const VkAccelerationStructureBuildGeometryInfoKHR* m_pInfos;

        void CopyDynamicAllocations( const DeviceProfilerDrawcallBuildAccelerationStructuresBasePayload& other );
        void FreeDynamicAllocations();
    };

    struct DeviceProfilerDrawcallBuildAccelerationStructuresPayload
        : DeviceProfilerDrawcallBuildAccelerationStructuresBasePayload<
              DeviceProfilerDrawcallType::eBuildAccelerationStructuresKHR>
    {
        const VkAccelerationStructureBuildRangeInfoKHR* const* m_ppRanges;

        void CopyDynamicAllocations( const DeviceProfilerDrawcallBuildAccelerationStructuresPayload& other );
        void FreeDynamicAllocations();
    };

    struct DeviceProfilerDrawcallBuildAccelerationStructuresIndirectPayload
        : DeviceProfilerDrawcallBuildAccelerationStructuresBasePayload<
              DeviceProfilerDrawcallType::eBuildAccelerationStructuresIndirectKHR>
    {
        const uint32_t* const* m_ppMaxPrimitiveCounts;

        void CopyDynamicAllocations( const DeviceProfilerDrawcallBuildAccelerationStructuresIndirectPayload& other );
        void FreeDynamicAllocations();
    };

    struct DeviceProfilerDrawcallCopyAccelerationStructurePayload
        : DeviceProfilerDrawcallBasePayload<
              DeviceProfilerDrawcallType::eCopyAccelerationStructureKHR>
    {
        VkAccelerationStructureKHRHandle m_Src;
        VkAccelerationStructureKHRHandle m_Dst;
        VkCopyAccelerationStructureModeKHR m_Mode;

        template<typename ProfilerT>
        inline void ResolveObjectHandles( const ProfilerT& profiler )
        {
            profiler.ResolveObjectHandle( m_Src );
            profiler.ResolveObjectHandle( m_Dst );
        }
    };

    struct DeviceProfilerDrawcallCopyAccelerationStructureToMemoryPayload
        : DeviceProfilerDrawcallBasePayload<
              DeviceProfilerDrawcallType::eCopyAccelerationStructureToMemoryKHR>
    {
        VkAccelerationStructureKHRHandle m_Src;
        VkDeviceOrHostAddressKHR m_Dst;
        VkCopyAccelerationStructureModeKHR m_Mode;

        template<typename ProfilerT>
        inline void ResolveObjectHandles( const ProfilerT& profiler )
        {
            profiler.ResolveObjectHandle( m_Src );
        }
    };

    struct DeviceProfilerDrawcallCopyMemoryToAccelerationStructurePayload
        : DeviceProfilerDrawcallBasePayload<
            DeviceProfilerDrawcallType::eCopyMemoryToAccelerationStructureKHR>
    {
        VkDeviceOrHostAddressConstKHR m_Src;
        VkAccelerationStructureKHRHandle m_Dst;
        VkCopyAccelerationStructureModeKHR m_Mode;

        template<typename ProfilerT>
        inline void ResolveObjectHandles( const ProfilerT& profiler )
        {
            profiler.ResolveObjectHandle( m_Dst );
        }
    };

    struct DeviceProfilerDrawcallBuildMicromapsPayload
        : DeviceProfilerDrawcallBasePayload<
              DeviceProfilerDrawcallType::eBuildMicromapsEXT>
    {
        uint32_t m_InfoCount;
        bool m_OwnsDynamicAllocations;
        const VkMicromapBuildInfoEXT* m_pInfos;

        void CopyDynamicAllocations( const DeviceProfilerDrawcallBuildMicromapsPayload& other );
        void FreeDynamicAllocations();
    };

    struct DeviceProfilerDrawcallCopyMicromapPayload
        : DeviceProfilerDrawcallBasePayload<
              DeviceProfilerDrawcallType::eCopyMicromapEXT>
    {
        VkMicromapEXTHandle m_Src;
        VkMicromapEXTHandle m_Dst;
        VkCopyMicromapModeEXT m_Mode;

        template<typename ProfilerT>
        inline void ResolveObjectHandles( const ProfilerT& profiler )
        {
            profiler.ResolveObjectHandle( m_Src );
            profiler.ResolveObjectHandle( m_Dst );
        }
    };

    struct DeviceProfilerDrawcallCopyMemoryToMicromapPayload
        : DeviceProfilerDrawcallBasePayload<
              DeviceProfilerDrawcallType::eCopyMemoryToMicromapEXT>
    {
        VkDeviceOrHostAddressConstKHR m_Src;
        VkMicromapEXTHandle m_Dst;
        VkCopyMicromapModeEXT m_Mode;

        template<typename ProfilerT>
        inline void ResolveObjectHandles( const ProfilerT& profiler )
        {
            profiler.ResolveObjectHandle( m_Dst );
        }
    };

    struct DeviceProfilerDrawcallCopyMicromapToMemoryPayload
        : DeviceProfilerDrawcallBasePayload<
              DeviceProfilerDrawcallType::eCopyMicromapToMemoryEXT>
    {
        VkMicromapEXTHandle m_Src;
        VkDeviceOrHostAddressKHR m_Dst;
        VkCopyMicromapModeEXT m_Mode;

        template<typename ProfilerT>
        inline void ResolveObjectHandles( const ProfilerT& profiler )
        {
            profiler.ResolveObjectHandle( m_Src );
        }
    };

// clang-format off
#define PROFILER_MAP_DRAWCALL_PAYLOAD( f, ... ) \
    f( DeviceProfilerDrawcallInsertDebugLabelPayload, m_InsertDebugLabel, __VA_ARGS__ ) \
    f( DeviceProfilerDrawcallBeginDebugLabelPayload, m_BeginDebugLabel, __VA_ARGS__ ) \
    f( DeviceProfilerDrawcallEndDebugLabelPayload, m_EndDebugLabel, __VA_ARGS__ ) \
    f( DeviceProfilerDrawcallDrawPayload, m_Draw, __VA_ARGS__ ) \
    f( DeviceProfilerDrawcallDrawIndexedPayload, m_DrawIndexed, __VA_ARGS__ ) \
    f( DeviceProfilerDrawcallDrawIndirectPayload, m_DrawIndirect, __VA_ARGS__ ) \
    f( DeviceProfilerDrawcallDrawIndexedIndirectPayload, m_DrawIndexedIndirect, __VA_ARGS__ ) \
    f( DeviceProfilerDrawcallDrawIndirectCountPayload, m_DrawIndirectCount, __VA_ARGS__ ) \
    f( DeviceProfilerDrawcallDrawIndexedIndirectCountPayload, m_DrawIndexedIndirectCount, __VA_ARGS__ ) \
    f( DeviceProfilerDrawcallDrawMeshTasksPayload, m_DrawMeshTasks, __VA_ARGS__ ) \
    f( DeviceProfilerDrawcallDrawMeshTasksIndirectPayload, m_DrawMeshTasksIndirect, __VA_ARGS__ ) \
    f( DeviceProfilerDrawcallDrawMeshTasksIndirectCountPayload, m_DrawMeshTasksIndirectCount, __VA_ARGS__ ) \
    f( DeviceProfilerDrawcallDrawMeshTasksNvPayload, m_DrawMeshTasksNV, __VA_ARGS__ ) \
    f( DeviceProfilerDrawcallDrawMeshTasksIndirectNvPayload, m_DrawMeshTasksIndirectNV, __VA_ARGS__ ) \
    f( DeviceProfilerDrawcallDrawMeshTasksIndirectCountNvPayload, m_DrawMeshTasksIndirectCountNV, __VA_ARGS__ ) \
    f( DeviceProfilerDrawcallDispatchPayload, m_Dispatch, __VA_ARGS__ ) \
    f( DeviceProfilerDrawcallDispatchIndirectPayload, m_DispatchIndirect, __VA_ARGS__ ) \
    f( DeviceProfilerDrawcallCopyBufferPayload, m_CopyBuffer, __VA_ARGS__ ) \
    f( DeviceProfilerDrawcallCopyBufferToImagePayload, m_CopyBufferToImage, __VA_ARGS__ ) \
    f( DeviceProfilerDrawcallCopyImagePayload, m_CopyImage, __VA_ARGS__ ) \
    f( DeviceProfilerDrawcallCopyImageToBufferPayload, m_CopyImageToBuffer, __VA_ARGS__ ) \
    f( DeviceProfilerDrawcallClearAttachmentsPayload, m_ClearAttachments, __VA_ARGS__ ) \
    f( DeviceProfilerDrawcallClearColorImagePayload, m_ClearColorImage, __VA_ARGS__ ) \
    f( DeviceProfilerDrawcallClearDepthStencilImagePayload, m_ClearDepthStencilImage, __VA_ARGS__ ) \
    f( DeviceProfilerDrawcallResolveImagePayload, m_ResolveImage, __VA_ARGS__ ) \
    f( DeviceProfilerDrawcallBlitImagePayload, m_BlitImage, __VA_ARGS__ ) \
    f( DeviceProfilerDrawcallFillBufferPayload, m_FillBuffer, __VA_ARGS__ ) \
    f( DeviceProfilerDrawcallUpdateBufferPayload, m_UpdateBuffer, __VA_ARGS__ ) \
    f( DeviceProfilerDrawcallTraceRaysPayload, m_TraceRays, __VA_ARGS__ ) \
    f( DeviceProfilerDrawcallTraceRaysIndirectPayload, m_TraceRaysIndirect, __VA_ARGS__ ) \
    f( DeviceProfilerDrawcallTraceRaysIndirect2Payload, m_TraceRaysIndirect2, __VA_ARGS__ ) \
    f( DeviceProfilerDrawcallBuildAccelerationStructuresPayload, m_BuildAccelerationStructures, __VA_ARGS__ ) \
    f( DeviceProfilerDrawcallBuildAccelerationStructuresIndirectPayload, m_BuildAccelerationStructuresIndirect, __VA_ARGS__ ) \
    f( DeviceProfilerDrawcallCopyAccelerationStructurePayload, m_CopyAccelerationStructure, __VA_ARGS__ ) \
    f( DeviceProfilerDrawcallCopyAccelerationStructureToMemoryPayload, m_CopyAccelerationStructureToMemory, __VA_ARGS__ ) \
    f( DeviceProfilerDrawcallCopyMemoryToAccelerationStructurePayload, m_CopyMemoryToAccelerationStructure, __VA_ARGS__ ) \
    f( DeviceProfilerDrawcallBuildMicromapsPayload, m_BuildMicromaps, __VA_ARGS__ ) \
    f( DeviceProfilerDrawcallCopyMicromapPayload, m_CopyMicromap, __VA_ARGS__ ) \
    f( DeviceProfilerDrawcallCopyMemoryToMicromapPayload, m_CopyMemoryToMicromap, __VA_ARGS__ ) \
    f( DeviceProfilerDrawcallCopyMicromapToMemoryPayload, m_CopyMicromapToMemory, __VA_ARGS__ )
    // clang-format on

    /***********************************************************************************\

    Structure:
        ProfilerDrawcallPayload

    Description:
        Contains data associated with the drawcall.

    \***********************************************************************************/
    union DeviceProfilerDrawcallPayload
    {
#define PROFILER_DECL_DRAWCALL_PAYLOAD( type, name, ... ) \
    type name;                                            \
    inline void operator=( const type& value ) { this->name = value; }

        PROFILER_DECL_DRAWCALL_PAYLOAD(
            DeviceProfilerDrawcallDebugLabelBasePayload<DeviceProfilerDrawcallType::eUnknown>,
            m_DebugLabel )

        PROFILER_MAP_DRAWCALL_PAYLOAD( PROFILER_DECL_DRAWCALL_PAYLOAD )

#undef PROFILER_DECL_DRAWCALL_PAYLOAD
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
        #define PROFILER_COPY_DYNAMIC_ALLOCATIONS( type, name, ... ) \
            case type::m_scDrawcallType: m_Payload.name.CopyDynamicAllocations( dc.m_Payload.name ); break;

            switch( m_Type )
            {
                PROFILER_MAP_DRAWCALL_PAYLOAD( PROFILER_COPY_DYNAMIC_ALLOCATIONS )
            }

        #undef PROFILER_COPY_DYNAMIC_ALLOCATIONS
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
        #define PROFILER_FREE_DYNAMIC_ALLOCATIONS( type, name, ... ) \
            case type::m_scDrawcallType: m_Payload.name.FreeDynamicAllocations(); break;

            switch( m_Type )
            {
                PROFILER_MAP_DRAWCALL_PAYLOAD( PROFILER_FREE_DYNAMIC_ALLOCATIONS )
            }

        #undef PROFILER_FREE_DYNAMIC_ALLOCATIONS
        }

        // Swap data of 2 drawcalls
        inline void Swap( DeviceProfilerDrawcall& dc )
        {
            std::swap( m_Type, dc.m_Type );
            std::swap( m_Payload, dc.m_Payload );
            std::swap( m_BeginTimestamp, dc.m_BeginTimestamp );
            std::swap( m_EndTimestamp, dc.m_EndTimestamp );
        }

        // Check if drawcall can have indirect payload
        inline bool HasIndirectPayload() const
        {
        #define PROFILER_CHECK_INDIRECT_PAYLOAD( type, name, ... ) \
            case type::m_scDrawcallType: return type::m_scHasIndirectPayload;

            switch( m_Type )
            {
                PROFILER_MAP_DRAWCALL_PAYLOAD( PROFILER_CHECK_INDIRECT_PAYLOAD )

            default:
                return false;
            }

        #undef PROFILER_CHECK_INDIRECT_PAYLOAD
        }

        // Resolve object handles in the payload
        template<typename ProfilerT>
        inline void ResolveObjectHandles( const ProfilerT& profiler )
        {
        #define PROFILER_RESOLVE_OBJECT_HANDLES( type, name, ... ) \
            case type::m_scDrawcallType: return m_Payload.name.ResolveObjectHandles( profiler );

            switch( m_Type )
            {
                PROFILER_MAP_DRAWCALL_PAYLOAD( PROFILER_RESOLVE_OBJECT_HANDLES )
            }

        #undef PROFILER_RESOLVE_OBJECT_HANDLES
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

        VkPipelineHandle                                    m_Handle = {};
        VkPipelineBindPoint                                 m_BindPoint = {};
        ProfilerShaderTuple                                 m_ShaderTuple = {};
        DeviceProfilerPipelineType                          m_Type = {};

        bool                                                m_Internal = false;

        bool                                                m_UsesRayQuery = false;
        bool                                                m_UsesRayTracing = false;
        bool                                                m_UsesMeshShading = false;
        bool                                                m_UsesShaderObjects = false;

        VkDeviceSize                                        m_RayTracingPipelineStackSize = {};

        std::shared_ptr<CreateInfo>                         m_pCreateInfo = nullptr;

        inline void Finalize()
        {
            // Prefetch shader capabilities.
            m_UsesRayQuery = m_ShaderTuple.UsesRayQuery();
            m_UsesRayTracing = m_ShaderTuple.UsesRayTracing();
            m_UsesMeshShading = m_ShaderTuple.UsesMeshShading();

            // Calculate pipeline hash.
            m_ShaderTuple.UpdateHash();
        }

        static std::shared_ptr<CreateInfo> CopyPipelineCreateInfo( const VkGraphicsPipelineCreateInfo* pCreateInfo );
        static std::shared_ptr<CreateInfo> CopyPipelineCreateInfo( const VkRayTracingPipelineCreateInfoKHR* pCreateInfo );
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
        // Mark subpasses that are not part of any render pass as implicit.
        // Required to handle commands scoped outside of render pass.
        static constexpr uint32_t                           ImplicitSubpassIndex = UINT32_MAX;

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
        VkRenderPassHandle                                  m_Handle = {};
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
        VkRenderPassHandle                                  m_Handle = {};
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

    Class:
        DeviceProfilerPerformanceCountersStreamResult

    Description:
        Holds a single performance counters stream sample.

    \***********************************************************************************/
    struct DeviceProfilerPerformanceCountersStreamResult
    {
        uint64_t                                            m_Timestamp;
        uint32_t                                            m_Marker;
        std::vector<VkProfilerPerformanceCounterResultEXT>  m_Results;
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerPerformanceCounterData

    Description:
        Holds performance query results for a single performance query pass.

    \***********************************************************************************/
    struct DeviceProfilerPerformanceCountersData
    {
        uint32_t                                            m_MetricsSetIndex = UINT32_MAX;
        std::vector<VkProfilerPerformanceCounterResultEXT>  m_Results = {};
        std::vector<DeviceProfilerPerformanceCountersStreamResult> m_StreamSamples = {};
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerCommandBufferData

    Description:
        Contains captured GPU timestamp data for single command buffer.

    \***********************************************************************************/
    struct DeviceProfilerCommandBufferData
    {
        VkCommandBufferHandle                               m_Handle = {};
        VkCommandBufferLevel                                m_Level = {};
        DeviceProfilerDrawcallStats                         m_Stats = {};
        DeviceProfilerTimestamp                             m_BeginTimestamp;
        DeviceProfilerTimestamp                             m_EndTimestamp;

        bool                                                m_DataValid = false;

        ContainerType<struct DeviceProfilerRenderPassData>  m_RenderPasses = {};

        DeviceProfilerPerformanceCountersData               m_PerformanceCounters = {};

        std::vector<uint8_t>                                m_IndirectPayload = {};

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
        std::vector<VkSemaphoreHandle>                      m_SignalSemaphores = {};
        std::vector<VkSemaphoreHandle>                      m_WaitSemaphores = {};

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
        VkQueueHandle                                       m_Handle = {};
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
        uint64_t m_BudgetSize = {};
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
        DeviceProfilerDeviceMemoryData

    Description:

    \***********************************************************************************/
    struct DeviceProfilerDeviceMemoryData
    {
        VkDeviceSize m_Size = {};
        uint32_t m_TypeIndex = {};
        uint32_t m_HeapIndex = {};
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerBufferMemoryBindingData

    Description:

    \***********************************************************************************/
    struct DeviceProfilerBufferMemoryBindingData
    {
        VkDeviceMemoryHandle m_Memory = {};
        VkDeviceSize m_MemoryOffset = {};
        VkDeviceSize m_BufferOffset = {};
        VkDeviceSize m_Size = {};
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerBufferMemoryData

    Description:

    \***********************************************************************************/
    struct DeviceProfilerBufferMemoryData
    {
        typedef std::variant<DeviceProfilerBufferMemoryBindingData, std::vector<DeviceProfilerBufferMemoryBindingData>>
            MemoryBindings;

        VkDeviceSize m_BufferSize = {};
        VkBufferCreateFlags m_BufferFlags = {};
        VkBufferUsageFlags m_BufferUsage = {};
        VkMemoryRequirements m_MemoryRequirements = {};
        MemoryBindings m_MemoryBindings = {};

        // Interface for querying number of memory bindings.
        // By default buffers are bound to single memory block, unless sparse binding is enabled.
        inline size_t GetMemoryBindingCount() const
        {
            if( m_BufferFlags & VK_BUFFER_CREATE_SPARSE_BINDING_BIT )
            {
                auto* pMemoryBindingsVector = std::get_if<std::vector<DeviceProfilerBufferMemoryBindingData>>( &m_MemoryBindings );
                if( pMemoryBindingsVector )
                {
                    return pMemoryBindingsVector->size();
                }
            }

            auto* pMemoryBinding = std::get_if<DeviceProfilerBufferMemoryBindingData>( &m_MemoryBindings );
            if( pMemoryBinding )
            {
                return 1;
            }

            return 0;
        }

        // Interface for getting pointer to the bound memory blocks.
        // By default buffers are bound to single memory block, unless sparse binding is enabled.
        inline const DeviceProfilerBufferMemoryBindingData* GetMemoryBindings() const
        {
            if( m_BufferFlags & VK_BUFFER_CREATE_SPARSE_BINDING_BIT )
            {
                auto* pMemoryBindingsVector = std::get_if<std::vector<DeviceProfilerBufferMemoryBindingData>>( &m_MemoryBindings );
                if( pMemoryBindingsVector )
                {
                    return pMemoryBindingsVector->data();
                }
            }

            auto* pMemoryBinding = std::get_if<DeviceProfilerBufferMemoryBindingData>( &m_MemoryBindings );
            if( pMemoryBinding )
            {
                return pMemoryBinding;
            }

            return nullptr;
        }
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerImageMemoryBindingData

    Description:

    \***********************************************************************************/
    enum class DeviceProfilerImageMemoryBindingType : uint32_t
    {
        eOpaque = 0,
        eBlock = 1
    };

    struct DeviceProfilerImageOpaqueMemoryBindingData
    {
        VkDeviceMemoryHandle m_Memory;
        VkDeviceSize m_MemoryOffset;
        VkDeviceSize m_ImageOffset;
        VkDeviceSize m_Size;
    };

    struct DeviceProfilerImageBlockMemoryBindingData
    {
        VkDeviceMemoryHandle m_Memory;
        VkDeviceSize m_MemoryOffset;
        VkImageSubresource m_ImageSubresource;
        VkOffset3D m_ImageOffset;
        VkExtent3D m_ImageExtent;
    };

    struct DeviceProfilerImageMemoryBindingData
    {
        DeviceProfilerImageMemoryBindingType m_Type;
        union
        {
            DeviceProfilerImageBlockMemoryBindingData m_Block;
            DeviceProfilerImageOpaqueMemoryBindingData m_Opaque;
        };

        DeviceProfilerImageMemoryBindingData()
        {
        }

        DeviceProfilerImageMemoryBindingData(const DeviceProfilerImageOpaqueMemoryBindingData& data)
            : m_Type( DeviceProfilerImageMemoryBindingType::eOpaque )
            , m_Opaque( data )
        {
        }

        DeviceProfilerImageMemoryBindingData( const DeviceProfilerImageBlockMemoryBindingData& data )
            : m_Type( DeviceProfilerImageMemoryBindingType::eBlock )
            , m_Block( data )
        {
        }
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerImageMemoryData

    Description:

    \***********************************************************************************/
    struct DeviceProfilerImageMemoryData
    {
        typedef std::variant<DeviceProfilerImageMemoryBindingData, std::vector<DeviceProfilerImageMemoryBindingData>>
            MemoryBindings;

        VkExtent3D m_ImageExtent = {};
        VkFormat m_ImageFormat = {};
        VkImageType m_ImageType = {};
        VkImageCreateFlags m_ImageFlags = {};
        VkImageUsageFlags m_ImageUsage = {};
        VkImageTiling m_ImageTiling = {};
        uint32_t m_ImageMipLevels = {};
        uint32_t m_ImageArrayLayers = {};
        VkMemoryRequirements m_MemoryRequirements = {};
        std::vector<VkSparseImageMemoryRequirements> m_SparseMemoryRequirements = {};
        MemoryBindings m_MemoryBindings;

        // Interface for querying number of memory bindings.
        // By default images are bound to single memory block, unless sparse binding is enabled.
        inline size_t GetMemoryBindingCount() const
        {
            if( m_ImageFlags & VK_IMAGE_CREATE_SPARSE_BINDING_BIT )
            {
                auto* pMemoryBindingsVector = std::get_if<std::vector<DeviceProfilerImageMemoryBindingData>>( &m_MemoryBindings );
                if( pMemoryBindingsVector )
                {
                    return pMemoryBindingsVector->size();
                }
            }

            auto* pMemoryBinding = std::get_if<DeviceProfilerImageMemoryBindingData>( &m_MemoryBindings );
            if( pMemoryBinding )
            {
                return 1;
            }

            return 0;
        }

        // Interface for getting pointer to the bound memory blocks.
        // By default images are bound to single memory block, unless sparse binding is enabled.
        inline const DeviceProfilerImageMemoryBindingData* GetMemoryBindings() const
        {
            if( m_ImageFlags & VK_IMAGE_CREATE_SPARSE_BINDING_BIT )
            {
                auto* pMemoryBindingsVector = std::get_if<std::vector<DeviceProfilerImageMemoryBindingData>>( &m_MemoryBindings );
                if( pMemoryBindingsVector )
                {
                    return pMemoryBindingsVector->data();
                }
            }

            auto* pMemoryBinding = std::get_if<DeviceProfilerImageMemoryBindingData>( &m_MemoryBindings );
            if( pMemoryBinding )
            {
                return pMemoryBinding;
            }

            return nullptr;
        }
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerAccelerationStructureMemoryData

    Description:

    \***********************************************************************************/
    struct DeviceProfilerAccelerationStructureMemoryData
    {
        VkAccelerationStructureTypeKHR m_Type = {};
        VkAccelerationStructureCreateFlagsKHR m_Flags = {};
        VkBufferHandle m_Buffer = {};
        VkDeviceSize m_Offset = {};
        VkDeviceSize m_Size = {};
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerMicromapMemoryData

    Description:

    \***********************************************************************************/
    struct DeviceProfilerMicromapMemoryData
    {
        VkMicromapTypeEXT m_Type = {};
        VkMicromapCreateFlagsEXT m_Flags = {};
        VkBufferHandle m_Buffer = {};
        VkDeviceSize m_Offset = {};
        VkDeviceSize m_Size = {};
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

        std::unordered_map<VkDeviceMemoryHandle, struct DeviceProfilerDeviceMemoryData> m_Allocations = {};
        std::unordered_map<VkBufferHandle, struct DeviceProfilerBufferMemoryData> m_Buffers = {};
        std::unordered_map<VkImageHandle, struct DeviceProfilerImageMemoryData> m_Images = {};
        std::unordered_map<VkAccelerationStructureKHRHandle, struct DeviceProfilerAccelerationStructureMemoryData>
            m_AccelerationStructures = {};
        std::unordered_map<VkMicromapEXTHandle, struct DeviceProfilerMicromapMemoryData> m_Micromaps = {};
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
        uint64_t                                            m_PerformanceCountersHostCalibratedTimestamp = {};
        uint64_t                                            m_PerformanceCountersDeviceCalibratedTimestamp = {};
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
        uint64_t                                            m_BeginTimestamp = {};
        uint64_t                                            m_EndTimestamp = {};

        VkProfilerFrameDelimiterEXT                         m_FrameDelimiter = {};

        DeviceProfilerMemoryData                            m_Memory = {};
        DeviceProfilerCPUData                               m_CPU = {};
        std::vector<struct TipRange>                        m_TIP = {};

        DeviceProfilerPerformanceCountersData               m_PerformanceCounters = {};

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

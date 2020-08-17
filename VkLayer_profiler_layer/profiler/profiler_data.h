#pragma once
#include "profiler_shader.h"
#include <assert.h>
#include <vector>
#include <list>
#include <cstring>
#include <vulkan/vulkan.h>
// Import extension structures
#include "profiler_ext/VkProfilerEXT.h"

namespace Profiler
{
    template<typename T> using ContainerType = std::list<T>;

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
        eDebugLabel = 0x00000001,
        eDraw = 0x00010000,
        eDrawIndexed = 0x00010001,
        eDrawIndirect = 0x00010002,
        eDrawIndexedIndirect = 0x00010003,
        eDrawIndirectCount = 0x00010004,
        eDrawIndexedIndirectCount = 0x00010005,
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
        eUpdateBuffer = 0x000D0000
    };

    /***********************************************************************************\

    Enumeration:
        ProfilerPipelineType

    Description:

    \***********************************************************************************/
    enum class DeviceProfilerPipelineType : uint32_t
    {
        eNone = 0x00000000,
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
        eUpdatBuffer = 0x000D0000,
        eBeginRenderPass = 0x000BFFFF,
        eEndRenderPass = 0x000EFFFF
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

    using DeviceProfilerDrawcallDrawIndexedIndirectPayload =
        struct DeviceProfilerDrawcallDrawIndirectPayload;

    struct DeviceProfilerDrawcallDrawIndirectCountPayload
    {
        VkBuffer m_Buffer;
        VkDeviceSize m_Offset;
        VkBuffer m_CountBuffer;
        VkDeviceSize m_CountOffset;
        uint32_t m_MaxDrawCount;
        uint32_t m_Stride;
    };

    using DeviceProfilerDrawcallDrawIndexedIndirectCountPayload =
        struct DeviceProfilerDrawcallDrawIndirectCountPayload;

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

    /***********************************************************************************\

    Structure:
        ProfilerDrawcallPayload

    Description:
        Contains data associated with the drawcall.

    \***********************************************************************************/
    union DeviceProfilerDrawcallPayload
    {
        DeviceProfilerDrawcallDebugLabelPayload m_DebugLabel;
        DeviceProfilerDrawcallDrawPayload m_Draw;
        DeviceProfilerDrawcallDrawIndexedPayload m_DrawIndexed;
        DeviceProfilerDrawcallDrawIndirectPayload m_DrawIndirect;
        DeviceProfilerDrawcallDrawIndexedIndirectPayload m_DrawIndexedIndirect;
        DeviceProfilerDrawcallDrawIndirectCountPayload m_DrawIndirectCount;
        DeviceProfilerDrawcallDrawIndexedIndirectCountPayload m_DrawIndexedIndirectCount;
        DeviceProfilerDrawcallDispatchPayload m_Dispatch;
        DeviceProfilerDrawcallDispatchIndirectPayload m_DispatchIndirect;
        DeviceProfilerDrawcallCopyBufferPayload m_CopyBuffer;
        DeviceProfilerDrawcallCopyBufferToImagePayload m_CopyBufferToImage;
        DeviceProfilerDrawcallCopyImagePayload m_CopyImage;
        DeviceProfilerDrawcallCopyImageToBufferPayload m_CopyImageToBuffer;
        DeviceProfilerDrawcallClearAttachmentsPayload m_ClearAttachments;
        DeviceProfilerDrawcallClearColorImagePayload m_ClearColorImage;
        DeviceProfilerDrawcallClearDepthStencilImagePayload m_ClearDepthStencilImage;
        DeviceProfilerDrawcallResolveImagePayload m_ResolveImage;
        DeviceProfilerDrawcallBlitImagePayload m_BlitImage;
        DeviceProfilerDrawcallFillBufferPayload m_FillBuffer;
        DeviceProfilerDrawcallUpdateBufferPayload m_UpdateBuffer;
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
        uint64_t                                            m_Ticks = {};

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
            , m_Ticks( dc.m_Ticks )
        {
            if( dc.m_Type == DeviceProfilerDrawcallType::eDebugLabel )
            {
                // Create copy of already stored string
                m_Payload.m_DebugLabel.m_pName = strdup( dc.m_Payload.m_DebugLabel.m_pName );
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
            if( m_Type == DeviceProfilerDrawcallType::eDebugLabel )
            {
                free( m_Payload.m_DebugLabel.m_pName );
            }
        }

        // Swap data of 2 drawcalls
        inline void Swap( DeviceProfilerDrawcall& dc )
        {
            std::swap( m_Type, dc.m_Type );
            std::swap( m_Payload, dc.m_Payload );
            std::swap( m_Ticks, dc.m_Ticks );
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
        Stores number of drawcalls.

    \***********************************************************************************/
    struct DeviceProfilerDrawcallStats
    {
        uint32_t m_DrawCount = {};
        uint32_t m_DrawIndirectCount = {};
        uint32_t m_DispatchCount = {};
        uint32_t m_DispatchIndirectCount = {};
        uint32_t m_CopyBufferCount = {};
        uint32_t m_CopyBufferToImageCount = {};
        uint32_t m_CopyImageCount = {};
        uint32_t m_CopyImageToBufferCount = {};
        uint32_t m_ClearColorCount = {};
        uint32_t m_ClearDepthStencilCount = {};
        uint32_t m_ResolveCount = {};
        uint32_t m_BlitImageCount = {};
        uint32_t m_FillBufferCount = {};
        uint32_t m_UpdateBufferCount = {};
        uint32_t m_PipelineBarrierCount = {};

        // Stat aggregation helper
        inline DeviceProfilerDrawcallStats& operator+=( const DeviceProfilerDrawcallStats& rh )
        {
            m_DrawCount += rh.m_DrawCount;
            m_DrawIndirectCount += rh.m_DrawIndirectCount;
            m_DispatchCount += rh.m_DispatchCount;
            m_DispatchIndirectCount += rh.m_DispatchIndirectCount;
            m_CopyBufferCount += rh.m_CopyBufferCount;
            m_CopyBufferToImageCount += rh.m_CopyBufferToImageCount;
            m_CopyImageCount += rh.m_CopyImageCount;
            m_CopyImageToBufferCount += rh.m_CopyImageToBufferCount;
            m_ClearColorCount += rh.m_ClearColorCount;
            m_ClearDepthStencilCount += rh.m_ClearDepthStencilCount;
            m_ResolveCount += rh.m_ResolveCount;
            m_BlitImageCount += rh.m_BlitImageCount;
            m_FillBufferCount += rh.m_FillBufferCount;
            m_UpdateBufferCount += rh.m_UpdateBufferCount;
            m_PipelineBarrierCount += rh.m_PipelineBarrierCount;
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
        VkPipeline                                          m_Handle = {};
        VkPipelineBindPoint                                 m_BindPoint = {};
        ProfilerShaderTuple                                 m_ShaderTuple = {};
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerPipelineData

    Description:
        Contains data collected per-pipeline.

    \***********************************************************************************/
    struct DeviceProfilerPipelineData
    {
        VkPipeline                                          m_Handle = {};
        uint32_t                                            m_Hash = {};
        uint64_t                                            m_Ticks = {};
        ContainerType<struct DeviceProfilerDrawcall>        m_Drawcalls = {};

        inline DeviceProfilerPipelineData() = default;

        inline DeviceProfilerPipelineData( const DeviceProfilerPipeline& pipeline )
            : m_Handle( pipeline.m_Handle )
            , m_Hash( pipeline.m_ShaderTuple.m_Hash )
        {
        }

        inline bool operator==( const DeviceProfilerPipelineData& rh ) const
        {
            return m_Hash == rh.m_Hash;
        }
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
        uint64_t                                            m_Ticks = {};

        ContainerType<struct DeviceProfilerPipelineData>    m_Pipelines = {};
        ContainerType<struct DeviceProfilerCommandBufferData> m_SecondaryCommandBuffers = {};
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
        std::vector<struct DeviceProfilerSubpass>           m_Subpasses = {};
        uint32_t                                            m_ClearColorAttachmentCount = {};
        uint32_t                                            m_ClearDepthStencilAttachmentCount = {};
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
        uint64_t                                            m_Ticks = {};
        uint64_t                                            m_BeginTicks = {};
        uint64_t                                            m_EndTicks = {};

        ContainerType<struct DeviceProfilerSubpassData>     m_Subpasses = {};
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
        uint64_t                                            m_Ticks = {};

        ContainerType<struct DeviceProfilerRenderPassData>  m_RenderPasses = {};

        std::vector<char>                                   m_PerformanceQueryReportINTEL = {};

        uint64_t                                            m_ProfilerCpuOverheadNs = {};
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
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerMemoryData

    Description:

    \***********************************************************************************/
    struct DeviceProfilerMemoryData
    {
        uint64_t m_TotalAllocationSize;
        uint64_t m_TotalAllocationCount;
        uint64_t m_DeviceLocalAllocationSize;
        uint64_t m_HostVisibleAllocationSize;
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerCPUData

    Description:

    \***********************************************************************************/
    struct DeviceProfilerCPUData
    {
        uint64_t m_TimeNs;
        uint64_t m_CommandBufferLookupTimeNs;
        uint64_t m_PipelineLookupTimeNs;
        uint64_t m_RenderPassLookupTimeNs;
        uint64_t m_CommandBufferProfilerCpuOverheadNs;
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

        std::vector<VkProfilerPerformanceCounterResultEXT>  m_VendorMetrics = {};
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
            return pipeline.m_Hash;
        }
    };
}

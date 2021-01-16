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

#include "VkProfilerEXT.h"
#include "VkDevice_functions.h"

using namespace Profiler;

/***************************************************************************************\

Class:
    RegionBuilder

Description:
    Helper class for filling VkProfilerRegionDataEXT structures.

\***************************************************************************************/
class RegionBuilder
{
private:
    template<typename T>
    inline static T safe_cast( size_t value )
    {
        assert( value <= std::numeric_limits<T>::max() );
        return static_cast<T>(value);
    }

    template<typename T>
    inline static VkResult safe_alloc( size_t count, T** pptr )
    {
        (*pptr) = new (std::nothrow) T[ count ];
        if( !(*pptr) )
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        return VK_SUCCESS;
    }

    template<typename T>
    inline static void safe_free( T* ptr )
    {
        delete[] ptr;
    }

    template<typename DataType, typename CallbackType>
    inline VkResult SerializeSubregions(
        const ContainerType<DataType>& data,
        CallbackType callback,
        VkProfilerRegionDataEXT& out )
    {
        // Subregions (VkRenderPass)
        out.subregionCount = safe_cast<uint32_t>( data.size() );

        // Allocate space for subregion list
        VkResult result = safe_alloc<VkProfilerRegionDataEXT>( data.size(), &out.pSubregions );

        // Data is stored in lists so indexing cannot be used
        auto it = data.begin();

        for( uint32_t i = 0; (result == VK_SUCCESS) && (i < data.size()); ++i, ++it )
        {
            // Serialize VkRenderPass
            result = (this->*callback)( *it, out.pSubregions[ i ] );
        }

        if( result != VK_SUCCESS )
        {
            // Revert from partially-initialized state
            FreeProfilerRegion( out );
        }

        assert( (result != VK_SUCCESS) || (it == data.end()) );
        return result;
    }

    inline VkProfilerCommandTypeEXT DrawcallTypeToCommandType( DeviceProfilerDrawcallType type ) const
    {
        static const std::unordered_map<DeviceProfilerDrawcallType, VkProfilerCommandTypeEXT> commandTypes =
        {
            { DeviceProfilerDrawcallType::eDraw,                        VK_PROFILER_COMMAND_DRAW_EXT },
            { DeviceProfilerDrawcallType::eDrawIndexed,                 VK_PROFILER_COMMAND_DRAW_INDEXED_EXT },
            { DeviceProfilerDrawcallType::eDrawIndirect,                VK_PROFILER_COMMAND_DRAW_INDIRECT_EXT },
            { DeviceProfilerDrawcallType::eDrawIndexedIndirect,         VK_PROFILER_COMMAND_DRAW_INDEXED_INDIRECT_EXT },
            { DeviceProfilerDrawcallType::eDrawIndirectCount,           VK_PROFILER_COMMAND_DRAW_INDIRECT_COUNT_EXT },
            { DeviceProfilerDrawcallType::eDrawIndexedIndirectCount,    VK_PROFILER_COMMAND_DRAW_INDEXED_INDIRECT_COUNT_EXT },
            { DeviceProfilerDrawcallType::eDispatch,                    VK_PROFILER_COMMAND_DISPATCH_EXT },
            { DeviceProfilerDrawcallType::eDispatchIndirect,            VK_PROFILER_COMMAND_DISPATCH_INDIRECT_EXT },
            { DeviceProfilerDrawcallType::eCopyBuffer,                  VK_PROFILER_COMMAND_COPY_BUFFER_EXT },
            { DeviceProfilerDrawcallType::eCopyBufferToImage,           VK_PROFILER_COMMAND_COPY_BUFFER_TO_IMAGE_EXT },
            { DeviceProfilerDrawcallType::eCopyImage,                   VK_PROFILER_COMMAND_COPY_IMAGE_EXT },
            { DeviceProfilerDrawcallType::eCopyImageToBuffer,           VK_PROFILER_COMMAND_COPY_IMAGE_TO_BUFFER_EXT },
            { DeviceProfilerDrawcallType::eClearAttachments,            VK_PROFILER_COMMAND_CLEAR_ATTACHMENTS_EXT },
            { DeviceProfilerDrawcallType::eClearColorImage,             VK_PROFILER_COMMAND_CLEAR_COLOR_IMAGE_EXT },
            { DeviceProfilerDrawcallType::eClearDepthStencilImage,      VK_PROFILER_COMMAND_CLEAR_DEPTH_STENCIL_IMAGE_EXT },
            { DeviceProfilerDrawcallType::eResolveImage,                VK_PROFILER_COMMAND_RESOLVE_IMAGE_EXT },
            { DeviceProfilerDrawcallType::eBlitImage,                   VK_PROFILER_COMMAND_BLIT_IMAGE_EXT },
            { DeviceProfilerDrawcallType::eFillBuffer,                  VK_PROFILER_COMMAND_FILL_BUFFER_EXT },
            { DeviceProfilerDrawcallType::eUpdateBuffer,                VK_PROFILER_COMMAND_UPDATE_BUFFER_EXT }
        };

        auto it = commandTypes.find( type );
        if( it != commandTypes.end() )
        {
            return it->second;
        }

        return VK_PROFILER_COMMAND_UNKNOWN_EXT;
    }

private:
    const float m_TimestampPeriodMs;

public:
    RegionBuilder( float timestampPeriod )
        : m_TimestampPeriodMs( timestampPeriod / 1000000.f )
    {
    }

    // Profiler region deallocation helper
    inline static void FreeProfilerRegion( VkProfilerRegionDataEXT& region )
    {
        for( uint32_t i = 0; i < region.subregionCount; ++i )
        {
            // Free subregions recursively
            FreeProfilerRegion( region.pSubregions[ i ] );
        }

        safe_free( region.pSubregions );

        region.subregionCount = 0;
        region.pSubregions = nullptr;

        // Free pNext chain
        VkBaseOutStructure* pStruct = reinterpret_cast<VkBaseOutStructure*>(region.pNext);

        while( pStruct )
        {
            VkBaseOutStructure* pNext = pStruct->pNext;
            safe_free( pStruct );
            pStruct = pNext;
        }

        region.pNext = nullptr;
    }

    // Drawcall serialization helper
    inline VkResult SerializeDrawcall( const DeviceProfilerDrawcall& data, VkProfilerRegionDataEXT& out )
    {
        out.sType = VK_STRUCTURE_TYPE_PROFILER_REGION_DATA_EXT;
        out.pNext = nullptr;
        out.regionType = VK_PROFILER_REGION_TYPE_COMMAND_EXT;
        out.duration = (data.m_EndTimestamp - data.m_BeginTimestamp) * m_TimestampPeriodMs;
        out.properties.command.type = DrawcallTypeToCommandType( data.m_Type );
        out.subregionCount = 0;
        out.pSubregions = nullptr;
        return VK_SUCCESS;
    }

    // Pipeline serialization helper
    inline VkResult SerializePipeline( const DeviceProfilerPipelineData& data, VkProfilerRegionDataEXT& out )
    {
        out.sType = VK_STRUCTURE_TYPE_PROFILER_REGION_DATA_EXT;
        out.pNext = nullptr;
        out.regionType = VK_PROFILER_REGION_TYPE_PIPELINE_EXT;
        out.duration = (data.m_EndTimestamp - data.m_BeginTimestamp) * m_TimestampPeriodMs;
        out.properties.pipeline.handle = data.m_Handle;
        return SerializeSubregions( data.m_Drawcalls, &RegionBuilder::SerializeDrawcall, out );
    }

    // Subpass serialization helper
    inline VkResult SerializeSubpass( const DeviceProfilerSubpassData& data, VkProfilerRegionDataEXT& out )
    {
        out.sType = VK_STRUCTURE_TYPE_PROFILER_REGION_DATA_EXT;
        out.pNext = nullptr;
        out.regionType = VK_PROFILER_REGION_TYPE_SUBPASS_EXT;
        out.duration = (data.m_EndTimestamp - data.m_BeginTimestamp) * m_TimestampPeriodMs;
        out.properties.subpass.index = data.m_Index;
        out.properties.subpass.contents = data.m_Contents;

        switch( data.m_Contents )
        {
        case VK_SUBPASS_CONTENTS_INLINE:
            return SerializeSubregions( data.m_Pipelines, &RegionBuilder::SerializePipeline, out );

        case VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS:
            return SerializeSubregions( data.m_SecondaryCommandBuffers, &RegionBuilder::SerializeCommandBuffer, out );
        }

        assert( !"Invalid subpass contents" );
        return VK_ERROR_UNKNOWN;
    }

    // VkRenderPass serialization helper
    inline VkResult SerializeRenderPass( const DeviceProfilerRenderPassData& data, VkProfilerRegionDataEXT& out )
    {
        out.sType = VK_STRUCTURE_TYPE_PROFILER_REGION_DATA_EXT;
        out.regionType = VK_PROFILER_REGION_TYPE_RENDER_PASS_EXT;
        out.duration = (data.m_EndTimestamp - data.m_BeginTimestamp) * m_TimestampPeriodMs;
        out.properties.renderPass.handle = data.m_Handle;

        VkProfilerRenderPassDataEXT* pRenderPassData = nullptr;
        VkResult result = safe_alloc<VkProfilerRenderPassDataEXT>( 1, &pRenderPassData );

        if( result == VK_SUCCESS )
        {
            // Fill additional render pass data
            pRenderPassData->sType = VK_STRUCTURE_TYPE_PROFILER_RENDER_PASS_DATA_EXT;
            pRenderPassData->pNext = nullptr;
            pRenderPassData->beginDuration = (data.m_Begin.m_EndTimestamp - data.m_Begin.m_BeginTimestamp) * m_TimestampPeriodMs;
            pRenderPassData->endDuration = (data.m_End.m_EndTimestamp - data.m_End.m_BeginTimestamp) * m_TimestampPeriodMs;
            out.pNext = pRenderPassData;

            result = SerializeSubregions( data.m_Subpasses, &RegionBuilder::SerializeSubpass, out );
        }

        return result;
    }

    // VkCommandBuffer serialization helper
    inline VkResult SerializeCommandBuffer( const DeviceProfilerCommandBufferData& data, VkProfilerRegionDataEXT& out )
    {
        out.sType = VK_STRUCTURE_TYPE_PROFILER_REGION_DATA_EXT;
        out.pNext = nullptr;
        out.regionType = VK_PROFILER_REGION_TYPE_COMMAND_BUFFER_EXT;
        out.duration = (data.m_EndTimestamp - data.m_BeginTimestamp) * m_TimestampPeriodMs;
        out.properties.commandBuffer.handle = data.m_Handle;
        out.properties.commandBuffer.level = data.m_Level;
        return SerializeSubregions( data.m_RenderPasses, &RegionBuilder::SerializeRenderPass, out );
    }

    // VkSubmitInfo serialization helper
    inline VkResult SerializeSubmitInfo( const DeviceProfilerSubmitData& data, VkProfilerRegionDataEXT& out )
    {
        out.sType = VK_STRUCTURE_TYPE_PROFILER_REGION_DATA_EXT;
        out.pNext = nullptr;
        out.regionType = VK_PROFILER_REGION_TYPE_SUBMIT_INFO_EXT;
        out.duration = 0;
        return SerializeSubregions( data.m_CommandBuffers, &RegionBuilder::SerializeCommandBuffer, out );
    }

    // vkQueueSubmit serialization helper
    inline VkResult SerializeSubmit( const DeviceProfilerSubmitBatchData& data, VkProfilerRegionDataEXT& out )
    {
        out.sType = VK_STRUCTURE_TYPE_PROFILER_REGION_DATA_EXT;
        out.pNext = nullptr;
        out.regionType = VK_PROFILER_REGION_TYPE_SUBMIT_EXT;
        out.duration = 0;
        out.properties.submit.queue = data.m_Handle;
        return SerializeSubregions( data.m_Submits, &RegionBuilder::SerializeSubmitInfo, out );
    }

    // Frame serialization helper
    inline VkResult SerializeFrame( const DeviceProfilerFrameData& data, VkProfilerRegionDataEXT& out )
    {
        out.sType = VK_STRUCTURE_TYPE_PROFILER_REGION_DATA_EXT;
        out.pNext = nullptr;
        out.regionType = VK_PROFILER_REGION_TYPE_FRAME_EXT;
        out.duration = data.m_Ticks * m_TimestampPeriodMs;
        return SerializeSubregions( data.m_Submits, &RegionBuilder::SerializeSubmit, out );
    }
};

/***************************************************************************************\

Function:
    vkSetProfilerModeEXT

Description:

\***************************************************************************************/
VKAPI_ATTR VkResult VKAPI_CALL vkSetProfilerModeEXT(
    VkDevice device,
    VkProfilerModeEXT mode )
{
    auto& dd = VkDevice_Functions::DeviceDispatch.Get( device );

    VkResult result = (VkResult)VK_ERROR_NOT_AVAILABLE_EXT;

    if( dd.Profiler.IsAvailable() )
    {
        // Set new mode
        result = dd.Profiler.SetMode( mode );
    }

    return result;
}

/***************************************************************************************\

Function:
    vkSetProfilerSyncModeEXT

Description:

\***************************************************************************************/
VKAPI_ATTR VkResult VKAPI_CALL vkSetProfilerSyncModeEXT(
    VkDevice device,
    VkProfilerSyncModeEXT syncMode )
{
    auto& dd = VkDevice_Functions::DeviceDispatch.Get( device );

    VkResult result = (VkResult)VK_ERROR_NOT_AVAILABLE_EXT;

    if( dd.Profiler.IsAvailable() )
    {
        // Set new synchronization mode
        result = dd.Profiler.SetSyncMode( syncMode );
    }

    return result;
}

/***************************************************************************************\

Function:
    vkGetProfilerFrameDataEXT

Description:
    Fill provided structure with data collected during the previous frame.
    Result values:
     VK_SUCCESS - function succeeded
     VK_NOT_READY - function called before first call to vkQueuePresentKHR
       or no profiling data available

\***************************************************************************************/
VKAPI_ATTR VkResult VKAPI_CALL vkGetProfilerFrameDataEXT(
    VkDevice device,
    VkProfilerDataEXT* pData )
{
    auto& dd = VkDevice_Functions::DeviceDispatch.Get( device );

    VkResult result = (VkResult)VK_ERROR_NOT_AVAILABLE_EXT;

    if( dd.Profiler.IsAvailable() )
    {
        // Get latest data from profiler
        DeviceProfilerFrameData data = dd.Profiler.GetData();

        if( !data.m_Submits.empty() )
        {
            // Serialize last frame
            result = RegionBuilder( dd.Device.pPhysicalDevice->Properties.limits.timestampPeriod )
                .SerializeFrame( data, pData->frame );
        }
        else
        {
            // Data not ready yet
            //  Check if application called vkQueuePresentKHR
            result = VK_NOT_READY;
        }
    }

    return result;
}

/***************************************************************************************\

Function:
    vkEnumerateProfilerPerformanceCounterPropertiesEXT

Description:

\***************************************************************************************/
VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateProfilerPerformanceCounterPropertiesEXT(
    VkDevice device,
    uint32_t* pProfilerMetricCount,
    VkProfilerPerformanceCounterPropertiesEXT* pProfilerMetricProperties )
{
    auto& dd = VkDevice_Functions::DeviceDispatch.Get( device );

    VkResult result = (VkResult)VK_ERROR_NOT_AVAILABLE_EXT;

    if( dd.Profiler.IsAvailable() )
    {
        // If profiler is available and no metrics are collected its still quite a success
        result = VK_SUCCESS;

        bool hasSufficientSpace = true;

        if( dd.Profiler.m_MetricsApiINTEL.IsAvailable() )
        {
            if( (*pProfilerMetricCount) == 0 )
            {
                // Return number of reported metrics
                (*pProfilerMetricCount) += dd.Profiler.m_MetricsApiINTEL.GetMetricsCount();
            }
            else
            {
                // Get reported metrics descriptions
                const std::vector<VkProfilerPerformanceCounterPropertiesEXT> intelMetricsProperties =
                    dd.Profiler.m_MetricsApiINTEL.GetMetricsProperties();

                const uint32_t returnedMetricPropertyCount =
                    std::template min<uint32_t>( (*pProfilerMetricCount), intelMetricsProperties.size() );

                std::memcpy( pProfilerMetricProperties,
                    intelMetricsProperties.data(),
                    returnedMetricPropertyCount * sizeof( VkProfilerPerformanceCounterPropertiesEXT ) );

                // There may be other metric sources that will be appended to the end
                pProfilerMetricProperties += returnedMetricPropertyCount;
                (*pProfilerMetricCount) -= returnedMetricPropertyCount;

                if( returnedMetricPropertyCount < intelMetricsProperties.size() )
                {
                    hasSufficientSpace = false;
                }
            }
        }

        // TODO: Other metric sources (VK_KHR_performance_query)

        if( (result == VK_SUCCESS) && !(hasSufficientSpace) )
        {
            // All vkEnumerate* functions return VK_INCOMPLETE when provided buffer was too small
            result = VK_INCOMPLETE;
        }
    }

    return result;
}

/***************************************************************************************\

Function:
    vkFreeProfilerFrameDataEXT

Description:

\***************************************************************************************/
VKAPI_ATTR void VKAPI_CALL vkFreeProfilerFrameDataEXT(
    VkDevice device,
    VkProfilerDataEXT* pData )
{
    RegionBuilder::FreeProfilerRegion( pData->frame );
}

/***************************************************************************************\

Function:
    vkFlushProfilerEXT

Description:
    Collect data submitted so far and begin next profiling run.
    Has the same effect as vkQueuePresentKHR except presenting anything.

\***************************************************************************************/
VKAPI_ATTR VkResult VKAPI_CALL vkFlushProfilerEXT(
    VkDevice device )
{
    auto& dd = VkDevice_Functions::DeviceDispatch.Get( device );

    VkResult result = (VkResult)VK_ERROR_NOT_AVAILABLE_EXT;

    if( dd.Profiler.IsAvailable() )
    {
        dd.Profiler.FinishFrame();
        result = VK_SUCCESS;
    }

    return result;
}

// Copyright (c) 2024-2026 Lukasz Stalmirski
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

#include "profiler_data.h"

template<typename T>
constexpr size_t GetPNextChainSize( const T* pStructure )
{
    // By default no pNext chains are captured.
    return 0;
}

template<typename T>
constexpr void* CopyPNextChain( const T* pStructure, std::byte** ppNext )
{
    // By default no pNext chains are captured.
    return nullptr;
}

template<typename T>
size_t GetStructureSize( const T* pStructure )
{
    if( pStructure != nullptr )
    {
        return sizeof( T ) +
               GetPNextChainSize( pStructure );
    }
    return 0;
}

template<typename T>
size_t GetStructureArraySize( const T* pStructures, size_t count )
{
    size_t size = 0;
    if( pStructures != nullptr )
    {
        for( size_t i = 0; i < count; ++i )
        {
            size += GetStructureSize( &pStructures[i] );
        }
    }
    return size;
}

template<typename T>
size_t GetStructureArraySize( const T* const* ppStructures, size_t count )
{
    size_t size = 0;
    if( ppStructures != nullptr )
    {
        for( size_t i = 0; i < count; ++i )
        {
            size += sizeof( T* ) +
                    GetStructureSize( ppStructures[i] );
        }
    }
    return size;
}

template<typename T>
void CopyStructureTo( T* pDst, const T* pSrc, std::byte** ppNext )
{
    if( pSrc != nullptr )
    {
        memcpy( pDst, pSrc, sizeof( T ) );
    }
}

template<typename T>
T* CopyStructure( const T* pSrc, std::byte** ppNext )
{
    if( pSrc != nullptr )
    {
        T* pDst = reinterpret_cast<T*>( *ppNext );
        *ppNext += sizeof( T );
        CopyStructureTo( pDst, pSrc, ppNext );
        return pDst;
    }
    return nullptr;
}

template<typename T>
T* CopyStructureArray( const T* pSrc, uint32_t count, std::byte** ppNext )
{
    if( pSrc != nullptr && count > 0 )
    {
        T* pDst = reinterpret_cast<T*>( *ppNext );
        *ppNext += sizeof( T ) * count;
        for( uint32_t i = 0; i < count; ++i )
        {
            CopyStructureTo( &pDst[i], &pSrc[i], ppNext );
        }
        return pDst;
    }
    return nullptr;
}

template<typename T>
T** CopyStructureArray( const T* const* ppSrc, uint32_t count, std::byte** ppNext )
{
    if( ppSrc != nullptr && count > 0 )
    {
        T** ppDst = reinterpret_cast<T**>( *ppNext );
        *ppNext += sizeof( T* ) * count;
        for( uint32_t i = 0; i < count; ++i )
        {
            ppDst[i] = CopyStructure( ppSrc[i], ppNext );
        }
        return ppDst;
    }
    return nullptr;
}

template<typename T>
void* AllocateMemoryForStructures( const T* pStructure, size_t count = 1 )
{
    size_t size = GetStructureArraySize( pStructure, count );
    if( size == 0 )
    {
        return nullptr;
    }
    return malloc( size );
}

template<>
size_t GetPNextChainSize( const VkAccelerationStructureBuildGeometryInfoKHR* pStructure )
{
    size_t size = 0;
    for( const auto& structure : Profiler::PNextIterator( pStructure->pNext ) )
    {
        switch( structure.sType )
        {
        case VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_MICROMAP_DATA_KHR:
            size += GetStructureSize( reinterpret_cast<const VkAccelerationStructureGeometryMicromapDataKHR*>( &structure ) );
            break;
        }
    }
    return size;
}

template<>
size_t GetStructureSize( const VkPipelineVertexInputStateCreateInfo* pStructure )
{
    if( pStructure != nullptr )
    {
        return sizeof( VkPipelineVertexInputStateCreateInfo ) +
               GetPNextChainSize( pStructure ) +
               GetStructureArraySize( pStructure->pVertexBindingDescriptions, pStructure->vertexBindingDescriptionCount ) +
               GetStructureArraySize( pStructure->pVertexAttributeDescriptions, pStructure->vertexAttributeDescriptionCount );
    }
    return 0;
}

template<>
size_t GetStructureSize( const VkPipelineViewportStateCreateInfo* pStructure )
{
    if( pStructure != nullptr )
    {
        return sizeof( VkPipelineViewportStateCreateInfo ) +
               GetPNextChainSize( pStructure ) +
               GetStructureArraySize( pStructure->pViewports, pStructure->viewportCount ) +
               GetStructureArraySize( pStructure->pScissors, pStructure->scissorCount );
    }
    return 0;
}

template<>
size_t GetStructureSize( const VkPipelineMultisampleStateCreateInfo* pStructure )
{
    if( pStructure != nullptr )
    {
        return sizeof( VkPipelineMultisampleStateCreateInfo ) +
               GetPNextChainSize( pStructure ) +
               GetStructureSize( pStructure->pSampleMask );
    }
    return 0;
}

template<>
size_t GetStructureSize( const VkPipelineColorBlendStateCreateInfo* pStructure )
{
    if( pStructure != nullptr )
    {
        return sizeof( VkPipelineColorBlendStateCreateInfo ) +
               GetPNextChainSize( pStructure ) +
               GetStructureArraySize( pStructure->pAttachments, pStructure->attachmentCount );
    }
    return 0;
}

template<>
size_t GetStructureSize( const VkPipelineDynamicStateCreateInfo* pStructure )
{
    if( pStructure != nullptr )
    {
        return sizeof( VkPipelineDynamicStateCreateInfo ) +
               GetPNextChainSize( pStructure ) +
               GetStructureArraySize( pStructure->pDynamicStates, pStructure->dynamicStateCount );
    }
    return 0;
}

template<>
size_t GetStructureSize( const VkRayTracingPipelineInterfaceCreateInfoKHR* pStructure )
{
    if( pStructure != nullptr )
    {
        return sizeof( VkRayTracingPipelineInterfaceCreateInfoKHR ) +
               GetPNextChainSize( pStructure );
    }
    return 0;
}

template<>
size_t GetStructureSize( const VkGraphicsPipelineCreateInfo* pStructure )
{
    if( pStructure != nullptr )
    {
        return sizeof( VkGraphicsPipelineCreateInfo ) +
               GetPNextChainSize( pStructure ) +
               GetStructureSize( pStructure->pVertexInputState ) +
               GetStructureSize( pStructure->pInputAssemblyState ) +
               GetStructureSize( pStructure->pTessellationState ) +
               GetStructureSize( pStructure->pViewportState ) +
               GetStructureSize( pStructure->pRasterizationState ) +
               GetStructureSize( pStructure->pMultisampleState ) +
               GetStructureSize( pStructure->pDepthStencilState ) +
               GetStructureSize( pStructure->pColorBlendState ) +
               GetStructureSize( pStructure->pDynamicState );
    }
    return 0;
}

template<>
size_t GetStructureSize( const VkRayTracingPipelineCreateInfoKHR* pStructure )
{
    if( pStructure != nullptr )
    {
        return sizeof( VkRayTracingPipelineCreateInfoKHR ) +
               GetPNextChainSize( pStructure ) +
               GetStructureArraySize( pStructure->pGroups, pStructure->groupCount ) +
               GetStructureSize( pStructure->pLibraryInterface ) +
               GetStructureSize( pStructure->pDynamicState );
    }
    return 0;
}

template<>
size_t GetStructureSize( const VkAccelerationStructureGeometryKHR* pStructure )
{
    if( pStructure != nullptr )
    {
        return sizeof( VkAccelerationStructureGeometryKHR ) +
               GetPNextChainSize( pStructure );
    }
    return 0;
}

template<>
size_t GetStructureSize( const VkAccelerationStructureBuildGeometryInfoKHR* pStructure )
{
    if( pStructure != nullptr )
    {
        return sizeof( VkAccelerationStructureBuildGeometryInfoKHR ) +
               GetPNextChainSize( pStructure ) +
               GetStructureArraySize( pStructure->pGeometries, pStructure->geometryCount ) +
               GetStructureArraySize( pStructure->ppGeometries, pStructure->geometryCount );
    }
    return 0;
}

template<>
size_t GetStructureSize( const VkAccelerationStructureBuildRangeInfoKHR* pStructure )
{
    if( pStructure != nullptr )
    {
        return sizeof( VkAccelerationStructureBuildRangeInfoKHR );
    }
    return 0;
}

template<>
size_t GetStructureSize( const VkMicromapBuildInfoEXT* pStructure )
{
    if( pStructure != nullptr )
    {
        return sizeof( VkMicromapBuildInfoEXT ) +
               GetPNextChainSize( pStructure ) +
               GetStructureArraySize( pStructure->pUsageCounts, pStructure->usageCountsCount ) +
               GetStructureArraySize( pStructure->ppUsageCounts, pStructure->usageCountsCount );
    }
    return 0;
}

template<typename T>
void CopyStructureToPNextChain( VkBaseOutStructure** ppChain, const VkBaseInStructure* pStructure, std::byte** ppNext )
{
    ( *ppChain )->pNext = reinterpret_cast<VkBaseOutStructure*>(
        CopyStructure( reinterpret_cast<const T*>( pStructure ), ppNext ) );

    ( *ppChain ) = ( *ppChain )->pNext;
}

template<>
void* CopyPNextChain( const VkAccelerationStructureBuildGeometryInfoKHR* pStructure, std::byte** ppNext )
{
    VkBaseOutStructure copied = {};
    VkBaseOutStructure* pLast = &copied;
    if( pStructure != nullptr )
    {
        for( const auto& structure : Profiler::PNextIterator( pStructure->pNext ) )
        {
            switch( structure.sType )
            {
            case VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_MICROMAP_DATA_KHR:
                CopyStructureToPNextChain<VkAccelerationStructureGeometryMicromapDataKHR>( &pLast, &structure, ppNext );
                break;
            }
        }
        pLast->pNext = nullptr;
    }
    return copied.pNext;
}

template<>
void CopyStructureTo( VkPipelineVertexInputStateCreateInfo* pDst, const VkPipelineVertexInputStateCreateInfo* pSrc, std::byte** ppNext )
{
    if( pSrc != nullptr )
    {
        memcpy( pDst, pSrc, sizeof( VkPipelineVertexInputStateCreateInfo ) );
        pDst->pNext = CopyPNextChain( pSrc->pNext, ppNext );
        pDst->pVertexBindingDescriptions = CopyStructureArray( pSrc->pVertexBindingDescriptions, pSrc->vertexBindingDescriptionCount, ppNext );
        pDst->pVertexAttributeDescriptions = CopyStructureArray( pSrc->pVertexAttributeDescriptions, pSrc->vertexAttributeDescriptionCount, ppNext );
    }
}

template<>
void CopyStructureTo( VkPipelineViewportStateCreateInfo* pDst, const VkPipelineViewportStateCreateInfo* pSrc, std::byte** ppNext )
{
    if( pSrc != nullptr )
    {
        memcpy( pDst, pSrc, sizeof( VkPipelineViewportStateCreateInfo ) );
        pDst->pNext = CopyPNextChain( pSrc->pNext, ppNext );
        pDst->pViewports = CopyStructureArray( pSrc->pViewports, pSrc->viewportCount, ppNext );
        pDst->pScissors = CopyStructureArray( pSrc->pScissors, pSrc->scissorCount, ppNext );
    }
}

template<>
void CopyStructureTo( VkPipelineMultisampleStateCreateInfo* pDst, const VkPipelineMultisampleStateCreateInfo* pSrc, std::byte** ppNext )
{
    if( pSrc != nullptr )
    {
        memcpy( pDst, pSrc, sizeof( VkPipelineMultisampleStateCreateInfo ) );
        pDst->pNext = CopyPNextChain( pSrc->pNext, ppNext );
        pDst->pSampleMask = CopyStructure( pSrc->pSampleMask, ppNext );
    }
}

template<>
void CopyStructureTo( VkPipelineColorBlendStateCreateInfo* pDst, const VkPipelineColorBlendStateCreateInfo* pSrc, std::byte** ppNext )
{
    if( pSrc != nullptr )
    {
        memcpy( pDst, pSrc, sizeof( VkPipelineColorBlendStateCreateInfo ) );
        pDst->pNext = CopyPNextChain( pSrc->pNext, ppNext );
        pDst->pAttachments = CopyStructureArray( pSrc->pAttachments, pSrc->attachmentCount, ppNext );
    }
}

template<>
void CopyStructureTo( VkPipelineDynamicStateCreateInfo* pDst, const VkPipelineDynamicStateCreateInfo* pSrc, std::byte** ppNext )
{
    if( pSrc != nullptr )
    {
        memcpy( pDst, pSrc, sizeof( VkPipelineDynamicStateCreateInfo ) );
        pDst->pNext = CopyPNextChain( pSrc->pNext, ppNext );
        pDst->pDynamicStates = CopyStructureArray( pSrc->pDynamicStates, pSrc->dynamicStateCount, ppNext );
    }
}

template<>
void CopyStructureTo( VkRayTracingPipelineInterfaceCreateInfoKHR* pDst, const VkRayTracingPipelineInterfaceCreateInfoKHR* pSrc, std::byte** ppNext )
{
    if( pSrc != nullptr )
    {
        memcpy( pDst, pSrc, sizeof( VkRayTracingPipelineInterfaceCreateInfoKHR ) );
        pDst->pNext = CopyPNextChain( pSrc->pNext, ppNext );
    }
}

template<>
void CopyStructureTo( VkGraphicsPipelineCreateInfo* pDst, const VkGraphicsPipelineCreateInfo* pSrc, std::byte** ppNext )
{
    if( pSrc != nullptr )
    {
        memcpy( pDst, pSrc, sizeof( VkGraphicsPipelineCreateInfo ) );
        pDst->pNext = CopyPNextChain( pSrc->pNext, ppNext );
        pDst->stageCount = 0;
        pDst->pStages = nullptr;
        pDst->pVertexInputState = CopyStructure( pSrc->pVertexInputState, ppNext );
        pDst->pInputAssemblyState = CopyStructure( pSrc->pInputAssemblyState, ppNext );
        pDst->pTessellationState = CopyStructure( pSrc->pTessellationState, ppNext );
        pDst->pViewportState = CopyStructure( pSrc->pViewportState, ppNext );
        pDst->pRasterizationState = CopyStructure( pSrc->pRasterizationState, ppNext );
        pDst->pMultisampleState = CopyStructure( pSrc->pMultisampleState, ppNext );
        pDst->pDepthStencilState = CopyStructure( pSrc->pDepthStencilState, ppNext );
        pDst->pColorBlendState = CopyStructure( pSrc->pColorBlendState, ppNext );
        pDst->pDynamicState = CopyStructure( pSrc->pDynamicState, ppNext );
    }
}

template<>
void CopyStructureTo( VkRayTracingPipelineCreateInfoKHR* pDst, const VkRayTracingPipelineCreateInfoKHR* pSrc, std::byte** ppNext )
{
    if( pSrc != nullptr )
    {
        memcpy( pDst, pSrc, sizeof( VkRayTracingPipelineCreateInfoKHR ) );
        pDst->pNext = CopyPNextChain( pSrc->pNext, ppNext );
        pDst->stageCount = 0;
        pDst->pStages = nullptr;
        pDst->pGroups = CopyStructureArray( pSrc->pGroups, pSrc->groupCount, ppNext );
        pDst->pLibraryInfo = nullptr;
        pDst->pLibraryInterface = CopyStructure( pSrc->pLibraryInterface, ppNext );
        pDst->pDynamicState = CopyStructure( pSrc->pDynamicState, ppNext );
    }
}

template<>
void CopyStructureTo( VkAccelerationStructureBuildGeometryInfoKHR* pDst, const VkAccelerationStructureBuildGeometryInfoKHR* pSrc, std::byte** ppNext )
{
    if( pSrc != nullptr )
    {
        memcpy( pDst, pSrc, sizeof( VkAccelerationStructureBuildGeometryInfoKHR ) );
        pDst->pNext = CopyPNextChain( pSrc->pNext, ppNext );
        pDst->pGeometries = CopyStructureArray( pSrc->pGeometries, pSrc->geometryCount, ppNext );
        pDst->ppGeometries = CopyStructureArray( pSrc->ppGeometries, pSrc->geometryCount, ppNext );
    }
}

template<>
void CopyStructureTo( VkMicromapBuildInfoEXT* pDst, const VkMicromapBuildInfoEXT* pSrc, std::byte** ppNext )
{
    if( pSrc != nullptr )
    {
        memcpy( pDst, pSrc, sizeof( VkMicromapBuildInfoEXT ) );
        pDst->pNext = CopyPNextChain( pSrc->pNext, ppNext );
        pDst->pUsageCounts = CopyStructureArray( pSrc->pUsageCounts, pSrc->usageCountsCount, ppNext );
        pDst->ppUsageCounts = CopyStructureArray( pSrc->ppUsageCounts, pSrc->usageCountsCount, ppNext );
    }
}

template<typename CreateInfoT>
std::shared_ptr<Profiler::DeviceProfilerPipeline::CreateInfo> CopyPipelineCreateInfoImpl( const CreateInfoT* pCreateInfo )
{
    void* pMemory = AllocateMemoryForStructures( pCreateInfo );
    if( pMemory != nullptr )
    {
        auto* pNext = reinterpret_cast<std::byte*>( pMemory );
        CopyStructure( pCreateInfo, &pNext );
    }
    return std::shared_ptr<Profiler::DeviceProfilerPipeline::CreateInfo>(
        reinterpret_cast<Profiler::DeviceProfilerPipeline::CreateInfo*>( pMemory ), free );
}

static VkAccelerationStructureBuildGeometryInfoKHR* CopyAccelerationStructureBuildGeometryInfos(
    uint32_t infoCount,
    const VkAccelerationStructureBuildGeometryInfoKHR* pInfos )
{
    void* pMemory = AllocateMemoryForStructures( pInfos, infoCount );
    if( pMemory != nullptr )
    {
        auto* pNext = reinterpret_cast<std::byte*>( pMemory );
        CopyStructureArray( pInfos, infoCount, &pNext );
    }
    return reinterpret_cast<VkAccelerationStructureBuildGeometryInfoKHR*>( pMemory );
}

static VkAccelerationStructureBuildRangeInfoKHR** CopyAccelerationStructureBuildRangeInfos(
    uint32_t infoCount,
    const VkAccelerationStructureBuildGeometryInfoKHR* pInfos,
    const VkAccelerationStructureBuildRangeInfoKHR* const* ppRanges )
{
    size_t size = 0;
    for( uint32_t i = 0; i < infoCount; ++i )
    {
        size += sizeof( VkAccelerationStructureBuildRangeInfoKHR* ) +
                sizeof( VkAccelerationStructureBuildRangeInfoKHR ) * pInfos[i].geometryCount;
    }

    void* pMemory = malloc( size );
    if( pMemory != nullptr )
    {
        auto** ppDstRanges = reinterpret_cast<VkAccelerationStructureBuildRangeInfoKHR**>( pMemory );
        auto* pDstRangeInfos = reinterpret_cast<VkAccelerationStructureBuildRangeInfoKHR*>( ppDstRanges + infoCount );
        for( uint32_t i = 0; i < infoCount; ++i )
        {
            memcpy( pDstRangeInfos, ppRanges[i], pInfos[i].geometryCount * sizeof( VkAccelerationStructureBuildRangeInfoKHR ) );
            ppDstRanges[i] = pDstRangeInfos;
            pDstRangeInfos += pInfos[i].geometryCount;
        }
    }

    return reinterpret_cast<VkAccelerationStructureBuildRangeInfoKHR**>( pMemory );
}

static uint32_t** CopyAccelerationStructureMaxPrimitiveCounts(
    uint32_t infoCount,
    const VkAccelerationStructureBuildGeometryInfoKHR* pInfos,
    const uint32_t* const* ppMaxPrimitiveCounts )
{
    size_t size = 0;
    for( uint32_t i = 0; i < infoCount; ++i )
    {
        size += sizeof( uint32_t* ) +
                sizeof( uint32_t ) * pInfos[i].geometryCount;
    }

    void* pMemory = malloc( size );
    if( pMemory != nullptr )
    {
        auto** ppDstMaxPrimitiveCounts = reinterpret_cast<uint32_t**>( pMemory );
        auto* pDstMaxPrimitiveCounts = reinterpret_cast<uint32_t*>( ppDstMaxPrimitiveCounts + infoCount );
        for( uint32_t i = 0; i < infoCount; ++i )
        {
            memcpy( pDstMaxPrimitiveCounts, ppMaxPrimitiveCounts[i], pInfos[i].geometryCount * sizeof( uint32_t ) );
            ppDstMaxPrimitiveCounts[i] = pDstMaxPrimitiveCounts;
            pDstMaxPrimitiveCounts += pInfos[i].geometryCount;
        }
    }

    return reinterpret_cast<uint32_t**>( pMemory );
}

static VkMicromapBuildInfoEXT* CopyMicromapBuildInfos(
    uint32_t infoCount,
    const VkMicromapBuildInfoEXT* pInfos )
{
    void* pMemory = AllocateMemoryForStructures( pInfos, infoCount );
    if( pMemory != nullptr )
    {
        auto* pNext = reinterpret_cast<std::byte*>( pMemory );
        CopyStructureArray( pInfos, infoCount, &pNext );
    }
    return reinterpret_cast<VkMicromapBuildInfoEXT*>( pMemory );
}

namespace Profiler
{
    std::shared_ptr<DeviceProfilerPipeline::CreateInfo> DeviceProfilerPipeline::CopyPipelineCreateInfo( const VkGraphicsPipelineCreateInfo* pCreateInfo )
    {
        return CopyPipelineCreateInfoImpl( pCreateInfo );
    }

    std::shared_ptr<DeviceProfilerPipeline::CreateInfo> DeviceProfilerPipeline::CopyPipelineCreateInfo( const VkRayTracingPipelineCreateInfoKHR* pCreateInfo )
    {
        return CopyPipelineCreateInfoImpl( pCreateInfo );
    }

    void DeviceProfilerDrawcallDrawMultiPayload::CopyDynamicAllocations( const DeviceProfilerDrawcallDrawMultiPayload& other )
    {
        m_pVertexInfo = CopyElements( other.m_DrawCount, other.m_pVertexInfo );

        m_OwnsDynamicAllocations = true;
    }

    void DeviceProfilerDrawcallDrawMultiPayload::FreeDynamicAllocations()
    {
        if( m_OwnsDynamicAllocations )
        {
            this->FreeConst( m_pVertexInfo );
        }
    }

    void DeviceProfilerDrawcallDrawMultiIndexedPayload::CopyDynamicAllocations( const DeviceProfilerDrawcallDrawMultiIndexedPayload& other )
    {
        VkMultiDrawIndexedInfoEXT* pIndexInfo = CopyElements( other.m_DrawCount, other.m_pIndexInfo );

        if( other.m_pVertexOffset != nullptr )
        {
            for( uint32_t i = 0; i < other.m_DrawCount; ++i )
            {
                pIndexInfo[i].vertexOffset = other.m_pVertexOffset[i];
            }
        }

        m_pIndexInfo = pIndexInfo;
        m_pVertexOffset = nullptr;

        m_OwnsDynamicAllocations = true;
    }

    void DeviceProfilerDrawcallDrawMultiIndexedPayload::FreeDynamicAllocations()
    {
        if( m_OwnsDynamicAllocations )
        {
            this->FreeConst( m_pIndexInfo );
        }
    }

    template<DeviceProfilerDrawcallType Type>
    void DeviceProfilerDrawcallBuildAccelerationStructuresBasePayload<Type>::CopyDynamicAllocations( const DeviceProfilerDrawcallBuildAccelerationStructuresBasePayload& other )
    {
        m_pInfos = CopyAccelerationStructureBuildGeometryInfos(
            other.m_InfoCount,
            other.m_pInfos );

        m_OwnsDynamicAllocations = true;
    }

    template<DeviceProfilerDrawcallType Type>
    void DeviceProfilerDrawcallBuildAccelerationStructuresBasePayload<Type>::FreeDynamicAllocations()
    {
        if( m_OwnsDynamicAllocations )
        {
            this->FreeConst( m_pInfos );
        }
    }

    void DeviceProfilerDrawcallBuildAccelerationStructuresPayload::CopyDynamicAllocations( const DeviceProfilerDrawcallBuildAccelerationStructuresPayload& other )
    {
        DeviceProfilerDrawcallBuildAccelerationStructuresBasePayload::CopyDynamicAllocations( other );

        m_ppRanges = CopyAccelerationStructureBuildRangeInfos(
            other.m_InfoCount,
            other.m_pInfos,
            other.m_ppRanges );
    }

    void DeviceProfilerDrawcallBuildAccelerationStructuresPayload::FreeDynamicAllocations()
    {
        DeviceProfilerDrawcallBuildAccelerationStructuresBasePayload::FreeDynamicAllocations();

        if( m_OwnsDynamicAllocations )
        {
            this->FreeConst( m_ppRanges );
        }
    }

    void DeviceProfilerDrawcallBuildAccelerationStructuresIndirectPayload::CopyDynamicAllocations( const DeviceProfilerDrawcallBuildAccelerationStructuresIndirectPayload& other )
    {
        DeviceProfilerDrawcallBuildAccelerationStructuresBasePayload::CopyDynamicAllocations( other );

        m_ppMaxPrimitiveCounts = CopyAccelerationStructureMaxPrimitiveCounts(
            other.m_InfoCount,
            other.m_pInfos,
            other.m_ppMaxPrimitiveCounts );
    }

    void DeviceProfilerDrawcallBuildAccelerationStructuresIndirectPayload::FreeDynamicAllocations()
    {
        DeviceProfilerDrawcallBuildAccelerationStructuresBasePayload::FreeDynamicAllocations();

        if( m_OwnsDynamicAllocations )
        {
            this->FreeConst( m_ppMaxPrimitiveCounts );
        }
    }

    void DeviceProfilerDrawcallBuildMicromapsPayload::CopyDynamicAllocations( const DeviceProfilerDrawcallBuildMicromapsPayload& other )
    {
        m_pInfos = CopyMicromapBuildInfos(
            other.m_InfoCount,
            other.m_pInfos );
    }

    void DeviceProfilerDrawcallBuildMicromapsPayload::FreeDynamicAllocations()
    {
        if( m_OwnsDynamicAllocations )
        {
            this->FreeConst( m_pInfos );
        }
    }
}

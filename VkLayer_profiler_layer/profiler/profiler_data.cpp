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
#include "profiler_helpers/profiler_copy_builder.h"

template<typename T>
constexpr size_t GetPNextChainSize( const T* )
{
    // By default no pNext chains are captured.
    return 0;
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
    return GetStructureSize( pStructures ) * count;
}

template<typename T>
size_t GetStructureArraySize( const T* const* ppStructures, size_t count )
{
    size_t size = 0;
    for( size_t i = 0; i < count; ++i )
    {
        size += GetStructureSize( ppStructures[i] );
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
void* AllocateMemoryForStructures( const T* pStructure, size_t count = 1 )
{
    size_t size = GetStructureArraySize( pStructure, count );
    if( size == 0 )
    {
        return nullptr;
    }
    return malloc( size );
}

template<typename T>
T* CopyStructure( const T* pSrc, std::byte** ppNext )
{
    if( pSrc != nullptr )
    {
        T* pDst = reinterpret_cast<T*>( *ppNext );
        memcpy( pDst, pSrc, sizeof( T ) );
        *ppNext += sizeof( T );
        return pDst;
    }
    return nullptr;
}

template<typename T>
void* CopyPNextChain( const T* pStructure, std::byte** ppNext )
{
    // By default no pNext chains are captured.
    return nullptr;
}

template<>
void* CopyPNextChain( const VkAccelerationStructureBuildGeometryInfoKHR* pStructure, std::byte** ppNext )
{
    if( pStructure != nullptr )
    {
        for( const auto& structure : Profiler::PNextIterator( pStructure->pNext ) )
        {
            switch( structure.sType )
            {
            case VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_MICROMAP_DATA_KHR:
                return CopyStructure( reinterpret_cast<const VkAccelerationStructureGeometryMicromapDataKHR*>( &structure ), ppNext );
            }
        }
    }
    return nullptr;
}

template<typename T>
T* CopyStructureArray( const T* pSrc, uint32_t count, std::byte** ppNext )
{
    if( pSrc != nullptr && count > 0 )
    {
        T* pDst = reinterpret_cast<T*>( *ppNext );
        memcpy( pDst, pSrc, sizeof( T ) * count );
        *ppNext += sizeof( T ) * count;
        return pDst;
    }
    return nullptr;
}

template<typename T>
T* CopyStructureArray( const T* const* ppSrc, uint32_t count, std::byte** ppNext )
{
    if( ppSrc != nullptr && count > 0 )
    {
        T* pDst = reinterpret_cast<T*>( *ppNext );
        for( uint32_t i = 0; i < count; ++i )
        {
            pDst[i] 
        }
        *ppNext += sizeof( T ) * count;
        return pDst;
    }
    return nullptr;
}

template<>
VkPipelineVertexInputStateCreateInfo* CopyStructure( const VkPipelineVertexInputStateCreateInfo* pSrc, std::byte** ppNext )
{
    if( pSrc != nullptr )
    {
        VkPipelineVertexInputStateCreateInfo* pDst = reinterpret_cast<VkPipelineVertexInputStateCreateInfo*>( *ppNext );
        *ppNext += sizeof( VkPipelineVertexInputStateCreateInfo );
        pDst->sType = pSrc->sType;
        pDst->pNext = CopyPNextChain( pSrc->pNext, ppNext );
        pDst->flags = pSrc->flags;
        pDst->vertexBindingDescriptionCount = pSrc->vertexBindingDescriptionCount;
        pDst->pVertexBindingDescriptions = CopyStructureArray( pSrc->pVertexBindingDescriptions, pSrc->vertexBindingDescriptionCount, ppNext );
        pDst->vertexAttributeDescriptionCount = pSrc->vertexAttributeDescriptionCount;
        pDst->pVertexAttributeDescriptions = CopyStructureArray( pSrc->pVertexAttributeDescriptions, pSrc->vertexAttributeDescriptionCount, ppNext );
        return pDst;
    }
    return nullptr;
}

template<>
VkPipelineViewportStateCreateInfo* CopyStructure( const VkPipelineViewportStateCreateInfo* pSrc, std::byte** ppNext )
{
    if( pSrc != nullptr )
    {
        VkPipelineViewportStateCreateInfo* pDst = reinterpret_cast<VkPipelineViewportStateCreateInfo*>( *ppNext );
        *ppNext += sizeof( VkPipelineViewportStateCreateInfo );
        pDst->sType = pSrc->sType;
        pDst->pNext = CopyPNextChain( pSrc->pNext, ppNext );
        pDst->flags = pSrc->flags;
        pDst->viewportCount = pSrc->viewportCount;
        pDst->pViewports = CopyStructureArray( pSrc->pViewports, pSrc->viewportCount, ppNext );
        pDst->scissorCount = pSrc->scissorCount;
        pDst->pScissors = CopyStructureArray( pSrc->pScissors, pSrc->scissorCount, ppNext );
        return pDst;
    }
    return nullptr;
}

template<>
VkPipelineMultisampleStateCreateInfo* CopyStructure( const VkPipelineMultisampleStateCreateInfo* pSrc, std::byte** ppNext )
{
    if( pSrc != nullptr )
    {
        VkPipelineMultisampleStateCreateInfo* pDst = reinterpret_cast<VkPipelineMultisampleStateCreateInfo*>( *ppNext );
        *ppNext += sizeof( VkPipelineMultisampleStateCreateInfo );
        pDst->sType = pSrc->sType;
        pDst->pNext = CopyPNextChain( pSrc->pNext, ppNext );
        pDst->flags = pSrc->flags;
        pDst->rasterizationSamples = pSrc->rasterizationSamples;
        pDst->sampleShadingEnable = pSrc->sampleShadingEnable;
        pDst->minSampleShading = pSrc->minSampleShading;
        pDst->pSampleMask = CopyStructure( pSrc->pSampleMask, ppNext );
        pDst->alphaToCoverageEnable = pSrc->alphaToCoverageEnable;
        pDst->alphaToOneEnable = pSrc->alphaToOneEnable;
        return pDst;
    }
    return nullptr;
}

template<>
VkPipelineColorBlendStateCreateInfo* CopyStructure( const VkPipelineColorBlendStateCreateInfo* pSrc, std::byte** ppNext )
{
    if( pSrc != nullptr )
    {
        VkPipelineColorBlendStateCreateInfo* pDst = reinterpret_cast<VkPipelineColorBlendStateCreateInfo*>( *ppNext );
        *ppNext += sizeof( VkPipelineColorBlendStateCreateInfo );
        pDst->sType = pSrc->sType;
        pDst->pNext = CopyPNextChain( pSrc->pNext, ppNext );
        pDst->flags = pSrc->flags;
        pDst->logicOpEnable = pSrc->logicOpEnable;
        pDst->logicOp = pSrc->logicOp;
        pDst->attachmentCount = pSrc->attachmentCount;
        pDst->pAttachments = CopyStructureArray( pSrc->pAttachments, pSrc->attachmentCount, ppNext );
        memcpy( pDst->blendConstants, pSrc->blendConstants, sizeof( pSrc->blendConstants ) );
        return pDst;
    }
    return nullptr;
}

template<>
VkPipelineDynamicStateCreateInfo* CopyStructure( const VkPipelineDynamicStateCreateInfo* pSrc, std::byte** ppNext )
{
    if( pSrc != nullptr )
    {
        VkPipelineDynamicStateCreateInfo* pDst = reinterpret_cast<VkPipelineDynamicStateCreateInfo*>( *ppNext );
        *ppNext += sizeof( VkPipelineDynamicStateCreateInfo );
        pDst->sType = pSrc->sType;
        pDst->pNext = CopyPNextChain( pSrc->pNext, ppNext );
        pDst->flags = pSrc->flags;
        pDst->dynamicStateCount = pSrc->dynamicStateCount;
        pDst->pDynamicStates = CopyStructureArray( pSrc->pDynamicStates, pSrc->dynamicStateCount, ppNext );
        return pDst;
    }
    return nullptr;
}

template<>
VkRayTracingPipelineInterfaceCreateInfoKHR* CopyStructure( const VkRayTracingPipelineInterfaceCreateInfoKHR* pSrc, std::byte** ppNext )
{
    if( pSrc != nullptr )
    {
        VkRayTracingPipelineInterfaceCreateInfoKHR* pDst = reinterpret_cast<VkRayTracingPipelineInterfaceCreateInfoKHR*>( *ppNext );
        *ppNext += sizeof( VkRayTracingPipelineInterfaceCreateInfoKHR );
        pDst->sType = pSrc->sType;
        pDst->pNext = CopyPNextChain( pSrc->pNext, ppNext );
        pDst->maxPipelineRayPayloadSize = pSrc->maxPipelineRayPayloadSize;
        pDst->maxPipelineRayHitAttributeSize = pSrc->maxPipelineRayHitAttributeSize;
        return pDst;
    }
    return nullptr;
}

template<>
VkGraphicsPipelineCreateInfo* CopyStructure( const VkGraphicsPipelineCreateInfo* pSrc, std::byte** ppNext )
{
    if( pSrc != nullptr )
    {
        VkGraphicsPipelineCreateInfo* pDst = reinterpret_cast<VkGraphicsPipelineCreateInfo*>( *ppNext );
        *ppNext += sizeof( VkGraphicsPipelineCreateInfo );
        pDst->sType = pSrc->sType;
        pDst->pNext = CopyPNextChain( pSrc->pNext, ppNext );
        pDst->flags = pSrc->flags;
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
        pDst->layout = pSrc->layout;
        pDst->renderPass = pSrc->renderPass;
        pDst->subpass = pSrc->subpass;
        pDst->basePipelineHandle = pSrc->basePipelineHandle;
        pDst->basePipelineIndex = pSrc->basePipelineIndex;
        return pDst;
    }
    return nullptr;
}

template<>
VkRayTracingPipelineCreateInfoKHR* CopyStructure( const VkRayTracingPipelineCreateInfoKHR* pSrc, std::byte** ppNext )
{
    if( pSrc != nullptr )
    {
        VkRayTracingPipelineCreateInfoKHR* pDst = reinterpret_cast<VkRayTracingPipelineCreateInfoKHR*>( *ppNext );
        *ppNext += sizeof( VkRayTracingPipelineCreateInfoKHR );
        pDst->sType = pSrc->sType;
        pDst->pNext = CopyPNextChain( pSrc->pNext, ppNext );
        pDst->flags = pSrc->flags;
        pDst->stageCount = 0;
        pDst->pStages = nullptr;
        pDst->groupCount = pSrc->groupCount;
        pDst->pGroups = CopyStructureArray( pSrc->pGroups, pSrc->groupCount, ppNext );
        pDst->maxPipelineRayRecursionDepth = pSrc->maxPipelineRayRecursionDepth;
        pDst->pLibraryInfo = nullptr;
        pDst->pLibraryInterface = CopyStructure( pSrc->pLibraryInterface, ppNext );
        pDst->pDynamicState = CopyStructure( pSrc->pDynamicState, ppNext );
        pDst->layout = pSrc->layout;
        pDst->basePipelineHandle = pSrc->basePipelineHandle;
        pDst->basePipelineIndex = pSrc->basePipelineIndex;
        return pDst;
    }
    return nullptr;
}

template<typename CreateInfoT>
std::shared_ptr<Profiler::DeviceProfilerPipeline::CreateInfo> CopyPipelineCreateInfoImpl( const CreateInfoT* pCreateInfo )
{
    size_t createInfoSize = GetStructureSize( pCreateInfo );
    if( createInfoSize == 0 )
    {
        return nullptr;
    }

    auto* ci = static_cast<Profiler::DeviceProfilerPipeline::CreateInfo*>( malloc( createInfoSize ) );
    if( ci == nullptr )
    {
        return nullptr;
    }

    // Get pointer to the first byte after the new create info for additional data.
    auto* pNext = reinterpret_cast<std::byte*>( ci );
    CopyStructure( pCreateInfo, &pNext );

    return std::shared_ptr<Profiler::DeviceProfilerPipeline::CreateInfo>( ci, free );
}

static VkAccelerationStructureBuildGeometryInfoKHR* CopyAccelerationStructureBuildGeometryInfos(
    uint32_t infoCount,
    const VkAccelerationStructureBuildGeometryInfoKHR* pInfos )
{
    Profiler::DeviceProfilerCopyBuilder builder;

    auto duplicatedInfos = builder.Write( pInfos, infoCount );
    for( uint32_t i = 0; i < infoCount; ++i )
    {
        auto geometries = builder.WriteUninitialized( &duplicatedInfos[i]->pGeometries, pInfos[i].geometryCount );

        if( pInfos[i].pGeometries != nullptr )
        {
            std::memcpy( geometries.GetPointer(),
                pInfos[i].pGeometries,
                pInfos[i].geometryCount * sizeof( VkAccelerationStructureGeometryKHR ) );
        }
        else if( pInfos[i].ppGeometries != nullptr )
        {
            for( uint32_t j = 0; j < pInfos[i].geometryCount; ++j )
            {
                std::memcpy( geometries[j].GetPointer(),
                    pInfos[i].ppGeometries[j],
                    sizeof( VkAccelerationStructureGeometryKHR ) );
            }
        }
    }

    return builder.GetMemoryCopy<VkAccelerationStructureBuildGeometryInfoKHR>();
}

static VkAccelerationStructureBuildRangeInfoKHR** CopyAccelerationStructureBuildRangeInfos(
    uint32_t infoCount,
    const VkAccelerationStructureBuildGeometryInfoKHR* pInfos,
    const VkAccelerationStructureBuildRangeInfoKHR* const* ppRanges )
{
    void* pMemory = AllocateMemoryForStructures( ppRanges, infoCount );
    if( pMemory )
    {
        auto* pNext = reinterpret_cast<std::byte*>( pMemory );
        auto* ppDuplicatedRanges = 
    }
    return reinterpret_cast<VkAccelerationStructureBuildRangeInfoKHR**>( pMemory );
}

static uint32_t** CopyAccelerationStructureMaxPrimitiveCounts(
    uint32_t infoCount,
    const VkAccelerationStructureBuildGeometryInfoKHR* pInfos,
    const uint32_t* const* ppMaxPrimitiveCounts )
{
    Profiler::DeviceProfilerCopyBuilder builder;

    auto duplicatedPrimitiveCounts = builder.Write( ppMaxPrimitiveCounts, infoCount );
    for( uint32_t i = 0; i < infoCount; ++i )
    {
        builder.Write( duplicatedPrimitiveCounts[i].GetPointer(), ppMaxPrimitiveCounts[i], pInfos[i].geometryCount );
    }

    return builder.GetMemoryCopy<uint32_t*>();
}

static VkMicromapBuildInfoEXT* CopyMicromapBuildInfos(
    uint32_t infoCount,
    const VkMicromapBuildInfoEXT* pInfos )
{
    void* pMemory = AllocateMemoryForStructures( pInfos, infoCount );
    if( pMemory )
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

// Copyright (c) 2024-2025 Lukasz Stalmirski
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

template<typename T, typename = void>
struct HasPNext : std::false_type {};

template<typename T>
struct HasPNext<T, std::void_t<decltype(&T::pNext)>> : std::true_type {};

size_t GetPNextChainSize( const void* pNext )
{
    // No pNext structures are captured right now.
    return 0;
}

template<typename T>
size_t GetStructureSize( const T* pStructure )
{
    size_t size = (pStructure != nullptr) ? sizeof( T ) : 0;

    // Include size of the pNext chain in the returned size.
    if constexpr( HasPNext<T>::value )
    {
        if( pStructure ) size += GetPNextChainSize( pStructure->pNext );
    }

    return size;
}

template<>
size_t GetStructureSize( const VkPipelineVertexInputStateCreateInfo* pStructure )
{
    if( pStructure != nullptr )
    {
        return sizeof( VkPipelineVertexInputStateCreateInfo ) +
            GetPNextChainSize( pStructure->pNext ) +
            GetStructureSize( pStructure->pVertexBindingDescriptions ) * pStructure->vertexBindingDescriptionCount +
            GetStructureSize( pStructure->pVertexAttributeDescriptions ) * pStructure->vertexAttributeDescriptionCount;
    }
    return 0;
}

template<>
size_t GetStructureSize( const VkPipelineViewportStateCreateInfo* pStructure )
{
    if( pStructure != nullptr )
    {
        return sizeof( VkPipelineViewportStateCreateInfo ) +
            GetPNextChainSize( pStructure->pNext ) +
            GetStructureSize( pStructure->pViewports ) * pStructure->viewportCount +
            GetStructureSize( pStructure->pScissors ) * pStructure->scissorCount;
    }
    return 0;
}

template<>
size_t GetStructureSize( const VkPipelineMultisampleStateCreateInfo* pStructure )
{
    if( pStructure != nullptr )
    {
        return sizeof( VkPipelineMultisampleStateCreateInfo ) +
            GetPNextChainSize( pStructure->pNext ) +
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
            GetPNextChainSize( pStructure->pNext ) +
            GetStructureSize( pStructure->pAttachments ) * pStructure->attachmentCount;
    }
    return 0;
}

template<>
size_t GetStructureSize( const VkPipelineDynamicStateCreateInfo* pStructure )
{
    if( pStructure != nullptr )
    {
        return sizeof( VkPipelineDynamicStateCreateInfo ) +
            GetPNextChainSize( pStructure->pNext ) +
            GetStructureSize( pStructure->pDynamicStates ) * pStructure->dynamicStateCount;
    }
    return 0;
}

template<>
size_t GetStructureSize( const VkRayTracingPipelineInterfaceCreateInfoKHR* pStructure )
{
    if( pStructure != nullptr )
    {
        return sizeof( VkRayTracingPipelineInterfaceCreateInfoKHR ) +
               GetPNextChainSize( pStructure->pNext );
    }
    return 0;
}

template<>
size_t GetStructureSize( const VkGraphicsPipelineCreateInfo* pStructure )
{
    if( pStructure != nullptr )
    {
        return sizeof( VkGraphicsPipelineCreateInfo ) +
            GetPNextChainSize( pStructure->pNext ) +
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
               GetPNextChainSize( pStructure->pNext ) +
               GetStructureSize( pStructure->pLibraryInterface ) +
               GetStructureSize( pStructure->pDynamicState );
    }
    return 0;
}

template<typename T>
T* CopyStructure( const T* pSrc, std::byte** ppNext )
{
    if( pSrc != nullptr )
    {
        T* pDst = reinterpret_cast<T*>(*ppNext);
        memcpy( pDst, pSrc, sizeof( T ) );
        *ppNext += sizeof( T );
        return pDst;
    }
    return nullptr;
}

void* CopyPNextChain( const void* pNext, std::byte** ppNext )
{
    // No pNext structures are captured right now.
    return nullptr;
}

template<typename T>
T* CopyStructureArray( const T* pSrc, uint32_t count, std::byte** ppNext )
{
    if( pSrc != nullptr && count > 0 )
    {
        T* pDst = reinterpret_cast<T*>(*ppNext);
        memcpy( pDst, pSrc, sizeof( T ) * count );
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
        VkPipelineVertexInputStateCreateInfo* pDst = reinterpret_cast<VkPipelineVertexInputStateCreateInfo*>(*ppNext);
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
        VkPipelineViewportStateCreateInfo* pDst = reinterpret_cast<VkPipelineViewportStateCreateInfo*>(*ppNext);
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
        VkPipelineMultisampleStateCreateInfo* pDst = reinterpret_cast<VkPipelineMultisampleStateCreateInfo*>(*ppNext);
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
        VkPipelineColorBlendStateCreateInfo* pDst = reinterpret_cast<VkPipelineColorBlendStateCreateInfo*>(*ppNext);
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
        VkPipelineDynamicStateCreateInfo* pDst = reinterpret_cast<VkPipelineDynamicStateCreateInfo*>(*ppNext);
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
        VkGraphicsPipelineCreateInfo* pDst = reinterpret_cast<VkGraphicsPipelineCreateInfo*>(*ppNext);
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
        pDst->groupCount = 0;
        pDst->pGroups = nullptr;
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

    VkAccelerationStructureBuildGeometryInfoKHR* DeviceProfilerDrawcall::CopyAccelerationStructureBuildGeometryInfos(
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

    VkAccelerationStructureBuildRangeInfoKHR** DeviceProfilerDrawcall::CopyAccelerationStructureBuildRangeInfos(
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

    uint32_t** DeviceProfilerDrawcall::CopyMaxPrimitiveCounts(
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

    VkMicromapBuildInfoEXT* DeviceProfilerDrawcall::CopyMicromapBuildInfos(
        uint32_t infoCount,
        const VkMicromapBuildInfoEXT* pInfos )
    {
        VkMicromapBuildInfoEXT* pDuplicatedInfos = CopyElements( infoCount, pInfos );

        // Create copy of usage counts of each build info
        for( uint32_t i = 0; i < infoCount; ++i )
        {
            VkMicromapBuildInfoEXT& buildInfo = pDuplicatedInfos[i];
            const uint32_t usageCount = buildInfo.usageCountsCount;

            if( buildInfo.pUsageCounts != nullptr )
            {
                buildInfo.pUsageCounts = CopyElements( usageCount, buildInfo.pUsageCounts );
            }

            else if( buildInfo.ppUsageCounts != nullptr )
            {
                VkMicromapUsageEXT* pUsageCounts =
                    reinterpret_cast<VkMicromapUsageEXT*>( malloc( usageCount * sizeof( VkMicromapUsageEXT ) ) );

                if( pUsageCounts != nullptr )
                {
                    for( uint32_t j = 0; j < usageCount; ++j )
                    {
                        pUsageCounts[j] = *buildInfo.ppUsageCounts[j];
                    }
                }

                buildInfo.pUsageCounts = pUsageCounts;
            }

            buildInfo.ppUsageCounts = nullptr;
        }

        return pDuplicatedInfos;
    }
}

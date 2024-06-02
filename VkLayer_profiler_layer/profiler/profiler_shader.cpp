// Copyright (c) 2024 Lukasz Stalmirski
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

#include "profiler_shader.h"

#include <assert.h>

namespace Profiler
{
    /***********************************************************************************\

    Structure:
        InternalData

    Description:
        Contains pipeline executable properties for a shader stage.

        Memory layout of this structure - optimized for size in memory due to possibly
        large number of pipelines.

        | --------------------------------------------------------- |
        | InternalData                                              |
        | Statistic[ m_StatisticsCount ]                            |
        | InternalRepresentations[ m_InternalRepresentationsCount ] |
        | Data...                                                   |
        | --------------------------------------------------------- |

    \***********************************************************************************/
    struct ProfilerShaderExecutable::InternalData
    {
        struct Statistic
        {
            uint32_t m_NameOffset;
            uint32_t m_DescriptionOffset;
            VkPipelineExecutableStatisticFormatKHR m_Format;
            VkPipelineExecutableStatisticValueKHR m_Value;
        };

        struct InternalRepresentation
        {
            uint32_t m_NameOffset;
            uint32_t m_DescriptionOffset;
            uint32_t m_DataOffset;
            uint32_t m_DataSize : 31;
            uint32_t m_IsText : 1;
        };

        uint32_t m_NameOffset;
        uint32_t m_DescriptionOffset;
        uint32_t m_StatisticsCount;
        uint32_t m_InternalRepresentationsCount;
        VkShaderStageFlags m_Stages;
        uint32_t m_SubgroupSize;

        /*******************************************************************************\
          Calculates space required to store all relevant pipeline executable data.
        \*******************************************************************************/
        static size_t CalculateSize(
            const VkPipelineExecutablePropertiesKHR* pProperties,
            uint32_t statisticsCount,
            const VkPipelineExecutableStatisticKHR* pStatistics,
            uint32_t internalRepresentationsCount,
            const VkPipelineExecutableInternalRepresentationKHR* pInternalRepresentations )
        {
            size_t requiredSize =
                sizeof( InternalData ) +
                sizeof( Statistic ) * statisticsCount +
                sizeof( InternalRepresentation ) * internalRepresentationsCount;

            // Reserve space for name and description of the executable.
            requiredSize += strlen( pProperties->name ) + 1;
            requiredSize += strlen( pProperties->description ) + 1;

            // Reserve space for names and descriptions of shader statistics.
            for( uint32_t i = 0; i < statisticsCount; ++i )
            {
                requiredSize += strlen( pStatistics[i].name ) + 1;
                requiredSize += strlen( pStatistics[i].description ) + 1;
            }

            // Reserve space for names, descriptions and data of internal representations.
            for( uint32_t i = 0; i < internalRepresentationsCount; ++i )
            {
                requiredSize += strlen( pInternalRepresentations[i].name ) + 1;
                requiredSize += strlen( pInternalRepresentations[i].description ) + 1;
                requiredSize += pInternalRepresentations[i].dataSize;
            }

            return requiredSize;
        }

        /*******************************************************************************\
          Initializes the shader executable internal data.
        \*******************************************************************************/
        void Initialize(
            size_t sizeOfThis,
            const VkPipelineExecutablePropertiesKHR* pProperties,
            uint32_t statisticsCount,
            const VkPipelineExecutableStatisticKHR* pStatistics,
            uint32_t internalRepresentationsCount,
            const VkPipelineExecutableInternalRepresentationKHR* pInternalRepresentations)
        {
            // These must be set before GetDataOffset is called.
            m_StatisticsCount = statisticsCount;
            m_InternalRepresentationsCount = internalRepresentationsCount;

            uint32_t offset = GetDataOffset();

            // Fill shader properties.
            m_NameOffset = AddString( pProperties->name, offset, sizeOfThis );
            m_DescriptionOffset = AddString( pProperties->description, offset, sizeOfThis );
            m_Stages = pProperties->stages;
            m_SubgroupSize = pProperties->subgroupSize;

            // Fill shader statistics.
            Statistic* pStatDescs = GetFirstStatistic();
            for( uint32_t i = 0; i < statisticsCount; ++i )
            {
                pStatDescs[i].m_NameOffset = AddString( pStatistics[i].name, offset, sizeOfThis );
                pStatDescs[i].m_DescriptionOffset = AddString( pStatistics[i].description, offset, sizeOfThis );
                pStatDescs[i].m_Format = pStatistics[i].format;
                pStatDescs[i].m_Value = pStatistics[i].value;
            }

            // Fill shader internal representations.
            InternalRepresentation* pReprDescs = GetFirstInternalRepresentation();
            for( uint32_t i = 0; i < internalRepresentationsCount; ++i )
            {
                pReprDescs[i].m_NameOffset = AddString( pInternalRepresentations[i].name, offset, sizeOfThis );
                pReprDescs[i].m_DescriptionOffset = AddString( pInternalRepresentations[i].description, offset, sizeOfThis );
                pReprDescs[i].m_DataOffset = AddData( pInternalRepresentations[i].pData, pInternalRepresentations[i].dataSize, offset, sizeOfThis );
                pReprDescs[i].m_DataSize = static_cast<uint32_t>( pInternalRepresentations[i].dataSize );
                pReprDescs[i].m_IsText = pInternalRepresentations[i].isText;
            }

            // Verify that the structure has been correctly allocated and there were no overruns.
            assert( offset == sizeOfThis );
        }

        /*******************************************************************************\
          Returns offset of the variable-sized data that is placed after this structure,
          the shader statistics array and the internal representations array.
        \*******************************************************************************/
        uint32_t GetDataOffset() const
        {
            return sizeof( *this ) +
                   sizeof( Statistic ) * m_StatisticsCount +
                   sizeof( InternalRepresentation ) * m_InternalRepresentationsCount;
        }

        /*******************************************************************************\
          Copies the null-terminated pString into [this] + dataOffset.
          Increments the dataOffset by the length+1 of the copied string.
        \*******************************************************************************/
        uint32_t AddString( const char* pString, uint32_t& offset, size_t sizeOfThis )
        {
            return AddData( pString, strlen( pString ) + 1, offset, sizeOfThis );
        }

        /*******************************************************************************\
          Copies the data into [this] + dataOffset.
          Increments the dataOffset by the size of the copied data.
        \*******************************************************************************/
        uint32_t AddData( const void* pData, size_t size, uint32_t& offset, size_t sizeOfThis )
        {
            (void)sizeOfThis;
            assert( offset + size <= sizeOfThis );
            memcpy( reinterpret_cast<char*>( this ) + offset, pData, size );
            return std::exchange( offset, offset + size );
        }

        /*******************************************************************************\
          Returns the null-terminated string that is stored at [this] + offset.
        \*******************************************************************************/
        const char* GetString( uint32_t offset ) const
        {
            return reinterpret_cast<const char*>( this ) + offset;
        }

        /*******************************************************************************\
          Returns the data that is stored at [this] + offset.
        \*******************************************************************************/
        const void* GetData( uint32_t offset ) const
        {
            return reinterpret_cast<const std::byte*>( this ) + offset;
        }

        /*******************************************************************************\
          Returns address of the shader statistics array.
        \*******************************************************************************/
        Statistic* GetFirstStatistic()
        {
            return reinterpret_cast<Statistic*>( this + 1 );
        }

        /*******************************************************************************\
          Returns address of the shader internal representations array.
        \*******************************************************************************/
        InternalRepresentation* GetFirstInternalRepresentation()
        {
            return reinterpret_cast<InternalRepresentation*>( GetFirstStatistic() + m_StatisticsCount );
        }
    };

    /***********************************************************************************\

    Function:
        Initialize

    Description:
        Initializes the shader executable properties info.

    \***********************************************************************************/
    VkResult ProfilerShaderExecutable::Initialize(
        const VkPipelineExecutablePropertiesKHR* pProperties,
        uint32_t statisticsCount,
        const VkPipelineExecutableStatisticKHR* pStatistics,
        uint32_t internalRepresentationsCount,
        const VkPipelineExecutableInternalRepresentationKHR* pInternalRepresentations )
    {
        size_t internalDataSize = InternalData::CalculateSize(
            pProperties,
            statisticsCount,
            pStatistics,
            internalRepresentationsCount,
            pInternalRepresentations );

        m_pInternalData.reset( static_cast<InternalData*>( malloc( internalDataSize ) ), free );
        if( !m_pInternalData )
        {
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }

        m_pInternalData->Initialize(
            internalDataSize,
            pProperties,
            statisticsCount,
            pStatistics,
            internalRepresentationsCount,
            pInternalRepresentations );

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        Initialized

    Description:
        Returns true if the shader executable info is available.

    \***********************************************************************************/
    bool ProfilerShaderExecutable::Initialized() const
    {
        return m_pInternalData != nullptr;
    }

    /***********************************************************************************\

    Function:
        GetName

    Description:
        Returns name of the shader.

    \***********************************************************************************/
    const char* ProfilerShaderExecutable::GetName() const
    {
        return m_pInternalData->GetString( m_pInternalData->m_NameOffset );
    }

    /***********************************************************************************\

    Function:
        GetDescription

    Description:
        Returns description of the shader.

    \***********************************************************************************/
    const char* ProfilerShaderExecutable::GetDescription() const
    {
        return m_pInternalData->GetString( m_pInternalData->m_DescriptionOffset );
    }

    /***********************************************************************************\

    Function:
        GetStages

    Description:
        Returns which shader stages (if any) were principally used as inputs to compile
        this pipeline executable.

    \***********************************************************************************/
    VkShaderStageFlags ProfilerShaderExecutable::GetStages() const
    {
        return m_pInternalData->m_Stages;
    }

    /***********************************************************************************\

    Function:
        GetSubgroupSize

    Description:
        Returns the subgroup size with which this pipeline executable is dispatched.

    \***********************************************************************************/
    uint32_t ProfilerShaderExecutable::GetSubgroupSize() const
    {
        return m_pInternalData->m_SubgroupSize;
    }

    /***********************************************************************************\

    Function:
        GetStatisticsCount

    Description:
        Returns number of shader statistics available.

    \***********************************************************************************/
    uint32_t ProfilerShaderExecutable::GetStatisticsCount() const
    {
        return m_pInternalData->m_StatisticsCount;
    }

    /***********************************************************************************\

    Function:
        GetStatistic

    Description:
        Returns a shader's statistic.

    \***********************************************************************************/
    ProfilerShaderStatistic ProfilerShaderExecutable::GetStatistic( uint32_t index ) const
    {
        ProfilerShaderStatistic stat;
        if( index < GetStatisticsCount() )
        {
            auto& statDesc = m_pInternalData->GetFirstStatistic()[index];
            stat.m_pName = m_pInternalData->GetString( statDesc.m_NameOffset );
            stat.m_pDescription = m_pInternalData->GetString( statDesc.m_DescriptionOffset );
            stat.m_Format = statDesc.m_Format;
            stat.m_Value = statDesc.m_Value;
        }
        return stat;
    }

    /***********************************************************************************\

    Function:
        GetInternalRepresentationsCount

    Description:
        Returns number of shader internal representations available.

    \***********************************************************************************/
    uint32_t ProfilerShaderExecutable::GetInternalRepresentationsCount() const
    {
        return m_pInternalData->m_InternalRepresentationsCount;
    }

    /***********************************************************************************\

    Function:
        GetInternalRepresentation

    Description:
        Returns a shader's internal representation.

    \***********************************************************************************/
    ProfilerShaderInternalRepresentation ProfilerShaderExecutable::GetInternalRepresentation( uint32_t index ) const
    {
        ProfilerShaderInternalRepresentation repr;
        if( index < GetInternalRepresentationsCount() )
        {
            auto& reprDesc = m_pInternalData->GetFirstInternalRepresentation()[index];
            repr.m_pName = m_pInternalData->GetString( reprDesc.m_NameOffset );
            repr.m_pDescription = m_pInternalData->GetString( reprDesc.m_DescriptionOffset );
            repr.m_pData = m_pInternalData->GetData( reprDesc.m_DataOffset );
            repr.m_DataSize = static_cast<size_t>( reprDesc.m_DataSize );
            repr.m_IsText = reprDesc.m_IsText;
        }
        return repr;
    }
}

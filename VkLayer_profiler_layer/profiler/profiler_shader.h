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
#include "profiler_helpers.h"
#include <algorithm>
#include <array>
#include <vector>
#include <set>
#include <stdint.h>
#include <vulkan/vulkan.h>
#include <spirv/unified1/spirv.h>

namespace Profiler
{
    struct ProfilerShaderModule
    {
        uint32_t m_Hash = 0;
        uint32_t m_IdentifierSize = 0;
        uint8_t m_Identifier[VK_MAX_SHADER_MODULE_IDENTIFIER_SIZE_EXT] = {};
        const char* m_pFileName = nullptr;
        std::vector<uint32_t> m_Bytecode = {};
        std::set<SpvCapability> m_Capabilities = {};

        ProfilerShaderModule() = default;
        ProfilerShaderModule( const uint32_t* pBytecode, size_t bytecodeSize, const uint8_t* pIdentifier, uint32_t identifierSize );
    };

    struct ProfilerShader
    {
        uint32_t m_Hash = 0;
        uint32_t m_Index = 0;
        VkShaderStageFlagBits m_Stage = {};
        std::string m_EntryPoint = {};
        std::shared_ptr<ProfilerShaderModule> m_pShaderModule = nullptr;
    };

    struct ProfilerShaderStatistic
    {
        const char* m_pName = nullptr;
        const char* m_pDescription = nullptr;
        VkPipelineExecutableStatisticFormatKHR m_Format = {};
        VkPipelineExecutableStatisticValueKHR m_Value = {};
    };

    struct ProfilerShaderInternalRepresentation
    {
        const char* m_pName = nullptr;
        const char* m_pDescription = nullptr;
        const void* m_pData = nullptr;
        size_t m_DataSize = 0;
        bool m_IsText = false;
    };

    struct ProfilerShaderExecutable
    {
        struct InternalData;
        std::shared_ptr<InternalData> m_pInternalData = nullptr;

        VkResult Initialize(
            const VkPipelineExecutablePropertiesKHR* pProperties,
            uint32_t statisticsCount,
            const VkPipelineExecutableStatisticKHR* pStatistics,
            uint32_t internalRepresentationsCount,
            VkPipelineExecutableInternalRepresentationKHR* pInternalRepresentations );

        bool Initialized() const;

        const char* GetName() const;
        const char* GetDescription() const;
        VkShaderStageFlags GetStages() const;
        uint32_t GetSubgroupSize() const;

        uint32_t GetStatisticsCount() const;
        ProfilerShaderStatistic GetStatistic( uint32_t index ) const;

        uint32_t GetInternalRepresentationsCount() const;
        ProfilerShaderInternalRepresentation GetInternalRepresentation( uint32_t index ) const;
    };

    struct ProfilerShaderTuple
    {
        uint32_t m_Hash = 0;

        std::vector<ProfilerShader> m_Shaders = {};
        std::vector<ProfilerShaderExecutable> m_ShaderExecutables = {};

        inline constexpr bool operator==( const ProfilerShaderTuple& rh ) const
        {
            return m_Hash == rh.m_Hash;
        }

        inline constexpr bool operator!=( const ProfilerShaderTuple& rh ) const
        {
            return !operator==( rh );
        }

        inline const ProfilerShader* GetFirstShaderAtStage( VkShaderStageFlagBits stage ) const
        {
            const size_t stageCount = m_Shaders.size();
            for( size_t i = 0; i < stageCount; ++i )
            {
                if( m_Shaders[ i ].m_Stage == stage )
                {
                    return &m_Shaders[ i ];
                }
            }
            return nullptr;
        }

        inline const ProfilerShader* GetShaderAtIndex( uint32_t index ) const
        {
            const size_t stageCount = m_Shaders.size();
            for( size_t i = 0; i < stageCount; ++i )
            {
                if( m_Shaders[ i ].m_Index == index )
                {
                    return &m_Shaders[ i ];
                }
            }
            return nullptr;
        }

        bool UsesRayQuery() const;
        bool UsesRayTracing() const;
        bool UsesMeshShading() const;

        void UpdateHash();

        std::string GetShaderStageHashesString( VkShaderStageFlags stages, bool skipEmptyStages = false ) const;
    };
}


namespace std
{
    template<>
    struct hash<Profiler::ProfilerShaderTuple>
    {
        inline size_t operator()( const Profiler::ProfilerShaderTuple& tuple ) const
        {
            return tuple.m_Hash;
        }
    };
}

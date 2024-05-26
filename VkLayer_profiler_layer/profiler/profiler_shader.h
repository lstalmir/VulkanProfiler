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

#pragma once
#include "profiler_helpers.h"
#include <algorithm>
#include <array>
#include <vector>
#include <stdint.h>
#include <vulkan/vulkan.h>
#include <spirv/unified1/spirv.h>

namespace Profiler
{
    struct ProfilerShaderModule
    {
        uint32_t m_Hash = 0;
        std::vector<uint32_t> m_Bytecode = {};
        std::vector<SpvCapability> m_Capabilities = {};
    };

    struct ProfilerShader
    {
        uint32_t m_Hash = 0;
        uint32_t m_Index = 0;
        VkShaderStageFlagBits m_Stage = {};
        std::string m_EntryPoint = {};
        std::shared_ptr<ProfilerShaderModule> m_pShaderModule = nullptr;
    };

    struct ProfilerShaderTuple
    {
        uint32_t m_Hash = 0;

        std::vector<ProfilerShader> m_Shaders = {};

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

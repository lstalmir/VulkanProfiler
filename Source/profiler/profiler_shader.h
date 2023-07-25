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
#include <array>
#include <stdint.h>
#include <vulkan/vulkan.h>
#include <spirv/unified1/spirv.h>

namespace Profiler
{
    struct ProfilerShaderTuple
    {
        uint32_t m_Hash = 0;

        BitsetArray<VkShaderStageFlagBits, uint32_t, 32> m_Stages = {};

        inline constexpr bool operator==( const ProfilerShaderTuple& rh ) const
        {
            return m_Hash == rh.m_Hash;
        }

        inline constexpr bool operator!=( const ProfilerShaderTuple& rh ) const
        {
            return !operator==( rh );
        }
    };

    struct ProfilerShaderModule
    {
        uint32_t                   m_Hash = 0;
        std::vector<SpvCapability> m_Capabilities = {};
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

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
    struct DeviceProfilerShaderModule
    {
        uint32_t                    m_Hash;
        std::vector<uint32_t>       m_Bytecode;
        std::vector<SpvCapability>  m_Capabilities = {};
    };

    struct DeviceProfilerPipelineShader
    {
        uint32_t                    m_Hash = 0;
        std::string                 m_EntryPoint = "";
        DeviceProfilerShaderModule* m_pShaderModule = nullptr;
    };

    struct DeviceProfilerPipelineShaderTuple
    {
        uint32_t m_Hash = 0;

        BitsetArray<VkShaderStageFlagBits, DeviceProfilerPipelineShader, 32> m_Shaders = {};

        inline constexpr bool operator==( const DeviceProfilerPipelineShaderTuple& rh ) const
        {
            return m_Hash == rh.m_Hash;
        }

        inline constexpr bool operator!=( const DeviceProfilerPipelineShaderTuple& rh ) const
        {
            return !operator==( rh );
        }
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerPipelineShaderExecutableProperties

    Description:
        Internal representations of the shader.

    \***********************************************************************************/
    struct DeviceProfilerPipelineShaderExecutableProperties
    {
        VkPipelineExecutablePropertiesKHR                          m_ExecutableProperties = {};
        std::vector<VkPipelineExecutableStatisticKHR>              m_ExecutableStatistics = {};
        std::vector<VkPipelineExecutableInternalRepresentationKHR> m_InternalRepresentations = {};
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerPipelineExecutableProperties

    Description:
        Internal representations of the pipeline and its shaders.

    \***********************************************************************************/
    struct DeviceProfilerPipelineExecutableProperties
    {
        std::vector<DeviceProfilerPipelineShaderExecutableProperties> m_Shaders = {};
    };

    typedef std::shared_ptr<DeviceProfilerPipelineExecutableProperties>
        DeviceProfilerPipelineExecutablePropertiesPtr;
}


namespace std
{
    template<>
    struct hash<Profiler::DeviceProfilerPipelineShaderTuple>
    {
        inline size_t operator()( const Profiler::DeviceProfilerPipelineShaderTuple& tuple ) const
        {
            return tuple.m_Hash;
        }
    };
}

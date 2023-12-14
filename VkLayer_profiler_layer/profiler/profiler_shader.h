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
        uint32_t                    m_Index = 0;
        VkShaderStageFlagBits       m_Stage = {};
        std::string                 m_EntryPoint = "";
        DeviceProfilerShaderModule* m_pShaderModule = nullptr;
    };

    /***********************************************************************************\

    Structure:
        DeviceProfilerPipelineShaderTuple

    Description:
        Collection of shaders that make up a single pipeline.

    \***********************************************************************************/
    struct DeviceProfilerPipelineShaderTuple
    {
        uint32_t m_Hash = 0;
        RuntimeArray<DeviceProfilerPipelineShader> m_Shaders = {};

        inline constexpr bool operator==( const DeviceProfilerPipelineShaderTuple& rh ) const
        {
            return m_Hash == rh.m_Hash;
        }

        inline constexpr bool operator!=( const DeviceProfilerPipelineShaderTuple& rh ) const
        {
            return !operator==( rh );
        }

        inline ArrayView<const DeviceProfilerPipelineShader> GetShadersAtStage(VkShaderStageFlagBits stage) const
        {
            size_t first = -1;
            for (; first < m_Shaders.size(); ++first)
                if (m_Shaders[first].m_Stage == stage)
                    break;

            size_t last = first + 1;
            for (; last < m_Shaders.size(); ++last)
                if (m_Shaders[last].m_Stage != stage)
                    break;

            if (first == m_Shaders.size())
                return {};

            const auto* pData = m_Shaders.data();
            return { pData + first, pData + last };
        }

        inline const DeviceProfilerPipelineShader* GetFirstShaderAtStage(VkShaderStageFlagBits stage) const
        {
            for (size_t i = 0; i < m_Shaders.size(); ++i)
                if (m_Shaders[i].m_Stage == stage)
                    return &m_Shaders[i];

            return nullptr;
        }

        inline const DeviceProfilerPipelineShader* GetShaderAtIndex( uint32_t index ) const
        {
            for (size_t i = 0; i < m_Shaders.size(); ++i)
                if (m_Shaders[i].m_Index == index)
                    return &m_Shaders[i];

            return nullptr;
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

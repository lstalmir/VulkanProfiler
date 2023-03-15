// Copyright (c) 2019-2022 Lukasz Stalmirski
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
#include "profiler_helpers/profiler_data_helpers.h"
#include "profiler_layer_objects/VkDevice_object.h"

#include <memory>

struct ImFont;
class TextEditor;

namespace Profiler
{
    struct DeviceProfilerPipelineData;

    class ProfilerShaderInspectorTab
    {
    public:
        ProfilerShaderInspectorTab(
            VkDevice_Object* pDevice,
            const DeviceProfilerPipelineData& pipeline,
            VkShaderStageFlagBits stage,
            ImFont* font );

        void Draw();

    private:
        VkDevice_Object&                  m_Device;
        DeviceProfilerStringSerializer    m_StringSerializer;

        ImFont*                           m_pImGuiCodeFont;

        // Shader stage that is being inspected in this tab.
        VkShaderStageFlagBits             m_ShaderStage;

        // Context of the shader module usage.
        const DeviceProfilerPipelineData& m_Pipeline;

        uint32_t                          m_PipelineExecutableIndex;
        VkPipelineExecutablePropertiesKHR m_PipelineExecutableProperties;
        std::vector<VkPipelineExecutableStatisticKHR> m_PipelineExecutableStatistics;
        std::vector<VkPipelineExecutableInternalRepresentationKHR> m_PipelineExecutableInternalRepresentations;

        std::string m_ShaderModuleDisassembly;

        // Widget that displays the shader code.
        std::unique_ptr<TextEditor> m_pTextEditor;

        int  m_CurrentTabIndex;
    };
}

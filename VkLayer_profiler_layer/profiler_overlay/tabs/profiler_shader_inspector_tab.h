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
#include "profiler/profiler_data.h"
#include "profiler_helpers/profiler_data_helpers.h"

#include <spirv/unified1/spirv.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct ImFont;
class TextEditor;

namespace Profiler
{
    struct VkDevice_Object;
    class DeviceProfilerStringSerializer;

    class DeviceProfilerShaderInspectorTab
    {
    public:
        struct Source
        {
            std::string m_Code;
            SpvSourceLanguage m_Language;
            uint32_t m_Filename;
        };

        struct SourceFilename
        {
            std::string m_ShortName;
            std::string m_FullPath;
        };

        struct SourceList
        {
            std::vector<Source> m_Sources;
            std::unordered_map<uint32_t, SourceFilename> m_Filenames;
        };

    public:
        DeviceProfilerShaderInspectorTab(
            VkDevice_Object& device,
            const DeviceProfilerPipelineData& pipeline,
            const DeviceProfilerPipelineShader& shader,
            ImFont* font );

        ~DeviceProfilerShaderInspectorTab();

        void Draw();

    private:
        VkDevice_Object& m_Device;
        DeviceProfilerStringSerializer m_StringSerializer;

        ImFont* m_pImGuiCodeFont;

        // Context of the shader module usage.
        VkPipeline m_PipelineHandle;
        DeviceProfilerPipelineExecutablePropertiesPtr m_pExecutableProperties;
        DeviceProfilerPipelineShader m_Shader;

        std::string m_ShaderModuleDisassembly;
        SourceList m_ShaderModuleSourceList;

        // Widgets that display the shader code.
        std::unordered_map<int, std::unique_ptr<TextEditor>> m_pTextEditors;
    };
} // namespace Profiler

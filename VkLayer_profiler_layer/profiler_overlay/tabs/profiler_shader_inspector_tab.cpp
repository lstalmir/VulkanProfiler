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

#include "profiler_shader_inspector_tab.h"

#include "profiler/profiler_data.h"

#include "../imgui_widgets/imgui_ex.h"
#include "../imgui_widgets/imgui_table_ex.h"

#include "../external/ImGuiColorTextEdit/TextEditor.h"

#include <spirv-tools/libspirv.h>


namespace Profiler
{
    /***********************************************************************************\

    Function:
        ProfilerShaderInspectorTab

    Description:
        Constructor.

    \***********************************************************************************/
    ProfilerShaderInspectorTab::ProfilerShaderInspectorTab(
        VkDevice_Object* pDevice,
        const DeviceProfilerPipelineData& pipeline,
        VkShaderStageFlagBits stage,
        ImFont* font )
        : m_Device( *pDevice )
        , m_StringSerializer( *pDevice )
        , m_pImGuiCodeFont( font )
        , m_ShaderStage( stage )
        , m_Pipeline( pipeline )
        , m_Executables()
        , m_pTextEditor( nullptr )
        , m_CurrentTabIndex( -1 )
        , m_Open( true )
    {
        // Get pipeline executable properties, if available.
        if( pDevice->EnabledExtensions.count( VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME ) )
        {
            VkPipelineInfoKHR pipelineInfo = {};
            pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INFO_KHR;
            pipelineInfo.pipeline = m_Pipeline.m_Handle;

            // Get number of executables compiled for this pipeline.
            uint32_t pipelineExecutableCount = 0;
            VkResult result = m_Device.Callbacks.GetPipelineExecutablePropertiesKHR(
                m_Device.Handle,
                &pipelineInfo,
                &pipelineExecutableCount,
                nullptr );

            if( (result == VK_SUCCESS) &&
                (pipelineExecutableCount > 0) )
            {
                std::vector<VkPipelineExecutablePropertiesKHR> pipelineExecutableProperties( pipelineExecutableCount );
                result = m_Device.Callbacks.GetPipelineExecutablePropertiesKHR(
                    m_Device.Handle,
                    &pipelineInfo,
                    &pipelineExecutableCount,
                    pipelineExecutableProperties.data() );

                if( result == VK_SUCCESS )
                {
                    uint32_t pipelineExecutableIndex = 0;

                    // Find the executable info for the inspected shader.
                    for( const auto& executable : pipelineExecutableProperties )
                    {
                        if( (executable.stages & stage) == stage )
                        {
                            auto& shaderExecutable = m_Executables.emplace_back();
                            shaderExecutable.m_Index = pipelineExecutableIndex;
                            shaderExecutable.m_Properties = executable;

                            // Get the statistics for the inspected shader.
                            VkPipelineExecutableInfoKHR pipelineExecutableInfo = {};
                            pipelineExecutableInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_INFO_KHR;
                            pipelineExecutableInfo.pipeline = m_Pipeline.m_Handle;
                            pipelineExecutableInfo.executableIndex = pipelineExecutableIndex;

                            uint32_t pipelineExecutableStatisticsCount = 0;
                            result = m_Device.Callbacks.GetPipelineExecutableStatisticsKHR(
                                m_Device.Handle,
                                &pipelineExecutableInfo,
                                &pipelineExecutableStatisticsCount,
                                nullptr );

                            if( (result == VK_SUCCESS) &&
                                (pipelineExecutableStatisticsCount > 0) )
                            {
                                shaderExecutable.m_Statistics.resize( pipelineExecutableStatisticsCount );
                                result = m_Device.Callbacks.GetPipelineExecutableStatisticsKHR(
                                    m_Device.Handle,
                                    &pipelineExecutableInfo,
                                    &pipelineExecutableStatisticsCount,
                                    shaderExecutable.m_Statistics.data() );

                                if( result != VK_SUCCESS )
                                {
                                    // Error occured while retrieving the pipeline executable statistics.
                                    shaderExecutable.m_Statistics.clear();
                                }
                            }

                            // Get the internal representation for the inspected shader.
                            uint32_t pipelineExecutableInternalRepresentationCount = 0;
                            result = m_Device.Callbacks.GetPipelineExecutableInternalRepresentationsKHR(
                                m_Device.Handle,
                                &pipelineExecutableInfo,
                                &pipelineExecutableInternalRepresentationCount,
                                nullptr );

                            if( (result == VK_SUCCESS) &&
                                (pipelineExecutableInternalRepresentationCount > 0) )
                            {
                                shaderExecutable.m_InternalRepresentations.resize( pipelineExecutableInternalRepresentationCount );
                                result = m_Device.Callbacks.GetPipelineExecutableInternalRepresentationsKHR(
                                    m_Device.Handle,
                                    &pipelineExecutableInfo,
                                    &pipelineExecutableInternalRepresentationCount,
                                    shaderExecutable.m_InternalRepresentations.data() );

                                if( result != VK_SUCCESS )
                                {
                                    // Error occured while retrieving the pipeline executable internal representations.
                                    shaderExecutable.m_InternalRepresentations.clear();
                                }
                            }
                        }

                        pipelineExecutableIndex++;
                    }
                }
            }
        }

        // Get the SPIR-V module associated with the inspected shader stage.
        const auto& bytecode = pipeline.m_ShaderTuple.m_Shaders[ stage ].m_pShaderModule->m_Bytecode;

        // Disassemble the SPIR-V module.
        spv_context spvContext = spvContextCreate( spv_target_env::SPV_ENV_UNIVERSAL_1_6 );

        spv_text spvDisassembly;
        spv_result_t result = spvBinaryToText( spvContext, bytecode.data(), bytecode.size(),
            SPV_BINARY_TO_TEXT_OPTION_FRIENDLY_NAMES,
            &spvDisassembly,
            nullptr );

        if( result == SPV_SUCCESS )
        {
            m_ShaderModuleDisassembly = spvDisassembly->str;
        }
        else
        {
            m_ShaderModuleDisassembly = "Failed to disassemble the shader module.";
        }

        spvTextDestroy( spvDisassembly );
        spvContextDestroy( spvContext );

        // Create the text editor.
        m_pTextEditor = std::make_unique<TextEditor>();
    }
    /***********************************************************************************\

    Function:
        ~ProfilerShaderInspectorTab

    Description:
        Destructor.

    \***********************************************************************************/
    ProfilerShaderInspectorTab::~ProfilerShaderInspectorTab()
    {
        // Empty, but required because TextEditor is incomplete in the header,
        // so the default destructor can't be used.
    }

    /***********************************************************************************\

    Function:
        Draw

    Description:
        Draws the shader inspector tab in the current profiler's window.

    \***********************************************************************************/
    void ProfilerShaderInspectorTab::Draw()
    {
        // Get the name of the shader module.
        const auto pipelineName = m_StringSerializer.GetName( m_Pipeline ) + " (" + std::to_string( m_ShaderStage ) + ")";

        if( ImGui::Begin( pipelineName.c_str(), &m_Open, ImGuiWindowFlags_AlwaysVerticalScrollbar ) )
        {
            // Print the internal shader representation.
            if( ImGui::BeginTabBar( (pipelineName + "_executable_properties").c_str() ) )
            {
                int tabIndex = 0;

                if( ImGui::BeginTabItem( "Disassembly" ) )
                {
                    ImGui::PushFont( m_pImGuiCodeFont );

                    if( m_CurrentTabIndex != tabIndex )
                    {
                        m_pTextEditor->SetText( m_ShaderModuleDisassembly );
                        m_CurrentTabIndex = tabIndex;
                    }

                    m_pTextEditor->Render( "Disassembly" );

                    ImGui::PopFont();
                    ImGui::EndTabItem();
                }

                for( const auto& executable : m_Executables )
                {
                    tabIndex++;

                    if (ImGui::BeginTabItem(executable.m_Properties.name))
                    {
                        // Print statistics, if available.
                        for (const auto& stat : executable.m_Statistics)
                        {
                            ImGui::TextUnformatted(stat.name);

                            if (ImGui::IsItemHovered() && stat.description[0])
                            {
                                ImGui::BeginTooltip();
                                ImGui::PushTextWrapPos(350.f);
                                ImGui::TextUnformatted(stat.description);
                                ImGui::PopTextWrapPos();
                                ImGui::EndTooltip();
                            }

                            switch (stat.format)
                            {
                            case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_BOOL32_KHR:
                                ImGuiX::TextAlignRight("%s", stat.value.b32 ? "True" : "False");
                                break;

                            case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_INT64_KHR:
                                ImGuiX::TextAlignRight("%lld", stat.value.i64);
                                break;

                            case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR:
                                ImGuiX::TextAlignRight("%llu", stat.value.u64);
                                break;

                            case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_FLOAT64_KHR:
                                ImGuiX::TextAlignRight("%lf", stat.value.f64);
                                break;
                            }
                        }

                        // Print internal representations, if available.
                        if (ImGui::BeginTabBar((pipelineName + "_internal_representations").c_str()))
                        {
                            for (const auto& representation : executable.m_InternalRepresentations)
                            {
                                if (ImGui::BeginTabItem(representation.name))
                                {
                                    ImGui::PushFont(m_pImGuiCodeFont);

                                    if (representation.isText)
                                    {
                                        if (m_CurrentTabIndex != tabIndex)
                                        {
                                            m_pTextEditor->SetText(static_cast<const char*>(representation.pData));
                                            m_CurrentTabIndex = tabIndex;
                                        }

                                        m_pTextEditor->Render("Disassembly");
                                    }
                                    else
                                    {
                                        ImGui::TextUnformatted("Binary");
                                    }

                                    ImGui::PopFont();
                                    ImGui::EndTabItem();
                                }
                            }

                            ImGui::EndTabBar();
                        }

                        ImGui::EndTabItem();
                    }
                }

                ImGui::EndTabBar();
            }
        }

        // End must be called regardless of the Begin() return value.
        ImGui::End();
    }
}

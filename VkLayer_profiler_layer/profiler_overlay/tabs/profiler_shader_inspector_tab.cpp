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
#include "profiler_layer_objects/VkDevice_object.h"

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
    DeviceProfilerShaderInspectorTab::DeviceProfilerShaderInspectorTab(
        VkDevice_Object& device,
        const DeviceProfilerPipelineData& pipeline,
        VkShaderStageFlagBits stage,
        ImFont* font )
        : m_Device( device )
        , m_StringSerializer( device )
        , m_pImGuiCodeFont( font )
        , m_ShaderStage( stage )
        , m_Pipeline( pipeline )
        , m_pTextEditors()
    {
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
    }

    /***********************************************************************************\

    Function:
        ~ProfilerShaderInspectorTab

    Description:
        Destructor.

    \***********************************************************************************/
    DeviceProfilerShaderInspectorTab::~DeviceProfilerShaderInspectorTab()
    {
    }

    /***********************************************************************************\

    Function:
        Draw

    Description:
        Draws the shader inspector tab in the current profiler's window.

    \***********************************************************************************/
    void DeviceProfilerShaderInspectorTab::Draw()
    {
        // Get the name of the shader module.
        const auto pipelineName = m_StringSerializer.GetName( m_Pipeline ) + " (" + std::to_string( m_ShaderStage ) + ")";

        // Print the shader's disassembly.
        if( ImGui::BeginTabBar( ("##" + pipelineName + "_internal_representations").c_str() ) )
        {
            int tabIndex = 0;
            if( ImGui::BeginTabItem( "Disassembly" ) )
            {
                ImGui::PushFont( m_pImGuiCodeFont );
                
                auto& pTabTextEditor = m_pTextEditors[ tabIndex ];
                if( !pTabTextEditor )
                {
                    pTabTextEditor = std::make_unique<TextEditor>();
                    pTabTextEditor->SetText( m_ShaderModuleDisassembly );
                }

                pTabTextEditor->Render( "Disassembly" );

                ImGui::PopFont();
                ImGui::EndTabItem();
            }
            
            // Print pipeline executable statistics if the extension is supported.
            if( m_Pipeline.m_pExecutableProperties )
            {
                for( const auto& executable : m_Pipeline.m_pExecutableProperties->m_Shaders )
                {
                    // Skip executables that are not part of the inspected stage.
                    if( (executable.m_ExecutableProperties.stages & m_ShaderStage) != m_ShaderStage )
                    {
                        continue;
                    }

                    tabIndex++;

                    if( ImGui::BeginTabItem( executable.m_ExecutableProperties.name ) )
                    {
                        // Print tooltip if the tab is open.
                        ImGuiX::TooltipUnformatted( executable.m_ExecutableProperties.description );

                        for( uint32_t i = 0; i < executable.m_ExecutableStatistics.size(); ++i )
                        {
                            const auto& statistic = executable.m_ExecutableStatistics[ i ];

                            ImGui::Text( "%s", statistic.name );
                            ImGuiX::TooltipUnformatted( statistic.description );

                            switch( statistic.format )
                            {
                            case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_BOOL32_KHR:
                                ImGuiX::TextAlignRight( "%s", statistic.value.b32 ? "True" : "False" );
                                break;

                            case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_INT64_KHR:
                                ImGuiX::TextAlignRight( "%lld", statistic.value.i64 );
                                break;

                            case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR:
                                ImGuiX::TextAlignRight( "%llu", statistic.value.u64 );
                                break;

                            case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_FLOAT64_KHR:
                                ImGuiX::TextAlignRight( "%lf", statistic.value.f64 );
                                break;
                            }
                        }

                        int internalRepresentationIndex = tabIndex;
                        for( const auto& internalRepresentation : executable.m_InternalRepresentations )
                        {
                            if( ImGui::CollapsingHeader( internalRepresentation.name ) )
                            {
                                ImGui::PushFont( m_pImGuiCodeFont );

                                if( internalRepresentation.isText )
                                {
                                    auto& pTabTextEditor = m_pTextEditors[ internalRepresentationIndex ];
                                    if( !pTabTextEditor )
                                    {
                                        pTabTextEditor = std::make_unique<TextEditor>();
                                        pTabTextEditor->SetText( static_cast<const char*>( internalRepresentation.pData ) );
                                    }

                                    pTabTextEditor->Render( "Disassembly" );
                                }
                                else
                                {
                                    ImGui::TextUnformatted( "Binary" );
                                }

                                ImGui::PopFont();
                                internalRepresentationIndex += 0x100;
                            }
                        }

                        ImGui::EndTabItem();
                    }
                    else
                    {
                        // Print tooltip if the tab is closed.
                        ImGuiX::TooltipUnformatted( executable.m_ExecutableProperties.description );
                    }
                }
            }

            ImGui::EndTabBar();
        }
    }
}

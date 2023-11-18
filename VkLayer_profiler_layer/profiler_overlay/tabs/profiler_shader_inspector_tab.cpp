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

#include <TextEditor.h>

#include <spirv-tools/libspirv.h>


namespace Profiler
{
    const TextEditor::LanguageDefinition& GetSpirvLanguageDefinition();

    /***********************************************************************************\

    Function:
        GetShaderLanguageDefinition

    Description:
        Returns a shader language definition.

    \***********************************************************************************/
    const TextEditor::LanguageDefinition& GetShaderLanguageDefinition(
        SpvSourceLanguage language )
    {
        const TextEditor::LanguageDefinition* pLanguageDefinition = nullptr;
        switch( language )
        {
        default:
        case SpvSourceLanguageUnknown:
        case SpvSourceLanguageOpenCL_C:
            pLanguageDefinition = &TextEditor::LanguageDefinition::C();
            break;
        case SpvSourceLanguageOpenCL_CPP:
        case SpvSourceLanguageCPP_for_OpenCL:
        case SpvSourceLanguageSYCL:
            pLanguageDefinition = &TextEditor::LanguageDefinition::CPlusPlus();
            break;
        case SpvSourceLanguageESSL:
        case SpvSourceLanguageGLSL:
            pLanguageDefinition = &TextEditor::LanguageDefinition::GLSL();
            break;
        case SpvSourceLanguageHLSL:
            pLanguageDefinition = &TextEditor::LanguageDefinition::HLSL();
            break;
        }
        assert( pLanguageDefinition );
        return *pLanguageDefinition;
    }

    /***********************************************************************************\

    Function:
        ReadSpirvOperand

    Description:
        Reads an operand from SPIR-V instruction.

    \***********************************************************************************/
    template <typename T>
    T ReadSpirvOperand(
        const spv_parsed_instruction_t* pParsedInstruction,
        uint32_t operand )
    {
        if( operand < pParsedInstruction->num_operands )
        {
            const uint32_t firstWord = pParsedInstruction->operands[ operand ].offset;
            return *reinterpret_cast<const T*>( pParsedInstruction->words + firstWord );
        }
        return T();
    }

    template <>
    const char* ReadSpirvOperand(
        const spv_parsed_instruction_t* pParsedInstruction,
        uint32_t operand )
    {
        if( operand < pParsedInstruction->num_operands )
        {
            const uint32_t firstWord = pParsedInstruction->operands[ operand ].offset;
            return reinterpret_cast<const char*>( pParsedInstruction->words + firstWord );
        }
        return nullptr;
    }

    /***********************************************************************************\

    Function:
        GetSpirvSource

    Description:
        Reads OpSource instructions into an array of shader sources.

    \***********************************************************************************/
    static spv_result_t GetSpirvSource(
        void* pUserData,
        const spv_parsed_instruction_t* pParsedInstruction )
    {
        auto& sources = *static_cast<DeviceProfilerShaderInspectorTab::SourceList*>( pUserData );

        // Save debug strings that contain paths to the shader sources.
        if( pParsedInstruction->opcode == SpvOpString )
        {
            const uint32_t id = ReadSpirvOperand<uint32_t>( pParsedInstruction, 0 );
            const char* pPath = ReadSpirvOperand<const char*>( pParsedInstruction, 1 );

            DeviceProfilerShaderInspectorTab::SourceFilename filename;
            filename.m_FullPath = pPath;
            filename.m_ShortName = std::filesystem::path( pPath ).filename().string();
            sources.m_Filenames[ id ] = filename;
        }

        // Save shader sources.
        if( pParsedInstruction->opcode == SpvOpSource )
        {
            const char* pSource = ReadSpirvOperand<const char*>( pParsedInstruction, 3 );
            if( pSource )
            {
                DeviceProfilerShaderInspectorTab::Source source;
                source.m_Code = pSource;
                source.m_Language = ReadSpirvOperand<SpvSourceLanguage>( pParsedInstruction, 0 );
                source.m_Filename = ReadSpirvOperand<uint32_t>( pParsedInstruction, 2 );
                sources.m_Sources.push_back( source );
            }
        }

        return SPV_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        CreateTextEditor

    Description:
        Creates a read-only text editor pre-initialized with a string.

    \***********************************************************************************/
    static std::unique_ptr<TextEditor> CreateTextEditor(
        const char* pString,
        const TextEditor::LanguageDefinition& languageDefinition = TextEditor::LanguageDefinition() )
    {
        std::unique_ptr<TextEditor> pTextEditor = std::make_unique<TextEditor>();
        pTextEditor->SetText( pString );
        pTextEditor->SetLanguageDefinition( languageDefinition );
        pTextEditor->SetReadOnly( true );
        pTextEditor->SetShowWhitespaces( false );
        return pTextEditor;
    }

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
        , m_ShaderModuleSourceList()
        , m_pTextEditors()
    {
        // Get the SPIR-V module associated with the inspected shader stage.
        const auto& bytecode = pipeline.m_ShaderTuple.m_Shaders[ stage ].m_pShaderModule->m_Bytecode;

        // Disassemble the SPIR-V module.
        spv_context spvContext = spvContextCreate( spv_target_env::SPV_ENV_UNIVERSAL_1_6 );

        spv_text spvDisassembly;
        spv_result_t result = spvBinaryToText( spvContext, bytecode.data(), bytecode.size(),
            SPV_BINARY_TO_TEXT_OPTION_FRIENDLY_NAMES |
                SPV_BINARY_TO_TEXT_OPTION_INDENT |
                SPV_BINARY_TO_TEXT_OPTION_COMMENT,
            &spvDisassembly,
            nullptr );

        if( result == SPV_SUCCESS )
        {
            m_ShaderModuleDisassembly = spvDisassembly->str;

            // Read OpSources from the bytecode.
            spvBinaryParse( spvContext, &m_ShaderModuleSourceList, bytecode.data(), bytecode.size(), nullptr, GetSpirvSource, nullptr );
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
        // Get height of the target window.
        const float height = ImGui::GetWindowHeight() - 125;

        // Get the name of the shader module.
        const auto pipelineName = m_StringSerializer.GetName( m_Pipeline ) + " (" + std::to_string( m_ShaderStage ) + ")";

        // Print the shader's disassembly.
        if( ImGui::BeginTabBar( ( "##" + pipelineName + "_internal_representations" ).c_str() ) )
        {
            int tabIndex = 0;
            if( ImGui::BeginTabItem( "Disassembly" ) )
            {
                // Get text editor with the SPIR-V disassembly.
                auto& pTabTextEditor = m_pTextEditors[ tabIndex ];
                if( !pTabTextEditor )
                {
                    pTabTextEditor = CreateTextEditor(
                        m_ShaderModuleDisassembly.c_str(),
                        GetSpirvLanguageDefinition() );
                }

                ImGui::PushFont( m_pImGuiCodeFont );
                pTabTextEditor->Render( "Disassembly", ImVec2( 0, height ) );
                ImGui::PopFont();
                ImGui::EndTabItem();
            }
            tabIndex++;

            // Print sources included in the SPIR-V.
            if( !m_ShaderModuleSourceList.m_Sources.empty() &&
                ImGui::BeginTabItem( "Sources" ) )
            {
                if( ImGui::BeginTabBar( "##Sources" ) )
                {
                    int sourceTabIndex = tabIndex;
                    for( const Source& source : m_ShaderModuleSourceList.m_Sources )
                    {
                        const SourceFilename& sourceFilename = m_ShaderModuleSourceList.m_Filenames.at( source.m_Filename );
                        if( ImGui::BeginTabItem( sourceFilename.m_ShortName.c_str() ) )
                        {
                            // Print tooltip if the tab is open.
                            ImGuiX::TooltipUnformatted( sourceFilename.m_FullPath.c_str() );

                            // Get text editor with the source.
                            auto& pTabTextEditor = m_pTextEditors[ sourceTabIndex ];
                            if( !pTabTextEditor )
                            {
                                pTabTextEditor = CreateTextEditor(
                                    source.m_Code.c_str(),
                                    GetShaderLanguageDefinition( source.m_Language ) );
                            }

                            ImGui::PushFont( m_pImGuiCodeFont );
                            pTabTextEditor->Render( sourceFilename.m_FullPath.c_str(), ImVec2( 0, height ) );
                            ImGui::PopFont();
                            ImGui::EndTabItem();
                        }
                        else
                        {
                            // Print tooltip if the tab is closed.
                            ImGuiX::TooltipUnformatted( sourceFilename.m_FullPath.c_str() );
                        }
                        sourceTabIndex += 0x100;
                    }
                    ImGui::EndTabBar();
                }
                ImGui::EndTabItem();
            }
            tabIndex++;

            // Print pipeline executable statistics if the extension is supported.
            if( m_Pipeline.m_pExecutableProperties )
            {
                for( const auto& executable : m_Pipeline.m_pExecutableProperties->m_Shaders )
                {
                    // Skip executables that are not part of the inspected stage.
                    if( ( executable.m_ExecutableProperties.stages & m_ShaderStage ) != m_ShaderStage )
                    {
                        continue;
                    }

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
                                    // Get text editor with the internal representation.
                                    auto& pTabTextEditor = m_pTextEditors[ internalRepresentationIndex ];
                                    if( !pTabTextEditor )
                                    {
                                        pTabTextEditor = CreateTextEditor( static_cast<const char*>( internalRepresentation.pData ) );
                                    }

                                    pTabTextEditor->Render( "Disassembly", ImVec2( 0, height ) );
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

                    tabIndex++;
                }
            }

            ImGui::EndTabBar();
        }
    }
}

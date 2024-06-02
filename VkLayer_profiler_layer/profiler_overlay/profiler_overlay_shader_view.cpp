// Copyright (c) 2024 Lukasz Stalmirski
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

#include "profiler_overlay_shader_view.h"
#include "profiler_overlay_fonts.h"
#include "profiler/profiler_helpers.h"
#include "profiler_layer_objects/VkDevice_object.h"

#include <string_view>

#include <spirv/unified1/spirv.h>
#include <spirv-tools/libspirv.h>

#include <imgui.h>
#include <TextEditor.h>

namespace
{
    struct Source
    {
        uint32_t            m_FilenameStringID;
        SpvSourceLanguage   m_Language;
        const char*         m_pData;
    };

    struct SourceList
    {
        std::vector<Source> m_Sources;
        std::unordered_map<uint32_t, const char*> m_pStrings;
    };

    /***********************************************************************************\

    Function:
        GetSpirvOperand

    Description:
        Reads an operand of a SPIR-V instruction.

    \***********************************************************************************/
    template<typename T>
    T GetSpirvOperand( const spv_parsed_instruction_t* pInstruction, uint32_t operand )
    {
        if( operand < pInstruction->num_operands )
        {
            const uint32_t firstWord = pInstruction->operands[ operand ].offset;
            return *reinterpret_cast<const T*>(pInstruction->words + firstWord);
        }
        return T();
    }

    /***********************************************************************************\

    Function:
        GetSpirvOperand

    Description:
        Specialization that reads a string operand of a SPIR-V instruction.

    \***********************************************************************************/
    template<>
    const char* GetSpirvOperand( const spv_parsed_instruction_t* pInstruction, uint32_t operand )
    {
        if( operand < pInstruction->num_operands )
        {
            const uint32_t firstWord = pInstruction->operands[ operand ].offset;
            return reinterpret_cast<const char*>(pInstruction->words + firstWord);
        }
        return nullptr;
    }

    /***********************************************************************************\

    Function:
        GetSpirvSources

    Description:
        Reads all OpSource instructions and returns a list of shader sources associated
        with the SPIR-V binary.

    \***********************************************************************************/
    static spv_result_t GetSpirvSources( void* pUserData, const spv_parsed_instruction_t* pInstruction )
    {
        SourceList* pSourceList = static_cast<SourceList*>(pUserData);
        assert( pSourceList );

        // OpStrings define paths to the embedded sources.
        if( pInstruction->opcode == SpvOpString )
        {
            const uint32_t id = GetSpirvOperand<uint32_t>( pInstruction, 0 );
            const char* pString = GetSpirvOperand<const char*>( pInstruction, 1 );
            if( pString )
            {
                pSourceList->m_pStrings[ id ] = pString;
            }
            return SPV_SUCCESS;
        }

        // OpSources may contain embedded sources.
        if( pInstruction->opcode == SpvOpSource )
        {
            const char* pData = GetSpirvOperand<const char*>( pInstruction, 3 );
            if( pData )
            {
                Source& source = pSourceList->m_Sources.emplace_back();
                source.m_Language = GetSpirvOperand<SpvSourceLanguage>( pInstruction, 0 );
                source.m_FilenameStringID = GetSpirvOperand<uint32_t>( pInstruction, 2 );
                source.m_pData = pData;
            }
            return SPV_SUCCESS;
        }

        return SPV_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        RemoveSpirvSources

    Description:
        Removes inline sources from disassembled spirv code.

    \***********************************************************************************/
    static void RemoveSpirvSources( spv_text text )
    {
        std::string_view source( text->str, text->length );

        auto offset = source.find( "OpSource" );
        while( offset != source.npos )
        {
            source = source.substr( offset + 8 );

            // Find end of the OpSource line and beginning of the source code.
            auto eofOffset = source.find( '\n' );
            auto codeOffset = source.find( '\"' );

            if( codeOffset < eofOffset )
            {
                source = source.substr( codeOffset );

                // Find end of the source code.
                offset = source.find( "\n\"\n" );

                if( offset != source.npos )
                {
                    // Remove the current range from the text.
                    const size_t removeSize = (offset + 2);
                    memmove( const_cast<char*>(source.data()), source.data() + removeSize, (source.length() + 1) - removeSize );
                }
            }

            offset = source.find( "OpSource" );
        }
    }

    /***********************************************************************************\

    Function:
        GetSpirvSourceShaderFormat

    Description:
        Converts SpvSourceLanguage to ShaderFormat.

    \***********************************************************************************/
    static constexpr Profiler::ShaderFormat GetSpirvSourceShaderFormat( SpvSourceLanguage language )
    {
        switch( language )
        {
        case SpvSourceLanguageESSL:
        case SpvSourceLanguageGLSL:
            return Profiler::ShaderFormat::eGlsl;

        case SpvSourceLanguageHLSL:
            return Profiler::ShaderFormat::eHlsl;

        case SpvSourceLanguageOpenCL_C:
        case SpvSourceLanguageOpenCL_CPP:
        case SpvSourceLanguageCPP_for_OpenCL:
            return Profiler::ShaderFormat::eCpp;

        default:
            return Profiler::ShaderFormat::eText;
        }
    }

    /***********************************************************************************\

    Function:
        GetSpirvLanguageDefinition

    Description:
        Returns a reference to the spirv language definition for syntax highlighting.

    \***********************************************************************************/
    static const TextEditor::LanguageDefinition& GetSpirvLanguageDefinition()
    {
        static bool initialized = false;
        static TextEditor::LanguageDefinition languageDefinition;

        if( !initialized )
        {
            // Initialize the language definition on the first call to this function.
            languageDefinition.mName = "SPIR-V";

            // Tokenizer.
            languageDefinition.mTokenRegexStrings.push_back( std::pair( "L?\\\"(\\\\.|[^\\\"])*\\\"", TextEditor::PaletteIndex::String ) );
            languageDefinition.mTokenRegexStrings.push_back( std::pair( "\\'\\\\?[^\\']\\'", TextEditor::PaletteIndex::CharLiteral ) );
            languageDefinition.mTokenRegexStrings.push_back( std::pair( "Op[a-zA-Z0-9]+", TextEditor::PaletteIndex::Keyword ) );
            languageDefinition.mTokenRegexStrings.push_back( std::pair( "[a-zA-Z_%][a-zA-Z0-9_]*", TextEditor::PaletteIndex::Identifier ) );
            languageDefinition.mTokenRegexStrings.push_back( std::pair( "[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", TextEditor::PaletteIndex::Number ) );
            languageDefinition.mTokenRegexStrings.push_back( std::pair( "[+-]?[0-9]+[Uu]?[lL]?[lL]?", TextEditor::PaletteIndex::Number ) );
            languageDefinition.mTokenRegexStrings.push_back( std::pair( "0[0-7]+[Uu]?[lL]?[lL]?", TextEditor::PaletteIndex::Number ) );
            languageDefinition.mTokenRegexStrings.push_back( std::pair( "0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", TextEditor::PaletteIndex::Number ) );
            languageDefinition.mTokenRegexStrings.push_back( std::pair( "[\\[\\]\\{\\}\\!\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\,\\.]", TextEditor::PaletteIndex::Punctuation ) );

            // Comments.
            languageDefinition.mSingleLineComment = ";";
            languageDefinition.mCommentStart = ";";
            languageDefinition.mCommentEnd = "\n";

            // Parser options.
            languageDefinition.mAutoIndentation = true;
            languageDefinition.mCaseSensitive = true;
        }

        return languageDefinition;
    }
}

namespace Profiler
{
    /***********************************************************************************\

    Function:
        OverlayShaderView

    Description:
        Constructor.

    \***********************************************************************************/
    OverlayShaderView::OverlayShaderView( const OverlayFonts& fonts )
        : m_Fonts( fonts )
        , m_pTextEditor( nullptr )
        , m_pShaderRepresentations( 0 )
        , m_SpvTargetEnv( SPV_ENV_UNIVERSAL_1_0 )
        , m_CurrentTabIndex( -1 )
    {
        m_pTextEditor = std::make_unique<TextEditor>();
        m_pTextEditor->SetReadOnly( true );
        m_pTextEditor->SetShowWhitespaces( false );
    }

    /***********************************************************************************\

    Function:
        ~OverlayShaderView

    Description:
        Destructor.

    \***********************************************************************************/
    OverlayShaderView::~OverlayShaderView()
    {
        Clear();
    }

    /***********************************************************************************\

    Function:
        SetTargetDevice

    Description:
        Selects the SPIR-V target env used for disassembling the shaders.

    \***********************************************************************************/
    void OverlayShaderView::SetTargetDevice( VkDevice_Object* pDevice )
    {
        m_SpvTargetEnv = SPV_ENV_UNIVERSAL_1_0;

        if( pDevice )
        {
            // Select the target env based on the api version used by the application.
            switch( pDevice->pInstance->ApplicationInfo.apiVersion )
            {
            case VK_API_VERSION_1_0:
                m_SpvTargetEnv = SPV_ENV_VULKAN_1_0;
                break;

            case VK_API_VERSION_1_1:
                m_SpvTargetEnv = pDevice->EnabledExtensions.count( VK_KHR_SPIRV_1_4_EXTENSION_NAME )
                    ? SPV_ENV_VULKAN_1_1_SPIRV_1_4
                    : SPV_ENV_VULKAN_1_1;
                break;

            case VK_API_VERSION_1_2:
                m_SpvTargetEnv = SPV_ENV_VULKAN_1_2;
                break;

            case VK_API_VERSION_1_3:
                m_SpvTargetEnv = SPV_ENV_VULKAN_1_3;
                break;
            }
        }
    }

    /***********************************************************************************\

    Function:
        Clear

    Description:
        Resets the shader view and removes all shader representations.

    \***********************************************************************************/
    void OverlayShaderView::Clear()
    {
        // Free all shader representations.
        for( ShaderRepresentation* pShaderRepresentation : m_pShaderRepresentations )
        {
            free( pShaderRepresentation );
        }

        m_pShaderRepresentations.clear();

        // Reset current tab index.
        m_CurrentTabIndex = -1;
    }

    /***********************************************************************************\

    Function:
        AddBytecode

    Description:
        Disassembles the SPIR-V binary to a human-readable assembly code and adds it
        as a "Disassembly" shader representation.

    \***********************************************************************************/
    void OverlayShaderView::AddBytecode( const uint32_t* pBinary, size_t wordCount )
    {
        spv_context context = spvContextCreate( static_cast<spv_target_env>(m_SpvTargetEnv) );
        spv_text text;

        // Disassembler options.
        uint32_t options =
            SPV_BINARY_TO_TEXT_OPTION_INDENT |
            SPV_BINARY_TO_TEXT_OPTION_COMMENT |
            SPV_BINARY_TO_TEXT_OPTION_FRIENDLY_NAMES;

        // Disassemble the binary.
        spv_result_t result = spvBinaryToText( context, pBinary, wordCount, options, &text, nullptr );

        if( result == SPV_SUCCESS )
        {
            // Remove inline OpSources, they will be moved to another tab.
            RemoveSpirvSources( text );

            AddShaderRepresentation( "Disassembly", text->str, text->length, ShaderFormat::eSpirv );

            // Parse shader sources that may be embedded into the binary.
            SourceList sourceList;
            spvBinaryParse( context, &sourceList, pBinary, wordCount, nullptr, GetSpirvSources, nullptr );

            for( Source& source : sourceList.m_Sources )
            {
                size_t dataLength = 0;

                // Calculate length of the embedded shader source.
                if( source.m_pData )
                {
                    dataLength = strlen( source.m_pData );
                }

                // Extract filename of the embedded source.
                const char* pFilename = sourceList.m_pStrings[ source.m_FilenameStringID ];
                if( !pFilename || !strlen( pFilename ) )
                {
                    pFilename = "Source";
                }

                // Use the last path component for tab name.
                const char* pBasename = strrchr( pFilename, '/' );
                if( !pBasename )
                {
                    pBasename = strrchr( pFilename, '\\' );
                }

                AddShaderRepresentation( pBasename ? pBasename + 1 : pFilename, source.m_pData, dataLength,
                    GetSpirvSourceShaderFormat( source.m_Language ) );
            }
        }

        spvTextDestroy( text );
        spvContextDestroy( context );
    }

    /***********************************************************************************\

    Function:
        AddShaderRepresentation

    Description:
        Adds a tab to the shader view with a text or binary shader representation.

    \***********************************************************************************/
    void OverlayShaderView::AddShaderRepresentation( const char* pName, const void* pData, size_t dataSize, ShaderFormat format )
    {
        size_t nameSize = strlen( pName ) + 1;

        // Compute total size required for the shader representation.
        size_t shaderRepresentationSize = sizeof( ShaderRepresentation );
        shaderRepresentationSize += nameSize;
        shaderRepresentationSize += dataSize;

        // Allocate the shader representation.
        ShaderRepresentation* pShaderRepresentation = static_cast<ShaderRepresentation*>( malloc( shaderRepresentationSize ) );
        if( !pShaderRepresentation )
        {
            // Try to allocate a smaller version of the representation, without the data.
            shaderRepresentationSize -= dataSize;
            pShaderRepresentation = static_cast<ShaderRepresentation*>( malloc( shaderRepresentationSize ) );

            if( !pShaderRepresentation )
            {
                return;
            }

            // Don't include the data in the shader representation.
            pData = nullptr;
            dataSize = 0;
        }

        pShaderRepresentation->m_pName = reinterpret_cast<char*>( pShaderRepresentation + 1 );
        ProfilerStringFunctions::CopyString( pShaderRepresentation->m_pName, nameSize, pName, nameSize );

        if( pData && dataSize > 0 )
        {
            // Save shader representation data.
            pShaderRepresentation->m_pData = pShaderRepresentation->m_pName + nameSize;
            pShaderRepresentation->m_DataSize = dataSize;
            memcpy( pShaderRepresentation->m_pData, pData, dataSize );
        }
        else
        {
            // No shader representation data provided or memory allocation failed.
            pShaderRepresentation->m_pData = nullptr;
            pShaderRepresentation->m_DataSize = 0;
        }

        pShaderRepresentation->m_Format = format;

        // Add the shader representation.
        m_pShaderRepresentations.push_back( pShaderRepresentation );
    }

    /***********************************************************************************\

    Function:
        Draw

    Description:
        Draws the shader view.

    \***********************************************************************************/
    void OverlayShaderView::Draw()
    {
        ImGui::PushFont( m_Fonts.GetDefaultFont() );
        ImGui::PushStyleVar( ImGuiStyleVar_TabRounding, 0.0f );

        if( ImGui::BeginTabBar( "ShaderRepresentations", ImGuiTabBarFlags_None ) )
        {
            // Draw shader representations in tabs.
            int tabIndex = 0;
            for( ShaderRepresentation* pShaderRepresentation : m_pShaderRepresentations )
            {
                DrawShaderRepresentation( tabIndex++, pShaderRepresentation );
            }

            ImGui::EndTabBar();
        }

        ImGui::PopStyleVar();
        ImGui::PopFont();
    }

    /***********************************************************************************\

    Function:
        Draw

    Description:
        Draws a tab with the shader representation.

    \***********************************************************************************/
    void OverlayShaderView::DrawShaderRepresentation( int tabIndex, ShaderRepresentation* pShaderRepresentation )
    {
        if( ImGui::BeginTabItem( pShaderRepresentation->m_pName ) )
        {
            // Early out if shader representation data is not available.
            if( !pShaderRepresentation->m_pData )
            {
                ImGui::TextUnformatted( "Shader representation data not available." );
                ImGui::EndTabItem();
                return;
            }

            // Update the text editor with the current tab.
            if( m_CurrentTabIndex != tabIndex )
            {
                if( pShaderRepresentation->m_Format != ShaderFormat::eBinary )
                {
                    const char* pText = reinterpret_cast<const char*>(pShaderRepresentation->m_pData);

                    // TODO: This creates a temporary copy of the shader representation.
                    m_pTextEditor->SetText( std::string( pText, pText + pShaderRepresentation->m_DataSize ) );

                    // Set syntax highlighting according to the format.
                    switch( pShaderRepresentation->m_Format )
                    {
                    case ShaderFormat::eSpirv:
                        m_pTextEditor->SetLanguageDefinition( GetSpirvLanguageDefinition() );
                        break;
                    case ShaderFormat::eGlsl:
                        m_pTextEditor->SetLanguageDefinition( TextEditor::LanguageDefinition::GLSL() );
                        break;
                    case ShaderFormat::eHlsl:
                        m_pTextEditor->SetLanguageDefinition( TextEditor::LanguageDefinition::HLSL() );
                        break;
                    case ShaderFormat::eCpp:
                        m_pTextEditor->SetLanguageDefinition( TextEditor::LanguageDefinition::CPlusPlus() );
                        break;
                    default:
                        m_pTextEditor->SetLanguageDefinition( TextEditor::LanguageDefinition() );
                        break;
                    }
                }

                m_CurrentTabIndex = tabIndex;
            }

            // Print shader representation data.
            ImGui::PushFont( m_Fonts.GetCodeFont() );

            m_pTextEditor->Render( "##ShaderRepresentationTextEdit" );

            ImGui::PopFont();
            ImGui::EndTabItem();
        }
    }
}

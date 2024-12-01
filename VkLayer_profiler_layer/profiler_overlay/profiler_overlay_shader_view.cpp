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
#include "profiler_overlay_resources.h"
#include "profiler/profiler_shader.h"
#include "profiler/profiler_helpers.h"
#include "profiler_layer_objects/VkDevice_object.h"

#include <string_view>
#include <sstream>
#include <filesystem>
#include <inttypes.h>

#include <spirv/unified1/spirv.h>
#include <spirv-tools/libspirv.h>
#include <spirv_cross_c.h>

#include <imgui.h>
#include <TextEditor.h>
#include <ImGuiFileDialog.h>

#include "imgui_widgets/imgui_ex.h"

#ifndef PROFILER_BUILD_SPIRV_DOCS
#define PROFILER_BUILD_SPIRV_DOCS 0
#endif

#if PROFILER_BUILD_SPIRV_DOCS
#include <spv_docs.h>
#endif

namespace
{
    struct Source
    {
        uint32_t            m_FilenameStringID;
        SpvSourceLanguage   m_Language;
        uint32_t            m_LanguageVersion;
        std::string         m_Data;
    };

    struct SpirvParseOutputData
    {
        std::vector<uint32_t> m_Bytecode;
        std::vector<Source> m_Sources;
        std::unordered_map<uint32_t, const char*> m_pStrings;
    };

    struct SpirvCrossUserData
    {
        Profiler::OverlayShaderView* m_pShaderView;
        const char*         m_pTabName;
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
        ParseSpirvHeader

    Description:
        Parses the SPIR-V header and writes it to the output.

    \***********************************************************************************/
    static spv_result_t ParseSpirvHeader( void* pUserData,
        spv_endianness_t endian,
        uint32_t magic,
        uint32_t version,
        uint32_t generator,
        uint32_t id_bound,
        uint32_t reserved )
    {
        SpirvParseOutputData* pOutput = static_cast<SpirvParseOutputData*>( pUserData );
        assert( pOutput );

        if( !pOutput )
        {
            return SPV_ERROR_INVALID_POINTER;
        }

        // SPIR-V is a sequence of 32-bit words, so the endianess is not important as long as it is consistent.
        // Store the header, and the following instructions, in the native endian.
        (void)endian;
        pOutput->m_Bytecode.push_back( magic );
        pOutput->m_Bytecode.push_back( version );
        pOutput->m_Bytecode.push_back( generator );
        pOutput->m_Bytecode.push_back( id_bound );
        pOutput->m_Bytecode.push_back( reserved );

        return SPV_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        ParseSpirvInstruction

    Description:
        Reads all OpSource instructions and returns a list of shader sources associated
        with the SPIR-V binary.

    \***********************************************************************************/
    static spv_result_t ParseSpirvInstruction( void* pUserData, const spv_parsed_instruction_t* pInstruction )
    {
        SpirvParseOutputData* pOutput = static_cast<SpirvParseOutputData*>( pUserData );
        assert( pOutput );

        if( !pOutput )
        {
            return SPV_ERROR_INVALID_POINTER;
        }

        bool insertInstruction = true;

        // OpStrings define paths to the embedded sources.
        if( pInstruction->opcode == SpvOpString )
        {
            const uint32_t id = GetSpirvOperand<uint32_t>( pInstruction, 0 );
            const char* pString = GetSpirvOperand<const char*>( pInstruction, 1 );
            if( pString )
            {
                pOutput->m_pStrings[id] = pString;
            }
        }

        // OpSources may contain embedded sources.
        else if( pInstruction->opcode == SpvOpSource )
        {
            Source& source = pOutput->m_Sources.emplace_back();
            source.m_Language = GetSpirvOperand<SpvSourceLanguage>( pInstruction, 0 );
            source.m_LanguageVersion = GetSpirvOperand<uint32_t>( pInstruction, 1 );
            source.m_FilenameStringID = GetSpirvOperand<uint32_t>( pInstruction, 2 );

            const char* pData = GetSpirvOperand<const char*>( pInstruction, 3 );
            if( pData )
            {
                source.m_Data.assign( pData );
            }

            insertInstruction = false;
        }

        // Continuation of the last OpSource/OpSourceContinued.
        else if( pInstruction->opcode == SpvOpSourceContinued )
        {
            Source& source = pOutput->m_Sources.back();
            source.m_Data.append( GetSpirvOperand<const char*>( pInstruction, 0 ) );
            insertInstruction = false;
        }

        if( insertInstruction )
        {
            pOutput->m_Bytecode.insert(
                pOutput->m_Bytecode.end(),
                pInstruction->words,
                pInstruction->words + pInstruction->num_words );
        }

        return SPV_SUCCESS;
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
        GetShaderFileExtension

    Description:
        Returns an extension associated with the shader format.

    \***********************************************************************************/
    static constexpr const char* GetShaderFileExtension( Profiler::ShaderFormat shaderFormat )
    {
        switch( shaderFormat )
        {
        default:
        case Profiler::ShaderFormat::eText:
            return ".txt";
        case Profiler::ShaderFormat::eBinary:
            return ".bin";
        case Profiler::ShaderFormat::eSpirv:
            return ".spvasm";
        case Profiler::ShaderFormat::eGlsl:
            return ".glsl";
        case Profiler::ShaderFormat::eHlsl:
            return ".hlsl";
        case Profiler::ShaderFormat::eCpp:
            return ".cpp";
        }
    }

    /***********************************************************************************\

    Function:
        GetShaderLanguageName

    Description:
        Returns a name of the shader language based on the format.

    \***********************************************************************************/
    static constexpr const char* GetShaderLanguageName( Profiler::ShaderFormat shaderFormat )
    {
        switch( shaderFormat )
        {
        default:
        case Profiler::ShaderFormat::eText:
            return "Text";
        case Profiler::ShaderFormat::eBinary:
            return "Binary";
        case Profiler::ShaderFormat::eSpirv:
            return "SPIR-V";
        case Profiler::ShaderFormat::eGlsl:
            return "GLSL";
        case Profiler::ShaderFormat::eHlsl:
            return "HLSL";
        case Profiler::ShaderFormat::eCpp:
            return "C++";
        }
    }

    /***********************************************************************************\

    Function:
        GetSpirvCrossBackend

    Description:
        Returns a SPIR-V Cross backend based on the source language.

    \***********************************************************************************/
    static constexpr spvc_backend GetSpirvCrossBackend( SpvSourceLanguage language )
    {
        switch (language)
        {
        default:
        case SpvSourceLanguageESSL:
        case SpvSourceLanguageGLSL:
            return SPVC_BACKEND_GLSL;

        case SpvSourceLanguageHLSL:
            return SPVC_BACKEND_HLSL;

        case SpvSourceLanguageOpenCL_C:
        case SpvSourceLanguageOpenCL_CPP:
        case SpvSourceLanguageCPP_for_OpenCL:
            return SPVC_BACKEND_CPP;
        }
    }

    /***********************************************************************************\

    Function:
        NormalizeShaderFileName

    Description:
        Converts all whitespaces in the input string to underscores in the output string.

    \***********************************************************************************/
    static std::string NormalizeShaderFileName( std::string nameComponent )
    {
        std::replace_if( nameComponent.begin(), nameComponent.end(), isspace, '_' );
        return nameComponent;
    }

    /***********************************************************************************\

    Function:
        GetSpirvLanguageDefinition

    Description:
        Returns a reference to the spirv language definition for syntax highlighting.

    \***********************************************************************************/
    static TextEditor::LanguageDefinition GetSpirvLanguageDefinition(
        [[maybe_unused]] const Profiler::OverlayResources& resources,
        [[maybe_unused]] bool& showSpirvDocs )
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

#if PROFILER_BUILD_SPIRV_DOCS
        // Documentation.
        languageDefinition.mTooltip = [&resources, &showSpirvDocs]( const char* identifier )
            {
                if( !showSpirvDocs )
                {
                    return;
                }

                // Find the instruction desc.
                const Profiler::SpirvOpCodeDesc* pDesc = nullptr;
                for( const Profiler::SpirvOpCodeDesc& spvOp : Profiler::SpirvOps )
                {
                    if( !strcmp( spvOp.m_pName, identifier ) )
                    {
                        pDesc = &spvOp;
                        break;
                    }
                }

                // Show the tooltip.
                if( pDesc )
                {
                    ImGui::SetNextWindowSizeConstraints( { 600, 0 }, { FLT_MAX, FLT_MAX } );
                    ImGui::BeginTooltip();

                    // Opcode name.
                    ImGui::PushFont( resources.GetBoldFont() );
                    ImGui::TextUnformatted( pDesc->m_pName );
                    ImGui::SetCursorPosY( ImGui::GetCursorPosY() + 5 );
                    ImGui::PopFont();

                    // Description.
                    const char* pDoc = pDesc->m_pDoc;
                    if( !pDoc || !*pDoc )
                    {
                        pDoc = "No documentation available for this instruction.";
                    }

                    ImGui::PushFont( resources.GetDefaultFont() );
                    ImGui::TextWrapped( "%s", pDoc );

                    // Operands.
                    if( pDesc->m_OperandsCount )
                    {
                        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + 10 );

                        ImGui::PushStyleColor( ImGuiCol_TableRowBg, { 1.0f, 1.0f, 1.0f, 0.025f } );
                        ImGui::BeginTable( "##SpvTooltipTable", pDesc->m_OperandsCount, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg );
                        for( uint32_t i = 0; i < pDesc->m_OperandsCount; ++i )
                        {
                            ImGui::TableNextColumn();
                            ImGui::TextUnformatted( pDesc->m_pOperands[ i ] );
                        }
                        ImGui::EndTable();
                        ImGui::PopStyleColor();
                    }

                    // Documentation source.
                    ImGui::PushStyleColor( ImGuiCol_Text, { 0.4f, 0.4f, 0.4f, 1.0f } );
                    ImGui::SetCursorPosY( ImGui::GetCursorPosY() + 10 );
                    ImGui::TextUnformatted( "From: " PROFILER_SPIRV_DOCS_URL );
                    ImGui::PopStyleColor();

                    ImGui::PopFont();
                    ImGui::EndTooltip();
                }
            };
#endif // PROFILER_BUILD_SPIRV_DOCS

        return languageDefinition;
    }
}

namespace Profiler
{
    struct OverlayShaderView::ShaderRepresentation
    {
        const char*              m_pName;
        const void*              m_pData;
        size_t                   m_DataSize;
        ShaderFormat             m_Format;
    };

    struct OverlayShaderView::ShaderExecutableRepresentation
        : OverlayShaderView::ShaderRepresentation
    {
        ProfilerShaderExecutable m_Executable;
        uint32_t                 m_InternalRepresentationIndex;
    };

    struct OverlayShaderView::ShaderExporter
    {
        IGFD::FileDialog       m_FileDialog;
        IGFD::FileDialogConfig m_FileDialogConfig;
        ShaderRepresentation*  m_pShaderRepresentation;
        ShaderFormat           m_ShaderFormat;
    };

    /***********************************************************************************\

    Function:
        OverlayShaderView

    Description:
        Constructor.

    \***********************************************************************************/
    OverlayShaderView::OverlayShaderView( const OverlayResources& resources )
        : m_Resources( resources )
        , m_pTextEditor( nullptr )
        , m_ShaderName( "shader" )
        , m_EntryPointName( "main" )
        , m_pShaderRepresentations( 0 )
        , m_SpvTargetEnv( SPV_ENV_UNIVERSAL_1_0 )
        , m_ShowSpirvDocs( PROFILER_BUILD_SPIRV_DOCS )
        , m_CurrentTabIndex( -1 )
        , m_DefaultWindowBgColor( 0 )
        , m_DefaultTitleBgColor( 0 )
        , m_DefaultTitleBgActiveColor( 0 )
        , m_pShaderExporter( nullptr )
        , m_ShaderSavedCallback( nullptr )
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
        InitializeStyles

    Description:
        Configures the shader viewer for the current ImGui style.

    \***********************************************************************************/
    void OverlayShaderView::InitializeStyles()
    {
        m_DefaultWindowBgColor = ImGui::GetColorU32( ImGuiCol_WindowBg );
        m_DefaultTitleBgColor = ImGui::GetColorU32( ImGuiCol_TitleBg );
        m_DefaultTitleBgActiveColor = ImGui::GetColorU32( ImGuiCol_TitleBgActive );
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
        SetShaderName

    Description:
        Sets the currently displayed shader name.
        The name is used to construct file names when saving the representations to file.

    \***********************************************************************************/
    void OverlayShaderView::SetShaderName( const std::string& name )
    {
        m_ShaderName = name;
    }

    /***********************************************************************************\

    Function:
        SetEntryPointName

    Description:
        Sets the entry point name used by the shader module.
        Required to correctly disassemble the SPIR-V binary into GLSL/HLSL.

    \***********************************************************************************/
    void OverlayShaderView::SetEntryPointName( const std::string& name )
    {
        m_EntryPointName = name;
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
            if( pShaderRepresentation->m_Format == m_scExecutableShaderFormat )
            {
                // Free the shader executable reference.
                ShaderExecutableRepresentation* pShaderExecutable = static_cast<ShaderExecutableRepresentation*>(pShaderRepresentation);
                pShaderExecutable->~ShaderExecutableRepresentation();
            }

            free( pShaderRepresentation );
        }

        m_ShaderName = "shader";
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

        // Parse the SPIR-V binary first and remove any instructions that will become redundant.
        SpirvParseOutputData parsedSpirv;
        spv_result_t result = spvBinaryParse( context, &parsedSpirv, pBinary, wordCount, ParseSpirvHeader, ParseSpirvInstruction, nullptr );

        if( result == SPV_SUCCESS )
        {
            pBinary = parsedSpirv.m_Bytecode.data();
            wordCount = parsedSpirv.m_Bytecode.size();
        }

        // Disassembler options.
        uint32_t options =
            SPV_BINARY_TO_TEXT_OPTION_INDENT |
            SPV_BINARY_TO_TEXT_OPTION_COMMENT |
            SPV_BINARY_TO_TEXT_OPTION_FRIENDLY_NAMES;

        // Disassemble the binary.
        result = spvBinaryToText( context, pBinary, wordCount, options, &text, nullptr );

        if( result == SPV_SUCCESS )
        {
            AddShaderRepresentation( "Disassembly", text->str, text->length, ShaderFormat::eSpirv );

            bool hasValidSources = false;

            for( Source& source : parsedSpirv.m_Sources )
            {
                // Skip sources with no embedded shader code.
                if( source.m_Data.empty() )
                {
                    continue;
                }

                // Extract filename of the embedded source.
                const char* pFilename = parsedSpirv.m_pStrings[ source.m_FilenameStringID ];
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

                AddShaderRepresentation( pBasename ? pBasename + 1 : pFilename, source.m_Data.c_str(), source.m_Data.length() + 1,
                    GetSpirvSourceShaderFormat( source.m_Language ) );

                hasValidSources = true;
            }

            // Disassemble the binary to a source using the SPIR-V Cross if sources are not available.
            if( !hasValidSources )
            {
                SpvSourceLanguage sourceLanguage = SpvSourceLanguageUnknown;
                uint32_t sourceLanguageVersion = 0;

                if( !parsedSpirv.m_Sources.empty() )
                {
                    const Source& source = parsedSpirv.m_Sources.front();
                    sourceLanguage = source.m_Language;
                    sourceLanguageVersion = source.m_LanguageVersion;

                    // Check if all sources of the shader module use the same language.
                    for( const Source& src : parsedSpirv.m_Sources )
                    {
                        if( src.m_Language != sourceLanguage )
                        {
                            sourceLanguage = SpvSourceLanguageUnknown;
                            sourceLanguageVersion = 0;
                            break;
                        }
                    }
                }

                if( sourceLanguage == SpvSourceLanguageUnknown )
                {
                    // Use the default language for the SPIR-V Cross.
                    sourceLanguage = SpvSourceLanguageGLSL;
                    sourceLanguageVersion = 460;
                }

                Profiler::ShaderFormat shaderFormat = GetSpirvSourceShaderFormat( sourceLanguage );

                std::string tabName;
                tabName += GetShaderLanguageName( shaderFormat );
                tabName += " (SPIR-V Cross)";

                // Create a SPIR-V Cross context.
                spvc_context spvcContext = nullptr;
                spvc_result spvcResult = spvc_context_create( &spvcContext );

                if( spvcResult == SPVC_SUCCESS )
                {
                    SpirvCrossUserData userData = {};
                    userData.m_pShaderView = this;
                    userData.m_pTabName = tabName.c_str();

                    // Catch errors reported by the SPIR-V Cross and display them in the shader view.
                    spvc_context_set_error_callback(
                        spvcContext, []( void* pUserData, const char* message ) {
                            SpirvCrossUserData* pData = static_cast<SpirvCrossUserData*>( pUserData );
                            std::string errorMessage = "SPIR-V Cross error:\n";
                            errorMessage.append( message );
                            pData->m_pShaderView->AddShaderRepresentation(
                                pData->m_pTabName, errorMessage.c_str(), errorMessage.length(), ShaderFormat::eText );
                        },
                        &userData );

                    // Parse the SPIR-V binary into an intermediate representation.
                    spvc_parsed_ir spvcParsedIR = nullptr;
                    spvcResult = spvc_context_parse_spirv( spvcContext, pBinary, wordCount, &spvcParsedIR );

                    if( spvcResult == SPVC_SUCCESS )
                    {
                        spvc_backend spvcCompilerBackend = GetSpirvCrossBackend( sourceLanguage );

                        // Create a compiler for the parsed SPIR-V binary.
                        spvc_compiler spvcCompiler = nullptr;
                        spvcResult = spvc_context_create_compiler(
                            spvcContext,
                            spvcCompilerBackend,
                            spvcParsedIR,
                            SPVC_CAPTURE_MODE_TAKE_OWNERSHIP,
                            &spvcCompiler );

                        if( spvcResult == SPVC_SUCCESS )
                        {
                            // Find the entry point used in the viewed shader module.
                            size_t spvcEntryPointCount = 0;
                            const spvc_entry_point* pSpvcEntryPoints = nullptr;
                            spvc_compiler_get_entry_points( spvcCompiler, &pSpvcEntryPoints, &spvcEntryPointCount );

                            size_t entryPointIndex = 0;
                            for( size_t i = 0; i < spvcEntryPointCount; ++i )
                            {
                                if( strcmp( pSpvcEntryPoints[ i ].name, m_EntryPointName.c_str() ) == 0 )
                                {
                                    entryPointIndex = i;
                                    break;
                                }
                            }

                            spvc_compiler_set_entry_point(
                                spvcCompiler, pSpvcEntryPoints[ entryPointIndex ].name, pSpvcEntryPoints[ entryPointIndex ].execution_model );

                            // Configure the compiler.
                            spvc_compiler_options options = nullptr;
                            spvc_compiler_create_compiler_options( spvcCompiler, &options );

                            switch( sourceLanguage )
                            {
                            case SpvSourceLanguageESSL:
                                spvc_compiler_options_set_bool( options, SPVC_COMPILER_OPTION_GLSL_ES, true );

                            case SpvSourceLanguageGLSL:
                                spvc_compiler_options_set_uint( options, SPVC_COMPILER_OPTION_GLSL_VERSION, sourceLanguageVersion );
                                spvc_compiler_options_set_bool( options, SPVC_COMPILER_OPTION_GLSL_VULKAN_SEMANTICS, true );
                                break;

                            case SpvSourceLanguageHLSL:
                                spvc_compiler_options_set_uint( options, SPVC_COMPILER_OPTION_HLSL_SHADER_MODEL, sourceLanguageVersion );
                                break;
                            }

                            spvc_compiler_install_compiler_options( spvcCompiler, options );

                            // Compile the SPIR-V binary.
                            const char* pSource = nullptr;
                            spvcResult = spvc_compiler_compile( spvcCompiler, &pSource );

                            if( spvcResult == SPVC_SUCCESS )
                            {
                                AddShaderRepresentation(
                                    tabName.c_str(), pSource, strlen( pSource ), shaderFormat );
                            }
                        }
                    }

                    spvc_context_destroy( spvcContext );
                }
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

        char* pShaderRepresentationName = reinterpret_cast<char*>( pShaderRepresentation + 1 );
        ProfilerStringFunctions::CopyString( pShaderRepresentationName, nameSize, pName, nameSize );
        pShaderRepresentation->m_pName = pShaderRepresentationName;

        if( pData && dataSize > 0 )
        {
            // Save shader representation data.
            void* pShaderRepresentationData = pShaderRepresentationName + nameSize;
            memcpy( pShaderRepresentationData, pData, dataSize );
            pShaderRepresentation->m_pData = pShaderRepresentationData;
            pShaderRepresentation->m_DataSize = dataSize;
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
        AddShaderExecutable

    Description:
        Adds a tab with the shader executable.

    \***********************************************************************************/
    void OverlayShaderView::AddShaderExecutable( const ProfilerShaderExecutable& executable )
    {
        constexpr size_t shaderRepresentationSize = sizeof( ShaderExecutableRepresentation );

        // Allocate the shader representation.
        ShaderExecutableRepresentation* pShaderRepresentation = static_cast<ShaderExecutableRepresentation*>(malloc( shaderRepresentationSize ));
        if( !pShaderRepresentation )
        {
            return;
        }

        // Call the constructor so that any non-primitive members are initialized.
        new (pShaderRepresentation) ShaderExecutableRepresentation();
        pShaderRepresentation->m_pName = executable.GetName();
        pShaderRepresentation->m_pData = nullptr;
        pShaderRepresentation->m_DataSize = 0;
        pShaderRepresentation->m_Format = m_scExecutableShaderFormat;
        pShaderRepresentation->m_Executable = executable;
        pShaderRepresentation->m_InternalRepresentationIndex = UINT32_MAX;

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
        ImGui::PushFont( m_Resources.GetDefaultFont() );
        ImGui::PushStyleVar( ImGuiStyleVar_TabRounding, 0.0f );

        [[maybe_unused]]
        ImVec2 cp = ImGui::GetCursorPos();

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

        UpdateShaderExporter();

        ImGui::PopStyleVar();
        ImGui::PopFont();
    }

    /***********************************************************************************\

    Function:
        DrawShaderRepresentation

    Description:
        Draws a tab with the shader representation.

    \***********************************************************************************/
    void OverlayShaderView::DrawShaderRepresentation( int tabIndex, ShaderRepresentation* pShaderRepresentation )
    {
        if( ImGui::BeginTabItem( pShaderRepresentation->m_pName ) )
        {
            ShaderFormat shaderRepresentationFormat = pShaderRepresentation->m_Format;

            bool tabChanged = (m_CurrentTabIndex != tabIndex);
            m_CurrentTabIndex = tabIndex;

            // Print shader executable statistics and select the internal representation.
            if( shaderRepresentationFormat == m_scExecutableShaderFormat )
            {
                ShaderExecutableRepresentation* pShaderExecutable = static_cast<ShaderExecutableRepresentation*>(pShaderRepresentation);
                DrawShaderStatistics( pShaderExecutable );

                if( SelectShaderInternalRepresentation( pShaderExecutable, &shaderRepresentationFormat ) )
                {
                    tabChanged = true;
                }
            }

            // Early out if shader representation data is not available.
            if( !pShaderRepresentation->m_pData )
            {
                ImGui::PushStyleColor( ImGuiCol_Text, IM_COL32( 128, 128, 128, 255 ) );
                ImGui::TextUnformatted( "Shader representation data not available." );
                ImGui::PopStyleColor();
                ImGui::EndTabItem();
                return;
            }

            // Update the text editor with the current tab.
            if( tabChanged )
            {
                if( shaderRepresentationFormat != ShaderFormat::eBinary )
                {
                    const char* pText = reinterpret_cast<const char*>(pShaderRepresentation->m_pData);

                    // TODO: This creates a temporary copy of the shader representation.
                    m_pTextEditor->SetText( std::string( pText, pText + pShaderRepresentation->m_DataSize ) );

                    // Set syntax highlighting according to the format.
                    switch( shaderRepresentationFormat )
                    {
                    case ShaderFormat::eSpirv:
                        m_pTextEditor->SetLanguageDefinition( GetSpirvLanguageDefinition( m_Resources, m_ShowSpirvDocs ) );
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
            }

            // Draw a toolbar with options.
            if( ImGui::Button( "Save" ) )
            {
                m_pShaderExporter = std::make_unique<ShaderExporter>();
                m_pShaderExporter->m_pShaderRepresentation = pShaderRepresentation;
                m_pShaderExporter->m_ShaderFormat = shaderRepresentationFormat;
            }
            if( ImGui::IsItemHovered() )
            {
                ImGui::SetTooltip( "Save current shader representation to file" );
            }
#if PROFILER_BUILD_SPIRV_DOCS
            if( shaderRepresentationFormat == ShaderFormat::eSpirv )
            {
                // Allow the user to disable the tooltips with documentation.
                ImGui::SameLine();
                ImGui::Checkbox( "Show SPIR-V documentation", &m_ShowSpirvDocs );
            }
#endif

            // Print shader representation data.
            ImGui::PushFont( m_Resources.GetCodeFont() );

            m_pTextEditor->Render( "##ShaderRepresentationTextEdit" );

            ImGui::PopFont();
            ImGui::EndTabItem();
        }
    }

    /***********************************************************************************\

    Function:
        DrawShaderStatistics

    Description:
        Draws a table with shader executable statistics.

    \***********************************************************************************/
    void OverlayShaderView::DrawShaderStatistics( ShaderExecutableRepresentation* pShaderExecutable )
    {
        ImGui::PushStyleColor( ImGuiCol_Header, IM_COL32( 40, 40, 43, 128 ) );

        if( ImGui::CollapsingHeader( "Shader executable properties", ImGuiTreeNodeFlags_DefaultOpen ) )
        {
            ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 20, 1 ) );
            if( ImGui::BeginTable( "###ShaderExecutableStatisticsTable", 2, ImGuiTableFlags_SizingFixedFit ) )
            {
                const uint32_t shaderStatisticsCount = pShaderExecutable->m_Executable.GetStatisticsCount();
                for( uint32_t i = 0; i < shaderStatisticsCount; ++i )
                {
                    ProfilerShaderStatistic statistic = pShaderExecutable->m_Executable.GetStatistic( i );
                    ImGui::TableNextRow();

                    // Statistic name column.
                    if( ImGui::TableNextColumn() )
                    {
                        ImGui::TextUnformatted( statistic.m_pName );

                        // Show a tooltip with a description of the statistic.
                        if( ImGui::IsItemHovered() && statistic.m_pDescription )
                        {
                            ImGui::SetTooltip( "%s", statistic.m_pDescription );
                        }
                    }

                    // Statistic value column.
                    if( ImGui::TableNextColumn() )
                    {
                        switch( statistic.m_Format )
                        {
                        case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_BOOL32_KHR:
                            ImGui::Text( "%s", statistic.m_Value.b32 ? "True" : "False" );
                            break;
                        case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_INT64_KHR:
                            ImGui::Text( "%" PRId64, statistic.m_Value.i64);
                            break;
                        case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR:
                            ImGui::Text( "%" PRIu64, statistic.m_Value.u64 );
                            break;
                        case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_FLOAT64_KHR:
                            ImGui::Text( "%lf", statistic.m_Value.f64 );
                            break;
                        }
                    }
                }
                ImGui::EndTable();
            }
            ImGui::PopStyleVar();
        }
        ImGui::PopStyleColor();
    }

    /***********************************************************************************\

    Function:
        SelectShaderInternalRepresentation

    Description:
        Draws a combo box for selecting inspected shader internal representation.

    \***********************************************************************************/
    bool OverlayShaderView::SelectShaderInternalRepresentation( ShaderExecutableRepresentation* pShaderExecutable, ShaderFormat* pShaderFormat )
    {
        bool internalRepresentationIndexChanged = false;

        // Overrides data pointer in the pShaderExecutable with the selected internalRepresentation.
        auto SelectInternalRepresentation = [&]( const ProfilerShaderInternalRepresentation& internalRepresentation )
            {
                internalRepresentationIndexChanged = true;
                pShaderExecutable->m_pData = internalRepresentation.m_pData;
                pShaderExecutable->m_DataSize = internalRepresentation.m_DataSize;
                *pShaderFormat = (internalRepresentation.m_IsText ? ShaderFormat::eText : ShaderFormat::eBinary);
            };

        // Disable the combo box if there are no available internal representations.
        const uint32_t shaderInternalRepresentationsCount = pShaderExecutable->m_Executable.GetInternalRepresentationsCount();
        ImGui::BeginDisabled( shaderInternalRepresentationsCount == 0 );

        const char* pComboBoxText = "Shader internal representations";
        if( shaderInternalRepresentationsCount > 0 )
        {
            // Initialize the index to the first available internal representation on the first call to this function.
            bool initializeInternalRepresentationData = false;
            if( pShaderExecutable->m_InternalRepresentationIndex == UINT32_MAX )
            {
                pShaderExecutable->m_InternalRepresentationIndex = 0;
                initializeInternalRepresentationData = true;
            }

            ProfilerShaderInternalRepresentation internalRepresentation =
                pShaderExecutable->m_Executable.GetInternalRepresentation( pShaderExecutable->m_InternalRepresentationIndex );

            if( initializeInternalRepresentationData )
            {
                SelectInternalRepresentation( internalRepresentation );
            }

            // Display currently selected representation name in the combo box preview.
            pComboBoxText = internalRepresentation.m_pName;
        }

        ImGui::PushItemWidth( -1 );
        ImGui::PushStyleColor( ImGuiCol_FrameBg, IM_COL32( 40, 40, 43, 128 ) );
        ImGui::PushStyleColor( ImGuiCol_Button, IM_COL32( 40, 40, 43, 255 ) );

        if( ImGui::BeginCombo( "###ShaderInternalRepresentationComboBox", pComboBoxText, ImGuiComboFlags_PopupAlignLeft ) )
        {
            // Combo box expanded, list all available internal representations.
            for( uint32_t index = 0; index < shaderInternalRepresentationsCount; ++index )
            {
                ProfilerShaderInternalRepresentation internalRepresentation =
                    pShaderExecutable->m_Executable.GetInternalRepresentation( index );

                if( ImGuiX::TSelectable( internalRepresentation.m_pName, pShaderExecutable->m_InternalRepresentationIndex, index ) )
                {
                    // User has selected a new internal representation.
                    SelectInternalRepresentation( internalRepresentation );
                }
            }

            ImGui::EndCombo();
        }

        ImGui::PopStyleColor( 2 );
        ImGui::EndDisabled();

        return internalRepresentationIndexChanged;
    }

    /***********************************************************************************\

    Function:
        SetShaderSavedCallback

    Description:
        Sets the function called when a shader is saved.

    \***********************************************************************************/
    void OverlayShaderView::SetShaderSavedCallback( ShaderSavedCallback callback )
    {
        m_ShaderSavedCallback = callback;
    }

    /***********************************************************************************\

    Function:
        UpdateShaderExporter

    Description:
        Saves the shader representation to a file.

    \***********************************************************************************/
    void OverlayShaderView::UpdateShaderExporter()
    {
        static const std::string scFileDialogId = "#ShaderSaveFileDialog";

        // Early-out if shader exporter is not available.
        if( m_pShaderExporter == nullptr )
        {
            return;
        }

        // Prevent the dialog from appearing transparent.
        const int numPushedColors = 3;
        ImGui::PushStyleColor( ImGuiCol_WindowBg, m_DefaultWindowBgColor );
        ImGui::PushStyleColor( ImGuiCol_TitleBg, m_DefaultTitleBgColor );
        ImGui::PushStyleColor( ImGuiCol_TitleBgActive, m_DefaultTitleBgActiveColor );

        if( !m_pShaderExporter->m_FileDialog.IsOpened() )
        {
            // Configure the file dialog.
            m_pShaderExporter->m_FileDialogConfig.fileName = GetDefaultShaderFileName(
                m_pShaderExporter->m_pShaderRepresentation,
                m_pShaderExporter->m_ShaderFormat );

            m_pShaderExporter->m_FileDialogConfig.flags = ImGuiFileDialogFlags_Default;

            // Set initial size and position of the dialog.
            ImGuiIO& io = ImGui::GetIO();
            ImVec2 size = io.DisplaySize;
            float scale = io.FontGlobalScale;
            size.x = std::min( size.x / 1.5f, 640.f * scale );
            size.y = std::min( size.y / 1.25f, 480.f * scale );
            ImGui::SetNextWindowSize( size );

            ImVec2 pos = io.DisplaySize;
            pos.x = (pos.x - size.x) / 2.0f;
            pos.y = (pos.y - size.y) / 2.0f;
            ImGui::SetNextWindowPos( pos );

            // Show the file dialog.
            m_pShaderExporter->m_FileDialog.OpenDialog(
                scFileDialogId,
                "Select shader save path",
                ".*",
                m_pShaderExporter->m_FileDialogConfig );
        }

        // Draw the file dialog until the user closes it.
        bool closed = m_pShaderExporter->m_FileDialog.Display(
            scFileDialogId,
            ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoSavedSettings );

        if( closed )
        {
            if( m_pShaderExporter->m_FileDialog.IsOk() )
            {
                SaveShaderToFile(
                    m_pShaderExporter->m_FileDialog.GetFilePathName(),
                    m_pShaderExporter->m_pShaderRepresentation,
                    m_pShaderExporter->m_ShaderFormat );
            }

            // Destroy the exporter.
            m_pShaderExporter.reset();
        }

        ImGui::PopStyleColor( numPushedColors );
    }

    /***********************************************************************************\

    Function:
        GetDefaultShaderFileName

    Description:
        Returns the default name of the file to write the shader to.

    \***********************************************************************************/
    std::string OverlayShaderView::GetDefaultShaderFileName( ShaderRepresentation* pShaderRepresentation, ShaderFormat shaderFormat ) const
    {
        std::stringstream stringBuilder;
        stringBuilder << ProfilerPlatformFunctions::GetProcessName() << "_";
        stringBuilder << ProfilerPlatformFunctions::GetCurrentProcessId() << "_";
        stringBuilder << m_ShaderName << "_";
        stringBuilder << pShaderRepresentation->m_pName;

        if( pShaderRepresentation->m_Format == m_scExecutableShaderFormat )
        {
            // Append internal representation name to the file name.
            ShaderExecutableRepresentation* pShaderExecutableRepresentation = static_cast<ShaderExecutableRepresentation*>(pShaderRepresentation);
            ProfilerShaderInternalRepresentation internalRepresentation = pShaderExecutableRepresentation->m_Executable.GetInternalRepresentation(
                pShaderExecutableRepresentation->m_InternalRepresentationIndex );

            stringBuilder << "_" << internalRepresentation.m_pName;
        }

        stringBuilder << GetShaderFileExtension( shaderFormat );
        return NormalizeShaderFileName( stringBuilder.str() );
    }

    /***********************************************************************************\

    Function:
        SaveShaderToFile

    Description:
        Saves the shader representation to a file.

    \***********************************************************************************/
    void OverlayShaderView::SaveShaderToFile( const std::string& path, ShaderRepresentation* pShaderRepresentation, ShaderFormat shaderFormat )
    {
        // Open file for writing.
        std::ios::openmode mode =
            std::ios::out |
            std::ios::trunc |
            ((shaderFormat == ShaderFormat::eBinary) ? std::ios::binary : std::ios::openmode());

        std::ofstream out( path );
        if( !out.is_open() )
        {
            if( m_ShaderSavedCallback )
            {
                m_ShaderSavedCallback(
                    false,
                    "Failed to open file for writing.\n" + path );
            }
            return;
        }

        // Write the shader.
        out.write( static_cast<const char*>(pShaderRepresentation->m_pData), pShaderRepresentation->m_DataSize );
        out.close();

        // Notify the listener.
        if( m_ShaderSavedCallback )
        {
            m_ShaderSavedCallback(
                true,
                "Shader written to:\n" + path );
        }
    }
}

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

#include <spirv-tools/libspirv.h>

#include <imgui.h>
#include <TextEditor.h>

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
            AddShaderRepresentation( "Disassembly", text->str, text->length, true );
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
    void OverlayShaderView::AddShaderRepresentation( const char* pName, const void* pData, size_t dataSize, bool isText )
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

        pShaderRepresentation->m_IsText = isText;

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
                if( pShaderRepresentation->m_IsText )
                {
                    const char* pText = reinterpret_cast<const char*>(pShaderRepresentation->m_pData);

                    // TODO: This creates a temporary copy of the shader representation.
                    m_pTextEditor->SetText( std::string( pText, pText + pShaderRepresentation->m_DataSize ) );
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

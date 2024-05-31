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

#include <imgui.h>
#include <TextEditor.h>

namespace Profiler
{
    OverlayShaderView::OverlayShaderView( const OverlayFonts& fonts )
        : m_Fonts( fonts )
        , m_pTextEditor( nullptr )
        , m_pShaderRepresentations( 0 )
    {
        m_pTextEditor = std::make_unique<TextEditor>();
        m_pTextEditor->SetReadOnly( true );
    }

    OverlayShaderView::~OverlayShaderView()
    {
        for( ShaderRepresentation* pShaderRepresentation : m_pShaderRepresentations )
        {
            free( pShaderRepresentation );
        }
    }

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

    void OverlayShaderView::Draw()
    {
        ImGui::PushFont( m_Fonts.GetDefaultFont() );

        if( ImGui::BeginTabBar( "ShaderRepresentations", ImGuiTabBarFlags_None ) )
        {
            // Draw shader representations in tabs.
            for( ShaderRepresentation* pShaderRepresentation : m_pShaderRepresentations )
            {
                DrawShaderRepresentation( pShaderRepresentation );
            }

            ImGui::EndTabBar();
        }

        ImGui::PopFont();
    }

    void OverlayShaderView::DrawShaderRepresentation( ShaderRepresentation* pShaderRepresentation )
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

            // Print shader representation data.
            ImGui::PushFont( m_Fonts.GetCodeFont() );



            ImGui::PopFont();
            ImGui::EndTabItem();
        }
    }
}

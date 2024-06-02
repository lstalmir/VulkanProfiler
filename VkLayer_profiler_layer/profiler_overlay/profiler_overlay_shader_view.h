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

#pragma once
#include <string>
#include <vector>
#include <memory>

// ImGuiColorTextEdit
class TextEditor;

namespace Profiler
{
    class OverlayFonts;

    /***********************************************************************************\

    Class:
        ShaderFormat

    Description:
        Format of the shader representation.

    \***********************************************************************************/
    enum class ShaderFormat
    {
        eText,
        eBinary,
        eSpirv,
        eGlsl,
        eHlsl,
        eCpp
    };

    /***********************************************************************************\

    Class:
        OverlayShaderView

    Description:
        Handles shader representations rendering.

    \***********************************************************************************/
    class OverlayShaderView
    {
    public:
        OverlayShaderView( const OverlayFonts& fonts );
        ~OverlayShaderView();

        OverlayShaderView( const OverlayShaderView& ) = delete;
        OverlayShaderView( OverlayShaderView&& ) = delete;

        void SetTargetDevice( struct VkDevice_Object* pDevice );

        void Clear();

        void AddBytecode( const uint32_t* pBinary, size_t wordCount );
        void AddShaderRepresentation( const char* pName, const void* pData, size_t dataSize, ShaderFormat format );

        void Draw();

    private:
        struct ShaderRepresentation
        {
            char*                          m_pName;
            void*                          m_pData;
            size_t                         m_DataSize;
            ShaderFormat                   m_Format;
        };

        const OverlayFonts&                m_Fonts;
        std::unique_ptr<TextEditor>        m_pTextEditor;

        std::vector<ShaderRepresentation*> m_pShaderRepresentations;

        int                                m_SpvTargetEnv;

        int                                m_CurrentTabIndex;

        void DrawShaderRepresentation( int tabIndex, ShaderRepresentation* pShaderRepresentation );
    };
}

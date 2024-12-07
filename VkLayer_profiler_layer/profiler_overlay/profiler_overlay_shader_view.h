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
#include <functional>

// ImGuiColorTextEdit
class TextEditor;
// ImGuiFileDialog
namespace IGFD { class FileDialog; }

namespace Profiler
{
    class OverlayResources;
    struct ProfilerShaderExecutable;

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
        OverlayShaderView( const OverlayResources& resources );
        ~OverlayShaderView();

        OverlayShaderView( const OverlayShaderView& ) = delete;
        OverlayShaderView( OverlayShaderView&& ) = delete;

        void InitializeStyles();
        void SetTargetDevice( struct VkDevice_Object* pDevice );
        void SetShaderName( const std::string& name );
        void SetEntryPointName( const std::string& name );
        void SetShaderIdentifier( uint32_t identifierSize, const uint8_t* pIdentifier );

        void Clear();

        void AddBytecode( const uint32_t* pBinary, size_t wordCount );
        void AddShaderRepresentation( const char* pName, const void* pData, size_t dataSize, ShaderFormat format );
        void AddShaderExecutable( const ProfilerShaderExecutable& executable );

        void Draw();

        typedef std::function<void( bool, const std::string& )> ShaderSavedCallback;
        void SetShaderSavedCallback( ShaderSavedCallback callback );

    private:
        struct ShaderRepresentation;
        struct ShaderExecutableRepresentation;
        struct ShaderExporter;

        static constexpr ShaderFormat      m_scExecutableShaderFormat = ShaderFormat( -1 );

        const OverlayResources&            m_Resources;
        std::unique_ptr<TextEditor>        m_pTextEditor;

        std::string                        m_ShaderName;
        std::string                        m_EntryPointName;
        std::string                        m_ShaderIdentifier;
        std::vector<ShaderRepresentation*> m_pShaderRepresentations;

        int                                m_SpvTargetEnv;
        bool                               m_ShowSpirvDocs;

        int                                m_CurrentTabIndex;

        uint32_t                           m_DefaultWindowBgColor;
        uint32_t                           m_DefaultTitleBgColor;
        uint32_t                           m_DefaultTitleBgActiveColor;

        std::unique_ptr<ShaderExporter>    m_pShaderExporter;
        ShaderSavedCallback                m_ShaderSavedCallback;

        void DrawShaderRepresentation( int tabIndex, ShaderRepresentation* pShaderRepresentation );
        void DrawShaderStatistics( ShaderExecutableRepresentation* pShaderExecutable );
        bool SelectShaderInternalRepresentation( ShaderExecutableRepresentation* pShaderExecutable, ShaderFormat* pShaderFormat );

        void UpdateShaderExporter();
        std::string GetDefaultShaderFileName( ShaderRepresentation* pShaderRepresentation, ShaderFormat shaderFormat ) const;
        void SaveShaderToFile( const std::string& path, ShaderRepresentation* pShaderRepresentation, ShaderFormat shaderFormat );
    };
}

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

#include "profiler_overlay_fonts.h"
#include "profiler/profiler_helpers.h"

#include <imgui.h>

#ifdef WIN32
#include <ShlObj.h> // SHGetKnownFolderPath
#endif

static constexpr const char* g_scDefaultFonts[] = {
#ifdef WIN32
    "segoeui.ttf",
    "tahoma.ttf",
#endif
#ifdef __linux__
    "Ubuntu-R.ttf",
    "LiberationSans-Regural.ttf",
#endif
    "DejaVuSans.ttf",
};

static constexpr const char* g_scCodeFonts[] = {
#ifdef WIN32
    "consolas.ttf",
    "cour.ttf",
#endif
#ifdef __linux__
    "UbuntuMono-R.ttf",
#endif
    "DejaVuSansMono.ttf",
};

namespace Profiler
{
    class OverlayFontFinder
    {
    public:
        OverlayFontFinder();

        std::filesystem::path GetFirstSupportedFont( const char* const* ppFonts, size_t fontCount ) const;

    private:
        std::vector<std::filesystem::path> m_FontSearchPaths;
    };

    /***********************************************************************************\

    Function:
        OverlayFontFinder

    Description:
        Constructor.

    \***********************************************************************************/
    OverlayFontFinder::OverlayFontFinder()
        : m_FontSearchPaths()
    {
#ifdef WIN32
        PWSTR pFontsDirectoryPath = nullptr;

        if( SUCCEEDED( SHGetKnownFolderPath( FOLDERID_Fonts, KF_FLAG_DEFAULT, nullptr, &pFontsDirectoryPath ) ) )
        {
            m_FontSearchPaths.push_back( pFontsDirectoryPath );
            CoTaskMemFree( pFontsDirectoryPath );
        }
#endif // WIN32
#ifdef __linux__
        // Linux distros use multiple font directories (or X server, TODO)
        m_FontSearchPaths = {
            "/usr/share/fonts",
            "/usr/local/share/fonts",
            "~/.fonts"
        };

        // Some systems may have these directories specified in conf file
        // https://stackoverflow.com/questions/3954223/platform-independent-way-to-get-font-directory
        const char* fontConfigurationFiles[] = {
            "/etc/fonts/fonts.conf",
            "/etc/fonts/local.conf"
        };

        std::vector<std::filesystem::path> configurationDirectories = {};

        for( const char* fontConfigurationFile : fontConfigurationFiles )
        {
            if( std::filesystem::exists( fontConfigurationFile ) )
            {
                // Try to open configuration file for reading
                std::ifstream conf( fontConfigurationFile );

                if( conf.is_open() )
                {
                    std::string line;

                    // conf is XML file, read line by line and find <dir> tag
                    while( std::getline( conf, line ) )
                    {
                        const size_t dirTagOpen = line.find( "<dir>" );
                        const size_t dirTagClose = line.find( "</dir>" );

                        // TODO: tags can be in different lines
                        if( ( dirTagOpen != std::string::npos ) && ( dirTagClose != std::string::npos ) )
                        {
                            configurationDirectories.push_back( line.substr( dirTagOpen + 5, dirTagClose - dirTagOpen - 5 ) );
                        }
                    }
                }
            }
        }

        if( !configurationDirectories.empty() )
        {
            // Override predefined font directories
            m_FontSearchPaths = configurationDirectories;
        }
#endif // __linux__
    }

    /***********************************************************************************\

    Function:
        GetFirstSupportedFont

    Description:
        Returns the first supported font from the list.

    \***********************************************************************************/
    std::filesystem::path OverlayFontFinder::GetFirstSupportedFont( const char* const* ppFonts, size_t fontCount ) const
    {
        for( size_t i = 0; i < fontCount; ++i )
        {
            const char* font = ppFonts[i];

            for( const std::filesystem::path& dir : m_FontSearchPaths )
            {
                // Fast path
                std::filesystem::path path = dir / font;
                if( std::filesystem::exists( path ) )
                {
                    return path;
                }

#ifdef __linux__
                // Search recursively
                path = ProfilerPlatformFunctions::FindFile( dir, font );
                if( !path.empty() )
                {
                    return path;
                }
#endif
            }
        }

        return std::filesystem::path();
    }

    /***********************************************************************************\

    Function:
        Initialize

    Description:
        Loads fonts for the overlay.

    \***********************************************************************************/
    void OverlayFonts::Initialize()
    {
        ImFontAtlas* fonts = ImGui::GetIO().Fonts;
        ImFont* defaultFont = fonts->AddFontDefault();
        m_pDefaultFont = defaultFont;
        m_pCodeFont = defaultFont;

        // Find font files
        OverlayFontFinder fontFinder;
        std::filesystem::path defaultFontPath = fontFinder.GetFirstSupportedFont( g_scDefaultFonts, std::size( g_scDefaultFonts ) );
        std::filesystem::path codeFontPath = fontFinder.GetFirstSupportedFont( g_scCodeFonts, std::size( g_scCodeFonts ) );

        // Include all glyphs in the font to support non-latin letters
        const ImWchar fontGlyphRange[] = { 0x20, 0xFFFF, 0 };

        if( !defaultFontPath.empty() )
        {
            m_pDefaultFont = fonts->AddFontFromFileTTF(
                defaultFontPath.string().c_str(), 16.0f, nullptr, fontGlyphRange );
        }

        if( !codeFontPath.empty() )
        {
            m_pCodeFont = fonts->AddFontFromFileTTF(
                codeFontPath.string().c_str(), 16.0f, nullptr, fontGlyphRange );
        }

        // Build atlas
        unsigned char* tex_pixels = NULL;
        int tex_w, tex_h;
        fonts->GetTexDataAsRGBA32( &tex_pixels, &tex_w, &tex_h );
    }

    /***********************************************************************************\

    Function:
        GetDefaultFont

    Description:
        Returns the default font.

    \***********************************************************************************/
    ImFont* OverlayFonts::GetDefaultFont() const
    {
        return m_pDefaultFont;
    }

    /***********************************************************************************\

    Function:
        GetCodeFont

    Description:
        Returns the code font.

    \***********************************************************************************/
    ImFont* OverlayFonts::GetCodeFont() const
    {
        return m_pCodeFont;
    }
}

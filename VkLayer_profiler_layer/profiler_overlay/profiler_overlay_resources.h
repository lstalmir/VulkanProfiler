// Copyright (c) 2024-2025 Lukasz Stalmirski
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
#include "profiler/profiler_helpers.h"
#include <stdint.h>

struct ImFont;

namespace Profiler
{
    class OverlayBackend;

    /***********************************************************************************\

    Class:
        OverlayIcon

    Description:
        Available overlay icons.

    \***********************************************************************************/
    enum class OverlayIcon
    {
        Copy,
        BookmarkEmpty,
        BookmarkFilled,
        IconCount
    };

    /***********************************************************************************\

    Class:
        OverlayResources

    Description:
        Manages the fonts and images used by the overlay.

    \***********************************************************************************/
    class OverlayResources
    {
    public:
        bool InitializeFonts();
        void Destroy();

        bool InitializeImages( OverlayBackend* pBackend );
        void DestroyImages();

        ImFont* GetDefaultFont() const;
        ImFont* GetBoldFont() const;
        ImFont* GetCodeFont() const;

        void* GetIcon( OverlayIcon icon ) const;

    private:
        OverlayBackend* m_pBackend = nullptr;

        ImFont* m_pDefaultFont = nullptr;
        ImFont* m_pBoldFont = nullptr;
        ImFont* m_pCodeFont = nullptr;

        EnumArray<OverlayIcon, void*, static_cast<size_t>( OverlayIcon::IconCount )>
            m_pIcons = { nullptr };

        void* CreateImage( const uint8_t* pAsset, int assetSize );

        template<size_t Size>
        void* CreateImage( const uint8_t ( &asset )[Size] )
        {
            return CreateImage( asset, static_cast<int>( Size ) );
        }
    };
}

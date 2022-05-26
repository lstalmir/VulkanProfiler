// Copyright (c) 2019-2021 Lukasz Stalmirski
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

#include "imgui_ex.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>

namespace
{
    ImU8 ComponentLerp( ImU8 a, ImU8 b, float s )
    {
        return (a * (1 - s)) + (b * s);
    }
}

namespace ImGuiX
{
    /***********************************************************************************\

    Function:
        TextAlignRight

    Description:
        Displays text in the next line, aligned to right.

    \***********************************************************************************/
    void TextAlignRight( float contentAreaWidth, const char* fmt, ... )
    {
        va_list args;
        va_start( args, fmt );

        char text[ 128 ];
        vsnprintf( text, sizeof( text ), fmt, args );

        va_end( args );

        uint32_t textSize = static_cast<uint32_t>(ImGui::CalcTextSize( text ).x);

        ImGui::SameLine( contentAreaWidth - textSize );
        ImGui::TextUnformatted( text );
    }

    /***********************************************************************************\

    Function:
        TextAlignRightSameLine

    Description:
        Displays text in the same line, aligned to right.

    \***********************************************************************************/
    void TextAlignRight( const char* fmt, ... )
    {
        va_list args;
        va_start( args, fmt );

        char text[ 128 ];
        vsnprintf( text, sizeof( text ), fmt, args );

        va_end( args );

        uint32_t textSize = static_cast<uint32_t>(ImGui::CalcTextSize( text ).x);

        ImGui::SameLine( ImGui::GetWindowContentRegionMax().x - textSize );
        ImGui::TextUnformatted( text );
    }

    /*************************************************************************\

    Function:
        ColorLerp

    Description:
        Interpolate colors.

    \*************************************************************************/
    ImU32 ColorLerp( ImU32 a, ImU32 b, float s )
    {
        const ImU8 o0 = ComponentLerp( (a >> 0) & 0xFF, (b >> 0) & 0xFF, s );
        const ImU8 o1 = ComponentLerp( (a >> 8) & 0xFF, (b >> 8) & 0xFF, s );
        const ImU8 o2 = ComponentLerp( (a >> 16) & 0xFF, (b >> 16) & 0xFF, s );
        const ImU8 o3 = ComponentLerp( (a >> 24) & 0xFF, (b >> 24) & 0xFF, s );
        return o0 | (o1 << 8) | (o2 << 16) | (o3 << 24);
    }

    /*************************************************************************\

    Function:
        Selectable

    Description:
        Selectable combo box item.

    \*************************************************************************/
    bool Selectable( const char* label, bool isSelected )
    {
        bool selectionChanged = false;

        if( ImGui::Selectable( label, isSelected ) )
        {
            // Selection changed.
            isSelected = true;
            selectionChanged = true;
        }

        if( isSelected )
        {
            ImGui::SetItemDefaultFocus();
        }

        return selectionChanged;
    }
}
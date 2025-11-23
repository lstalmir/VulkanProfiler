// Copyright (c) 2019-2025 Lukasz Stalmirski
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
#include <imgui.h>

namespace ImGuiX
{
    /*************************************************************************\

    Function:
        TextAlignRight

    Description:
        Print text aligned to the right.

    \*************************************************************************/
    void TextAlignRight( float contentAreaWidth, const char* fmt, ... );

    /*************************************************************************\

    Function:
        TextAlignRight

    Description:
        Print text aligned to the right.

    \*************************************************************************/
    void TextAlignRight( const char* fmt, ... );

    /*************************************************************************\

    Function:
        Badge

    Description:
        Print text with a color background.

    \*************************************************************************/
    void Badge( ImU32 color, float rounding, const char* fmt, ... );
    
    /*************************************************************************\

    Function:
        Badge

    Description:
        Print text with a color background.

    \*************************************************************************/
    void BadgeUnformatted( ImU32 color, float rounding, const char* text );

    /*************************************************************************\

    Function:
        ColorLerp

    Description:
        Interpolate colors.

    \*************************************************************************/
    ImU32 ColorLerp( ImU32, ImU32, float );

    /*************************************************************************\

    Function:
        ColorAlpha

    Description:
        Set color alpha.

    \*************************************************************************/
    ImU32 ColorAlpha( ImU32, float );

    /*************************************************************************\

    Function:
        Darker

    Description:
        Return darker color.

    \*************************************************************************/
    ImU32 Darker( ImU32, float = 0.75f );

    /*************************************************************************\

    Function:
        BeginSlimCombo

    Description:
        Compact combo box.

    \*************************************************************************/
    bool BeginSlimCombo( const char* label, const char* preview_value, ImGuiComboFlags flags = 0 );
    void EndSlimCombo();

    /*************************************************************************\

    Function:
        Selectable

    Description:
        Selectable combo box item.

    \*************************************************************************/
    bool Selectable( const char*, bool );

    /*************************************************************************\

    Function:
        TSelectable

    Description:
        Selectable combo box item.

    \*************************************************************************/
    template<typename T>
    bool TSelectable( const char* label, T& actual, const T& expected )
    {
        if( Selectable( label, (actual == expected) ) )
        {
            actual = expected;
            return true;
        }
        return false;
    }

    /*************************************************************************\

    Function:
        GetWindowDockSpaceID

    Description:
        Returns ID of the dock space the current window is docked into.
        Returns 0 if the window is not docked to any dock space.

    \*************************************************************************/
    ImGuiID GetWindowDockSpaceID();

    /*************************************************************************\

    Function:
        SetPadding

    Description:
        Add a padding around the next element.

    \*************************************************************************/
    void BeginPadding( float top, float right, float left );
    void BeginPadding( float all );
    void EndPadding( float bottom );

    /*************************************************************************\

    Function:
        ToggleButton

    Description:
        Button that retains its state.

    \*************************************************************************/
    bool ToggleButton( const char* label, bool& value, ImVec2 size = ImVec2( 0, 0 ) );

    /*************************************************************************\

    Function:
        ImageToggleButton

    Description:
        ImageButton that retains its state.

    \*************************************************************************/
    bool ImageToggleButton(
        const char* str_id,
        bool& value,
        ImTextureID user_texture_id,
        const ImVec2& image_size,
        const ImVec2& uv0 = ImVec2( 0, 0 ),
        const ImVec2& uv1 = ImVec2( 1, 1 ),
        const ImVec4& bg_col = ImVec4( 0, 0, 0, 0 ),
        const ImVec4& tint_col = ImVec4( 1, 1, 1, 1 ) );
}

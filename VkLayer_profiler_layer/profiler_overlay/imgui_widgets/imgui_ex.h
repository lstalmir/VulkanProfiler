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
}

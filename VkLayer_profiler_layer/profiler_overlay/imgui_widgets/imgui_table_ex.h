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

enum ImGuiXTableColumnFlags_
{
    ImGuiXTableColumnFlags_AlignHeaderRight = 1 << 0,
};

typedef int ImGuiXTableColumnFlags;

namespace ImGuiX
{
    /*************************************************************************\

    Function:
        TableGetColumnWidth

    Description:
        Retrieve width of the current column (-1) or specified column.

    \*************************************************************************/
    float TableGetColumnWidth(
        int column_index = -1 );

    /*************************************************************************\

    Function:
        TableSetupColumn

    Description:
        Setup a column in the current table.

    \*************************************************************************/
    void TableSetupColumn(
        const char* label,
        ImGuiTableColumnFlags flags = 0,
        ImGuiXTableColumnFlags xflags = 0,
        float init_width_or_weight = 0.f,
        ImU32 user_id = 0 );

    /*************************************************************************\

    Function:
        TableHeadersRow

    Description:
        Draw a headers row with custom styling and font.

    \*************************************************************************/
    void TableHeadersRow(
        ImFont* font = nullptr );

    /*************************************************************************\

    Function:
        TableTextColumn

    Description:
        Shortcut for TableNextColumn and formatted Text item.

    \*************************************************************************/
    bool TableTextColumn(
        const char* format,
        ... );

    /*************************************************************************\

    Function:
        TableBorderInnerH

    Description:
        Draw a horizontal line inside the table.

    \*************************************************************************/
    void TableBorderInnerH( ImU32 color, float thickness );
    void TableBorderInnerH( float thickness = 1.0f );
}

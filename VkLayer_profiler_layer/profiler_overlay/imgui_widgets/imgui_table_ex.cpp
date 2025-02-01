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

// Enable internal ImVec operators
#define IMGUI_DEFINE_MATH_OPERATORS

#include "imgui_table_ex.h"
#include <imgui_internal.h>

namespace ImGuiX
{
    // Fast access to native ImGui functions and structures.
    using namespace ImGui;

    /*************************************************************************\

        TableGetColumnWidth

    \*************************************************************************/
    float TableGetColumnWidth(
        int column_index )
    {
        ImGuiTable* table = GImGui->CurrentTable;

        if( column_index == -1 )
            column_index = table->CurrentColumn;

        return table->Columns[ column_index ].WidthGiven;
    }

    /*************************************************************************\

        TableHeadersRow

    \*************************************************************************/
    void TableHeadersRow(
        ImFont* font )
    {
        ImGuiContext& g = *GImGui;
        ImGuiTable* table = ImGui::GetCurrentTable();
        IM_ASSERT( table );

        const float row_y1 = ImGui::GetCursorScreenPos().y;
        const float row_height = TableGetHeaderRowHeight();

        // Draw the headers row.
        ImGui::TableNextRow( 0, row_height );

        if( font )
        {
            ImGui::PushFont( font );
        }

        for( int i = 0; i < table->DeclColumnsCount; ++i )
        {
            if( ImGui::TableNextColumn() )
            {
                if( table->Columns[i].Flags & ImGuiTableColumnFlags_NoHeaderLabel )
                {
                    continue;
                }

                ImS16 offset = table->Columns[i].NameOffset;
                if( offset != -1 )
                {
                    ImGui::TextUnformatted( &table->ColumnsNames.Buf.Data[offset] );
                }
            }
        }

        if( font )
        {
            ImGui::PopFont();
        }

        // Handle context menu in headers row.
        if( !( table->Flags & ImGuiTableFlags_ContextMenuInBody ) )
        {
            ImVec2 mouse_pos = ImGui::GetMousePos();
            if( IsMouseReleased( ImGuiMouseButton_Right ) )
                if( mouse_pos.y >= row_y1 && mouse_pos.y < row_y1 + row_height )
                    TableOpenContextMenu( table->ColumnsCount );
        }

        // Draw a horizontal line below the headers row.
        ImU32 col = ImGui::GetColorU32( ImGuiCol_TableBorderLight );
        ImVec2 hr_beg = { table->BorderX1, table->RowPosY1 + g.FontSize + g.Style.CellPadding.y + 2 };
        ImVec2 hr_end = { table->BorderX2, hr_beg.y };

        ImDrawList* dl = ImGui::GetWindowDrawList();
        IM_ASSERT( dl );
        dl->AddLine( hr_beg, hr_end, col, 1.5f );
    }

    /*************************************************************************\

        TableTextColumn

    \*************************************************************************/
    bool TableTextColumn(
        const char* format,
        ... )
    {
        if( ImGui::TableNextColumn() )
        {
            va_list args;
            va_start( args, format );
            ImGui::TextV( format, args );
            va_end( args );
            return true;
        }

        return false;
    }
}

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
}

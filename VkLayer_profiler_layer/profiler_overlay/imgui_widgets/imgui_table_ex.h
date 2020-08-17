#pragma once
#include <imgui.h>

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
}

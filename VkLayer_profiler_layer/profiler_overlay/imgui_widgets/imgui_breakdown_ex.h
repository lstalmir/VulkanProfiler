#pragma once
#include <imgui.h>

namespace ImGuiX
{
    /*************************************************************************\

    Function:
        PlotBreakdownEx

    Description:
        Extended version of ImGui's histogram.

    Input:
        label          Title of the histogram
        values         Widths of the bars
        values_count   Number of elements in values_x and values_y arrays
        values_offset  First element offset
        tooltip_text   Tooltip text to display over each breakdown element
        graph_size     Size of the histogram

    \*************************************************************************/
    void PlotBreakdownEx(
        const char* label,
        const float* values,
        int values_count,
        int values_offset = 0,
        const char** tooltip_text = NULL,
        ImVec2 graph_size = ImVec2( 0, 0 ) );
}

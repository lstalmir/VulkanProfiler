#pragma once
#include <imgui.h>

namespace ImGuiX
{
    /*************************************************************************\

    Function:
        PlotHistogramEx

    Description:
        Extended version of ImGui's histogram.
        Allows to control x axis (width of bars) for better visualization.

    Input:
        label          Title of the histogram
        values_x       Widths of the bars
        values_y       Heights of the bars
        values_count   Number of elements in values_x and values_y arrays
        values_offset  First element offset
        overlay_text   Text to display on the histogram
        scale_min      Minimal value of the y axis
        scale_max      Maximal value of the y axis
        graph_size     Size of the histogram
        stride         Element stride

    \*************************************************************************/
    void PlotHistogramEx(
        const char* label,
        const float* values_x,
        const float* values_y,
        int values_count,
        int values_offset = 0,
        const char* overlay_text = NULL,
        float scale_min = FLT_MAX,
        float scale_max = FLT_MAX,
        ImVec2 graph_size = ImVec2( 0, 0 ),
        int stride = sizeof( float ) );
}

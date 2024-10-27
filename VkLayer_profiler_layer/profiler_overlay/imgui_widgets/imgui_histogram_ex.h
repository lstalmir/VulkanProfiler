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
#include <functional>

namespace ImGuiX
{
    /*************************************************************************\

    Enum:
        HistogramFlags

    Description:
        Flags for PlotHistogramEx function.

    \*************************************************************************/
    enum HistogramFlags
    {
        HistogramFlags_None = 0,
        HistogramFlags_NoHover = 1 << 0,
        HistogramFlags_NoScale = 2 << 0
    };

    /*************************************************************************\

    Structure:
        HistogramColumnData

    Description:
        Contains single histogram column data.

    Members:
        x              Width of the bar
        y              Height of the bar
        userData       Pointer to the custom data associated with the bar

    \*************************************************************************/
    struct HistogramColumnData
    {
        float x;
        float y;
        ImU32 color;
        const void* userData;
    };

    typedef void(HistogramColumnHoverCallback)( const HistogramColumnData& data );
    typedef void(HistogramColumnClickCallback)( const HistogramColumnData& data );

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
        const HistogramColumnData* values,
        int values_count,
        int values_offset = 0,
        int values_stride = sizeof( HistogramColumnData ),
        const char* overlay_text = NULL,
        float scale_min = FLT_MAX,
        float scale_max = FLT_MAX,
        ImVec2 graph_size = ImVec2( 0, 0 ),
        int flags = HistogramFlags_None,
        std::function<HistogramColumnHoverCallback> hover_cb = NULL,
        std::function<HistogramColumnClickCallback> click_cb = NULL );
}

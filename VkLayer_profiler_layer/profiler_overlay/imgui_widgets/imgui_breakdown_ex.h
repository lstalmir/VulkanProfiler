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
        PlotBreakdownEx

    Description:
        Extended version of ImGui's histogram.

    Input:
        label          Title of the histogram
        values         Widths of the bars
        values_count   Number of elements in values array
        values_offset  First element offset
        hovered_index  Index of the hovered value on output
        colors         Colors of the bars, if nullptr then random colors are used
        graph_size     Size of the histogram

    \*************************************************************************/
    void PlotBreakdownEx(
        const char* label,
        const float* values,
        int values_count,
        int values_offset = 0,
        int* hovered_index = nullptr,
        const ImU32* colors = nullptr,
        ImVec2 graph_size = ImVec2( 0, 0 ) );
}

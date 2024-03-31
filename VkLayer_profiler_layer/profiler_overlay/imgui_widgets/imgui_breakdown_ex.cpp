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

// Enable internal ImVec operators
#define IMGUI_DEFINE_MATH_OPERATORS

#include "imgui_breakdown_ex.h"
#include <imgui_internal.h>

#include <random>

namespace ImGuiX
{
    // Fast access to native ImGui functions and structures.
    using namespace ImGui;

    /*************************************************************************\

        PlotBreakdownEx

    \*************************************************************************/
    void PlotBreakdownEx(
        const char* label,
        const float* values,
        int values_count,
        int values_offset,
        const char** tooltip_text,
        ImVec2 graph_size )
    {
        std::mt19937 random( 0 );

        // Implementation is based on ImGui::PlotEx function (which is called by PlotHistogram).

        ImGuiContext& g = *GImGui;
        ImGuiWindow* window = GetCurrentWindow();
        if( window->SkipItems )
            return;

        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID( label );

        PushItemWidth( -1 );

        const ImVec2 label_size = CalcTextSize( label, NULL, true );
        if( graph_size.x == 0.0f )
            graph_size.x = CalcItemWidth();
        if( graph_size.y == 0.0f )
            graph_size.y = label_size.y + (style.FramePadding.y * 2);

        const ImRect frame_bb( window->DC.CursorPos, window->DC.CursorPos + graph_size );
        const ImRect inner_bb( frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding );
        const ImRect total_bb( frame_bb.Min, frame_bb.Max );
        ItemSize( total_bb, style.FramePadding.y );
        if( !ItemAdd( total_bb, 0, &frame_bb ) )
            return;

        const bool hovered =
            ItemHoverable( frame_bb, id, 0 ) &&
            inner_bb.Contains( g.IO.MousePos );

        // Determine horizontal scale from values
        float x_size = 0.f;
        for( int i = 0; i < values_count; i++ )
            x_size += values[ i ];

        int idx_hovered = -1;
        if( values_count > 0 )
        {
            int res_w = ImMin( (int)graph_size.x, values_count );
            int item_count = values_count;

            const int v_step = values_count / res_w;
            const float t_step = 1.0f / (float)x_size;

            float v0 = 1;
            float t0 = 0.0f;
            ImVec2 tp0 = ImVec2( t0, 1.0f );

            const ImU32 col_base = GetColorU32( ImGuiCol_PlotHistogram );

            bool tooltipDrawn = false;

            for( int n = 0; n < res_w; n++ )
            {
                const int v1_idx = n * v_step;
                IM_ASSERT( v1_idx >= 0 && v1_idx < values_count );
                const float t1 = t0 + t_step * ImMax( 1.f, values[ (v1_idx + values_offset) % values_count ] );
                const float v1 = 0;
                const ImVec2 tp1 = ImVec2( t1, 0.0f );

                // NB: Draw calls are merged together by the DrawList system. Still, we should render our batch are lower level to save a bit of CPU.
                ImVec2 pos0 = ImLerp( inner_bb.Min, inner_bb.Max, tp0 );
                ImVec2 pos1 = ImLerp( inner_bb.Min, inner_bb.Max, ImVec2( tp1.x, 0 ) );
                if( pos1.x >= pos0.x + 2.0f )
                    pos1.x -= 1.0f;

                const ImVec2 posMin = ImMin( pos0, pos1 );
                const ImVec2 posMax = ImMax( pos0, pos1 );

                if( tooltip_text &&
                    hovered &&
                    tooltipDrawn == false &&
                    ImRect( posMin, posMax ).Contains( g.IO.MousePos ) )
                {
                    // Draw tooltip
                    SetTooltip( "%s", tooltip_text[ v1_idx ] );
                    // Don't check other blocks
                    tooltipDrawn = true;
                }
                    
                window->DrawList->AddRectFilled( pos0, pos1, col_base + random() );

                {
                    char overlayText[ 64 ] = "0.00 MB";
                    if( x_size )
                    {
                        snprintf( overlayText, sizeof( overlayText ), "%.2f MB (%.1f%%)",
                            values[ (v1_idx + values_offset) % values_count ] / 1048576.f,
                            values[ (v1_idx + values_offset) % values_count ] * 100.f / x_size );
                    }

                    RenderTextClipped( posMin, posMax, overlayText, overlayText + 64, nullptr );
                }

                t0 = t1;
                tp0.x = tp1.x;
            }
        }
    }
}

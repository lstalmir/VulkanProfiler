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

#include "imgui_histogram_ex.h"
#include <imgui_internal.h>
#include <algorithm>

namespace ImGuiX
{
    // Fast access to native ImGui functions and structures.
    using namespace ImGui;

    /*************************************************************************\

    Function:
        GetHistogramColumnData

    Description:
        Index values with specified stride.

    \*************************************************************************/
    inline static const HistogramColumnData& GetHistogramColumnData(
        const HistogramColumnData* values,
        int values_stride,
        int index )
    {
        const ImU8* ptr = reinterpret_cast<const ImU8*>(values);
        return *reinterpret_cast<const HistogramColumnData*>(ptr + (index * values_stride));
    }

    /*************************************************************************\

    Function:
        Clamp

    \*************************************************************************/
    template<typename T>
    inline static T Clamp( T value, T min, T max )
    {
        return std::min( max, std::max( min, value ) );
    }

    /*************************************************************************\

    Function:
        ColorSaturation

    Description:
        Control saturation of the color.

    \*************************************************************************/
    inline static ImU32 ColorSaturation( ImU32 color, float saturation )
    {
        // Compute average component value
        ImU32 r = ((color) & 0xFF);
        ImU32 g = ((color >> 8) & 0xFF);
        ImU32 b = ((color >> 16) & 0xFF);
        float avg = (r + g + b) / 3.0f;

        // Scale differences
        float r_diff_scaled = (r - avg) * saturation;
        float g_diff_scaled = (g - avg) * saturation;
        float b_diff_scaled = (b - avg) * saturation;

        // Convert back to 0-255 ranges
        r = static_cast<ImU32>(Clamp<int>( static_cast<int>(avg + r_diff_scaled), 0, 255 ));
        g = static_cast<ImU32>(Clamp<int>( static_cast<int>(avg + g_diff_scaled), 0, 255 ));
        b = static_cast<ImU32>(Clamp<int>( static_cast<int>(avg + b_diff_scaled), 0, 255 ));

        // Construct output color value
        return (r) | (g << 8) | (b << 16) | (color & 0xFF000000);
    }

    /*************************************************************************\

        PlotHistogramEx

    \*************************************************************************/
    void PlotHistogramEx(
        const char* label,
        const HistogramColumnData* values,
        int values_count,
        int values_offset,
        int values_stride,
        const char* overlay_text,
        float scale_min,
        float scale_max,
        ImVec2 graph_size,
        std::function<HistogramColumnHoverCallback> hover_cb,
        std::function<HistogramColumnClickCallback> click_cb )
    {
        // Implementation is based on ImGui::PlotEx function (which is called by PlotHistogram).

        ImGuiContext& g = *GImGui;
        ImGuiWindow* window = GetCurrentWindow();
        if( window->SkipItems )
            return;

        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID( label );

        const ImVec2 label_size = CalcTextSize( label, NULL, true );
        if( graph_size.x == 0.0f )
            graph_size.x = CalcItemWidth();
        if( graph_size.y == 0.0f )
            graph_size.y = label_size.y + (style.FramePadding.y * 2);

        const ImRect frame_bb( window->DC.CursorPos, window->DC.CursorPos + graph_size );
        const ImRect inner_bb( frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding );
        const ImRect total_bb( frame_bb.Min, frame_bb.Max + ImVec2( label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0 ) );
        ItemSize( total_bb, style.FramePadding.y );
        if( !ItemAdd( total_bb, 0, &frame_bb ) )
            return;

        const bool hovered =
            ItemHoverable( frame_bb, id, 0 ) &&
            inner_bb.Contains( g.IO.MousePos );

        // Determine scale from values if not specified
        if( scale_min == FLT_MAX || scale_max == FLT_MAX )
        {
            float y_min = FLT_MAX;
            float y_max = -FLT_MAX;
            for( int i = 0; i < values_count; i++ )
            {
                const float y = GetHistogramColumnData( values, values_stride, i ).y;
                if( y != y ) // Ignore NaN values
                    continue;
                y_min = ImMin( y_min, y );
                y_max = ImMax( y_max, y );
            }
            if( scale_min == FLT_MAX )
                scale_min = y_min;
            if( scale_max == FLT_MAX )
                scale_max = y_max;
        }

        // Adjust scale to have some normalized value
        {
            scale_min = floorf( scale_min / 10000.f ) * 10000.f;
            scale_max = ceilf( scale_max / 10000.f ) * 10000.f;
        }

        // Determine horizontal scale from values
        float x_size = 0.f;
        for( int i = 0; i < values_count; i++ )
            x_size += GetHistogramColumnData( values, values_stride, i ).x;

        //RenderFrame( frame_bb.Min, frame_bb.Max, GetColorU32( ImGuiCol_FrameBg ), true, style.FrameRounding );

        // Render horizontal lines
        {
            // Divide range to 10 equal parts
            const float step = inner_bb.GetHeight() / 10.f;
            const ImU32 lineCol = GetColorU32( ImGuiCol_Separator, 0.2f );

            for( int i = 0; i < 10; ++i )
            {
                window->DrawList->AddLine(
                    { inner_bb.Min.x, inner_bb.Min.y + (i * step) },
                    { inner_bb.Max.x, inner_bb.Min.y + (i * step) },
                    lineCol );
            }
        }

        int idx_hovered = -1;
        if( values_count > 0 )
        {
            int res_w = ImMin( (int)graph_size.x, values_count );
            int item_count = values_count;

            const int v_step = values_count / res_w;
            const float t_step = 1.0f / (float)x_size;
            const float inv_scale = (scale_min == scale_max) ? 0.0f : (1.0f / (scale_max - scale_min));

            float v0 = GetHistogramColumnData( values, values_stride, (0 + values_offset) % values_count ).y;
            float t0 = 0.0f;
            ImVec2 tp0 = ImVec2( t0, 1.0f - ImSaturate( (v0 - scale_min) * inv_scale ) );                       // Point in the normalized space of our target rectangle
            float histogram_zero_line_t = (scale_min * scale_max < 0.0f) ? (-scale_min * inv_scale) : (scale_min < 0.0f ? 0.0f : 1.0f);   // Where does the zero line stands

            bool hovered_column_found = false;

            for( int n = 0; n < res_w; n++ )
            {
                const int v1_idx = n * v_step;
                IM_ASSERT( v1_idx >= 0 && v1_idx < values_count );
                const auto& column_data = GetHistogramColumnData( values, values_stride, (v1_idx + values_offset) % values_count );
                const float t1 = t0 + t_step * ImMax( 1.f, column_data.x );
                const float v1 = GetHistogramColumnData( values, values_stride, (v1_idx + values_offset + 1) % values_count ).y;
                const ImVec2 tp1 = ImVec2( t1, 1.0f - ImSaturate( (v1 - scale_min) * inv_scale ) );

                const ImU32 col_base = column_data.color;
                const ImU32 col_hovered = ColorSaturation( col_base, 1.5f );

                // NB: Draw calls are merged together by the DrawList system. Still, we should render our batch are lower level to save a bit of CPU.
                ImVec2 pos0 = ImLerp( inner_bb.Min, inner_bb.Max, tp0 );
                ImVec2 pos1 = ImLerp( inner_bb.Min, inner_bb.Max, ImVec2( tp1.x, histogram_zero_line_t ) );
                if( pos1.x >= pos0.x + 2.0f )
                    pos1.x -= 1.0f;

                if( hovered &&
                    hovered_column_found == false &&
                    ImRect( pos0, pos1 ).Contains( g.IO.MousePos ) )
                {
                    if( hover_cb )
                        hover_cb( column_data );
                    if( click_cb && GetIO().MouseClicked[ ImGuiMouseButton_Left ] )
                        click_cb( column_data );

                    window->DrawList->AddRectFilled( pos0, pos1, col_hovered );
                    // Don't check other blocks
                    hovered_column_found = true;
                }
                else
                {
                    window->DrawList->AddRectFilled( pos0, pos1, col_base );
                }

                v0 = v1;
                t0 = t1;
                tp0 = tp1;
            }
        }

        // Text overlay
        if( overlay_text )
            RenderTextClipped( ImVec2( frame_bb.Min.x, frame_bb.Min.y + style.FramePadding.y ), frame_bb.Max, overlay_text, NULL, NULL, ImVec2( 0.5f, 0.0f ) );

        if( label_size.x > 0.0f )
            RenderText( ImVec2( frame_bb.Max.x + style.ItemInnerSpacing.x, inner_bb.Min.y ), label );

        // Render scale
        {
            const float range = scale_max - scale_min;

            char scale[ 16 ] = { 0 };
            if( range < 100000.f )
                sprintf( scale, "%.0f", range );

            else if( range < 10000000000000.f )
                sprintf( scale, "%.0fk", ceilf( range / 1000.f ) );

            RenderText( inner_bb.Min, scale );
        }
    }
}

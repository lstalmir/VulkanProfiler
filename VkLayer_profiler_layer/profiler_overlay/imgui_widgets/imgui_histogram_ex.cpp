// Copyright (c) 2019-2024 Lukasz Stalmirski
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
        int flags,
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

        // Determine scale from values if not specified
        double x_size = 0.f;
        if( scale_min == FLT_MAX || scale_max == FLT_MAX )
        {
            float y_min = FLT_MAX;
            float y_max = -FLT_MAX;
            for( int i = 0; i < values_count; i++ )
            {
                const auto& d = GetHistogramColumnData( values, values_stride, i );
                if( d.y != d.y ) // Ignore NaN values
                    continue;
                y_min = ImMin( y_min, d.y );
                y_max = ImMax( y_max, d.y );
                x_size += (double)d.x;
            }
            if( scale_min == FLT_MAX )
                scale_min = y_min;
            if( scale_max == FLT_MAX )
                scale_max = y_max;
        }

        RenderFrame( frame_bb.Min, frame_bb.Max, GetColorU32( ImGuiCol_FrameBg ), true, style.FrameRounding );

        // Render horizontal lines
        if( ( flags & HistogramFlags_NoScale ) == 0 )
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

        float prev_pos = 0;
        for( int i = 0; i < values_count; ++i )
        {
            const auto& data = GetHistogramColumnData( values, values_stride, i );
            if( data.y != data.y ) // Ignore NaN values
                continue;

            const float x_norm = data.x / float(x_size);
            const float y_norm = (data.y - scale_min) / (scale_max - scale_min);

            const float x_pos = inner_bb.Min.x + prev_pos;
            const float y_pos = inner_bb.Min.y + inner_bb.GetHeight() * (1.f - y_norm);

            // Draw column
            {
                const float column_width = inner_bb.GetWidth() * x_norm - 1;
                const float column_height = inner_bb.GetHeight() * y_norm;
                prev_pos += column_width + 1;

                if( column_width < 1.f || column_height < 1.f )
                    continue;

                const ImRect column_bb( { x_pos, y_pos }, { x_pos + column_width, inner_bb.Max.y } );
                const bool hovered_column =
                    ( flags & HistogramFlags_NoHover ) == 0 &&
                    column_bb.Contains( g.IO.MousePos );

                window->DrawList->AddRectFilled(
                    column_bb.Min,
                    column_bb.Max,
                    hovered_column ? ColorSaturation( data.color, 1.5f ) : data.color );

                if( hovered_column )
                {
                    if( click_cb && IsMouseClicked( ImGuiMouseButton_Left ) )
                    {
                        click_cb( data );
                    }

                    if( hover_cb )
                    {
                        hover_cb( data );
                    }
                }
            }
        }

        // Text overlay
        if( overlay_text )
            RenderTextClipped( ImVec2( frame_bb.Min.x, frame_bb.Min.y + style.FramePadding.y ), frame_bb.Max, overlay_text, NULL, NULL, ImVec2( 0.5f, 0.0f ) );

        if( label_size.x > 0.0f )
            RenderText( ImVec2( frame_bb.Max.x + style.ItemInnerSpacing.x, inner_bb.Min.y ), label );

        // Render scale
        if( ( flags & HistogramFlags_NoScale ) == 0 )
        {
            const float range = scale_max - scale_min;

            char scale[ 16 ] = { 0 };
            if( range < 100000.f )
                snprintf( scale, sizeof( scale ), "%.0f", range );

            else if( range < 10000000000000.f )
                snprintf( scale, sizeof( scale ), "%.0fk", ceilf( range / 1000.f ) );

            RenderText( inner_bb.Min, scale );
        }
    }
}

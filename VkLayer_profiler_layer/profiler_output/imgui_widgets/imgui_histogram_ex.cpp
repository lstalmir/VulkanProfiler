// Enable internal ImVec operators
#define IMGUI_DEFINE_MATH_OPERATORS

#include "imgui_histogram_ex.h"
#include <imgui_internal.h>

namespace ImGuiX
{
    // Fast access to native ImGui functions and structures.
    using namespace ImGui;

    /*************************************************************************\

        PlotHistogramEx

    \*************************************************************************/
    void PlotHistogramEx(
        const char* label,
        const float* values_x,
        const float* values_y,
        int values_count,
        int values_offset,
        const char* overlay_text,
        float scale_min,
        float scale_max,
        ImVec2 graph_size,
        int stride )
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
        const bool hovered = ItemHoverable( frame_bb, id );

        // Determine scale from values if not specified
        if( scale_min == FLT_MAX || scale_max == FLT_MAX )
        {
            float y_min = FLT_MAX;
            float y_max = -FLT_MAX;
            for( int i = 0; i < values_count; i++ )
            {
                const float y = values_y[ i ];
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

        // Determine horizontal scale from values
        float x_size = 0.f;
        for( int i = 0; i < values_count; i++ )
            x_size += values_x[ i ];

        RenderFrame( frame_bb.Min, frame_bb.Max, GetColorU32( ImGuiCol_FrameBg ), true, style.FrameRounding );

        int idx_hovered = -1;
        if( values_count > 0 )
        {
            int res_w = ImMin( (int)graph_size.x, values_count );
            int item_count = values_count;

            // Tooltip on hover
            if( hovered && inner_bb.Contains( g.IO.MousePos ) )
            {
                const float t = ImClamp( (g.IO.MousePos.x - inner_bb.Min.x) / (inner_bb.Max.x - inner_bb.Min.x), 0.0f, 0.9999f );
                const int v_idx = (int)(t * item_count);
                IM_ASSERT( v_idx >= 0 && v_idx < values_count );

                const float v0 = values_y[ (v_idx + values_offset) % values_count ];
                const float v1 = values_y[ (v_idx + 1 + values_offset) % values_count ];
                SetTooltip( "%d: %8.4g", v_idx, v0 );
                idx_hovered = v_idx;
            }

            const float t_step = 1.0f / (float)x_size;
            const float inv_scale = (scale_min == scale_max) ? 0.0f : (1.0f / (scale_max - scale_min));

            const int v_step = values_count / res_w;

            float v0 = values_y[ (0 + values_offset) % values_count ];
            float t0 = 0.0f;
            ImVec2 tp0 = ImVec2( t0, 1.0f - ImSaturate( (v0 - scale_min) * inv_scale ) );                       // Point in the normalized space of our target rectangle
            float histogram_zero_line_t = (scale_min * scale_max < 0.0f) ? (-scale_min * inv_scale) : (scale_min < 0.0f ? 0.0f : 1.0f);   // Where does the zero line stands

            const ImU32 col_base = GetColorU32( ImGuiCol_PlotHistogram );
            const ImU32 col_hovered = GetColorU32( ImGuiCol_PlotHistogramHovered );

            for( int n = 0; n < res_w; n++ )
            {
                const int v1_idx = n * v_step;
                IM_ASSERT( v1_idx >= 0 && v1_idx < values_count );
                const float t1 = t0 + t_step * ImMax( 1.f, values_x[ (v1_idx + values_offset) % values_count ] );
                const float v1 = values_y[ (v1_idx + values_offset + 1) % values_count ];
                const ImVec2 tp1 = ImVec2( t1, 1.0f - ImSaturate( (v1 - scale_min) * inv_scale ) );

                // NB: Draw calls are merged together by the DrawList system. Still, we should render our batch are lower level to save a bit of CPU.
                ImVec2 pos0 = ImLerp( inner_bb.Min, inner_bb.Max, tp0 );
                ImVec2 pos1 = ImLerp( inner_bb.Min, inner_bb.Max, ImVec2( tp1.x, histogram_zero_line_t ) );
                if( pos1.x >= pos0.x + 2.0f )
                    pos1.x -= 1.0f;
                window->DrawList->AddRectFilled( pos0, pos1, idx_hovered == v1_idx ? col_hovered : col_base );

                t0 = t1;
                tp0 = tp1;
            }
        }

        // Text overlay
        if( overlay_text )
            RenderTextClipped( ImVec2( frame_bb.Min.x, frame_bb.Min.y + style.FramePadding.y ), frame_bb.Max, overlay_text, NULL, NULL, ImVec2( 0.5f, 0.0f ) );

        if( label_size.x > 0.0f )
            RenderText( ImVec2( frame_bb.Max.x + style.ItemInnerSpacing.x, inner_bb.Min.y ), label );
    }
}

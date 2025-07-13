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

#include "imgui_ex.h"
#include <imgui_internal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>

namespace
{
    ImU8 ComponentLerp( ImU8 a, ImU8 b, float s )
    {
        return static_cast<ImU8>((a * (1 - s)) + (b * s));
    }
}

namespace ImGuiX
{
    /***********************************************************************************\

    Function:
        TextAlignRightV

    Description:
        Displays text in the next line, aligned to right.

    \***********************************************************************************/
    void TextAlignRightV( float contentAreaWidth, const char* fmt, va_list args )
    {
        thread_local char text[1024];
        vsnprintf( text, sizeof( text ), fmt, args );

        ImVec2 textSize = ImGui::CalcTextSize( text );
        ImGui::SameLine( 0, 0 );
        ImGui::Dummy( textSize );
        ImGui::SameLine( contentAreaWidth - textSize.x );
        ImGui::TextUnformatted( text );
    }

    /***********************************************************************************\

    Function:
        TextAlignRight

    Description:
        Displays text in the next line, aligned to right.

    \***********************************************************************************/
    void TextAlignRight( float contentAreaWidth, const char* fmt, ... )
    {
        va_list args;
        va_start( args, fmt );
        TextAlignRightV( contentAreaWidth, fmt, args );
        va_end( args );
    }

    /***********************************************************************************\

    Function:
        TextAlignRightSameLine

    Description:
        Displays text in the same line, aligned to right.

    \***********************************************************************************/
    void TextAlignRight( const char* fmt, ... )
    {
        va_list args;
        va_start( args, fmt );
        TextAlignRightV( ImGui::GetWindowContentRegionMax().x, fmt, args );
        va_end( args );
    }
    
    /*************************************************************************\

    Function:
        Badge

    Description:
        Print text with a color background.

    \*************************************************************************/
    void Badge( ImU32 color, float rounding, const char* fmt, ... )
    {
        va_list args;
        va_start( args, fmt );

        char text[ 128 ];
        vsnprintf( text, sizeof( text ), fmt, args );

        va_end( args );

        BadgeUnformatted( color, rounding, text );
    }
    
    /*************************************************************************\

    Function:
        Badge

    Description:
        Print text with a color background.

    \*************************************************************************/
    void BadgeUnformatted( ImU32 color, float rounding, const char* text )
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        ImDrawList* dl = window->DrawList;

        ImVec2 textSize = ImGui::CalcTextSize( text );

        ImVec2 origin = window->DC.CursorPos;
        ImVec2 lt = origin; lt.x -= 2;
        ImVec2 rb = origin; rb.x += textSize.x + 2; rb.y += textSize.y + 1;

        // Draw the background.
        dl->AddRectFilled( lt, rb, color, rounding, (rounding > 0.f) ? ImDrawFlags_RoundCornersAll : ImDrawFlags_RoundCornersNone );

        // Draw the text.
        ImGui::TextUnformatted( text );
    }

    /*************************************************************************\

    Function:
        ColorLerp

    Description:
        Interpolate colors.

    \*************************************************************************/
    ImU32 ColorLerp( ImU32 a, ImU32 b, float s )
    {
        const ImU8 o0 = ComponentLerp( (a >> 0) & 0xFF, (b >> 0) & 0xFF, s );
        const ImU8 o1 = ComponentLerp( (a >> 8) & 0xFF, (b >> 8) & 0xFF, s );
        const ImU8 o2 = ComponentLerp( (a >> 16) & 0xFF, (b >> 16) & 0xFF, s );
        const ImU8 o3 = ComponentLerp( (a >> 24) & 0xFF, (b >> 24) & 0xFF, s );
        return o0 | (o1 << 8) | (o2 << 16) | (o3 << 24);
    }

    /*************************************************************************\

    Function:
        Darker

    Description:
        Return darker color.

    \*************************************************************************/
    ImU32 Darker( ImU32 color, float factor )
    {
        const ImU8 r = static_cast<ImU8>( ImClamp( ((color >> 0) & 0xFF) * factor, 0.f, 255.f ) );
        const ImU8 g = static_cast<ImU8>( ImClamp( ((color >> 8) & 0xFF) * factor, 0.f, 255.f ) );
        const ImU8 b = static_cast<ImU8>( ImClamp( ((color >> 16) & 0xFF) * factor, 0.f, 255.f ) );
        return (color & 0xFF000000) | (b << 16) | (g << 8) | (r);
    }

    /*************************************************************************\

    Function:
        ColorAlpha

    Description:
        Set color alpha.

    \*************************************************************************/
    ImU32 ColorAlpha( ImU32 color, float alpha )
    {
        ImU32 c = color & 0x00FFFFFF;
        ImU8 a = ImClamp( 255.f * alpha, 0.f, 255.f );
        return ( c & 0x00FFFFFF ) | ( a << 24 );
    }

    /*************************************************************************\

    Function:
        BeginSlimCombo

    Description:
        Compact combo box.

    \*************************************************************************/
    bool BeginSlimCombo( const char* label, const char* preview_value, ImGuiComboFlags flags )
    {
        ImGuiContext& g = *GImGui;
        ImGuiWindow* window = ImGui::GetCurrentWindow();

        ImGuiNextWindowDataFlags backup_next_window_data_flags = g.NextWindowData.Flags;
        g.NextWindowData.ClearFlags(); // We behave like Begin() and need to consume those values
        if( window->SkipItems )
            return false;

        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID( label );
        IM_ASSERT( ( flags & ( ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview ) ) != ( ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview ) ); // Can't use both flags together
        if( flags & ImGuiComboFlags_WidthFitPreview )
            IM_ASSERT( ( flags & ( ImGuiComboFlags_NoPreview | (ImGuiComboFlags)ImGuiComboFlags_CustomPreview ) ) == 0 );

        const float frame_height = ImGui::GetFrameHeight();
        const float arrow_scale = 0.65f;
        const float arrow_size = ( flags & ImGuiComboFlags_NoArrowButton ) ? 0.0f : ( frame_height * arrow_scale );
        const float arrow_padding = arrow_size * ( 1.0f - arrow_scale ) / 1.5f;

        const ImVec2 label_size = ImGui::CalcTextSize( label, nullptr, true );
        const float preview_width = ImGui::CalcTextSize( preview_value, nullptr, true ).x;
        const float w = ( arrow_size + label_size.x + style.FramePadding.x * 2.0f ) + ( ( flags & ImGuiComboFlags_NoPreview ) ? 0.0f : preview_width );

        ImRect bb( window->DC.CursorPos, window->DC.CursorPos );
        bb.Max.x += w;
        bb.Max.y += label_size.y + style.FramePadding.y * 2.0f;

        ImRect total_bb( bb.Min, bb.Max );
        ImGui::ItemSize( total_bb, style.FramePadding.y );
        if( !ImGui::ItemAdd( total_bb, id, &bb ) )
            return false;

        // Open on click
        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior( bb, id, &hovered, &held );
        const ImGuiID popup_id = ImHashStr( "##ComboPopup", 0, id );
        bool popup_open = ImGui::IsPopupOpen( popup_id, ImGuiPopupFlags_None );
        if( pressed && !popup_open )
        {
            ImGui::OpenPopupEx( popup_id, ImGuiPopupFlags_None );
            popup_open = true;
        }

        // Render shape
        const ImU32 frame_col = ImGui::GetColorU32( hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg );
        const float value_x2 = ImMax( bb.Min.x, bb.Max.x - arrow_size );
        ImGui::RenderNavHighlight( bb, id );

        if( !( flags & ImGuiComboFlags_NoArrowButton ) )
        {
            ImU32 bg_col = ImGui::GetColorU32( ( popup_open || hovered ) ? ImGuiCol_ButtonHovered : ImGuiCol_Button );
            ImU32 text_col = ImGui::GetColorU32( ImGuiCol_Text );
            if( value_x2 + arrow_size <= bb.Max.x )
            {
                ImGui::RenderArrow( window->DrawList, ImVec2( value_x2, bb.Min.y + style.FramePadding.y + arrow_padding ), text_col, ImGuiDir_Down, arrow_scale );
            }
        }

        // Custom preview
        if( flags & ImGuiComboFlags_CustomPreview )
        {
            g.ComboPreviewData.PreviewRect = ImRect( bb.Min.x, bb.Min.y, value_x2, bb.Max.y );
            IM_ASSERT( preview_value == NULL || preview_value[0] == 0 );
            preview_value = NULL;
        }

        // Render label and preview
        if( label_size.x > 0 )
        {
            ImGui::RenderText( ImVec2( bb.Min.x, bb.Min.y + style.FramePadding.y ), label );
        }

        if( preview_value != NULL && !( flags & ImGuiComboFlags_NoPreview ) )
        {
            ImVec2 pos_min = bb.Min;
            pos_min.x += style.ItemInnerSpacing.x + label_size.x;
            pos_min.y += style.FramePadding.y;

            ImU32 line_colf = ImGui::GetColorU32( ImGuiCol_TextLink );
            ImGui::PushStyleColor( ImGuiCol_Text, line_colf );
            if( g.LogEnabled )
                ImGui::LogSetNextTextDecoration( "{", "}" );
            ImGui::RenderText( pos_min, preview_value );
            ImGui::PopStyleColor();
        }

        if( !popup_open )
            return false;

        g.NextWindowData.Flags = backup_next_window_data_flags;
        return ImGui::BeginComboPopup( popup_id, bb, flags );
    }

    /*************************************************************************\

    Function:
        EndSlimCombo

    Description:
        Compact combo box.

    \*************************************************************************/
    void EndSlimCombo()
    {
        ImGuiContext& g = *GImGui;
        ImGui::EndPopup();
        g.BeginComboDepth--;
    }

    /*************************************************************************\

    Function:
        Selectable

    Description:
        Selectable combo box item.

    \*************************************************************************/
    bool Selectable( const char* label, bool isSelected )
    {
        bool selectionChanged = false;

        if( ImGui::Selectable( label, isSelected ) )
        {
            // Selection changed.
            isSelected = true;
            selectionChanged = true;
        }

        if( isSelected )
        {
            ImGui::SetItemDefaultFocus();
        }

        return selectionChanged;
    }

    /*************************************************************************\

    Function:
        GetWindowDockSpaceID

    Description:
        Returns ID of the dock space the current window is docked into.
        Returns 0 if the window is not docked to any dock space.

    \*************************************************************************/
    ImGuiID GetWindowDockSpaceID()
    {
        ImGuiWindow* pWindow = GImGui->CurrentWindow;
        if( !pWindow )
            return 0;

        if( !pWindow->DockIsActive )
            return 0;

        ImGuiDockNode* pNode = pWindow->DockNode;
        while( pNode && !pNode->IsDockSpace() )
            pNode = pNode->ParentNode;

        if( !pNode )
            return 0;

        return pNode->ID;
    }

    /*************************************************************************\

    Function:
        BeginPadding

    Description:
        Add a padding around the next element.

    \*************************************************************************/
    void BeginPadding( float top, float right, float left )
    {
        // Tables support.
        ImGuiTable* table = ImGui::GetCurrentTable();
        if( table )
        {
            table->RowPosY1 += top;
            table->RowPosY2 += top;
        }

        ImVec2 cp = ImGui::GetCursorPos();
        cp.x += left;
        cp.y += top;
        ImGui::SetCursorPos( cp );

        ImVec2 rc = ImGui::GetContentRegionMax();
        ImGui::SetNextItemWidth( rc.x - (left + right) );
    }

    void BeginPadding( float all )
    {
        BeginPadding( all, all, all );
    }

    void EndPadding( float bottom )
    {
        // Tables support.
        ImGuiTable* table = ImGui::GetCurrentTable();
        if( table )
        {
            table->RowPosY1 += bottom;
            table->RowPosY2 += bottom;
        }

        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + bottom );
    }
}
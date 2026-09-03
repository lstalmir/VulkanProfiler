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
#include "profiler_overlay_types.h"
#include "profiler_overlay_layer_backend.h"
#include "profiler_overlay_layer_backend_xkb.h"
#include <imgui.h>
#include <xcb/xcb.h>

namespace Profiler
{
    /***********************************************************************************\

    Class:
        OverlayLayerXcbPlatformBackend

    Description:
        Implementation of the backend for X11 XCB.

    \***********************************************************************************/
    class OverlayLayerXcbPlatformBackend
        : public OverlayLayerPlatformBackend
    {
    public:
        OverlayLayerXcbPlatformBackend( xcb_window_t window );
        ~OverlayLayerXcbPlatformBackend();

        void NewFrame() override;

    private:
        ImGuiContext* m_pImGuiContext;
        OverlayLayerXkbBackend* m_pXkbBackend;

        xcb_connection_t* m_Connection;
        xcb_window_t m_AppWindow;
        xcb_window_t m_InputWindow;
        ImVector<xcb_rectangle_t> m_InputRects;

        xcb_atom_t m_ClipboardSelectionAtom;
        xcb_atom_t m_ClipboardPropertyAtom;
        char* m_pClipboardText;
    
        xcb_atom_t m_TargetsAtom;
        xcb_atom_t m_TextAtom;
        xcb_atom_t m_StringAtom;
        xcb_atom_t m_Utf8StringAtom;

        bool GetRootCoordinates( xcb_window_t root, Int2& out ) const;

        xcb_get_geometry_reply_t GetGeometry( xcb_drawable_t );
        xcb_atom_t InternAtom( const char* pName, bool onlyIfExists = false );

        void UpdateMousePos();
        void SetClipboardText( const char* pText );
    
        static void SetClipboardTextFn( ImGuiContext*, const char* );
    };
}

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
#include "profiler_overlay_layer_backend.h"
#include "profiler_overlay_layer_backend_xkb.h"
#include <imgui.h>
#include <X11/Xlib.h>

namespace Profiler
{
    /***********************************************************************************\

    Class:
        OverlayLayerXlibPlatformBackend

    Description:
        Implementation of the backend for X11 Xlib.

    \***********************************************************************************/
    class OverlayLayerXlibPlatformBackend
        : public OverlayLayerPlatformBackend
    {
    public:
        OverlayLayerXlibPlatformBackend( Window window );
        ~OverlayLayerXlibPlatformBackend();

        void NewFrame() override;

    private:
        ImGuiContext* m_pImGuiContext;
        OverlayLayerXkbBackend* m_pXkbBackend;

        Display* m_Display;
        Window m_AppWindow;
        Window m_InputWindow;
        ImVector<XRectangle> m_InputRects;

        Atom m_ClipboardSelectionAtom;
        Atom m_ClipboardPropertyAtom;
        char* m_pClipboardText;

        Atom m_TargetsAtom;
        Atom m_TextAtom;
        Atom m_StringAtom;
        Atom m_Utf8StringAtom;

        void UpdateMousePos();
        void SetClipboardText( const char* pText );

        static void SetClipboardTextFn( ImGuiContext* pContext, const char* pText );
    };
}

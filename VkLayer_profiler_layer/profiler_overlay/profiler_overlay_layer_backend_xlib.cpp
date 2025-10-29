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

#include "profiler_overlay_layer_backend_xlib.h"
#include <imgui_internal.h>
#include <X11/extensions/shape.h>
#include <stdlib.h>
#include <array>
#include <mutex>

namespace Profiler
{
    extern std::mutex s_ImGuiMutex;

    /***********************************************************************************\

    Function:
        OverlayLayerXlibPlatformBackend

    Description:
        Constructor.

        s_ImGuiMutex must be locked before creating the window context.

    \***********************************************************************************/
    OverlayLayerXlibPlatformBackend::OverlayLayerXlibPlatformBackend( Window window ) try
        : m_pImGuiContext( nullptr )
        , m_pXkbBackend( nullptr )
        , m_Display( nullptr )
        , m_AppWindow( window )
        , m_InputWindow( None )
        , m_ClipboardSelectionAtom( None )
        , m_ClipboardPropertyAtom( None )
        , m_pClipboardText( nullptr )
        , m_TargetsAtom( None )
        , m_TextAtom( None )
        , m_StringAtom( None )
        , m_Utf8StringAtom( None )
    {
        // Create XKB backend
        m_pXkbBackend = new OverlayLayerXkbBackend();

        // Connect to X server
        m_Display = XOpenDisplay( nullptr );
        if( !m_Display )
            throw;

        XWindowAttributes windowAttributes;
        // TODO: error check
        XGetWindowAttributes( m_Display, window, &windowAttributes );

        int screen = XScreenNumberOfScreen( windowAttributes.screen );

        XVisualInfo vinfo;
        if( !XMatchVisualInfo( m_Display, screen, 32, TrueColor, &vinfo ) )
            throw;

        Colormap colormap = XCreateColormap( m_Display, windowAttributes.root, vinfo.visual, AllocNone );

        XSetWindowAttributes attr = {};
        attr.colormap = colormap;
        attr.override_redirect = true;
        attr.background_pixel = 0;
        attr.border_pixel = 0;
        attr.event_mask = KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask;

        int root_x, root_y;
        Window child;
        XTranslateCoordinates( m_Display, m_AppWindow, windowAttributes.root, 0, 0, &root_x, &root_y, &child );

        m_InputWindow = XCreateWindow(
            m_Display,
            windowAttributes.root,
            root_x,
            root_y,
            windowAttributes.width,
            windowAttributes.height,
            0, vinfo.depth,
            InputOutput,
            vinfo.visual,
            CWColormap | CWOverrideRedirect | CWBackPixel | CWBorderPixel | CWEventMask,
            &attr );

        if( !m_InputWindow )
            throw;

        // TODO: error check
        XMapWindow( m_Display, m_InputWindow );

        // Initialize clipboard
        m_ClipboardSelectionAtom = XInternAtom( m_Display, "CLIPBOARD", False );
        m_ClipboardPropertyAtom = XInternAtom( m_Display, "PROFILER_OVERLAY_CLIPBOARD", False );
        m_TargetsAtom = XInternAtom( m_Display, "TARGETS", False );
        m_TextAtom = XInternAtom( m_Display, "TEXT", False );
        m_StringAtom = XInternAtom( m_Display, "STRING", False );
        m_Utf8StringAtom = XInternAtom( m_Display, "UTF8_STRING", False );

        ImGuiIO& io = ImGui::GetIO();
        io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
        io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
        io.BackendPlatformName = "xlib";
        io.BackendPlatformUserData = this;

        ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
        platformIO.Platform_GetClipboardTextFn = nullptr;
        platformIO.Platform_SetClipboardTextFn = OverlayLayerXlibPlatformBackend::SetClipboardTextFn;

        m_pImGuiContext = ImGui::GetCurrentContext();
    }
    catch (...)
    {
        // Cleanup the partially initialized context.
        OverlayLayerXlibPlatformBackend::~OverlayLayerXlibPlatformBackend();
        throw;
    }

    /***********************************************************************************\

    Function:
        ~OverlayLayerXlibPlatformBackend

    Description:
        Destructor.

        s_ImGuiMutex must be locked before destroying the window context.

    \***********************************************************************************/
    OverlayLayerXlibPlatformBackend::~OverlayLayerXlibPlatformBackend()
    {
        free( m_pClipboardText );
        m_pClipboardText = nullptr;

        if( m_InputWindow ) XDestroyWindow( m_Display, m_InputWindow );
        m_InputWindow = None;
        m_AppWindow = None;

        if( m_Display ) XCloseDisplay( m_Display );
        m_Display = nullptr;

        delete m_pXkbBackend;
        m_pXkbBackend = nullptr;

        if( m_pImGuiContext )
        {
            assert( ImGui::GetCurrentContext() == m_pImGuiContext );

            ImGuiIO& io = ImGui::GetIO();
            io.BackendFlags = 0;
            io.BackendPlatformName = nullptr;
            io.BackendPlatformUserData = nullptr;

            ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
            platformIO.Platform_GetClipboardTextFn = nullptr;
            platformIO.Platform_SetClipboardTextFn = nullptr;
        }
    }

    /***********************************************************************************\

    Function:
        NewFrame

    Description:
        Handle incoming events.

    \***********************************************************************************/
    void OverlayLayerXlibPlatformBackend::NewFrame()
    {
        // Validate the current ImGui context.
        ImGuiContext* pContext = ImGui::GetCurrentContext();
        IM_ASSERT(pContext && "OverlayLayerXlibPlatformBackend::NewFrame called when no ImGui context was set.");
        IM_ASSERT(pContext == m_pImGuiContext && "OverlayLayerXlibPlatformBackend::NewFrame called with different context than the one used for initialization.");
        if( !pContext )
            return;

        ImGuiIO& io = ImGui::GetIO();
        IM_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer back-end. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");

        // Setup display size (every frame to accommodate for window resizing)
        XWindowAttributes windowAttributes;
        XGetWindowAttributes( m_Display, m_AppWindow, &windowAttributes );
        io.DisplaySize = ImVec2((float)(windowAttributes.width), (float)(windowAttributes.height));

        int root_x, root_y;
        Window child;
        XTranslateCoordinates( m_Display, m_AppWindow, windowAttributes.root, 0, 0, &root_x, &root_y, &child );

        XWindowChanges inputWindowChanges = {};
        inputWindowChanges.x = root_x;
        inputWindowChanges.y = root_y;
        inputWindowChanges.width = windowAttributes.width;
        inputWindowChanges.height = windowAttributes.height;
        XConfigureWindow( m_Display, m_InputWindow, CWX | CWY | CWWidth | CWHeight, &inputWindowChanges );

        // Update OS mouse position
        UpdateMousePos();

        // Update input capture rects
        m_InputRects.resize( 0 );

        for( ImGuiWindow* pWindow : GImGui->Windows )
        {
            if( pWindow && pWindow->WasActive )
            {
                XRectangle rect;
                rect.x = static_cast<short>( pWindow->Pos.x );
                rect.y = static_cast<short>( pWindow->Pos.y );
                rect.width = static_cast<unsigned short>( pWindow->Size.x );
                rect.height = static_cast<unsigned short>( pWindow->Size.y );
                m_InputRects.push_back( rect );
            }
        }

        XShapeCombineRectangles(
            m_Display,
            m_InputWindow,
            ShapeInput,
            0, 0,
            m_InputRects.Data,
            m_InputRects.Size,
            ShapeSet,
            Unsorted );

        // Handle incoming input events
        // Don't block if there are no pending events
        while( XEventsQueued( m_Display, QueuedAlready ) )
        {
            XEvent event;
            // TODO: error check
            XNextEvent( m_Display, &event );

            switch( event.type )
            {
            case SelectionRequest:
            {
                // Send current selection
                XEvent selectionEvent = {};
                selectionEvent.type = SelectionNotify;
                selectionEvent.xselection.display = m_Display;
                selectionEvent.xselection.requestor = event.xselectionrequest.requestor;
                selectionEvent.xselection.selection = event.xselectionrequest.selection;
                selectionEvent.xselection.target = event.xselectionrequest.target;
                selectionEvent.xselection.time = event.xselectionrequest.time;

                if( event.xselectionrequest.target == m_TargetsAtom )
                {
                    // Send list of available conversions
                    selectionEvent.xselection.property = event.xselectionrequest.property;

                    Atom targets[] = {
                        m_TargetsAtom,
                        m_TextAtom,
                        m_StringAtom,
                        m_Utf8StringAtom
                    };

                    XChangeProperty(
                        m_Display,
                        event.xselectionrequest.requestor,
                        event.xselectionrequest.property,
                        event.xselectionrequest.target, 32,
                        PropModeReplace,
                        reinterpret_cast<const unsigned char*>( targets ),
                        std::size( targets ) );
                }

                if( event.xselectionrequest.target == m_TextAtom ||
                    event.xselectionrequest.target == m_StringAtom ||
                    event.xselectionrequest.target == m_Utf8StringAtom )
                {
                    // Send selection as string
                    selectionEvent.xselection.property = event.xselectionrequest.property;

                    uint32_t clipboardTextLength = 0;
                    if( m_pClipboardText )
                    {
                        clipboardTextLength = strlen( m_pClipboardText );
                    }

                    XChangeProperty(
                        m_Display,
                        event.xselectionrequest.requestor,
                        event.xselectionrequest.property,
                        event.xselectionrequest.target, 8,
                        PropModeReplace,
                        reinterpret_cast<const unsigned char*>( m_pClipboardText ),
                        clipboardTextLength );
                }

                // Notify the requestor that the selection is ready
                XSendEvent( m_Display, event.xselectionrequest.requestor, False, 0, &selectionEvent );
                break;
            }

            case MotionNotify:
            {
                // Update mouse position
                io.MousePos.x = event.xmotion.x;
                io.MousePos.y = event.xmotion.y;
                break;
            }

            case ButtonPress:
            {
                // First 3 buttons are mouse buttons, 4 and 5 are wheel scroll
                if( event.xbutton.button < Button4 )
                {
                    int button = 0;
                    if( event.xbutton.button == Button1 ) button = 0;
                    if( event.xbutton.button == Button2 ) button = 2;
                    if( event.xbutton.button == Button3 ) button = 1;
                    // TODO: XGrabPointer?
                    io.MouseDown[ button ] = true;
                }
                else
                {
                    // TODO: scroll speed
                    io.MouseWheel += event.xbutton.button == Button4 ? 1 : -1;
                }
                break;
            }

            case ButtonRelease:
            {
                // First 3 buttons are mouse buttons
                if( event.xbutton.button < Button4 )
                {
                    int button = 0;
                    if( event.xbutton.button == Button1 ) button = 0;
                    if( event.xbutton.button == Button2 ) button = 2;
                    if( event.xbutton.button == Button3 ) button = 1;
                    io.MouseDown[ button ] = false;
                    // TODO: XUngrabPointer?
                }
                break;
            }

            case KeyPress:
            case KeyRelease:
            {
                m_pXkbBackend->AddKeyEvent( event.xkey.keycode, (event.type == KeyPress) );
                break;
            }
            }
        }

        XFlush( m_Display );
    }

    /***********************************************************************************\

    Function:
        UpdateMousePos

    Description:

    \***********************************************************************************/
    void OverlayLayerXlibPlatformBackend::UpdateMousePos()
    {
        ImGuiIO& io = ImGui::GetIO();

        // Set OS mouse position if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
        if( io.WantSetMousePos )
        {
            XPoint pos = { (short)io.MousePos.x, (short)io.MousePos.y };
            XWarpPointer( m_Display, None, m_InputWindow, 0, 0, 0, 0, pos.x, pos.y );
        }
    }

    /***********************************************************************************\

    Function:
        SetClipboardText

    Description:
        Copy text to clipboard.

    \***********************************************************************************/
    void OverlayLayerXlibPlatformBackend::SetClipboardText( const char* pText )
    {
        // Clear previous selection
        free( m_pClipboardText );
        m_pClipboardText = nullptr;

        // Copy text to local storage
        size_t clipboardTextLength = strlen( pText ? pText : "" );
        if( clipboardTextLength )
        {
            m_pClipboardText = reinterpret_cast<char*>( malloc( clipboardTextLength + 1 ) );
            if( !m_pClipboardText )
            {
                return;
            }

            memcpy( m_pClipboardText, pText, clipboardTextLength + 1 );
        }

        // Notify X server that new selection is available
        XSetSelectionOwner( m_Display, m_ClipboardSelectionAtom, m_InputWindow, CurrentTime );
    }

    /***********************************************************************************\

    Function:
        SetClipboardTextFn

    Description:
        Copy text to clipboard.

    \***********************************************************************************/
    void OverlayLayerXlibPlatformBackend::SetClipboardTextFn( ImGuiContext* pContext, const char* pText )
    {
        OverlayLayerXlibPlatformBackend* pBackend =
            static_cast<OverlayLayerXlibPlatformBackend*>( pContext->IO.BackendPlatformUserData );
        IM_ASSERT( pBackend );
        IM_ASSERT( pBackend->m_pImGuiContext == pContext );

        pBackend->SetClipboardText( pText );
    }
}

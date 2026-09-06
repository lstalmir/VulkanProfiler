// Copyright (c) 2019-2026 Lukasz Stalmirski
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

    // Flags provided by this backend.
    static constexpr ImGuiBackendFlags s_XlibPlatformBackendFlags =
        ImGuiBackendFlags_HasSetMousePos |
        ImGuiBackendFlags_HasMouseCursors;

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
        , m_HasKeyboardGrab( false )
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

        XSetWindowAttributes setWindowAttributes = {};
        setWindowAttributes.override_redirect = true;
        setWindowAttributes.event_mask =
            KeyPressMask |
            KeyReleaseMask |
            ButtonPressMask |
            ButtonReleaseMask |
            PointerMotionMask;

        Int2 rootPosition( 0, 0 );
        // TODO: error check
        GetRootCoordinates( windowAttributes.root, rootPosition );

        m_InputWindow = XCreateWindow(
            m_Display,
            windowAttributes.root,
            rootPosition.x,
            rootPosition.y,
            windowAttributes.width,
            windowAttributes.height,
            0, CopyFromParent,
            InputOnly,
            CopyFromParent,
            CWOverrideRedirect | CWEventMask,
            &setWindowAttributes );

        if( !m_InputWindow )
            throw;

        // TODO: error check
        XShapeCombineMask( m_Display, m_InputWindow, ShapeBounding, 0, 0, None, ShapeSet );
        XMapWindow( m_Display, m_InputWindow );

        // Initialize clipboard
        m_ClipboardSelectionAtom = XInternAtom( m_Display, "CLIPBOARD", False );
        m_ClipboardPropertyAtom = XInternAtom( m_Display, "PROFILER_OVERLAY_CLIPBOARD", False );
        m_TargetsAtom = XInternAtom( m_Display, "TARGETS", False );
        m_TextAtom = XInternAtom( m_Display, "TEXT", False );
        m_StringAtom = XInternAtom( m_Display, "STRING", False );
        m_Utf8StringAtom = XInternAtom( m_Display, "UTF8_STRING", False );

        ImGuiIO& io = ImGui::GetIO();
        io.BackendFlags |= s_XlibPlatformBackendFlags;
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

        SetKeyboardGrab( false );

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
            io.BackendFlags &= ~s_XlibPlatformBackendFlags;
            io.BackendPlatformName = nullptr;
            io.BackendPlatformUserData = nullptr;

            ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
            platformIO.ClearPlatformHandlers();
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

        XWindowChanges inputWindowChanges = {};
        inputWindowChanges.width = windowAttributes.width;
        inputWindowChanges.height = windowAttributes.height;

        int inputWindowChangeMask = CWWidth | CWHeight;

        Int2 rootPosition( 0, 0 );
        if( GetRootCoordinates( windowAttributes.root, rootPosition ) )
        {
            inputWindowChanges.x = rootPosition.x;
            inputWindowChanges.y = rootPosition.y;
            inputWindowChangeMask |= CWX | CWY;
        }

        XConfigureWindow( m_Display, m_InputWindow, inputWindowChangeMask, &inputWindowChanges );

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

        XShapeCombineMask(
            m_Display,
            m_InputWindow,
            ShapeBounding,
            0, 0,
            None,
            ShapeSet );

        XShapeCombineRectangles(
            m_Display,
            m_InputWindow,
            ShapeInput,
            0, 0,
            m_InputRects.Data,
            m_InputRects.Size,
            ShapeSet,
            Unsorted );

        // Grab the keyboard if ImGui wants to capture text (e.g. an InputText is focused).
        // Otherwise the input window won't receive any keyboard events.
        SetKeyboardGrab( io.WantCaptureKeyboard );

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
                io.AddMouseSourceEvent( ImGuiMouseSource_Mouse );
                io.AddMousePosEvent( static_cast<float>( event.xmotion.x ), static_cast<float>( event.xmotion.y ) );
                break;
            }

            case ButtonPress:
            case ButtonRelease:
            {
                // First 3 buttons are mouse buttons, 4 and 5 are wheel scroll
                if( event.xbutton.button < Button4 )
                {
                    int button = -1;
                    if( event.xbutton.button == Button1 ) button = 0;
                    if( event.xbutton.button == Button2 ) button = 2;
                    if( event.xbutton.button == Button3 ) button = 1;
                    // TODO: XGrabPointer?
                    if( button != -1 )
                    {
                        io.AddMouseSourceEvent( ImGuiMouseSource_Mouse );
                        io.AddMouseButtonEvent( button, (event.type == ButtonPress) );
                    }
                }
                else
                {
                    if( event.type == ButtonPress )
                    {
                        // TODO: scroll speed
                        io.AddMouseSourceEvent( ImGuiMouseSource_Mouse );
                        io.AddMouseWheelEvent( 0, (event.xbutton.button == Button4) ? 1 : -1 );
                    }
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
        GetRootCoordinates

    Description:

    \***********************************************************************************/
    bool OverlayLayerXlibPlatformBackend::GetRootCoordinates( Window root, Int2& out ) const
    {
        Window child;
        int result = XTranslateCoordinates(
            m_Display,
            m_AppWindow,
            root,
            0, 0,
            &out.x,
            &out.y,
            &child );

        return ( result == 0 );
    }

    /***********************************************************************************\

    Function:
        SetKeyboardGrab

    Description:
        Activate or deactivate exclusive keyboard grab in order to receive key events.

    \***********************************************************************************/
    void OverlayLayerXlibPlatformBackend::SetKeyboardGrab( bool grab )
    {
        if( grab && !m_HasKeyboardGrab )
        {
            // Acquire keyboard.
            XGrabKeyboard(
                m_Display,
                m_InputWindow,
                1,
                GrabModeAsync,
                GrabModeAsync,
                CurrentTime );

            m_HasKeyboardGrab = true;
        }
        else if( !grab && m_HasKeyboardGrab )
        {
            // Release keyboard.
            XUngrabKeyboard(
                m_Display,
                CurrentTime );

            m_HasKeyboardGrab = false;

            // Clear ImGuiIO key states as no key release events will be received.
            ImGui::GetIO().ClearInputKeys();
        }
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

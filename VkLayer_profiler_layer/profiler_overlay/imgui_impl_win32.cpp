#include "imgui_impl_win32.h"
#include "imgui/examples/imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND, UINT, WPARAM, LPARAM );

// Define static fields
LockableUnorderedMap<HWND, ImGui_ImplWin32_Context*> ImGui_ImplWin32_Context::s_pWin32Contexts;

/***********************************************************************************\

Function:
    ImGui_ImpWin32_Context

Description:
    Constructor.

\***********************************************************************************/
ImGui_ImplWin32_Context::ImGui_ImplWin32_Context( ImGuiContext* pImGuiContext, HWND hWnd )
    : m_AppModule( nullptr )
    , m_AppWindow( hWnd )
    , m_AppWindowProc( nullptr )
    , m_pImGuiContext( pImGuiContext )
{
    // Get handle to the DLL module
    m_AppModule = (HMODULE)GetWindowLongPtr( m_AppWindow, GWLP_HINSTANCE );

    // Store default window procedure
    m_AppWindowProc = (WNDPROC)GetWindowLongPtr( m_AppWindow, GWLP_WNDPROC );

    s_pWin32Contexts.interlocked_try_emplace( m_AppWindow, this );

    // Override window procedure
    SetWindowLongPtr( m_AppWindow, GWLP_WNDPROC, (LONG_PTR)ImGui_ImplWin32_Context::WindowProc );

    if( !ImGui_ImplWin32_Init( m_AppWindow ) )
        InitError();
}

/***********************************************************************************\

Function:
    ~ImGui_ImpWin32_Context

Description:
    Destructor.

\***********************************************************************************/
ImGui_ImplWin32_Context::~ImGui_ImplWin32_Context()
{
    // Restore original window procedure
    SetWindowLongPtr( m_AppWindow, GWLP_WNDPROC, (LONG_PTR)m_AppWindowProc );

    ImGui_ImplWin32_Shutdown();

    // Erase from map
    s_pWin32Contexts.interlocked_erase( m_AppWindow );

    m_AppWindowProc = nullptr;
    m_AppWindow = nullptr;
    m_AppModule = nullptr;
}

/***********************************************************************************\

Function:
    NewFrame

Description:

\***********************************************************************************/
void ImGui_ImplWin32_Context::NewFrame()
{
    ImGui_ImplWin32_NewFrame();
}

/***********************************************************************************\

Function:
    InitError

Description:
    Cleanup from partially initialized state

\***********************************************************************************/
void ImGui_ImplWin32_Context::InitError()
{
    ImGui_ImplWin32_Context::~ImGui_ImplWin32_Context();
    throw;
}

/***********************************************************************************\

Function:
    WindowProc

Description:

\***********************************************************************************/
LRESULT CALLBACK ImGui_ImplWin32_Context::WindowProc( HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam )
{
    // Get default window procedure
    ImGui_ImplWin32_Context* context = s_pWin32Contexts.interlocked_at( hWnd );

    // Switch ImGui context
    ImGui::SetCurrentContext( context->m_pImGuiContext );

    if( Msg >= WM_MOUSEFIRST &&
        Msg <= WM_MOUSELAST )
    {
        ImGui_ImplWin32_WndProcHandler( hWnd, Msg, wParam, lParam );

        // Don't pass captured mouse events to the application
        if( ImGui::GetIO().WantCaptureMouse )
            return 0;
    }

    if( Msg == WM_CHAR && wParam == '`' )
    {
        // Toggle cursor
        SetCursor( LoadCursorA( nullptr, IDC_ARROW ) );
        // TODO
    }

    return CallWindowProc( context->m_AppWindowProc, hWnd, Msg, wParam, lParam );
}

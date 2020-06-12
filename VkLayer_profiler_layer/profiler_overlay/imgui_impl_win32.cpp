#include "imgui_impl_win32.h"
#include "imgui/examples/imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND, UINT, WPARAM, LPARAM );

// Define static fields
LockableUnorderedMap<HWND, WNDPROC> ImGui_ImplWin32_Context::s_AppWindowProcs;

/***********************************************************************************\

Function:
    ImGui_ImpWin32_Context

Description:
    Constructor.

\***********************************************************************************/
ImGui_ImplWin32_Context::ImGui_ImplWin32_Context( HWND hWnd )
    : m_AppModule( nullptr )
    , m_AppWindow( hWnd )
{
    // Get handle to the DLL module
    m_AppModule = (HMODULE)GetWindowLongPtr( m_AppWindow, GWLP_HINSTANCE );

    // Store default window procedure
    WNDPROC defaultWndProc = (WNDPROC)GetWindowLongPtr( m_AppWindow, GWLP_WNDPROC );

    s_AppWindowProcs.interlocked_try_emplace( m_AppWindow, defaultWndProc );

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
    ImGui_ImplWin32_Shutdown();

    // Erase from map
    s_AppWindowProcs.interlocked_erase( m_AppWindow );

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
    WNDPROC defaultWndProc = s_AppWindowProcs.interlocked_at( hWnd );

    if( Msg >= WM_MOUSEFIRST &&
        Msg <= WM_MOUSELAST )
    {
        ImGui_ImplWin32_WndProcHandler( hWnd, Msg, wParam, lParam );

        // Don't pass captured mouse events to the application
        if( ImGui::GetIO().WantCaptureMouse )
            return 0;
    }

    return CallWindowProc( defaultWndProc, hWnd, Msg, wParam, lParam );
}

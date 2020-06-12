#pragma once
#include "imgui_window.h"
#include "lockable_unordered_map.h"

#include <Windows.h>

class ImGui_ImplWin32_Context : public ImGui_Window_Context
{
public:
    ImGui_ImplWin32_Context( HWND hWnd );
    ~ImGui_ImplWin32_Context();

    void NewFrame() override;

private:
    HMODULE m_AppModule;
    HWND m_AppWindow;

    // Must be available from static WindowProc
    static LockableUnorderedMap<HWND, WNDPROC> s_AppWindowProcs;

    void InitError();

    static LRESULT CALLBACK WindowProc( HWND, UINT, WPARAM, LPARAM );
};

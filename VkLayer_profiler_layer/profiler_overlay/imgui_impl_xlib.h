#pragma once
#include "imgui_window.h"
#include <X11/Xlib.h>

class ImGui_ImplXlib_Context : public ImGui_Window_Context
{
public:
    ImGui_ImplXlib_Context( Window window );
    ~ImGui_ImplXlib_Context();

    const char* GetName() const override;

    void NewFrame() override;

private:
    Display* m_Display;
    XIM m_IM;
    Window m_AppWindow;
    Window m_InputWindow;

    void InitError();
    bool IsChild( Window, Window ) const;

    void UpdateMousePos();
};

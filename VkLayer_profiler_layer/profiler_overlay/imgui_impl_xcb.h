#pragma once
#include "imgui_window.h"
#include <xcb/xcb.h>

class ImGui_ImplXcb_Context : public ImGui_Window_Context
{
public:
    ImGui_ImplXcb_Context( xcb_window_t window );
    ~ImGui_ImplXcb_Context();

    void NewFrame() override;
    void UpdateWindowRect() override;

private:
    xcb_connection_t* m_Connection;
    xcb_window_t m_AppWindow;
    xcb_window_t m_InputWindow;

    void InitError();

    xcb_get_geometry_reply_t GetGeometry( xcb_drawable_t );

    void UpdateMousePos();
};

#pragma once
#include <X11/Xlib.h>

bool ImGui_ImplXlib_Init( Window window );
void ImGui_ImplXlib_Shutdown();
void ImGui_ImplXlib_NewFrame();

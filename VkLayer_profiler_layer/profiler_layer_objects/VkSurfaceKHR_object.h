#pragma once
#include <vk_layer.h>

namespace Profiler
{
    enum class OSWindowHandleType
    {
        eInvalid,
        #ifdef VK_USE_PLATFORM_WIN32_KHR
        eWin32 = 1,
        #endif
        #ifdef VK_USE_PLATFORM_WAYLAND_KHR
        eWayland = 2,
        #endif
        #ifdef VK_USE_PLATFORM_XLIB_KHR
        eX11 = 3
        #endif
    };

    struct OSWindowHandle
    {
        OSWindowHandleType Type;
        union
        {
            #ifdef VK_USE_PLATFORM_WIN32_KHR
            HWND Win32Handle;
            #endif
            #ifdef VK_USE_PLATFORM_WAYLAND_KHR
            // TODO
            #endif
            #ifdef VK_USE_PLATFORM_XLIB_KHR
            Window X11Handle;
            #endif
        };

        inline operator bool () const { return Type != OSWindowHandleType::eInvalid; }

        inline OSWindowHandle() : Type( OSWindowHandleType::eInvalid ) {}

        #ifdef VK_USE_PLATFORM_WIN32_KHR
        inline OSWindowHandle( HWND handle ) : Type( OSWindowHandleType::eWin32 ) { Win32Handle = handle; }
        #endif

        #ifdef VK_USE_PLATFORM_XLIB_KHR
        inline OSWindowHandle( Window handle ) : Type( OSWindowHandleType::eX11 ) { X11Handle = handle; }
        #endif
    };

    struct VkSurfaceKHR_Object
    {
        VkSurfaceKHR Handle;
        OSWindowHandle Window;
    };
}

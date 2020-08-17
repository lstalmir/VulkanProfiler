#pragma once
#include "vk_dispatch_tables.h"

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
        #ifdef VK_USE_PLATFORM_XCB_KHR
        eXcb = 3,
        #endif
        #ifdef VK_USE_PLATFORM_XLIB_KHR
        eX11 = 4
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
            #ifdef VK_USE_PLATFORM_XCB_KHR
            xcb_window_t XcbHandle;
            #endif
            #ifdef VK_USE_PLATFORM_XLIB_KHR
            Window X11Handle;
            #endif
        };

        inline operator bool() const { return Type != OSWindowHandleType::eInvalid; }

        inline OSWindowHandle() : Type( OSWindowHandleType::eInvalid ) {}

        #ifdef VK_USE_PLATFORM_WIN32_KHR
        inline OSWindowHandle( HWND handle ) : Type( OSWindowHandleType::eWin32 ) { Win32Handle = handle; }
        #endif

        #ifdef VK_USE_PLATFORM_XCB_KHR
        inline OSWindowHandle( xcb_window_t handle ) : Type( OSWindowHandleType::eXcb ) { XcbHandle = handle; }
        #endif

        #ifdef VK_USE_PLATFORM_XLIB_KHR
        inline OSWindowHandle( Window handle ) : Type( OSWindowHandleType::eX11 ) { X11Handle = handle; }
        #endif

        inline bool operator==( const OSWindowHandle& rh ) const
        {
            if( Type != rh.Type ) { return false; }
            switch( Type )
            {
                #ifdef VK_USE_PLATFORM_WIN32_KHR
            case OSWindowHandleType::eWin32: return (Win32Handle == rh.Win32Handle);
                #endif
                #ifdef VK_USE_PLATFORM_WAYLAND_KHR
                #error Wayland not supported
                #endif
                #ifdef VK_USE_PLATFORM_XCB_KHR
            case OSWindowHandleType::eXcb: return (XcbHandle == rh.XcbHandle);
                #endif
                #ifdef VK_USE_PLATFORM_XLIB_KHR
            case OSWindowHandleType::eX11: return (X11Handle == rh.X11Handle);
                #endif
            }
            return false;
        }
    };

    struct VkSurfaceKhr_Object
    {
        VkSurfaceKHR Handle;
        OSWindowHandle Window;
    };
}

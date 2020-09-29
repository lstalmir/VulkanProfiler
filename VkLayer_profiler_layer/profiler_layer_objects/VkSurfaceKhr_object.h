// Copyright (c) 2020 Lukasz Stalmirski
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
        eXlib = 4
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
            Window XlibHandle;
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
        inline OSWindowHandle( Window handle ) : Type( OSWindowHandleType::eXlib ) { XlibHandle = handle; }
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
            case OSWindowHandleType::eXlib: return (XlibHandle == rh.XlibHandle);
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

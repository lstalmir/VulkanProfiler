// Copyright (c) 2025 Lukasz Stalmirski
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
#include <stdint.h>

struct ImDrawData;
struct ImVec2;

namespace Profiler
{
    /***********************************************************************************\

    Class:
        OverlayBackend

    Description:
        Backend interface for the overlay.

    \***********************************************************************************/
    class OverlayBackend
    {
    public:
        virtual ~OverlayBackend() = default;

        virtual bool PrepareImGuiBackend() = 0;
        virtual void DestroyImGuiBackend() = 0;

        virtual void WaitIdle() {}

        virtual bool NewFrame() = 0;
        virtual void RenderDrawData( ImDrawData* draw_data ) = 0;

        virtual float GetDPIScale() const = 0;
        virtual ImVec2 GetRenderArea() const = 0;

        virtual uint64_t CreateImage( int width, int height, const void* pData ) = 0;
        virtual void DestroyImage( uint64_t image ) = 0;
        virtual void CreateFontsImage() = 0;
        virtual void DestroyFontsImage() = 0;
    };
}

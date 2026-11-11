// Copyright (c) 2026 Lukasz Stalmirski
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
#include "profiler_overlay/profiler_overlay_backend.h"

#include <SDL3/SDL.h>
#include <vector>

namespace Profiler::GUI
{
    /***********************************************************************************\

    Class:
        SDLRendererBackend

    Description:
        Overlay backend implementation using SDL_Renderer.

    \***********************************************************************************/
    class SDLRendererBackend
        : public OverlayBackend
    {
    public:
        SDLRendererBackend( SDL_Window* pWindow );
        ~SDLRendererBackend();

        bool PrepareImGuiBackend() override;
        void DestroyImGuiBackend() override;

        void WaitIdle() override;

        bool NewFrame() override;
        void RenderDrawData( ImDrawData* pDrawData ) override;

        float GetDPIScale() const override;
        ImVec2 GetRenderArea() const override;

        int CreateImage( int width, int height, const void* pData ) override;
        void DestroyImage( int image ) override;
        void CreateFontsImage() override;
        void DestroyFontsImage() override;
        uint64_t GetImageHandle( int image ) override;

        void ProcessEvent( SDL_Event* pEvent );
        void Present();

    private:
        SDL_Window* m_pWindow;
        SDL_Renderer* m_pRenderer;
        SDL_GPUDevice* m_pDevice;

        std::vector<SDL_Texture*> m_pImages;

        SDL_Texture** GetImageTexture( int image );
    };
}

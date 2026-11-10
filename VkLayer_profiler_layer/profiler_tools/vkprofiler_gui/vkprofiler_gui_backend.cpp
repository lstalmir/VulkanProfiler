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

#include "vkprofiler_gui_backend.h"

#include <iostream>
#include <memory>

namespace Profiler::GUI
{
    /***********************************************************************************\

    Function:
        SDLRendererBackend

    Description:
        Constructor.

    \***********************************************************************************/
    SDLRendererBackend::SDLRendererBackend( SDL_Window* pWindow )
        : m_pWindow( pWindow )
        , m_pRenderer( nullptr )
        , m_pDevice( nullptr )
    {
    }

    /***********************************************************************************\

    Function:
        ~SDLRendererBackend

    Description:
        Destructor.

    \***********************************************************************************/
    SDLRendererBackend::~SDLRendererBackend()
    {
    }

    /***********************************************************************************\

    Function:
        PrepareImGuiBackend

    Description:
        Prepares the ImGui backend.

    \***********************************************************************************/
    bool SDLRendererBackend::PrepareImGuiBackend()
    {
        m_pRenderer = SDL_CreateRenderer( m_pWindow, nullptr );
        if( !m_pRenderer )
        {
            std::cerr << "Failed to create SDL renderer: " << SDL_GetError() << std::endl;
            return false;
        }

        if( !ImGui_ImplSDL3_InitForSDLRenderer( m_pWindow, m_pRenderer ) )
        {
            std::cerr << "Failed to initialize ImGui SDL3 backend" << std::endl;

            SDL_DestroyRenderer( m_pRenderer );
            return false;
        }

        if( !ImGui_ImplSDLRenderer3_Init( m_pRenderer ) )
        {
            std::cerr << "Failed to initialize ImGui SDL Renderer backend" << std::endl;

            ImGui_ImplSDL3_Shutdown();

            SDL_DestroyRenderer( m_pRenderer );
            m_pRenderer = nullptr;

            return false;
        }

        m_pDevice = SDL_GetGPURendererDevice( m_pRenderer );

        return true;
    }

    /***********************************************************************************\

    Function:
        DestroyImGuiBackend

    Description:
        Destroys the ImGui backend.

    \***********************************************************************************/
    void SDLRendererBackend::DestroyImGuiBackend()
    {
        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();

        SDL_DestroyRenderer( m_pRenderer );
        m_pRenderer = nullptr;
    }

    /***********************************************************************************\

    Function:
        WaitIdle

    Description:
        Waits for the GPU to finish all pending operations.

    \***********************************************************************************/
    void SDLRendererBackend::WaitIdle()
    {
        if( m_pDevice )
        {
            SDL_WaitForGPUIdle( m_pDevice );
        }
    }

    /***********************************************************************************\

    Function:
        NewFrame

    Description:
        Starts rendering a new frame.

    \***********************************************************************************/
    bool SDLRendererBackend::NewFrame()
    {
        SDL_RenderClear( m_pRenderer );

        ImGui_ImplSDL3_NewFrame();
        ImGui_ImplSDLRenderer3_NewFrame();
        return true;
    }

    /***********************************************************************************\

    Function:
        RenderDrawData

    Description:
        Submits ImGui draw data to the GPU.

    \***********************************************************************************/
    void SDLRendererBackend::RenderDrawData( ImDrawData* draw_data )
    {
        ImGui_ImplSDLRenderer3_RenderDrawData( draw_data, m_pRenderer );
    }

    /***********************************************************************************\

    Function:
        GetDPIScale

    Description:
        Returns the current DPI scale factor.

    \***********************************************************************************/
    float SDLRendererBackend::GetDPIScale() const
    {
        return 1.0f;
    }

    /***********************************************************************************\

    Function:
        GetRenderArea

    Description:
        Returns the current render area.

    \***********************************************************************************/
    ImVec2 SDLRendererBackend::GetRenderArea() const
    {
        int w, h;
        SDL_GetWindowSize( m_pWindow, &w, &h );
        return ImVec2( static_cast<float>( w ), static_cast<float>( h ) );
    }

    /***********************************************************************************\

    Function:
        CreateImage

    Description:
        Creates an image from raw pixel data.

    \***********************************************************************************/
    int SDLRendererBackend::CreateImage( int width, int height, const void* pData )
    {
        SDL_Surface* pSurface = SDL_CreateSurface( width, height, SDL_PIXELFORMAT_RGBA8888 );
        if( !pSurface )
        {
            return 0;
        }

        if( pData )
        {
            if( !SDL_LockSurface( pSurface ) )
            {
                SDL_DestroySurface( pSurface );
                return 0;
            }

            std::memcpy( pSurface->pixels, pData, static_cast<size_t>( width * height * 4 ) );
            SDL_UnlockSurface( pSurface );
        }

        SDL_Texture* pTexture = SDL_CreateTextureFromSurface(
            m_pRenderer,
            pSurface );

        SDL_DestroySurface( pSurface );

        return static_cast<uint64_t>( reinterpret_cast<uintptr_t>( pTexture ) );
    }

    /***********************************************************************************\

    Function:
        DestroyImage

    Description:
        Destroys an image.

    \***********************************************************************************/
    void SDLRendererBackend::DestroyImage( int image )
    {
        SDL_Texture* pTexture = reinterpret_cast<SDL_Texture*>( static_cast<uintptr_t>( image ) );
        if( pTexture )
        {
            SDL_DestroyTexture( pTexture );
        }
    }

    /***********************************************************************************\

    Function:
        CreateFontsImage

    Description:
        Creates the fonts image.

    \***********************************************************************************/
    void SDLRendererBackend::CreateFontsImage()
    {
        ImGui_ImplSDLRenderer3_CreateFontsTexture();
    }

    /***********************************************************************************\

    Function:
        DestroyFontsImage

    Description:
        Destroys the fonts image.

    \***********************************************************************************/
    void SDLRendererBackend::DestroyFontsImage()
    {
        ImGui_ImplSDLRenderer3_DestroyFontsTexture();
    }

    /***********************************************************************************\

    Function:
        GetImageHandle

    Description:
        Returns the current handle to the image.

    \***********************************************************************************/
    uint64_t SDLRendererBackend::GetImageHandle( int image )
    {
        return 0;
    }

    /***********************************************************************************\

    Function:
        ProcessEvent

    Description:
        Processes an SDL event.

    \***********************************************************************************/
    void SDLRendererBackend::ProcessEvent( SDL_Event* pEvent )
    {
        ImGui_ImplSDL3_ProcessEvent( pEvent );
    }

    /***********************************************************************************\

    Function:
        Present

    Description:
        Presents the rendered frame to the screen.

    \***********************************************************************************/
    void SDLRendererBackend::Present()
    {
        SDL_RenderPresent( m_pRenderer );
    }
}

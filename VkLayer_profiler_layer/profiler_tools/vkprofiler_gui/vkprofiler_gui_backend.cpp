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

#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>

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
        , m_pImages()
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
    void SDLRendererBackend::RenderDrawData( ImDrawData* pDrawData )
    {
        ImGui_ImplSDLRenderer3_RenderDrawData( pDrawData, m_pRenderer );
    }

    /***********************************************************************************\

    Function:
        GetDPIScale

    Description:
        Returns the current DPI scale factor.

    \***********************************************************************************/
    float SDLRendererBackend::GetDPIScale() const
    {
        float scale = SDL_GetWindowDisplayScale( m_pWindow );
        if( scale != 0.0f )
        {
            return scale;
        }

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
        SDL_Rect viewport;

        if( SDL_GetRenderViewport( m_pRenderer, &viewport ) )
        {
            return ImVec2( viewport.w, viewport.h );
        }

        if( SDL_GetWindowSize( m_pWindow, &viewport.w, &viewport.h ) )
        {
            return ImVec2( viewport.w, viewport.h );
        }

        return ImVec2( 0.0f, 0.0f );
    }

    /***********************************************************************************\

    Function:
        CreateImage

    Description:
        Creates an image from raw pixel data.

    \***********************************************************************************/
    int SDLRendererBackend::CreateImage( int width, int height, const void* pData )
    {
        auto it = std::find( m_pImages.begin(), m_pImages.end(), nullptr );
        if( it == m_pImages.end() )
        {
            try
            {
                it = m_pImages.insert( m_pImages.end(), nullptr );
            }
            catch( const std::bad_alloc& )
            {
                return -1;
            }
        }

        SDL_Surface* pSurface = SDL_CreateSurface( width, height, SDL_PIXELFORMAT_RGBA8888 );
        if( !pSurface )
        {
            return -1;
        }

        if( pData )
        {
            if( !SDL_LockSurface( pSurface ) )
            {
                SDL_DestroySurface( pSurface );
                return -1;
            }

            std::memcpy( pSurface->pixels, pData, static_cast<size_t>( width * height * 4 ) );
            SDL_UnlockSurface( pSurface );
        }

        *it = SDL_CreateTextureFromSurface( m_pRenderer, pSurface );
        SDL_DestroySurface( pSurface );

        return static_cast<int>( std::distance( m_pImages.begin(), it ) );
    }

    /***********************************************************************************\

    Function:
        DestroyImage

    Description:
        Destroys an image.

    \***********************************************************************************/
    void SDLRendererBackend::DestroyImage( int image )
    {
        SDL_Texture** ppTexture = GetImageTexture( image );
        if( ppTexture && *ppTexture )
        {
            SDL_DestroyTexture( *ppTexture );
            *ppTexture = nullptr;
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
        // Created implicitly by ImGui backend.
    }

    /***********************************************************************************\

    Function:
        DestroyFontsImage

    Description:
        Destroys the fonts image.

    \***********************************************************************************/
    void SDLRendererBackend::DestroyFontsImage()
    {
        // Created implicitly by ImGui backend.
    }

    /***********************************************************************************\

    Function:
        GetImageHandle

    Description:
        Returns the current handle to the image.

    \***********************************************************************************/
    uint64_t SDLRendererBackend::GetImageHandle( int image )
    {
        SDL_Texture** ppTexture = GetImageTexture( image );
        if( ppTexture && *ppTexture )
        {
            return static_cast<uint64_t>(
                reinterpret_cast<uintptr_t>( *ppTexture ) );
        }

        return 0;
    }

    /***********************************************************************************\

    Function:
        GetImageTexture

    Description:
        Returns the SDL_Texture pointer for the given image index,
        or nullptr if no such image exists.

    \***********************************************************************************/
    SDL_Texture** SDLRendererBackend::GetImageTexture( int image )
    {
        if( ( image >= 0 ) &&
            ( image < m_pImages.size() ) &&
            ( m_pImages[image] != nullptr ) )
        {
            return &m_pImages[image];
        }

        return nullptr;
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

// Include the actual backend implementation.
#include <backends/imgui_impl_sdl3.cpp>
#include <backends/imgui_impl_sdlrenderer3.cpp>

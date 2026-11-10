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

#include "vkprofiler_gui.h"
#include "vkprofiler_gui_backend.h"

#include "profiler/profiler_frontend.h"
#include "profiler_overlay/profiler_overlay.h"
#include "profiler_overlay/profiler_overlay_backend.h"

#include <iostream>

namespace Profiler::GUI
{
    /***********************************************************************************\

    Function:
        Application

    Description:
        Constructor.

    \***********************************************************************************/
    Application::Application()
        : m_pWindow( nullptr )
        , m_pBackend( nullptr )
        , m_pFrontend( nullptr )
        , m_pGui( nullptr )
    {
    }

    /***********************************************************************************\

    Function:
        ~Application

    Description:
        Destructor.

    \***********************************************************************************/
    Application::~Application()
    {
        delete m_pGui;
        delete m_pFrontend;
        delete m_pBackend;

        if( m_pWindow != nullptr )
        {
            SDL_DestroyWindow( m_pWindow );
        }

        SDL_Quit();
    }

    /***********************************************************************************\

    Function:
        ParseCommandLine

    Description:
        Parsees command line arguments.

    \***********************************************************************************/
    bool Application::ParseCommandLine( int argc, const char* const* argv )
    {
        return true;
    }

    /***********************************************************************************\

    Function:
        Initialize

    Description:
        Initializes the application.

    \***********************************************************************************/
    bool Application::Initialize()
    {
        // Initialize SDL library.
        if( !SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS ) )
        {
            std::cerr << "Failed to initialize SDL library: " << SDL_GetError() << std::endl;
            return false;
        }

        // Prepare window and 2D renderer for the overlay backend.
        m_pWindow = SDL_CreateWindow( "Vulkan Profiler", 640, 720, SDL_WINDOW_RESIZABLE );
        if( !m_pWindow )
        {
            std::cerr << "Failed to create SDL window: " << SDL_GetError() << std::endl;
            return false;
        }

        // Initialize the overlay backend.
        m_pBackend = new( std::nothrow ) SDLRendererBackend( m_pWindow );
        if( !m_pBackend )
        {
            std::cerr << "Failed to initialize overlay backend." << std::endl;
            return false;
        }

        // Initialize the device profiler frontend.
        // TODO: Choose depending on the data source.

        // Initialize the overlay output.
        m_pGui = new( std::nothrow ) ProfilerOverlayOutput( *m_pFrontend, *m_pBackend );
        if( !m_pGui || !m_pGui->Initialize() )
        {
            std::cerr << "Failed to initialize overlay output." << std::endl;
            return false;
        }

        return true;
    }

    /***********************************************************************************\

    Function:
        Help

    Description:
        Prints help message.

    \***********************************************************************************/
    void Application::Help()
    {
    }

    /***********************************************************************************\

    Function:
        Run

    Description:
        Runs the application's main event loop.

    \***********************************************************************************/
    int Application::Run()
    {
        bool running = true;
        while( running )
        {
            SDL_Event event;
            while( SDL_PollEvent( &event ) )
            {
                if( event.type == SDL_EVENT_QUIT )
                {
                    running = false;
                }

                m_pBackend->ProcessEvent( &event );
            }

            if( running )
            {
                m_pGui->Update();
                m_pGui->Present();

                m_pBackend->Present();
            }
        }

        return 0;
    }
}

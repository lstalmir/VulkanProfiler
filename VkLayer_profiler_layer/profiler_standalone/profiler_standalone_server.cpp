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

#include "profiler_standalone_server.h"

#include "profiler_layer_objects/VkDevice_object.h"

namespace Profiler
{
    NetworkServer::NetworkServer()
        : m_pDevice( nullptr )
        , m_ServerThread()
        , m_ListenSocket()
        , m_LocalSocket()
        , m_ClientSockets()
    {
    }

    bool NetworkServer::Initialize( VkDevice_Object* pDevice, const char* pAddress, uint16_t port )
    {
        m_pDevice = pDevice;

        m_ListenSocket.Initialize();
        m_ListenSocket.Bind( pAddress, port );
        m_ListenSocket.Listen();

        m_ServerThread = std::thread( &NetworkServer::ServerThreadProc, this );

        m_LocalSocket.Initialize();
        m_LocalSocket.Connect( pAddress, port );

        return true;
    }

    void NetworkServer::Destroy()
    {
        if( m_ServerThread.joinable() )
        {
            const NetworkRequest quitRequest = NetworkRequest::eQuit;
            m_LocalSocket.Send( &quitRequest, sizeof( quitRequest ) );
            m_ServerThread.join();
        }

        for( NetworkSocket& socket : m_ClientSockets )
        {
            socket.Destroy();
        }

        m_ClientSockets.clear();

        m_LocalSocket.Destroy();
        m_ListenSocket.Destroy();
    }

    void NetworkServer::ServerThreadProc()
    {
        std::vector<NetworkSocket*> pSelectSockets;

        NetworkBuffer responseBuffer;

        bool quitRequestReceived = false;
        while( !quitRequestReceived )
        {
            pSelectSockets.clear();
            pSelectSockets.push_back( &m_ListenSocket );

            for( NetworkSocket& socket : m_ClientSockets )
            {
                pSelectSockets.push_back( &socket );
            }

            if( NetworkSocket::Select( pSelectSockets.data(), pSelectSockets.size(), 1 ) )
            {
                if( m_ListenSocket.IsSet() )
                {
                    NetworkSocket socket;

                    if( m_ListenSocket.Accept( socket ) )
                    {
                        m_ClientSockets.push_back( std::move( socket ) );
                    }
                }

                for( NetworkSocket& socket : m_ClientSockets )
                {
                    if( socket.IsSet() )
                    {
                        NetworkRequest request;
                        socket.Receive( &request, sizeof( request ) );

                        switch( request )
                        {
                        case NetworkRequest::eQuit:
                            quitRequestReceived = true;
                            break;

                        case NetworkRequest::eGetApplicationInfo: {
                            const VkApplicationInfo& appInfo = m_pDevice->pInstance->ApplicationInfo;
                            responseBuffer.Clear();
                            responseBuffer.Write( appInfo );
                            socket.Send( responseBuffer.GetDataPointer(), responseBuffer.GetDataSize() );
                            break;
                        }
                        }
                    }
                }
            }
        }
    }
}

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

#include "profiler_standalone.h"
#include <algorithm>

#ifdef WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#define GetInvalidSocket() ( (uintptr_t)( INVALID_SOCKET ) )
#define GetNativeSocket( handle ) ( (SOCKET)( handle ) )
#endif

namespace
{
    static bool ResolveAddress( const char* pAddress, uint16_t port, struct addrinfo** ppAddrInfo )
    {
        char portStr[ 6 ] = {};
        snprintf( portStr, 6, "%hu", port );

        struct addrinfo addressHints = {};
        addressHints.ai_family = AF_INET;
        addressHints.ai_socktype = SOCK_STREAM;
        addressHints.ai_protocol = IPPROTO_IPV4;

        int result = getaddrinfo( pAddress, portStr, &addressHints, ppAddrInfo );
        return ( result != -1 );
    }
}

namespace Profiler
{
    NetworkSocket::NetworkSocket()
        : m_Handle( GetInvalidSocket() )
    {
    }

    NetworkSocket::NetworkSocket( NetworkSocket&& s )
        : m_Handle( std::exchange( s.m_Handle, GetInvalidSocket() ) )
    {
    }

    NetworkSocket& NetworkSocket::operator=( NetworkSocket&& s )
    {
        Destroy();
        m_Handle = std::exchange( s.m_Handle, GetInvalidSocket() );
        return *this;
    }

    NetworkSocket::~NetworkSocket()
    {
        Destroy();
    }

    bool NetworkSocket::Initialize()
    {
        m_Handle = (uintptr_t)socket( AF_INET, SOCK_STREAM, IPPROTO_IPV4 );

        if( m_Handle == GetInvalidSocket() )
        {
            return false;
        }

        return true;
    }

    void NetworkSocket::Destroy()
    {
        if( m_Handle != GetInvalidSocket() )
        {
            closesocket( GetNativeSocket( m_Handle ) );
            m_Handle = GetInvalidSocket();
        }
    }

    bool NetworkSocket::IsValid() const
    {
        return m_Handle != GetInvalidSocket();
    }

    bool NetworkSocket::IsSet() const
    {
        return m_IsSet;
    }

    bool NetworkSocket::Bind( const char* pAddress, uint16_t port )
    {
        struct addrinfo* pAddressInfo = nullptr;
        if( !ResolveAddress( pAddress, port, &pAddressInfo ) )
        {
            return false;
        }

        int result = bind(
            GetNativeSocket( m_Handle ),
            pAddressInfo->ai_addr,
            pAddressInfo->ai_addrlen );

        freeaddrinfo( pAddressInfo );
        return ( result != -1 );
    }

    bool NetworkSocket::Listen()
    {
        int result = listen( GetNativeSocket( m_Handle ), SOMAXCONN );
        return ( result != -1 );
    }

    bool NetworkSocket::Connect( const char* pAddress, uint16_t port )
    {
        struct addrinfo* pAddressInfo = nullptr;
        if( !ResolveAddress( pAddress, port, &pAddressInfo ) )
        {
            return false;
        }

        int result = connect(
            GetNativeSocket( m_Handle ),
            pAddressInfo->ai_addr,
            pAddressInfo->ai_addrlen );

        freeaddrinfo( pAddressInfo );
        return ( result != -1 );
    }

    bool NetworkSocket::Accept( NetworkSocket& client )
    {
        client.m_Handle = (uintptr_t)accept(
            GetNativeSocket( m_Handle ),
            nullptr,
            nullptr );

        return ( client.m_Handle != GetInvalidSocket() );
    }

    bool NetworkSocket::Select( NetworkSocket** ppSockets, size_t count, uint32_t timeout )
    {
        struct timeval tv = {};
        tv.tv_sec = ( timeout / 1000 );
        tv.tv_usec = ( timeout * 1000 ) % 1000000;

        struct fd_set set = {};
        FD_ZERO( &set );

        int nfds = 0;

        for( size_t i = 0; i < count; ++i )
        {
            ppSockets[ i ]->m_IsSet = false;

            if( ppSockets[ i ]->IsValid() )
            {
                FD_SET( GetNativeSocket( ppSockets[ i ]->m_Handle ), &set );
                nfds = std::max( nfds, static_cast<int>( ppSockets[ i ]->m_Handle + 1 ) );
            }
        }

        int result = select( nfds, &set, nullptr, nullptr, &tv );
        if( result > 0 )
        {
            for( uint32_t i = 0; i < count; ++i )
            {
                if( ppSockets[ i ]->IsValid() )
                {
                    ppSockets[ i ]->m_IsSet =
                        FD_ISSET( GetNativeSocket( ppSockets[ i ]->m_Handle ), &set );
                }
            }
        }

        return ( result > 0 );
    }

    int NetworkSocket::Send( const void* pBuffer, size_t size )
    {
        return send(
            GetNativeSocket( m_Handle ),
            reinterpret_cast<const char*>( pBuffer ),
            static_cast<int>( size ),
            0 );
    }

    int NetworkSocket::Receive( void* pBuffer, size_t size )
    {
        return recv(
            GetNativeSocket( m_Handle ),
            reinterpret_cast<char*>( pBuffer ),
            static_cast<int>( size ),
            0 );
    }
}

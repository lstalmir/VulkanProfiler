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

#define SendChecked( socket, buffer, size )                   \
    {                                                         \
        const size_t sendSize = ( size );                     \
        int result = ( socket ).Send( ( buffer ), sendSize ); \
        if( static_cast<size_t>( result ) != sendSize )       \
        {                                                     \
            return result;                                    \
        }                                                     \
    }

namespace
{
    static bool ResolveAddress( const char* pAddress, uint16_t port, struct addrinfo** ppAddrInfo, int flags = 0 )
    {
        char portStr[ 6 ] = {};
        snprintf( portStr, 6, "%hu", port );

        struct addrinfo addressHints = {};
        addressHints.ai_family = AF_INET;
        addressHints.ai_socktype = SOCK_STREAM;
        addressHints.ai_protocol = IPPROTO_TCP;
        addressHints.ai_flags = flags;

        int result = getaddrinfo( pAddress, portStr, &addressHints, ppAddrInfo );
        return ( result == 0 );
    }
}

namespace Profiler
{
    bool NetworkPlatformFunctions::Initialize()
    {
#ifdef WIN32
        WSADATA wsaData = {};
        return WSAStartup( MAKEWORD( 2, 2 ), &wsaData ) == 0;
#endif
    }

    void NetworkPlatformFunctions::Destroy()
    {
#ifdef WIN32
        WSACleanup();
#endif
    }

    NetworkSocket::NetworkSocket()
        : m_Handle( GetInvalidSocket() )
        , m_IsSet( false )
    {
    }

    NetworkSocket::NetworkSocket( NetworkSocket&& s )
        : m_Handle( std::exchange( s.m_Handle, GetInvalidSocket() ) )
        , m_IsSet( std::exchange( s.m_IsSet, false ) )
    {
    }

    NetworkSocket& NetworkSocket::operator=( NetworkSocket&& s )
    {
        Destroy();
        m_Handle = std::exchange( s.m_Handle, GetInvalidSocket() );
        m_IsSet = std::exchange( s.m_IsSet, false );
        return *this;
    }

    NetworkSocket::~NetworkSocket()
    {
        Destroy();
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

    bool NetworkSocket::Listen( const char* pAddress, uint16_t port )
    {
        struct addrinfo* pAddressInfo = nullptr;
        if( !ResolveAddress( pAddress, port, &pAddressInfo, AI_PASSIVE ) )
        {
            return false;
        }

        m_Handle = (uintptr_t)socket(
            pAddressInfo->ai_family,
            pAddressInfo->ai_socktype,
            pAddressInfo->ai_protocol );

        if( m_Handle == GetInvalidSocket() )
        {
            return false;
        }

        int result = bind(
            GetNativeSocket( m_Handle ),
            pAddressInfo->ai_addr,
            pAddressInfo->ai_addrlen );

        freeaddrinfo( pAddressInfo );

        if( result == 0 )
        {
            result = listen( GetNativeSocket( m_Handle ), SOMAXCONN );
        }

        if( result != 0 )
        {
            Destroy();
            return false;
        }

        return true;
    }

    bool NetworkSocket::Connect( const char* pAddress, uint16_t port )
    {
        struct addrinfo* pAddressInfo = nullptr;
        if( !ResolveAddress( pAddress, port, &pAddressInfo ) )
        {
            return false;
        }

        m_Handle = (uintptr_t)socket(
            pAddressInfo->ai_family,
            pAddressInfo->ai_socktype,
            pAddressInfo->ai_protocol );

        if( m_Handle == GetInvalidSocket() )
        {
            return false;
        }

        int result = connect(
            GetNativeSocket( m_Handle ),
            pAddressInfo->ai_addr,
            pAddressInfo->ai_addrlen );

        freeaddrinfo( pAddressInfo );

        if( result != 0 )
        {
            Destroy();
            return false;
        }

        return true;
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

    int NetworkSocket::Send( const NetworkPacket& packet )
    {
        uint32_t dataSize = packet.GetDataSize();
        SendChecked( *this, &dataSize, sizeof( dataSize ) );
        SendChecked( *this, packet.GetDataPointer(), dataSize );
        return dataSize;
    }

    int NetworkSocket::Send( const NetworkBuffer& buffer )
    {
        int totalBytesSent = 0;

        const NetworkPacket* pPacket = buffer.GetFirstPacket();
        while( pPacket )
        {
            int result = Send( *pPacket );
            if( result == -1 )
            {
                return -1;
            }

            totalBytesSent += result;

            if( result != pPacket->GetDataSize() )
            {
                return totalBytesSent;
            }

            pPacket = pPacket->GetNextPacket();
        }

        return totalBytesSent;
    }

    int NetworkSocket::Receive( void* pBuffer, size_t size )
    {
        return recv(
            GetNativeSocket( m_Handle ),
            reinterpret_cast<char*>( pBuffer ),
            static_cast<int>( size ),
            0 );
    }

    int NetworkSocket::Receive( NetworkPacket& packet )
    {
        uint32_t dataSize = 0;

        int result = Receive( &dataSize, sizeof( dataSize ) );
        if( result == -1 )
        {
            return -1;
        }

        if( !packet.HasSpace( dataSize ) )
        {
            packet.Resize( packet.GetDataSize() + dataSize );
        }

        result = Receive( packet.AllocateSpace( dataSize ), dataSize );
        if( result == -1 )
        {
            return -1;
        }

        return dataSize;
    }
}

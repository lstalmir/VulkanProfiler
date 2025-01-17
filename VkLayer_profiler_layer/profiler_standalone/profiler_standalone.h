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
#include <vector>
#include <string>

#include <vulkan/vulkan.h>
#include <vk_serialization.h>

namespace Profiler
{
    inline constexpr uint16_t bswap( uint16_t value )
    {
        return ( value << 8 ) | ( value >> 8 );
    }

    inline constexpr int16_t bswap( int16_t value )
    {
        return ( value << 8 ) | ( value >> 8 );
    }

    inline constexpr uint32_t bswap( uint32_t value )
    {
        return ( ( value & 0x000000FF ) << 24 ) |
               ( ( value & 0x0000FF00 ) << 8 ) |
               ( ( value & 0x00FF0000 ) >> 8 ) |
               ( ( value & 0xFF000000 ) >> 24 );
    }

    inline constexpr int32_t bswap( int32_t value )
    {
        return ( ( value & 0x000000FF ) << 24 ) |
               ( ( value & 0x0000FF00 ) << 8 ) |
               ( ( value & 0x00FF0000 ) >> 8 ) |
               ( ( value & 0xFF000000 ) >> 24 );
    }

    inline constexpr uint64_t bswap( uint64_t value )
    {
        return ( ( value & 0x00000000000000FF ) << 56 ) |
               ( ( value & 0x000000000000FF00 ) << 40 ) |
               ( ( value & 0x0000000000FF0000 ) << 24 ) |
               ( ( value & 0x00000000FF000000 ) << 8 ) |
               ( ( value & 0x000000FF00000000 ) >> 8 ) |
               ( ( value & 0x0000FF0000000000 ) >> 24 ) |
               ( ( value & 0x00FF000000000000 ) >> 40 ) |
               ( ( value & 0xFF00000000000000 ) >> 56 );
    }

    inline constexpr int64_t bswap( int64_t value )
    {
        return ( ( value & 0x00000000000000FF ) << 56 ) |
               ( ( value & 0x000000000000FF00 ) << 40 ) |
               ( ( value & 0x0000000000FF0000 ) << 24 ) |
               ( ( value & 0x00000000FF000000 ) << 8 ) |
               ( ( value & 0x000000FF00000000 ) >> 8 ) |
               ( ( value & 0x0000FF0000000000 ) >> 24 ) |
               ( ( value & 0x00FF000000000000 ) >> 40 ) |
               ( ( value & 0xFF00000000000000 ) >> 56 );
    }

    inline constexpr float bswap( float value )
    {
        union
        {
            float f;
            uint32_t i;
        } u = { value };
        u.i = bswap( u.i );
        return u.f;
    }

    inline constexpr double bswap( double value )
    {
        union
        {
            double d;
            uint64_t i;
        } u = { value };
        u.i = bswap( u.i );
        return u.d;
    }

    template<typename T>
    inline constexpr T le( T value )
    {
        if constexpr( PROFILER_IS_BIG_ENDIAN )
            return bswap( value );
        return value;
    }

    template<typename T>
    inline constexpr T be( T value )
    {
        if constexpr( PROFILER_IS_BIG_ENDIAN )
            return value;
        return bswap( value );
    }

    template<typename T>
    struct Binary
    {
        T value;
    };

    template<typename T>
    inline Binary<T&> binary( T& value ) { return { value }; }

    template<typename T>
    inline Binary<const T&> binary( const T& value ) { return { value }; }

    template<typename Stream, typename T>
    Stream& operator<<( Stream& stream, Binary<T> binary )
    {
        stream.write( reinterpret_cast<const char*>( &binary.value ), sizeof( T ) );
        return stream;
    }

    template<typename Stream, typename T>
    Stream& operator>>( Stream& stream, Binary<T&> binary )
    {
        stream.read( reinterpret_cast<char*>( &binary.value ), sizeof( T ) );
        return stream;
    }

    enum class NetworkRequest : uint8_t
    {
        eGetServerInfo = 0,
        eGetApplicationInfo = 1,
        eGetObjectName = 2,
        eGetPerformanceData = 3,
        eGetMemoryData = 4,
        eQuit = 0xFF
    };

    class NetworkSocket
    {
    public:
        NetworkSocket();
        ~NetworkSocket();

        NetworkSocket( NetworkSocket&& );
        NetworkSocket& operator=( NetworkSocket&& );

        NetworkSocket( const NetworkSocket& ) = delete;
        NetworkSocket& operator=( const NetworkSocket& ) = delete;

        bool Initialize();
        void Destroy();

        bool IsValid() const;
        bool IsSet() const;

        bool Bind( const char* pAddress, uint16_t port );
        bool Listen();
        bool Connect( const char* pAddress, uint16_t port );
        bool Accept( NetworkSocket& client );

        int Send( const void* pData, size_t size );
        int Send( const class NetworkPacket& packet );
        int Send( const class NetworkBuffer& buffer );

        int Receive( void* pBuffer, size_t size );
        int Receive( class NetworkPacket& packet );

        static bool Select( NetworkSocket** ppSockets, size_t count, uint32_t timeout = 0 );

    private:
        uintptr_t m_Handle;
        bool m_IsSet;
    };

    class NetworkPacket
    {
    public:
        inline explicit NetworkPacket( std::nullptr_t )
            : m_pData( nullptr )
            , m_Size( 0 )
            , m_Offset( 0 )
            , m_pNextPacket( nullptr )
        {
        }

        inline explicit NetworkPacket( uint32_t size = 65536 )
            : m_pData( malloc( size ) )
            , m_Size( size )
            , m_Offset( 0 )
            , m_pNextPacket( nullptr )
        {
        }

        inline NetworkPacket( NetworkPacket&& packet )
            : NetworkPacket( nullptr )
        {
            Swap( packet );
        }

        inline NetworkPacket& operator=( NetworkPacket&& packet )
        {
            NetworkPacket( std::move( packet ) ).Swap( *this );
            return *this;
        }

        NetworkPacket( const NetworkPacket& ) = delete;
        NetworkPacket& operator=( const NetworkPacket& ) = delete;

        inline ~NetworkPacket()
        {
            free( m_pData );
            delete m_pNextPacket;
        }

        inline bool HasNextPacket() const { return m_pNextPacket != nullptr && m_pNextPacket->m_Offset != 0; }
        inline bool HasSpace( uint32_t size ) const { return m_Offset + size <= m_Size; }

        inline uint32_t GetPacketSize() const { return m_Size; }
        inline uint32_t GetDataSize() const { return m_Offset; }
        inline void* GetDataPointer() { return m_pData; }
        inline const void* GetDataPointer() const { return m_pData; }

        inline void Resize( uint32_t size )
        {
            void* pData = realloc( m_pData, size );
            if( pData )
            {
                m_pData = pData;
                m_Size = size;
            }
        }

        inline void* AllocateSpace( uint32_t size )
        {
            void* ptr = static_cast<uint8_t*>( m_pData ) + m_Offset;
            m_Offset += size;
            return ptr;
        }

        inline NetworkPacket* GetNextPacket( uint32_t size = 0 )
        {
            if( !m_pNextPacket )
            {
                m_pNextPacket = new NetworkPacket( std::max( m_Size, size ) );
            }
            return m_pNextPacket;
        }

        inline const NetworkPacket* GetNextPacket() const
        {
            return m_pNextPacket;
        }

        inline void Swap( NetworkPacket& packet )
        {
            std::swap( m_pData, packet.m_pData );
            std::swap( m_Size, packet.m_Size );
            std::swap( m_Offset, packet.m_Offset );
            std::swap( m_pNextPacket, packet.m_pNextPacket );
        }

        inline void Clear()
        {
            m_Offset = 0;
            if( m_pNextPacket )
            {
                m_pNextPacket->Clear();
            }
        }

    private:
        void* m_pData;
        uint32_t m_Size;
        uint32_t m_Offset;
        NetworkPacket* m_pNextPacket;
    };

    class NetworkBuffer
    {
    public:
        inline explicit NetworkBuffer( uint32_t packetSize = 65536 )
            : m_pHead( new NetworkPacket( packetSize ) )
            , m_pTail( m_pHead )
            , m_PacketSize( packetSize )
        {
        }

        inline void Clear()
        {
            m_pTail = m_pHead;
            m_pHead->Clear();
        }

        template<typename T>
        inline auto operator<<( T value )
            -> typename std::enable_if<std::is_integral<T>::value && !std::is_enum<T>::value, NetworkBuffer&>::type
        {
            value = le( value );
            memcpy( AllocateSpace( sizeof( T ) ), &value, sizeof( T ) );
            return *this;
        }

        template<typename EnumT>
        inline auto operator<<( EnumT value )
            -> typename std::enable_if<std::is_enum<EnumT>::value, NetworkBuffer&>::type
        {
            return *this << static_cast<typename std::underlying_type<EnumT>::type>( value );
        }

        inline NetworkBuffer& operator<<( const char* value )
        {
            const uint32_t size = static_cast<uint32_t>( strlen( value ) );
            *this << size;
            memcpy( AllocateSpace( size ), value, size );
            return *this;
        }

        template<typename T>
        inline NetworkBuffer& operator<<( const std::vector<T>& value )
        {
            const uint32_t count = static_cast<uint32_t>( value.size() );
            *this << count;

            for( size_t i = 0; i < size; ++i )
            {
                *this << value[i];
            }
        }

        inline NetworkBuffer& operator<<( const std::string& value )
        {
            const uint32_t count = static_cast<uint32_t>( value.length() );
            *this << count;
            memcpy( AllocateSpace( count ), value.c_str(), count );
            return *this;
        }

        NetworkPacket* GetFirstPacket() { return m_pHead; }
        const NetworkPacket* GetFirstPacket() const { return m_pHead; }

        NetworkPacket* GetLastPacket() { return m_pTail; }
        const NetworkPacket* GetLastPacket() const { return m_pTail; }

    private:
        NetworkPacket* m_pHead;
        NetworkPacket* m_pTail;
        uint32_t m_PacketSize;

        inline void* AllocateSpace( uint32_t size )
        {
            if( !m_pTail->HasSpace( size ) )
            {
                m_pTail = m_pTail->GetNextPacket( size );
            }
            return m_pTail->AllocateSpace( size );
        }
    };
}

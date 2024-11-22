// Copyright (c) 2024 Lukasz Stalmirski
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
#include <assert.h>
#include <xutility>

template<typename T>
class RingBuffer
{
public:
    RingBuffer()
        : m_Elements( nullptr )
        , m_Size( 0 )
        , m_Head( 0 )
        , m_Capacity( 0 )
    {}

    explicit RingBuffer( size_t capacity )
        : m_Elements( new T[ capacity ] )
        , m_Size( 0 )
        , m_Head( 0 )
        , m_Capacity( capacity )
    {}

    RingBuffer( const RingBuffer& other )
        : RingBuffer()
    {
        if( other.m_Capacity )
        {
            m_Elements = new T[ other.m_Capacity ];
            _copy_to( m_Elements, other.m_Elements, other.m_Head, other.m_Size, other.m_Capacity );
            m_Capacity = other.m_Capacity;
            m_Size = other.m_Size;
            m_Head = 0;
        }
    }

    RingBuffer( RingBuffer&& other ) noexcept
        : RingBuffer()
    {
        _swap( other );
    }

    ~RingBuffer()
    {
        delete[] m_Elements;
    }

    void resize( size_t capacity )
    {
        T* elements = new T[ capacity ];
        _copy_to( elements, m_Elements, m_Head, m_Size, m_Capacity );
        delete[] m_Elements;
        m_Elements = elements;
        m_Capacity = capacity;
        m_Head = 0;
    }

    void push_back( const T& value )
    {
        assert( m_Capacity );
        m_Elements[ _last() ] = value;
        if( m_Size == m_Capacity ) m_Head++;
        if( m_Size < m_Capacity ) m_Size++;
    }

    void push_back( T&& value )
    {
        assert( m_Capacity );
        m_Elements[ _last() ] = std::move( value );
        if( m_Size == m_Capacity ) m_Head++;
        if( m_Size < m_Capacity ) m_Size++;
    }

    void pop_back()
    {
        if( m_Size > 0 ) m_Size--;
    }

    void clear()
    {
        m_Size = 0;
        m_Head = 0;
    }

    T& front()
    {
        assert( m_Elements && m_Size );
        return m_Elements[ _first() ];
    }

    const T& front() const
    {
        assert( m_Elements && m_Size );
        return m_Elements[ _first() ];
    }

    T& back()
    {
        assert( m_Elements && m_Size );
        return m_Elements[ _last() ];
    }

    const T& back() const
    {
        assert( m_Elements && m_Size );
        return m_Elements[ _last() ];
    }

    size_t capacity() const
    {
        return m_Capacity;
    }

    size_t size() const
    {
        return m_Size;
    }

    bool empty() const
    {
        return m_Size == 0;
    }

    T& at( size_t index )
    {
        assert( m_Elements && m_Size );
        return m_Elements[ (m_Head + index) % m_Capacity ];
    }

    const T& at( size_t index ) const
    {
        assert( m_Elements && m_Size );
        return m_Elements[ (m_Head + index) % m_Capacity ];
    }

    T& operator[]( size_t index )
    {
        return at( index );
    }

    const T& operator[]( size_t index ) const
    {
        return at( index );
    }

    template<typename _t>
    class _iterator
    {
    public:
        bool operator==( const _iterator& other ) const
        {
            return (m_pElements == other.m_pElements) && (m_Index == other.m_Index);
        }

        bool operator!=( const _iterator& other ) const
        {
            return (m_pElements != other.m_pElements) || (m_Index != other.m_Index);
        }

        _iterator& operator++()
        {
            m_Index = (m_Index + 1) % m_Capacity;
            return *this;
        }

        _iterator operator++( int )
        {
            _iterator prev( *this );
            this->operator++();
            return prev;
        }

        _t& operator*() const
        {
            return m_pElements[ m_Index ];
        }

    private:
        _t* m_pElements;
        size_t m_Index;
        size_t m_Capacity;

        _iterator( _t* pElements, size_t index, size_t capacity )
            : m_Elements( pElements )
            , m_Index( index )
            , m_Capacity( capacity )
        {}

        friend class RingBuffer;
    };

    using iterator = _iterator<T>;
    using const_iterator = _iterator<const T>;

    iterator begin() { return iterator( m_Elements, _first(), m_Capacity ); }
    iterator end() { return iterator( m_Elements, _last(), m_Capacity ); }

    const_iterator begin() const { return const_iterator( m_Elements, _first(), m_Capacity ); }
    const_iterator end() const { return const_iterator( m_Elements, _last(), m_Capacity ); }

    RingBuffer& operator=( const RingBuffer& other )
    {
        RingBuffer( other )._swap( *this );
        return *this;
    }

    RingBuffer& operator=( RingBuffer&& other ) noexcept
    {
        RingBuffer( std::move( other ) )._swap( *this );
        return *this;
    }

private:
    T*     m_Elements;
    size_t m_Size;
    size_t m_Head;
    size_t m_Capacity;

    size_t _first() const { return m_Head; }
    size_t _last() const { return (m_Head + m_Size) % m_Capacity; }

    void _swap( RingBuffer& other )
    {
        std::swap( m_Elements, other.m_Elements );
        std::swap( m_Capacity, other.m_Capacity );
        std::swap( m_Size, other.m_Size );
        std::swap( m_Head, other.m_Head );
    }

    static void _copy_to( T* dst, T* src, size_t shead, size_t ssize, size_t scap )
    {
        if( !ssize ) return;
        size_t di = 0, si = shead;
        for( ; si < scap; ++si, ++di )        dst[ di ] = src[ si ];
        for( si = 0; di < ssize; ++si, ++di ) dst[ di ] = src[ si ];
    }
};

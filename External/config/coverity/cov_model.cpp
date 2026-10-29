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

namespace std
{
    struct adopt_lock_t;
    struct defer_lock_t;

    template<typename Mutex>
    class unique_lock
    {
        bool m_Locked;

    public:
        explicit unique_lock( Mutex& m )
        {
            __coverity_lock_alias__( this, &m );
            __coverity_exclusive_lock_acquire__( this );
            m_Locked = true;
        }

        explicit unique_lock( Mutex& m, const adopt_lock_t& )
        {
            __coverity_lock_alias__( this, &m );
            __coverity_assert_locked__( this );
            m_Locked = true;
        }

        explicit unique_lock( Mutex& m, const defer_lock_t& )
        {
            __coverity_lock_alias__( this, &m );
            m_Locked = false;
        }

        ~unique_lock()
        {
            if( m_Locked ) unlock();
        }

        void lock()
        {
            __coverity_exclusive_lock_acquire__( this );
            m_Locked = true;
        }

        void unlock()
        {
            __coverity_exclusive_lock_release__( this );
            m_Locked = false;
        }
    };
}

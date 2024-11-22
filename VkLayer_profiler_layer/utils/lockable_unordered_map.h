// Copyright (c) 2019-2021 Lukasz Stalmirski
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
#include <mutex>
#include <shared_mutex>
#include <unordered_map>

/***********************************************************************************\

Class:
    ConcurrentMap

Description:
    std::(unordered_)map with mutex

\***********************************************************************************/
template<typename KeyType, typename ValueType>
class ConcurrentMap : std::unordered_map<KeyType, ValueType>
{
private:
    using BaseType = std::unordered_map<KeyType, ValueType>;

    mutable std::shared_mutex m_Mtx;

public:
    // Inherit constructors and types
    using BaseType::BaseType;
    using typename BaseType::iterator;
    using typename BaseType::const_iterator;

    // Lock access to the map
    void lock() const { m_Mtx.lock(); }

    // Unlock access to the map
    void unlock() const { m_Mtx.unlock(); }

    // Try to lock access to the map
    bool try_lock() const { return m_Mtx.try_lock(); }

    // Acquire a non-exclusive lock to the map
    void lock_shared() const { m_Mtx.lock_shared(); }

    // Release a non-exclusive lock
    void unlock_shared() const { m_Mtx.unlock_shared(); }

    // Try to acquire a non-exclusive lock to the map
    bool try_lock_shared() const { return m_Mtx.try_lock_shared(); }

    // Remove all elements from the map (thread-safe)
    void clear()
    {
        std::scoped_lock lk( m_Mtx );
        BaseType::clear();
    }

    // Check if collection contains any elements
    bool empty() const
    {
        std::shared_lock lk( m_Mtx );
        return BaseType::empty();
    }

    // Get number of elements in the collection
    size_t size() const
    {
        std::shared_lock lk( m_Mtx );
        return BaseType::size();
    }

    // Insert new value into map (thread-safe)
    void insert( const KeyType& key, const ValueType& value )
    {
        std::scoped_lock lk( m_Mtx );
        BaseType::emplace( key, value );
    }

    // Insert new value into map (thread-safe)
    void insert( const KeyType& key, ValueType&& value )
    {
        std::scoped_lock lk( m_Mtx );
        BaseType::emplace( key, std::move( value ) );
    }

    // Insert new value into map
    void unsafe_insert( const KeyType& key, const ValueType& value )
    {
        BaseType::emplace( key, value );
    }

    // Insert new value into map
    void unsafe_insert( const KeyType& key, ValueType&& value )
    {
        BaseType::emplace( key, std::move( value ) );
    }

    // Remove value at key (thread-safe)
    template <typename Where>
    auto remove(Where&& where)
    {
        std::scoped_lock lk( m_Mtx );
        return BaseType::erase(where);
    }

    // Remove value at iterator
    template <typename Where>
    auto unsafe_remove(Where&& where)
    {
        return BaseType::erase( where );
    }

    // Get value at key (thread-safe)
    ValueType& at( const KeyType& key )
    {
        std::shared_lock lk( m_Mtx );
        return BaseType::at( key );
    }

    // Get value at key
    ValueType& unsafe_at( const KeyType& key )
    {
        return BaseType::at( key );
    }

    // Get value at key (thread-safe)
    const ValueType& at( const KeyType& key ) const
    {
        std::shared_lock lk( m_Mtx );
        return BaseType::at( key );
    }

    // Get value at key
    const ValueType& unsafe_at( const KeyType& key ) const
    {
        return BaseType::at( key );
    }

    // Check if map contains value at key, return that value (thread-safe)
    bool find( const KeyType& key, ValueType* out ) const
    {
        std::shared_lock lk( m_Mtx );
        auto it = BaseType::find( key );
        if( it != BaseType::end() )
        {
            (*out) = it->second;
            return true;
        }
        return false;
    }

    // Check if map contains value at key, return that value
    auto unsafe_find( const KeyType& key ) const
    {
        return BaseType::find( key );
    }

    // Iterators (unsafe)
    iterator begin() { return BaseType::begin(); }
    iterator end() { return BaseType::end(); }
    const_iterator begin() const { return BaseType::begin(); }
    const_iterator end() const { return BaseType::end(); }
    const_iterator cbegin() const { return BaseType::begin(); }
    const_iterator cend() const { return BaseType::end(); }

    // Get the snapshot of the current state of the map.
    BaseType snapshot() const
    {
        std::shared_lock lk( m_Mtx );
        return BaseType( *this );
    }
};

#pragma once
#include <unordered_map>
#include <mutex>

/***********************************************************************************\

Class:
    lockable_unordered_map

Description:
    std::unordered_map with mutex

\***********************************************************************************/
template<typename KeyType, typename ValueType,
    typename HashFn = std::hash<KeyType>,
    typename EqualFn = std::equal_to<KeyType>,
    typename AllocFn = std::allocator<std::pair<const KeyType, ValueType>>>
class LockableUnorderedMap
    : public std::unordered_map<KeyType, ValueType, HashFn, EqualFn, AllocFn>
{
private:
    using BaseType = std::unordered_map<KeyType, ValueType, HashFn, EqualFn, AllocFn>;

    mutable std::mutex m_Mtx;

public:
    // Inherit constructors
    using BaseType::unordered_map;

    // Lock access to the map
    void lock() { m_Mtx.lock(); }

    // Unlock access to the map
    void unlock() { m_Mtx.unlock(); }

    // Try to lock access to the map
    bool try_lock() { return m_Mtx.try_lock(); }

    // Get map's element atomically
    const ValueType& interlocked_at( const KeyType& key ) const
    {
        std::scoped_lock lk( m_Mtx );
        return BaseType::at( key );
    }

    // Get map's element atomically
    ValueType& interlocked_at( const KeyType& key )
    {
        std::scoped_lock lk( m_Mtx );
        return BaseType::at( key );
    }

    // Remove element from map atomically
    size_t interlocked_erase( const KeyType& key )
    {
        std::scoped_lock lk( m_Mtx );
        return BaseType::erase( key );
    }

    // Try to insert new element to map atomically
    template<typename... Args>
    auto interlocked_try_emplace( const KeyType& key, Args... arguments )
    {
        std::scoped_lock lk( m_Mtx );
        return BaseType::try_emplace( key, arguments... );
    }

    // Insert new element to map atomically
    template<typename... Args>
    auto interlocked_emplace( const KeyType& key, Args... arguments )
    {
        std::scoped_lock lk( m_Mtx );
        return BaseType::emplace( key, arguments... );
    }

    // Try to get element
    bool interlocked_find( const KeyType& key, ValueType* out )
    {
        std::scoped_lock lk( m_Mtx );
        auto it = BaseType::find( key );
        if( it != BaseType::end() )
        {
            (*out) = it->second;
            return true;
        }
        return false;
    }
};

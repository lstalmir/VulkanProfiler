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
#include <vulkan/vulkan.h>
#include <vulkan/vk_icd.h>
#include <memory>

template<typename T, typename U, typename... Args>
inline VkResult vk_new( U** ptr, Args&&... args ) noexcept
{
    ( *ptr ) = nullptr;
    try
    {
        ( *ptr ) = new U( nullptr );
        ( *ptr )->m_pImpl = new T( std::forward<Args>( args )... );
        return VK_SUCCESS;
    }
    catch( const std::bad_alloc& )
    {
        delete *ptr;
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
    catch( VkResult result )
    {
        delete *ptr;
        return result;
    }
    catch( ... )
    {
        delete *ptr;
        return VK_ERROR_UNKNOWN;
    }
}

template<typename T, typename U, typename... Args>
inline VkResult vk_new_nondispatchable( U* ptr, Args&&... args ) noexcept
{
    try
    {
        ( *ptr ) = new T( std::forward<Args>( args )... );
        return VK_SUCCESS;
    }
    catch( const std::bad_alloc& )
    {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
    catch( VkResult result )
    {
        return result;
    }
    catch( ... )
    {
        return VK_ERROR_UNKNOWN;
    }
}

inline void vk_check( VkResult result )
{
    if( result != VK_SUCCESS )
    {
        throw result;
    }
}

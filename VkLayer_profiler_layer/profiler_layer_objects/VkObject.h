// Copyright (c) 2019-2025 Lukasz Stalmirski
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
#include <farmhash.h>
#include <vulkan/vulkan.h>
#include <utility>

namespace Profiler
{
    template<typename VkObjectT, bool IsPointer = std::is_pointer_v<VkObjectT>>
    struct VkObjectTraits;

    // VkObject traits for pointer-typed handles
    template<typename VkObjectT>
    struct VkObjectTraits<VkObjectT, true /*IsPointer*/>
    {
        inline static constexpr uint64_t GetObjectHandleAsUint64( VkObjectT object )
        {
            return static_cast<uint64_t>( *reinterpret_cast<uintptr_t*>( &object ) );
        }

        inline static constexpr VkObjectT GetObjectHandleAsVulkanHandle( uint64_t object )
        {
            uintptr_t uintptrObj = static_cast<uintptr_t>( object );
            return *reinterpret_cast<VkObjectT*>( &uintptrObj );
        }
    };

    // VkObject traits for non-pointer-typed handles
    template<typename VkObjectT>
    struct VkObjectTraits<VkObjectT, false /*IsPointer*/>
    {
        inline static constexpr uint64_t GetObjectHandleAsUint64( VkObjectT object )
        {
            return static_cast<uint64_t>( object );
        }

        inline static constexpr VkObjectT GetObjectHandleAsVulkanHandle( uint64_t object )
        {
            return static_cast<VkObjectT>( object );
        }
    };

    /***********************************************************************************\

    Class:
        VkObject_Runtime_Traits

    Description:
        Contains Vulkan handle data accessible in runtime.

    \***********************************************************************************/
    struct VkObjectRuntimeTraits
    {
        const VkObjectType ObjectType                                       = VK_OBJECT_TYPE_UNKNOWN;
        const VkDebugReportObjectTypeEXT DebugReportObjectType              = VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT;
        const char* ObjectTypeName                                          = "Unknown object type";
        const bool ShouldHaveDebugName                                      = false;

        inline VkObjectRuntimeTraits() = default;

        inline constexpr VkObjectRuntimeTraits(
            VkObjectType objectType,
            VkDebugReportObjectTypeEXT debugReportObjectType,
            const char* pObjectTypeName,
            bool shouldHaveDebugName )
            : ObjectType( objectType )
            , DebugReportObjectType( debugReportObjectType )
            , ObjectTypeName( pObjectTypeName )
            , ShouldHaveDebugName( shouldHaveDebugName )
        {
        }

        inline static constexpr VkObjectRuntimeTraits FromObjectType( VkObjectType objectType )
        {
            if( objectType != VK_OBJECT_TYPE_UNKNOWN )
            {
            #define VK_OBJECT_FN( TYPE, OBJECT_TYPE, DEBUG_REPORT_OBJECT_TYPE, SHOULD_HAVE_DEBUG_NAME ) \
                if( objectType == OBJECT_TYPE )                                                         \
                {                                                                                       \
                    return VkObjectRuntimeTraits(                                                       \
                        OBJECT_TYPE,                                                                    \
                        DEBUG_REPORT_OBJECT_TYPE,                                                       \
                        #TYPE,                                                                          \
                        SHOULD_HAVE_DEBUG_NAME );                                                       \
                }

            // Define runtime traits for each Vulkan object
            #include "VkObject_Types.inl"
            }

            return VkObjectRuntimeTraits();
        }

        inline static constexpr VkObjectRuntimeTraits FromObjectType( VkDebugReportObjectTypeEXT objectType )
        {
            if( objectType != VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT )
            {
            #define VK_OBJECT_FN( TYPE, OBJECT_TYPE, DEBUG_REPORT_OBJECT_TYPE, SHOULD_HAVE_DEBUG_NAME ) \
                if( objectType == DEBUG_REPORT_OBJECT_TYPE )                                            \
                {                                                                                       \
                    return VkObjectRuntimeTraits(                                                       \
                        OBJECT_TYPE,                                                                    \
                        DEBUG_REPORT_OBJECT_TYPE,                                                       \
                        #TYPE,                                                                          \
                        SHOULD_HAVE_DEBUG_NAME );                                                       \
                }

            // Define runtime traits for each Vulkan object
            #include "VkObject_Types.inl"
            }

            return VkObjectRuntimeTraits();
        }
    };

    /***********************************************************************************\

    Class:
        VkObject

    Description:
        Common wrapper for all Vulkan handles with additional metadata.

    \***********************************************************************************/
    struct VkObject
    {
        uint64_t m_Handle;
        uint32_t m_CreateTime;
        VkObjectType m_Type;

        inline constexpr VkObject( decltype( VK_NULL_HANDLE ) = VK_NULL_HANDLE )
            : m_Handle( 0 )
            , m_CreateTime( 0 )
            , m_Type( VK_OBJECT_TYPE_UNKNOWN )
        {
        }

        template<typename VkObjectT, typename VkObjectTypeEnumT>
        inline constexpr VkObject( VkObjectT object, VkObjectTypeEnumT objectType, uint32_t createTime = 0 )
            : VkObject( object, VkObjectRuntimeTraits::FromObjectType( objectType ), createTime )
        {
        }

        template<typename VkObjectT>
        inline constexpr VkObject( VkObjectT object, const VkObjectRuntimeTraits& traits, uint32_t createTime = 0 )
            : m_Handle( VkObjectTraits<VkObjectT>::GetObjectHandleAsUint64( object ) )
            , m_CreateTime( createTime )
            , m_Type( traits.ObjectType )
        {
        }

        inline constexpr uint64_t GetHandleAsUint64() const
        {
            return m_Handle;
        }

        inline constexpr bool operator==( decltype( VK_NULL_HANDLE ) ) const
        {
            return m_Handle == 0;
        }

        inline constexpr bool operator!=( decltype( VK_NULL_HANDLE ) ) const
        {
            return m_Handle != 0;
        }

        inline constexpr bool operator==( const VkObject& rh ) const
        {
            return ( m_Handle == rh.m_Handle ) && ( m_CreateTime == rh.m_CreateTime ) && ( m_Type == rh.m_Type );
        }

        inline constexpr bool operator!=( const VkObject& rh ) const
        {
            return !( *this == rh );
        }
    };
    
    #define VK_OBJECT_FN( TYPE, OBJECT_TYPE, DEBUG_REPORT_OBJECT_TYPE, SHOULD_HAVE_DEBUG_NAME ) \
        struct TYPE##Handle : public VkObject                                                   \
        {                                                                                       \
            inline constexpr TYPE##Handle( decltype( VK_NULL_HANDLE ) = VK_NULL_HANDLE )        \
                : VkObject( nullptr )                                                           \
            {                                                                                   \
            }                                                                                   \
                                                                                                \
            inline constexpr TYPE##Handle( TYPE object, uint64_t createTime = 0 )               \
                : VkObject( object, OBJECT_TYPE, createTime )                                   \
            {                                                                                   \
            }                                                                                   \
                                                                                                \
            inline constexpr TYPE##Handle( const VkObject& object )                             \
                : VkObject( object )                                                            \
            {                                                                                   \
                assert( m_Type == OBJECT_TYPE );                                                \
            }                                                                                   \
                                                                                                \
            inline TYPE GetVulkanHandle() const                                                 \
            {                                                                                   \
                return VkObjectTraits<TYPE>::GetObjectHandleAsVulkanHandle( m_Handle );         \
            }                                                                                   \
                                                                                                \
            inline operator TYPE() const                                                        \
            {                                                                                   \
                return GetVulkanHandle();                                                       \
            }                                                                                   \
        };

    #include "VkObject_Types.inl"
}

template<>
struct std::hash<Profiler::VkObject>
{
    inline size_t operator()( const Profiler::VkObject& obj ) const
    {
        return Farmhash::Hash(
            reinterpret_cast<const char*>( &obj ),
            sizeof( obj ) );
    }
};

#define VK_OBJECT_FN( TYPE, OBJECT_TYPE, DEBUG_REPORT_OBJECT_TYPE, SHOULD_HAVE_DEBUG_NAME ) \
    template<>                                                                              \
    struct std::hash<Profiler::TYPE##Handle>                                                \
    {                                                                                       \
        inline size_t operator()( const Profiler::TYPE##Handle& obj ) const                 \
        {                                                                                   \
            return Farmhash::Hash(                                                          \
                reinterpret_cast<const char*>( &obj ),                                      \
                sizeof( obj ) );                                                            \
        }                                                                                   \
    };

#include "VkObject_Types.inl"

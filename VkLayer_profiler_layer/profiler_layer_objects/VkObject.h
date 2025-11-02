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
    namespace Detail
    {
        template<typename VkObjectT>
        static constexpr VkObjectType GetCompileTimeObjectType()
        {
        #define VK_OBJECT_FN( TYPE, OBJECT_TYPE, DEBUG_REPORT_OBJECT_TYPE, SHOULD_HAVE_DEBUG_NAME ) \
            if constexpr( std::is_pointer_v<VkObjectT> && std::is_same_v<VkObjectT, TYPE> )         \
            {                                                                                       \
                return OBJECT_TYPE;                                                                 \
            }

        #include "VkObject_Types.inl"

            return VK_OBJECT_TYPE_UNKNOWN;
        }
    }

    template<typename VkObjectT, bool IsPointer = std::is_pointer_v<VkObjectT>>
    struct VkObject_Traits;

    // VkObject traits for pointer-typed handles
    template<typename VkObjectT>
    struct VkObject_Traits<VkObjectT, true /*IsPointer*/>
    {
        inline static constexpr uint64_t GetObjectHandleAsUint64( VkObjectT object )
        {
            return static_cast<uint64_t>(*reinterpret_cast<uintptr_t*>(&object));
        }

        inline static constexpr VkObjectT GetObjectHandleAsVulkanHandle( uint64_t object )
        {
            uintptr_t uintptrObj = static_cast<uintptr_t>(object);
            return *reinterpret_cast<VkObjectT*>(&uintptrObj);
        }

        inline static constexpr VkObjectType GetCompileTimeObjectType()
        {
            return Detail::GetCompileTimeObjectType<VkObjectT>();
        }
    };

    // VkObject traits for non-pointer-typed handles
    template<typename VkObjectT>
    struct VkObject_Traits<VkObjectT, false /*IsPointer*/>
    {
        inline static constexpr uint64_t GetObjectHandleAsUint64( VkObjectT object )
        {
            return static_cast<uint64_t>(object);
        }

        inline static constexpr VkObjectT GetObjectHandleAsVulkanHandle( uint64_t object )
        {
            return static_cast<VkObjectT>(object);
        }

        inline static constexpr VkObjectType GetCompileTimeObjectType()
        {
            return Detail::GetCompileTimeObjectType<VkObjectT>();
        }
    };

    /***********************************************************************************\

    Class:
        VkObject_Runtime_Traits

    Description:
        Contains Vulkan handle data accessible in runtime.

    \***********************************************************************************/
    struct VkObject_Runtime_Traits
    {
        const VkObjectType ObjectType                                       = VK_OBJECT_TYPE_UNKNOWN;
        const VkDebugReportObjectTypeEXT DebugReportObjectType              = VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT;
        const char* ObjectTypeName                                          = "Unknown object type";
        const bool ShouldHaveDebugName                                      = false;

        inline VkObject_Runtime_Traits() = default;

        inline constexpr VkObject_Runtime_Traits(
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

        inline static constexpr VkObject_Runtime_Traits FromObjectType( VkObjectType objectType )
        {
            if( objectType != VK_OBJECT_TYPE_UNKNOWN )
            {
            #define VK_OBJECT_FN( TYPE, OBJECT_TYPE, DEBUG_REPORT_OBJECT_TYPE, SHOULD_HAVE_DEBUG_NAME ) \
                if( objectType == OBJECT_TYPE )                                                         \
                {                                                                                       \
                    return VkObject_Runtime_Traits(                                                     \
                        OBJECT_TYPE,                                                                    \
                        DEBUG_REPORT_OBJECT_TYPE,                                                       \
                        #TYPE,                                                                          \
                        SHOULD_HAVE_DEBUG_NAME );                                                       \
                }

            // Define runtime traits for each Vulkan object
            #include "VkObject_Types.inl"
            }

            return VkObject_Runtime_Traits();
        }

        inline static constexpr VkObject_Runtime_Traits FromObjectType( VkDebugReportObjectTypeEXT objectType )
        {
            if( objectType != VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT )
            {
            #define VK_OBJECT_FN( TYPE, OBJECT_TYPE, DEBUG_REPORT_OBJECT_TYPE, SHOULD_HAVE_DEBUG_NAME ) \
                if( objectType == DEBUG_REPORT_OBJECT_TYPE )                                            \
                {                                                                                       \
                    return VkObject_Runtime_Traits(                                                     \
                        OBJECT_TYPE,                                                                    \
                        DEBUG_REPORT_OBJECT_TYPE,                                                       \
                        #TYPE,                                                                          \
                        SHOULD_HAVE_DEBUG_NAME );                                                       \
                }

            // Define runtime traits for each Vulkan object
            #include "VkObject_Types.inl"
            }

            return VkObject_Runtime_Traits();
        }
    };

    /***********************************************************************************\

    Class:
        VkObjectHandle

    Description:
        Common wrapper for all Vulkan handles with additional metadata.

    \***********************************************************************************/
    template<typename VkObjectT>
    struct VkObjectHandle
    {
        VkObjectT m_Handle;
        uint32_t m_CreateTime;
        VkObjectType m_Type;

        inline constexpr VkObjectHandle( std::nullptr_t = nullptr )
            : m_Handle( 0 )
            , m_CreateTime( 0 )
            , m_Type( VK_OBJECT_TYPE_UNKNOWN )
        {
        }

        inline constexpr VkObjectHandle(
                VkObjectT object,
                VkObjectType type = VK_OBJECT_TYPE_UNKNOWN,
                uint64_t timestamp = 0 )
            : m_Handle( object )
            , m_CreateTime( static_cast<uint32_t>( timestamp ) )
            , m_Type( type )
        {
        }

        inline constexpr bool operator==( const VkObjectT& rh ) const
        {
            return ( m_Handle == rh );
        }

        inline constexpr bool operator!=( const VkObjectT& rh ) const
        {
            return !( *this == rh );
        }

        inline constexpr bool operator==( const VkObjectHandle& rh ) const
        {
            return ( m_Handle == rh.m_Handle ) && ( m_CreateTime == rh.m_CreateTime ) && ( m_Type == rh.m_Type );
        }

        inline constexpr bool operator!=( const VkObjectHandle& rh ) const
        {
            return !( *this == rh );
        }

        inline constexpr operator VkObjectT() const
        {
            return m_Handle;
        }

        inline constexpr uint64_t GetHandleAsUint64() const
        {
            return VkObject_Traits<VkObjectT>::GetObjectHandleAsUint64( m_Handle );
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

        inline constexpr VkObject( std::nullptr_t = nullptr )
            : m_Handle( 0 )
            , m_CreateTime( 0 )
            , m_Type( VK_OBJECT_TYPE_UNKNOWN )
        {
        }

        template<typename VkObjectT>
        inline constexpr VkObject( const VkObjectHandle<VkObjectT>& object )
            : m_Handle( VkObject_Traits<VkObjectT>::GetObjectHandleAsUint64( object.m_Handle ) )
            , m_CreateTime( object.m_CreateTime )
            , m_Type( object.m_Type )
        {
        }

        template<typename VkObjectT, typename VkObjectTypeEnumT>
        inline constexpr VkObject( VkObjectT object, VkObjectTypeEnumT objectType )
            : VkObject( object, VkObject_Runtime_Traits::FromObjectType( objectType ) )
        {
        }

        template<typename VkObjectT>
        inline constexpr VkObject( VkObjectT object, const VkObject_Runtime_Traits& traits )
            : m_Handle( VkObject_Traits<VkObjectT>::GetObjectHandleAsUint64( object ) )
            , m_CreateTime( 0 )
            , m_Type( traits.ObjectType )
        {
        }

        template<typename VkObjectT>
        inline constexpr VkObjectHandle<VkObjectT> GetHandle() const
        {
            return VkObjectHandle<VkObjectT>(
                VkObject_Traits<VkObjectT>::GetObjectHandleAsVulkanHandle( m_Handle ), m_Type, m_CreateTime );
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
}

template<typename VkObjectT>
struct std::hash<Profiler::VkObjectHandle<VkObjectT>>
{
    inline size_t operator()( const Profiler::VkObjectHandle<VkObjectT>& obj ) const
    {
        const Profiler::VkObject object( obj );
        return Farmhash::Hash(
            reinterpret_cast<const char*>( &object ),
            sizeof( object ) );
    }
};

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

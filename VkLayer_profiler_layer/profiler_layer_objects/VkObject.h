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
#include <vulkan/vulkan.h>
#include <utility>

namespace Profiler
{
    template<typename VkObjectT, bool IsPointer>
    struct VkObject_Traits_Base;

    // VkObject_Traits_Base for pointer-typed handles
    template<typename VkObjectT>
    struct VkObject_Traits_Base<VkObjectT, true /*IsPointer*/>
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
    };

    // VkObject_Traits_Base for non-pointer-typed handles
    template<typename VkObjectT>
    struct VkObject_Traits_Base<VkObjectT, false /*IsPointer*/>
    {
        inline static constexpr uint64_t GetObjectHandleAsUint64( VkObjectT object )
        {
            return static_cast<uint64_t>(object);
        }

        inline static constexpr VkObjectT GetObjectHandleAsVulkanHandle( uint64_t object )
        {
            return static_cast<VkObjectT>(object);
        }
    };

    /***********************************************************************************\

    Class:
        VkObject_Traits

    Description:
        Contains Vulkan handle data accessible during compilation.

    \***********************************************************************************/
    template<typename VkObjectT>
    struct VkObject_Traits : VkObject_Traits_Base<VkObjectT, std::is_pointer_v<VkObjectT>>
    {
        using Base = VkObject_Traits_Base<VkObjectT, std::is_pointer_v<VkObjectT>>;
        using Base::GetObjectHandleAsUint64;
        using Base::GetObjectHandleAsVulkanHandle;
        static constexpr VkObjectType ObjectType                            = VK_OBJECT_TYPE_UNKNOWN;
        static constexpr VkDebugReportObjectTypeEXT DebugReportObjectType   = VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT;
        static constexpr const char ObjectTypeName[]                        = "Unknown object type";
        static constexpr bool ShouldHaveDebugName                           = false;
    };
    
    #define VK_OBJECT_FN( TYPE, OBJECT_TYPE, DEBUG_REPORT_OBJECT_TYPE, SHOULD_HAVE_DEBUG_NAME )         \
    template<> struct VkObject_Traits<TYPE> : VkObject_Traits_Base<TYPE, std::is_pointer_v<TYPE>> {     \
        using Base = VkObject_Traits_Base<TYPE, std::is_pointer_v<TYPE>>;                               \
        using Base::GetObjectHandleAsUint64;                                                            \
        using Base::GetObjectHandleAsVulkanHandle;                                                      \
        static constexpr VkObjectType ObjectType                            = OBJECT_TYPE;              \
        static constexpr VkDebugReportObjectTypeEXT DebugReportObjectType   = DEBUG_REPORT_OBJECT_TYPE; \
        static constexpr const char ObjectTypeName[]                        = #TYPE;                    \
        static constexpr bool ShouldHaveDebugName                           = SHOULD_HAVE_DEBUG_NAME; };

    // Define compile-time traits for each Vulkan object
    #include "VkObject_Types.inl"

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

        inline VkObject_Runtime_Traits(
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

        template<typename VkObjectT>
        inline static VkObject_Runtime_Traits FromHandleType()
        {
            return VkObject_Runtime_Traits(
                VkObject_Traits<VkObjectT>::ObjectType,
                VkObject_Traits<VkObjectT>::DebugReportObjectType,
                VkObject_Traits<VkObjectT>::ObjectTypeName,
                VkObject_Traits<VkObjectT>::ShouldHaveDebugName );
        }

        inline static VkObject_Runtime_Traits FromObjectType( VkObjectType objectType )
        {
            const static std::unordered_map<VkObjectType, VkObject_Runtime_Traits> runtimeTraits =
            {
                #define VK_OBJECT_FN( TYPE, OBJECT_TYPE, DEBUG_REPORT_OBJECT_TYPE, SHOULD_HAVE_DEBUG_NAME ) \
                { OBJECT_TYPE, VkObject_Runtime_Traits::FromHandleType<TYPE>() },

                // Define runtime traits for each Vulkan object
                #include "VkObject_Types.inl"
            };

            auto it = runtimeTraits.find( objectType );
            if( it != runtimeTraits.end() )
            {
                return it->second;
            }

            return VkObject_Runtime_Traits();
        }

        inline static VkObject_Runtime_Traits FromObjectType( VkDebugReportObjectTypeEXT objectType )
        {
            const static std::unordered_map<VkDebugReportObjectTypeEXT, VkObject_Runtime_Traits> runtimeTraits =
            {
                #define VK_OBJECT_FN( TYPE, OBJECT_TYPE, DEBUG_REPORT_OBJECT_TYPE, SHOULD_HAVE_DEBUG_NAME ) \
                { DEBUG_REPORT_OBJECT_TYPE, VkObject_Runtime_Traits::FromHandleType<TYPE>() },

                // Define runtime traits for each Vulkan object
                #include "VkObject_Types.inl"
            };

            auto it = runtimeTraits.find( objectType );
            if( it != runtimeTraits.end() )
            {
                return it->second;
            }

            return VkObject_Runtime_Traits();
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
        VkObjectType m_Type;
        const char* m_pTypeName;

        inline VkObject()
            : m_Handle( 0 )
            , m_Type( VK_OBJECT_TYPE_UNKNOWN )
            , m_pTypeName( "Unknown object type" )
        {
        }

        template<typename VkObjectT>
        inline VkObject( VkObjectT object )
            : m_Handle( VkObject_Traits<VkObjectT>::GetObjectHandleAsUint64( object ) )
            , m_Type( VkObject_Traits<VkObjectT>::ObjectType )
            , m_pTypeName( VkObject_Traits<VkObjectT>::ObjectTypeName )
        {
        }

        template<typename VkObjectTypeEnumT>
        inline VkObject( uint64_t object, VkObjectTypeEnumT objectType )
            : VkObject( object, VkObject_Runtime_Traits::FromObjectType( objectType ) )
        {
        }

        inline VkObject( uint64_t object, const VkObject_Runtime_Traits& traits )
            : m_Handle( object )
            , m_Type( traits.ObjectType )
            , m_pTypeName( traits.ObjectTypeName )
        {
        }

        inline bool operator==( const VkObject& rh ) const
        {
            return (m_Handle == rh.m_Handle) && (m_Type == rh.m_Type);
        }
    };
}

template<>
struct std::hash<Profiler::VkObject>
{
    inline size_t operator()( const Profiler::VkObject& obj ) const
    {
        // Combine handle value and object type using boost way
        return static_cast<size_t>(obj.m_Handle ^ (
            (static_cast<uint64_t>(obj.m_Type)) +
            (0x9e3779b97f4a7c16) +
            (obj.m_Handle << 6) +
            (obj.m_Handle >> 2)));
    }
};

#pragma once
#include <array>
#include <stdint.h>
#include <vulkan/vulkan.h>

namespace Profiler
{
    struct ProfilerShaderTuple
    {
        uint32_t m_Hash = 0;

        uint32_t m_Vert = 0;
        uint32_t m_Tesc = 0;
        uint32_t m_Tese = 0;
        uint32_t m_Geom = 0;
        uint32_t m_Frag = 0;
        uint32_t m_Comp = 0;

        inline constexpr bool operator==( const ProfilerShaderTuple& rh ) const
        {
            return m_Hash == rh.m_Hash;
        }

        inline constexpr bool operator!=( const ProfilerShaderTuple& rh ) const
        {
            return !operator==( rh );
        }
    };
}


namespace std
{
    template<>
    struct hash<Profiler::ProfilerShaderTuple>
    {
        inline size_t operator()( const Profiler::ProfilerShaderTuple& tuple ) const
        {
            return tuple.m_Hash;
        }
    };
}

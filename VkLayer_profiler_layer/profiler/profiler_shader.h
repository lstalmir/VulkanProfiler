#pragma once
#include <array>
#include <stdint.h>
#include <vulkan.h>

namespace Profiler
{
    struct ProfilerShaderTuple
    {
        uint32_t m_Vert = 0;
        uint32_t m_Tesc = 0;
        uint32_t m_Tese = 0;
        uint32_t m_Geom = 0;
        uint32_t m_Frag = 0;
        uint32_t m_Comp = 0;

        inline constexpr bool operator==( const ProfilerShaderTuple& rh ) const
        {
            return m_Frag == rh.m_Frag
                && m_Vert == rh.m_Vert
                && m_Comp == rh.m_Comp
                && m_Geom == rh.m_Geom
                && m_Tesc == rh.m_Tesc
                && m_Tese == rh.m_Tese;
        }

        inline constexpr bool operator!=( const ProfilerShaderTuple& rh ) const
        {
            return !operator==( rh );
        }
    };
}

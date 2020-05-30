#pragma once
#include <vulkan/vulkan.h>

namespace Profiler
{
    /***********************************************************************************\

    Enumeration:
        ProfilerDrawcallType

    Description:
        Profiled drawcall types.

    \***********************************************************************************/
    enum class ProfilerDrawcallType
    {
        eDraw,
        eDispatch,
        eCopy,
        eClear,
        eResolve
    };

    /***********************************************************************************\

    Structure:
        ProfilerDrawcall

    Description:
        Contains data collected per-drawcall.

    \***********************************************************************************/
    struct ProfilerDrawcall
    {
        ProfilerDrawcallType m_Type;
        uint64_t m_Ticks;
    };
}

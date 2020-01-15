#pragma once
#include <stdint.h>

namespace Profiler
{
    /***********************************************************************************\

    Enum:
        ProfilerMode

    Description:
        Profiling frequency

    \***********************************************************************************/
    enum class ProfilerMode : uint32_t
    {
        ePerDrawcall,
        ePerPipeline,
        ePerRenderPass,
        ePerFrame
    };
}

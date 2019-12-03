#pragma once

namespace Profiler
{
    /***********************************************************************************\

    Enum:
        ProfilerMode

    Description:
        Profiling frequency

    \***********************************************************************************/
    enum class ProfilerMode
    {
        ePerDrawcall,
        ePerPipeline,
        ePerRenderPass,
        ePerFrame
    };
}

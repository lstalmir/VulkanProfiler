#pragma once
#include "counters.h"

namespace Profiler
{
    /***********************************************************************************\

    Structure:
        FrameStats

    Description:
        Per-frame API call statistics.

    \***********************************************************************************/
    struct FrameStats
    {
        uint64_t drawCount;
        uint64_t indirectDrawCount;
        uint64_t dispatchCount;
        uint64_t indirectDispatchCount;
        uint64_t copyCount;
        uint64_t submitCount;
    };
}

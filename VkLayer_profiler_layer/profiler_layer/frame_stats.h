#pragma once
#include "counters.h"
#include <atomic>

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
        std::atomic_uint64_t drawCount;
        std::atomic_uint64_t indirectDrawCount;
        std::atomic_uint64_t dispatchCount;
        std::atomic_uint64_t indirectDispatchCount;
        std::atomic_uint64_t copyCount;
        std::atomic_uint64_t submitCount;

        // Reset all frame stat counters
        inline void Reset()
        {
            drawCount = 0;
            indirectDrawCount = 0;
            dispatchCount = 0;
            indirectDispatchCount = 0;
            copyCount = 0;
            submitCount = 0;
        }
    };
}

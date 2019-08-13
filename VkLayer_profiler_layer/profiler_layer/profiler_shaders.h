#pragma once
#include "shaders/profiler_overlay_draw_stats_input.h"

namespace Profiler
{
    struct ProfilerShaders
    {
        inline static
#       include "profiler_shaders.generated.h"
    };
}



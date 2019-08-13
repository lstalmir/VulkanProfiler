#define INCLUDED_FROM_HLSL

// Import input/output structures
#include "profiler_overlay_draw_stats_input.h"

float4 main( PixelShaderInput input ) : SV_Target0
{
    return input.position;
}

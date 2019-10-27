#pragma once
#include "shaders/profiler_overlay_draw_stats_input.h"
#include "profiler_overlay_draw_stats_shaders.generated.h"
#include <unordered_map>

namespace Profiler
{
    enum class ProfilerShaderType
    {
        profiler_overlay_draw_stats_frag,
        profiler_overlay_draw_stats_vert,
        num_shaders
    };

#define DEFINE_PROFILER_SHADER_MAP_ENTRY( NAME ) \
    { ProfilerShaderType::NAME, { \
        static_cast<uint32_t>(sizeof( ProfilerShaders::NAME )), \
        ProfilerShaders::NAME } }

    inline static const std::unordered_map<ProfilerShaderType, std::pair<uint32_t, const uint32_t*>> ProfilerShadersMap =
    {
        DEFINE_PROFILER_SHADER_MAP_ENTRY( profiler_overlay_draw_stats_frag ),
        DEFINE_PROFILER_SHADER_MAP_ENTRY( profiler_overlay_draw_stats_vert )
    };
}

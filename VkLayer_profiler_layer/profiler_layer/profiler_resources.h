#pragma once
#include "profiler_resources.generated.h"
#include <unordered_map>

namespace Profiler
{
    enum class ProfilerResourceType
    {
        profiler_font_glyphs,
        num_resources
    };

#define DEFINE_PROFILER_RESOURCE_MAP_ENTRY( NAME ) \
    { ProfilerResourceType::NAME, { \
        sizeof( ProfilerResources::NAME ), \
        ProfilerResources::NAME } }

    inline static const std::unordered_map<ProfilerResourceType, std::pair<uint32_t, const uint8_t*>> ProfilerResourcesMap =
    {
        DEFINE_PROFILER_RESOURCE_MAP_ENTRY( profiler_font_glyphs )
    };
}

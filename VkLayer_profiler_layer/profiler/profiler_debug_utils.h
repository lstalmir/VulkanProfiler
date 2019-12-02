#pragma once
#include <map>
#include <stdint.h>
#include <string>

namespace Profiler
{
    class ProfilerDebugUtils
    {
    public:
        ProfilerDebugUtils();

        void SetDebugObjectName( uint64_t, const char* );

        std::string GetDebugObjectName( uint64_t );

    protected:
        std::map<uint64_t, std::string> m_ObjectNames;
    };
}

#include "profiler_debug_utils.h"
#include "profiler_helpers.h"

namespace Profiler
{
    ProfilerDebugUtils::ProfilerDebugUtils()
        : m_ObjectNames()
    {
    }

    void ProfilerDebugUtils::SetDebugObjectName( uint64_t objectHandle, const char* pObjectName )
    {
        m_ObjectNames.insert_or_assign( objectHandle, pObjectName );
    }

    std::string ProfilerDebugUtils::GetDebugObjectName( uint64_t objectHandle )
    {
        auto it = m_ObjectNames.find( objectHandle );
        if( it == m_ObjectNames.end() )
        {
            char unnamedObjectName[20] = "0x";
            u64tohex( unnamedObjectName + 2, objectHandle );
            return unnamedObjectName;
        }
        return (it->second).c_str();
    }
}

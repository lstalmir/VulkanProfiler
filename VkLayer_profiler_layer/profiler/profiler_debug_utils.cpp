#include "profiler_debug_utils.h"

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
            _ui64toa_s( objectHandle, unnamedObjectName + 2, 18, 16 );
            return unnamedObjectName;
        }
        return (it->second).c_str();
    }
}

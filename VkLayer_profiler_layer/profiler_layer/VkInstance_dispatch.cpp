#include "VkInstance_dispatch.h"

namespace Profiler
{
    VkInstance_Dispatch InstanceDispatch;

    /***************************************************************************************\

    Function:
        GetInstanceDispatchTable

    Description:
        Retrieves LayerDispatchTable from the dispatcher object.

    \***************************************************************************************/
    VkInstance_LayerDispatchTable& VkInstance_Dispatch::GetInstanceDispatchTable(
        VkInstance instance )
    {
        std::unique_lock<std::mutex> lk( m_InstanceDispatchMtx );

        auto it = m_InstanceDispatch.find( instance );
        if( it == m_InstanceDispatch.end() )
        {
            // TODO error
        }

        return it->second;
    }

    /***************************************************************************************\

    Function:
        CreateInstanceDispatchTable

    Description:
        Creates new LayerDispatchTable and stores it in the dispatcher object.

    \***************************************************************************************/
    VkInstance_LayerDispatchTable& VkInstance_Dispatch::CreateInstanceDispatchTable(
        VkInstance instance,
        PFN_vkGetInstanceProcAddr gpa )
    {
        std::unique_lock<std::mutex> lk( m_InstanceDispatchMtx );

        auto it = m_InstanceDispatch.try_emplace( instance,
            VkInstance_LayerDispatchTable( instance, gpa ) );

        if( !it.second )
        {
            // TODO error, should have created new value
        }

        return it.first->second;
    }

    /***************************************************************************************\

    Function:
        DestroyInstanceDispatchTable

    Description:
        Removes LayerDispatchTable from the dispatcher object.

    \***************************************************************************************/
    void VkInstance_Dispatch::DestroyInstanceDispatchTable(
        VkInstance instance )
    {
        std::unique_lock<std::mutex> lk( m_InstanceDispatchMtx );
        m_InstanceDispatch.erase( instance );
    }
}

// Copyright (c) 2019-2021 Lukasz Stalmirski
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "profiler_sync.h"
#include "profiler_helpers.h"
#include <assert.h>

#ifdef WIN32
#define VK_TIME_DOMAIN_HIGH_RESOLUTION_CLOCK_NATIVE VK_TIME_DOMAIN_QUERY_PERFORMANCE_COUNTER_EXT
#else
#define VK_TIME_DOMAIN_HIGH_RESOLUTION_CLOCK_NATIVE VK_TIME_DOMAIN_CLOCK_MONOTONIC_EXT
#endif

namespace Profiler
{
    /***********************************************************************************\

    Function:
        DeviceProfilerSynchronization

    Description:
        Constructor.

    \***********************************************************************************/
    DeviceProfilerSynchronization::DeviceProfilerSynchronization()
        : m_pDevice( nullptr )
        , m_pfnGetCalibratedTimestamps( nullptr )
    {
    }

    /***********************************************************************************\

    Function:
        Initialize

    Description:
        Setup resources needed for device synchronization.

    \***********************************************************************************/
    VkResult DeviceProfilerSynchronization::Initialize( VkDevice_Object* pDevice )
    {
        assert( !m_pDevice );
        m_pDevice = pDevice;
        m_pfnGetCalibratedTimestamps = nullptr;

        // Use either KHR or EXT version of the extension.
        if( m_pDevice->EnabledExtensions.count( VK_KHR_CALIBRATED_TIMESTAMPS_EXTENSION_NAME ) )
        {
            m_pfnGetCalibratedTimestamps = m_pDevice->Callbacks.GetCalibratedTimestampsKHR;
        }
        else if( m_pDevice->EnabledExtensions.count( VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME ) )
        {
            m_pfnGetCalibratedTimestamps = m_pDevice->Callbacks.GetCalibratedTimestampsEXT;
        }

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:
        Cleanup internal resources.

    \***********************************************************************************/
    void DeviceProfilerSynchronization::Destroy()
    {
        m_pDevice = nullptr;
        m_pfnGetCalibratedTimestamps = nullptr;
    }

    /***********************************************************************************\

    Function:
        WaitForDevice

    Description:
        Synchronize CPU and GPU. Wait until there are no tasks executing on the device.

    \***********************************************************************************/
    void DeviceProfilerSynchronization::WaitForDevice()
    {
        assert( m_pDevice );
        m_pDevice->Callbacks.DeviceWaitIdle( m_pDevice->Handle );
    }

    /***********************************************************************************\

    Function:
        WaitForQueue

    Description:
        Synchronize CPU and GPU. Wait until there are no tasks executing on the queue.

    \***********************************************************************************/
    void DeviceProfilerSynchronization::WaitForQueue( VkQueue queue )
    {
        assert( m_pDevice );
        m_pDevice->Callbacks.QueueWaitIdle( queue );
    }

    /***********************************************************************************\

    Function:
        WaitForFences

    Description:
        Synchronize CPU and GPU. Wait until there are no tasks executing on the queue.

    \***********************************************************************************/
    void DeviceProfilerSynchronization::WaitForFence( VkFence fence, uint64_t timeout )
    {
        assert( m_pDevice );
        m_pDevice->Callbacks.WaitForFences( m_pDevice->Handle, 1, &fence, false, timeout );
    }

    /***********************************************************************************\

    Function:
        GetSynchronizationTimestamps

    Description:
        Returns calibrated timestamps on success.

    \***********************************************************************************/
    bool DeviceProfilerSynchronization::GetSynchronizationTimestamps(
        uint64_t* pHostCalibratedTimestamp,
        uint64_t* pDeviceCalibratedTimestamp ) const
    {
        bool success = false;

        if( m_pfnGetCalibratedTimestamps != nullptr )
        {
            VkCalibratedTimestampInfoEXT timestampInfos[ 2 ] = {};
            timestampInfos[ 0 ].sType = VK_STRUCTURE_TYPE_CALIBRATED_TIMESTAMP_INFO_EXT;
            timestampInfos[ 0 ].timeDomain = VK_TIME_DOMAIN_HIGH_RESOLUTION_CLOCK_NATIVE;
            timestampInfos[ 1 ].sType = VK_STRUCTURE_TYPE_CALIBRATED_TIMESTAMP_INFO_EXT;
            timestampInfos[ 1 ].timeDomain = VK_TIME_DOMAIN_DEVICE_EXT;

            uint64_t timestamps[ 2 ];
            uint64_t maxDeviations[ 2 ];

            assert( m_pDevice );
            VkResult result = m_pfnGetCalibratedTimestamps(
                m_pDevice->Handle, 2, timestampInfos, timestamps, maxDeviations );

            if( result == VK_SUCCESS )
            {
                *pHostCalibratedTimestamp = timestamps[ 0 ];
                *pDeviceCalibratedTimestamp = timestamps[ 1 ];
                success = true;
            }
        }

        return success;
    }
}

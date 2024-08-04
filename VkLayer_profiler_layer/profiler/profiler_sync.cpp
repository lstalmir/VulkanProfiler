// Copyright (c) 2019-2024 Lukasz Stalmirski
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
#include "profiler_counters.h"
#include "profiler_data.h"
#include <assert.h>

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
        , m_pfnGetCalibratedTimestampsEXT( nullptr )
        , m_HostTimeDomain( OSGetDefaultTimeDomain() )
        , m_DeviceTimeDomain( VK_TIME_DOMAIN_DEVICE_EXT )
        , m_HostCalibratedTimestamp( 0 )
        , m_DeviceCalibratedTimestamp( 0 )
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
        VkResult result = VK_SUCCESS;
        m_pDevice = pDevice;

        // Use VK_EXT_calibrated_timestamps (or KHR equivalent).
        if( m_pDevice != nullptr )
        {
            PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT pfnGetPhysicalDeviceCalibrateableTimeDomainsEXT = nullptr;

            // Use the available extension.
            if( m_pDevice->EnabledExtensions.count( VK_KHR_CALIBRATED_TIMESTAMPS_EXTENSION_NAME ) )
            {
                m_pfnGetCalibratedTimestampsEXT = m_pDevice->Callbacks.GetCalibratedTimestampsKHR;
                pfnGetPhysicalDeviceCalibrateableTimeDomainsEXT =
                    m_pDevice->pInstance->Callbacks.GetPhysicalDeviceCalibrateableTimeDomainsKHR;
            }
            else if( m_pDevice->EnabledExtensions.count( VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME ) )
            {
                m_pfnGetCalibratedTimestampsEXT = m_pDevice->Callbacks.GetCalibratedTimestampsEXT;
                pfnGetPhysicalDeviceCalibrateableTimeDomainsEXT =
                    m_pDevice->pInstance->Callbacks.GetPhysicalDeviceCalibrateableTimeDomainsEXT;
            }
            else
            {
                result = VK_ERROR_EXTENSION_NOT_PRESENT;
            }

            // Enumerate available time domains.
            if( result == VK_SUCCESS && pfnGetPhysicalDeviceCalibrateableTimeDomainsEXT != nullptr )
            {
                uint32_t calibrateableTimeDomainCount = 0;
                result = pfnGetPhysicalDeviceCalibrateableTimeDomainsEXT(
                    m_pDevice->pPhysicalDevice->Handle,
                    &calibrateableTimeDomainCount,
                    nullptr );

                std::vector<VkTimeDomainEXT> calibrateableTimeDomains( calibrateableTimeDomainCount );
                if( result == VK_SUCCESS && calibrateableTimeDomainCount > 0 )
                {
                    result = pfnGetPhysicalDeviceCalibrateableTimeDomainsEXT(
                        m_pDevice->pPhysicalDevice->Handle,
                        &calibrateableTimeDomainCount,
                        calibrateableTimeDomains.data() );
                }

                if( result == VK_SUCCESS && calibrateableTimeDomainCount > 0 )
                {
                    m_HostTimeDomain = OSGetPreferredTimeDomain(
                        calibrateableTimeDomainCount,
                        calibrateableTimeDomains.data() );
                }
            }

            if( result != VK_SUCCESS )
            {
                // Query of timestamp calibration capabilities failed.
                // Disable the extension.
                m_pfnGetCalibratedTimestampsEXT = nullptr;
            }
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
        m_pfnGetCalibratedTimestampsEXT = nullptr;
        m_HostTimeDomain = OSGetDefaultTimeDomain();
        m_DeviceTimeDomain = VK_TIME_DOMAIN_DEVICE_EXT;
        m_HostCalibratedTimestamp = 0;
        m_DeviceCalibratedTimestamp = 0;
    }

    /***********************************************************************************\

    Function:
        WaitForDevice

    Description:
        Synchronize CPU and GPU. Wait until there are no tasks executing on the device.

    \***********************************************************************************/
    void DeviceProfilerSynchronization::WaitForDevice()
    {
        TipGuardDbg tip( m_pDevice->TIP, __func__ );

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
        TipGuardDbg tip( m_pDevice->TIP, __func__ );

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
        TipGuardDbg tip( m_pDevice->TIP, __func__ );

        assert( m_pDevice );
        m_pDevice->Callbacks.WaitForFences( m_pDevice->Handle, 1, &fence, false, timeout );
    }

    /***********************************************************************************\

    Function:
        SendSynchronizationTimestamps

    Description:
        Calibrate timestamps.

    \***********************************************************************************/
    void DeviceProfilerSynchronization::SendSynchronizationTimestamps()
    {
        TipGuardDbg tip( m_pDevice->TIP, __func__ );

        assert( m_pDevice );

        VkResult result = VK_ERROR_EXTENSION_NOT_PRESENT;
        if( m_pfnGetCalibratedTimestampsEXT != nullptr )
        {
            // Collected timestamp types.
            enum : uint32_t
            {
                eHostTimestamp,
                eDeviceTimestamp,
                eTimestampCount
            };

            VkCalibratedTimestampInfoEXT timestampInfos[ eTimestampCount ] = {};
            timestampInfos[ eHostTimestamp ].sType = VK_STRUCTURE_TYPE_CALIBRATED_TIMESTAMP_INFO_EXT;
            timestampInfos[ eHostTimestamp ].timeDomain = m_HostTimeDomain;
            timestampInfos[ eDeviceTimestamp ].sType = VK_STRUCTURE_TYPE_CALIBRATED_TIMESTAMP_INFO_EXT;
            timestampInfos[ eDeviceTimestamp ].timeDomain = m_DeviceTimeDomain;

            uint64_t timestamps[ eTimestampCount ] = {};
            uint64_t maxDeviations[ eTimestampCount ] = {};

            // Get the calibrated timestamps.
            // TODO: maxDeviations can be used to evaluate whether the calibration was successful.
            result = m_pfnGetCalibratedTimestampsEXT(
                m_pDevice->Handle,
                eTimestampCount,
                timestampInfos,
                timestamps,
                maxDeviations );

            if( result == VK_SUCCESS )
            {
                m_HostCalibratedTimestamp = timestamps[ eHostTimestamp ];
                m_DeviceCalibratedTimestamp = timestamps[ eDeviceTimestamp ];
            }
        }

        // Calibration failed, timestamps not available.
        if( result != VK_SUCCESS )
        {
            m_HostCalibratedTimestamp = 0;
            m_DeviceCalibratedTimestamp = 0;
        }
    }

    /***********************************************************************************\

    Function:
        GetSynchronizationTimestamps

    Description:
        Retrieve timestamps submitted in SendSynchronizationTimestamps.

    \***********************************************************************************/
    DeviceProfilerSynchronizationTimestamps DeviceProfilerSynchronization::GetSynchronizationTimestamps() const
    {
        DeviceProfilerSynchronizationTimestamps syncTimestamps;
        syncTimestamps.m_HostTimeDomain = m_HostTimeDomain;
        syncTimestamps.m_HostCalibratedTimestamp = m_HostCalibratedTimestamp;
        syncTimestamps.m_DeviceCalibratedTimestamp = m_DeviceCalibratedTimestamp;
        return syncTimestamps;
    }

    /***********************************************************************************\

    Function:
        GetHostTimeDomain

    Description:
        Returns time domain selected for host timestamps.

    \***********************************************************************************/
    VkTimeDomainEXT DeviceProfilerSynchronization::GetHostTimeDomain() const
    {
        return m_HostTimeDomain;
    }

    /***********************************************************************************\

    Function:
        GetDeviceTimeDomain

    Description:
        Returns time domain used by GPU timestamps.

    \***********************************************************************************/
    VkTimeDomainEXT DeviceProfilerSynchronization::GetDeviceTimeDomain() const
    {
        return m_DeviceTimeDomain;
    }
}

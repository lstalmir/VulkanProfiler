// Copyright (c) 2019-2025 Lukasz Stalmirski
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
#include "profiler_performance_counters.h"
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
        , m_pPerformanceCounters( nullptr )
        , m_pfnGetCalibratedTimestampsEXT( nullptr )
        , m_HostTimeDomain( OSGetDefaultTimeDomain() )
        , m_CreateHostTimestamp( 0 )
        , m_CreateDeviceTimestamp( 0 )
        , m_CreatePerformanceCountersHostTimestamp( 0 )
        , m_CreatePerformanceCountersDeviceTimestamp( 0 )
    {
    }

    /***********************************************************************************\

    Function:
        Initialize

    Description:
        Setup resources needed for device synchronization.

    \***********************************************************************************/
    VkResult DeviceProfilerSynchronization::Initialize( VkDevice_Object* pDevice, DeviceProfilerPerformanceCounters* pPerformanceCounters )
    {
        VkResult result = VK_SUCCESS;
        m_pDevice = pDevice;
        m_pPerformanceCounters = pPerformanceCounters;

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

            // Query initial timestamps.
            if( result == VK_SUCCESS )
            {
                auto createTimestamps = GetSynchronizationTimestamps();
                m_CreateHostTimestamp = createTimestamps.m_HostCalibratedTimestamp;
                m_CreateDeviceTimestamp = createTimestamps.m_DeviceCalibratedTimestamp;
            }

            if( result != VK_SUCCESS )
            {
                // Query of timestamp calibration capabilities failed.
                // Disable the extension.
                m_pfnGetCalibratedTimestampsEXT = nullptr;
            }
        }

        if( m_pPerformanceCounters != nullptr )
        {
            // Performance counters may use different time domains.
            m_pPerformanceCounters->ReadStreamSynchronizationTimestamps(
                &m_CreatePerformanceCountersDeviceTimestamp,
                &m_CreatePerformanceCountersHostTimestamp );
        }

        // Always return success, the extension is optional.
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
        m_pPerformanceCounters = nullptr;
        m_pfnGetCalibratedTimestampsEXT = nullptr;
        m_HostTimeDomain = OSGetDefaultTimeDomain();
        m_CreateHostTimestamp = 0;
        m_CreateDeviceTimestamp = 0;
        m_CreatePerformanceCountersHostTimestamp = 0;
        m_CreatePerformanceCountersDeviceTimestamp = 0;
    }

    /***********************************************************************************\

    Function:
        WaitForDevice

    Description:
        Synchronize CPU and GPU. Wait until there are no tasks executing on the device.

    \***********************************************************************************/
    void DeviceProfilerSynchronization::WaitForDevice()
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

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
        TipGuard tip( m_pDevice->TIP, __func__ );

        assert( m_pDevice );

        // Synchronize host access to the queue object in case the overlay tries to use it.
        VkQueue_Object_InternalScope queueScope( m_pDevice->Queues.at( queue ) );

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
        TipGuard tip( m_pDevice->TIP, __func__ );

        assert( m_pDevice );
        m_pDevice->Callbacks.WaitForFences( m_pDevice->Handle, 1, &fence, false, timeout );
    }

    /***********************************************************************************\

    Function:
        GetSynchronizationTimestamps

    Description:
        Calibrate timestamps.

    \***********************************************************************************/
    DeviceProfilerSynchronizationTimestamps DeviceProfilerSynchronization::GetSynchronizationTimestamps() const
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        assert( m_pDevice );

        DeviceProfilerSynchronizationTimestamps output = {};

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
            timestampInfos[ eDeviceTimestamp ].timeDomain = VK_TIME_DOMAIN_DEVICE_EXT;

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
                output.m_HostTimeDomain = m_HostTimeDomain;
                output.m_HostCalibratedTimestamp = timestamps[ eHostTimestamp ];
                output.m_DeviceCalibratedTimestamp = timestamps[ eDeviceTimestamp ];
            }
        }

        if( m_pPerformanceCounters != nullptr )
        {
            // Performance counters may use different time domains.
            m_pPerformanceCounters->ReadStreamSynchronizationTimestamps(
                &output.m_PerformanceCountersDeviceCalibratedTimestamp,
                &output.m_PerformanceCountersHostCalibratedTimestamp );
        }

        return output;
    }

    /***********************************************************************************\

    Function:
        GetCreateTimestamps

    Description:
        Get creation timestamps.

    \***********************************************************************************/
    DeviceProfilerSynchronizationTimestamps DeviceProfilerSynchronization::GetCreateTimestamps() const
    {
        DeviceProfilerSynchronizationTimestamps output = {};
        output.m_HostTimeDomain = m_HostTimeDomain;
        output.m_HostCalibratedTimestamp = m_CreateHostTimestamp;
        output.m_DeviceCalibratedTimestamp = m_CreateDeviceTimestamp;
        output.m_PerformanceCountersHostCalibratedTimestamp = m_CreatePerformanceCountersHostTimestamp;
        output.m_PerformanceCountersDeviceCalibratedTimestamp = m_CreatePerformanceCountersDeviceTimestamp;
        return output;
    }

    /***********************************************************************************\

    Function:
        GetCreateTimestamps

    Description:
        Get creation timestamps.

    \***********************************************************************************/
    uint64_t DeviceProfilerSynchronization::GetCreateTimestamp( VkTimeDomainEXT domain ) const
    {
        if( domain == VK_TIME_DOMAIN_DEVICE_EXT )
        {
            return m_CreateDeviceTimestamp;
        }

        if( domain == m_HostTimeDomain )
        {
            return m_CreateHostTimestamp;
        }

        return 0;
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
}

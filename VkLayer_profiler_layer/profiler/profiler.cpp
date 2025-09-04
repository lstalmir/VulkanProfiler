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

#include "profiler.h"
#include "profiler_command_buffer.h"
#include "profiler_helpers.h"
#include <farmhash.h>
#include <inttypes.h>
#include <sstream>
#include <fstream>

namespace
{
    template<typename RenderPassCreateInfo>
    static inline void CountRenderPassAttachmentClears(
        Profiler::DeviceProfilerRenderPass& renderPass,
        const RenderPassCreateInfo* pCreateInfo )
    {
        for( uint32_t attachmentIndex = 0; attachmentIndex < pCreateInfo->attachmentCount; ++attachmentIndex )
        {
            const auto& attachment = pCreateInfo->pAttachments[ attachmentIndex ];
            const auto imageFormatAspectFlags = Profiler::GetFormatAllAspectFlags( attachment.format );

            // Color attachment clear
            if( (imageFormatAspectFlags & VK_IMAGE_ASPECT_COLOR_BIT) &&
                (attachment.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR) )
            {
                renderPass.m_ClearColorAttachmentCount++;
            }

            bool hasDepthClear = false;

            // Depth attachment clear
            if( (imageFormatAspectFlags & VK_IMAGE_ASPECT_DEPTH_BIT) &&
                (attachment.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR) )
            {
                hasDepthClear = true;
                renderPass.m_ClearDepthStencilAttachmentCount++;
            }

            // Stencil attachment clear
            if( (imageFormatAspectFlags & VK_IMAGE_ASPECT_STENCIL_BIT) &&
                (attachment.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_CLEAR) )
            {
                // Treat depth-stencil clear as one (just like vkCmdClearDepthStencilImage call)
                if( !(hasDepthClear) )
                {
                    renderPass.m_ClearDepthStencilAttachmentCount++;
                }
            }
        }
    }

    template<typename SubpassDescription>
    static inline void CountSubpassAttachmentResolves(
        Profiler::DeviceProfilerSubpass& subpass,
        const SubpassDescription& subpassDescription )
    {
        if( subpassDescription.pResolveAttachments )
        {
            for( uint32_t attachmentIndex = 0; attachmentIndex < subpassDescription.colorAttachmentCount; ++attachmentIndex )
            {
                // Attachments which are not resolved have VK_ATTACHMENT_UNUSED set
                if( subpassDescription.pResolveAttachments[ attachmentIndex ].attachment != VK_ATTACHMENT_UNUSED )
                {
                    subpass.m_ResolveCount++;
                }
            }
        }
    }

    template<typename SubmitInfo>
    struct SubmitInfoTraits;

    template<>
    struct SubmitInfoTraits<VkSubmitInfo>
    {
        PROFILER_FORCE_INLINE static uint32_t CommandBufferCount( const VkSubmitInfo& info ) { return info.commandBufferCount; }
        PROFILER_FORCE_INLINE static uint32_t SignalSemaphoreCount( const VkSubmitInfo& info ) { return info.signalSemaphoreCount; }
        PROFILER_FORCE_INLINE static uint32_t WaitSemaphoreCount( const VkSubmitInfo& info ) { return info.waitSemaphoreCount; }
        PROFILER_FORCE_INLINE static VkCommandBuffer CommandBuffer( const VkSubmitInfo& info, uint32_t i ) { return info.pCommandBuffers[ i ]; }
        PROFILER_FORCE_INLINE static VkSemaphore SignalSemaphore( const VkSubmitInfo& info, uint32_t i ) { return info.pSignalSemaphores[ i ]; }
        PROFILER_FORCE_INLINE static VkSemaphore WaitSemaphore( const VkSubmitInfo& info, uint32_t i ) { return info.pWaitSemaphores[ i ]; }
    };

    template<>
    struct SubmitInfoTraits<VkSubmitInfo2>
    {
        PROFILER_FORCE_INLINE static uint32_t CommandBufferCount( const VkSubmitInfo2& info ) { return info.commandBufferInfoCount; }
        PROFILER_FORCE_INLINE static uint32_t SignalSemaphoreCount( const VkSubmitInfo2& info ) { return info.signalSemaphoreInfoCount; }
        PROFILER_FORCE_INLINE static uint32_t WaitSemaphoreCount( const VkSubmitInfo2& info ) { return info.waitSemaphoreInfoCount; }
        PROFILER_FORCE_INLINE static VkCommandBuffer CommandBuffer( const VkSubmitInfo2& info, uint32_t i ) { return info.pCommandBufferInfos[ i ].commandBuffer; }
        PROFILER_FORCE_INLINE static VkSemaphore SignalSemaphore( const VkSubmitInfo2& info, uint32_t i ) { return info.pSignalSemaphoreInfos[ i ].semaphore; }
        PROFILER_FORCE_INLINE static VkSemaphore WaitSemaphore( const VkSubmitInfo2& info, uint32_t i ) { return info.pWaitSemaphoreInfos[ i ].semaphore; }
    };
}

namespace Profiler
{
    /***********************************************************************************\

    Function:
        DeviceProfiler

    Description:
        Constructor

    \***********************************************************************************/
    DeviceProfiler::DeviceProfiler()
        : m_pDevice( nullptr )
        , m_Config()
        , m_DataMutex()
        , m_pData()
        , m_MemoryManager()
        , m_DataAggregator()
        , m_NextFrameIndex( 0 )
        , m_DataBufferSize( 1 )
        , m_MinDataBufferSize( 1 )
        , m_LastFrameBeginTimestamp( 0 )
        , m_CpuTimestampCounter()
        , m_CpuFpsCounter()
        , m_MemoryTracker()
        , m_pCommandBuffers()
        , m_pCommandPools()
        , m_SubmitFence( VK_NULL_HANDLE )
        , m_PipelineExecutablePropertiesEnabled( false )
        , m_ShaderModuleIdentifierEnabled( false )
        , m_pStablePowerStateHandle( nullptr )
    {
    }

    /***********************************************************************************\

    Function:
        SetupDeviceCreateInfo

    Description:
        Get list of optional device extensions that may be utilized by the profiler.

    \***********************************************************************************/
    void DeviceProfiler::SetupDeviceCreateInfo(
        VkPhysicalDevice_Object& physicalDevice,
        const ProfilerLayerSettings& settings,
        std::unordered_set<std::string>& deviceExtensions,
        PNextChain& devicePNextChain )
    {
        // Check if profiler create info was provided.
        const VkProfilerCreateInfoEXT* pProfilerCreateInfo =
            devicePNextChain.Find<VkProfilerCreateInfoEXT>( VK_STRUCTURE_TYPE_PROFILER_CREATE_INFO_EXT );

        // Load configuration that will be used by the profiler.
        DeviceProfilerConfig config;
        DeviceProfiler::LoadConfiguration( settings, pProfilerCreateInfo, &config );

        // Enumerate available extensions.
        uint32_t extensionCount = 0;
        physicalDevice.pInstance->Callbacks.EnumerateDeviceExtensionProperties(
            physicalDevice.Handle, nullptr, &extensionCount, nullptr );

        std::vector<VkExtensionProperties> availableExtensions( extensionCount );
        physicalDevice.pInstance->Callbacks.EnumerateDeviceExtensionProperties(
            physicalDevice.Handle, nullptr, &extensionCount, availableExtensions.data() );

        std::unordered_set<std::string> availableExtensionNames;
        for( const VkExtensionProperties& extension : availableExtensions )
        {
            availableExtensionNames.insert( extension.extensionName );
        }

        // Enable shader module identifier if available.
        if( availableExtensionNames.count( VK_EXT_SHADER_MODULE_IDENTIFIER_EXTENSION_NAME ) &&
            !devicePNextChain.Contains( VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_MODULE_IDENTIFIER_FEATURES_EXT ) )
        {
            bool enableShaderModuleIdentifier = false;

            if( ( physicalDevice.pInstance->ApplicationInfo.apiVersion >= VK_API_VERSION_1_3 ) &&
                ( physicalDevice.Properties.apiVersion >= VK_API_VERSION_1_3 ) )
            {
                enableShaderModuleIdentifier = true;
            }
            else
            {
                if( availableExtensionNames.count( VK_EXT_PIPELINE_CREATION_CACHE_CONTROL_EXTENSION_NAME ) )
                {
                    if( ( ( physicalDevice.pInstance->ApplicationInfo.apiVersion >= VK_API_VERSION_1_1 ) &&
                            ( physicalDevice.Properties.apiVersion >= VK_API_VERSION_1_1 ) ) ||
                        ( physicalDevice.pInstance->EnabledExtensions.count( VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME ) ) )
                    {
                        deviceExtensions.insert( VK_EXT_PIPELINE_CREATION_CACHE_CONTROL_EXTENSION_NAME );
                        enableShaderModuleIdentifier = true;
                    }
                }
            }

            if( enableShaderModuleIdentifier )
            {
                // Enable shader module identifiers.
                deviceExtensions.insert( VK_EXT_SHADER_MODULE_IDENTIFIER_EXTENSION_NAME );

                VkPhysicalDeviceShaderModuleIdentifierFeaturesEXT shaderModuleIdentifierFeatures = {};
                shaderModuleIdentifierFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_MODULE_IDENTIFIER_FEATURES_EXT;
                shaderModuleIdentifierFeatures.shaderModuleIdentifier = VK_TRUE;

                devicePNextChain.Append( shaderModuleIdentifierFeatures );
            }
        }

        if( config.m_EnablePerformanceQueryExt )
        {
            if( availableExtensionNames.count( VK_INTEL_PERFORMANCE_QUERY_EXTENSION_NAME ) )
            {
                // Enable MDAPI data collection on Intel GPUs.
                deviceExtensions.insert( VK_INTEL_PERFORMANCE_QUERY_EXTENSION_NAME );
            }
        }

        if( config.m_EnablePipelineExecutablePropertiesExt )
        {
            if( availableExtensionNames.count( VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME ) &&
                !devicePNextChain.Contains( VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_EXECUTABLE_PROPERTIES_FEATURES_KHR ) )
            {
                // Enable pipeline executable properties capture.
                deviceExtensions.insert( VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME );

                VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR pipelineExecutablePropertiesFeatures = {};
                pipelineExecutablePropertiesFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_EXECUTABLE_PROPERTIES_FEATURES_KHR;
                pipelineExecutablePropertiesFeatures.pipelineExecutableInfo = VK_TRUE;

                devicePNextChain.Append( pipelineExecutablePropertiesFeatures );
            }
        }

        // Enable calibrated timestamps extension to synchronize CPU and GPU events in traces.
        if( availableExtensionNames.count( VK_KHR_CALIBRATED_TIMESTAMPS_EXTENSION_NAME ) )
        {
            deviceExtensions.insert( VK_KHR_CALIBRATED_TIMESTAMPS_EXTENSION_NAME );
        }
        else if( availableExtensionNames.count( VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME ) )
        {
            deviceExtensions.insert( VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME );
        }

        // Enable memory budget extension to track memory usage.
        if( availableExtensionNames.count( VK_EXT_MEMORY_BUDGET_EXTENSION_NAME ) )
        {
            if( ( physicalDevice.pInstance->ApplicationInfo.apiVersion >= VK_API_VERSION_1_1 &&
                    physicalDevice.Properties.apiVersion >= VK_API_VERSION_1_1 ) ||
                physicalDevice.pInstance->EnabledExtensions.count( VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME ) )
            {
                deviceExtensions.insert( VK_EXT_MEMORY_BUDGET_EXTENSION_NAME );
            }
        }
    }

    /***********************************************************************************\

    Function:
        SetupInstanceCreateInfo

    Description:
        Get list of optional instance extensions that may be utilized by the profiler.

    \***********************************************************************************/
    void DeviceProfiler::SetupInstanceCreateInfo(
        const VkInstanceCreateInfo& createInfo,
        PFN_vkGetInstanceProcAddr pfnGetInstanceProcAddr,
        std::unordered_set<std::string>& instanceExtensions )
    {
        std::unordered_set<std::string> availableInstanceExtensions;

        // Try to enumerate available extensions.
        if( pfnGetInstanceProcAddr != nullptr )
        {
            PFN_vkEnumerateInstanceExtensionProperties pfnEnumerateInstanceExtensionProperties =
                (PFN_vkEnumerateInstanceExtensionProperties)pfnGetInstanceProcAddr( nullptr, "vkEnumerateInstanceExtensionProperties" );

            if( pfnEnumerateInstanceExtensionProperties != nullptr )
            {
                uint32_t extensionCount = 0;
                pfnEnumerateInstanceExtensionProperties( nullptr, &extensionCount, nullptr );

                std::vector<VkExtensionProperties> extensions( extensionCount );
                pfnEnumerateInstanceExtensionProperties( nullptr, &extensionCount, extensions.data() );

                for( const VkExtensionProperties& extension : extensions )
                {
                    availableInstanceExtensions.insert( extension.extensionName );
                }
            }
        }

        // Enable extensions required by some of the features to work correctly.
        if( ( createInfo.pApplicationInfo == nullptr ) ||
            ( createInfo.pApplicationInfo->apiVersion == 0 ||
                createInfo.pApplicationInfo->apiVersion == VK_API_VERSION_1_0 ) )
        {
            if( availableInstanceExtensions.count( VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME ) )
            {
                // Required by Vk_EXT_shader_module_identifier and VK_EXT_memory_budget.
                instanceExtensions.insert( VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME );
            }
        }
    }

    /***********************************************************************************\

    Function:
        LoadConfiguration

    Description:
        Loads the configuration structure from all available sources.

    \***********************************************************************************/
    void DeviceProfiler::LoadConfiguration( const ProfilerLayerSettings& settings, const VkProfilerCreateInfoEXT* pCreateInfo, DeviceProfilerConfig* pConfig )
    {
        *pConfig = DeviceProfilerConfig( settings );

        // Load configuration from file (if exists).
        const std::filesystem::path& configFilename =
            ProfilerPlatformFunctions::GetApplicationDir() / "VK_LAYER_profiler_config.ini";

        if( std::filesystem::exists( configFilename ) )
        {
            pConfig->LoadFromFile( configFilename );
        }

        // Check if application provided create info
        if( pCreateInfo )
        {
            pConfig->LoadFromCreateInfo( pCreateInfo );
        }

        // Configure the profiler from the environment.
        pConfig->LoadFromEnvironment();
    }

    /***********************************************************************************\

    Function:
        Initialize

    Description:
        Initializes profiler resources.

    \***********************************************************************************/
    VkResult DeviceProfiler::Initialize( VkDevice_Object* pDevice, const VkDeviceCreateInfo* pCreateInfo )
    {
        m_pDevice = pDevice;

        // Frame #0 is allocated by the aggregator.
        m_NextFrameIndex = 1;

        // Check if profiler create info was provided.
        const PNextChain pNextChain( pCreateInfo->pNext );
        const VkProfilerCreateInfoEXT* pProfilerCreateInfo =
            pNextChain.Find<VkProfilerCreateInfoEXT>( VK_STRUCTURE_TYPE_PROFILER_CREATE_INFO_EXT );

        // Configure the profiler.
        DeviceProfiler::LoadConfiguration( pDevice->pInstance->LayerSettings, pProfilerCreateInfo, &m_Config );

        // Check if preemption is enabled
        // It may break the results
        if( ProfilerPlatformFunctions::IsPreemptionEnabled() )
        {
            // Sample per drawcall to avoid DMA packet splits between timestamps
            //m_Config.m_SamplingMode = ProfilerMode::ePerDrawcall;
            #if 0
            m_Output.Summary.Message = "Preemption enabled. Profiler will collect results per drawcall.";
            m_Overlay.Summary.Message = "Preemption enabled. Results may be unstable.";
            #endif
        }

        // Create submit fence
        VkFenceCreateInfo fenceCreateInfo;
        ClearStructure( &fenceCreateInfo, VK_STRUCTURE_TYPE_FENCE_CREATE_INFO );

        DESTROYANDRETURNONFAIL( m_pDevice->Callbacks.CreateFence(
            m_pDevice->Handle, &fenceCreateInfo, nullptr, &m_SubmitFence ) );

        // Prepare for memory usage tracking
        m_MemoryTracker.Initialize( m_pDevice );

        // Enable vendor-specific extensions
        if( m_pDevice->EnabledExtensions.count( VK_INTEL_PERFORMANCE_QUERY_EXTENSION_NAME ) )
        {
            m_MetricsApiINTEL.Initialize( m_pDevice );
        }

        // Capture pipeline statistics and internal representations for debugging
        m_PipelineExecutablePropertiesEnabled =
            m_Config.m_EnablePipelineExecutablePropertiesExt &&
            m_pDevice->EnabledExtensions.count( VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME );

        if( m_PipelineExecutablePropertiesEnabled )
        {
            const VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR* pPipelineExecutablePropertiesFeatures =
                pNextChain.Find<VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR>( VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_EXECUTABLE_PROPERTIES_FEATURES_KHR );

            m_PipelineExecutablePropertiesEnabled =
                ( pPipelineExecutablePropertiesFeatures != nullptr ) &&
                ( pPipelineExecutablePropertiesFeatures->pipelineExecutableInfo == VK_TRUE );
        }

        // Collect shader module identifiers if available
        m_ShaderModuleIdentifierEnabled =
            m_pDevice->EnabledExtensions.count( VK_EXT_SHADER_MODULE_IDENTIFIER_EXTENSION_NAME );

        if( m_ShaderModuleIdentifierEnabled )
        {
            const VkPhysicalDeviceShaderModuleIdentifierFeaturesEXT* pShaderModuleIdentifierFeatures =
                pNextChain.Find<VkPhysicalDeviceShaderModuleIdentifierFeaturesEXT>( VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_MODULE_IDENTIFIER_FEATURES_EXT );

            m_ShaderModuleIdentifierEnabled =
                ( pShaderModuleIdentifierFeatures != nullptr ) &&
                ( pShaderModuleIdentifierFeatures->shaderModuleIdentifier == VK_TRUE );
        }

        // Initialize synchroniation manager
        DESTROYANDRETURNONFAIL( m_Synchronization.Initialize( m_pDevice ) );

        VkTimeDomainEXT hostTimeDomain = m_Synchronization.GetHostTimeDomain();
        m_CpuTimestampCounter.SetTimeDomain( hostTimeDomain );
        m_CpuFpsCounter.SetTimeDomain( hostTimeDomain );

        m_pDevice->TIP.SetTimeDomain( hostTimeDomain );

        // Initialize memory manager
        DESTROYANDRETURNONFAIL( m_MemoryManager.Initialize( m_pDevice ) );

        // Initialize aggregator
        DESTROYANDRETURNONFAIL( m_DataAggregator.Initialize( this ) );

        m_pData = m_DataAggregator.GetAggregatedData();
        assert( !m_pData.empty() );

        // Initialize internal pipelines
        CreateInternalPipeline( DeviceProfilerPipelineType::eCopyBuffer, "CopyBuffer" );
        CreateInternalPipeline( DeviceProfilerPipelineType::eCopyBufferToImage, "CopyBufferToImage" );
        CreateInternalPipeline( DeviceProfilerPipelineType::eCopyImage, "CopyImage" );
        CreateInternalPipeline( DeviceProfilerPipelineType::eCopyImageToBuffer, "CopyImageToBuffer" );
        CreateInternalPipeline( DeviceProfilerPipelineType::eClearAttachments, "ClearAttachments" );
        CreateInternalPipeline( DeviceProfilerPipelineType::eClearColorImage, "ClearColorImage" );
        CreateInternalPipeline( DeviceProfilerPipelineType::eClearDepthStencilImage, "ClearDepthStencilImage" );
        CreateInternalPipeline( DeviceProfilerPipelineType::eResolveImage, "ResolveImage" );
        CreateInternalPipeline( DeviceProfilerPipelineType::eBlitImage, "BlitImage" );
        CreateInternalPipeline( DeviceProfilerPipelineType::eFillBuffer, "FillBuffer" );
        CreateInternalPipeline( DeviceProfilerPipelineType::eUpdateBuffer, "UpdateBuffer" );
        CreateInternalPipeline( DeviceProfilerPipelineType::eBeginRenderPass, "BeginRenderPass" );
        CreateInternalPipeline( DeviceProfilerPipelineType::eEndRenderPass, "EndRenderPass" );
        CreateInternalPipeline( DeviceProfilerPipelineType::eBuildAccelerationStructuresKHR, "BuildAccelerationStructuresKHR" );
        CreateInternalPipeline( DeviceProfilerPipelineType::eCopyAccelerationStructureKHR, "CopyAccelerationStructureKHR" );
        CreateInternalPipeline( DeviceProfilerPipelineType::eCopyAccelerationStructureToMemoryKHR, "CopyAccelerationStructureToMemoryKHR" );
        CreateInternalPipeline( DeviceProfilerPipelineType::eCopyMemoryToAccelerationStructureKHR, "CopyMemoryToAccelerationStructureKHR" );
        CreateInternalPipeline( DeviceProfilerPipelineType::eBuildMicromapsEXT, "BuildMircomapsEXT" );
        CreateInternalPipeline( DeviceProfilerPipelineType::eCopyMicromapEXT, "CopyMicromapEXT" );
        CreateInternalPipeline( DeviceProfilerPipelineType::eCopyMicromapToMemoryEXT, "CopyMicromapToMemoryEXT" );
        CreateInternalPipeline( DeviceProfilerPipelineType::eCopyMemoryToMicromapEXT, "CopyMemoryToMicromapEXT" );

        if( m_Config.m_SetStablePowerState )
        {
            // Set stable power state.
            ProfilerPlatformFunctions::SetStablePowerState( m_pDevice, &m_pStablePowerStateHandle );
        }

        // Begin profiling of the first frame.
        BeginNextFrame();

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:
        Frees resources allocated by the profiler.

    \***********************************************************************************/
    void DeviceProfiler::Destroy()
    {
        TipRangeId tip = m_pDevice->TIP.BeginFunction( __func__ );

        // Begin a fake frame at the end to allow finalization of the last submitted frame.
        BeginNextFrame();
        ResolveFrameData( tip );

        // Reset members and destroy resources.
        m_DeferredOperationCallbacks.clear();

        m_pCommandBuffers.clear();
        m_pCommandPools.clear();

        m_MemoryTracker.Destroy();

        m_Synchronization.Destroy();
        m_MemoryManager.Destroy();

        m_DataAggregator.Destroy();

        m_MetricsApiINTEL.Destroy();

        if( m_SubmitFence != VK_NULL_HANDLE )
        {
            m_pDevice->Callbacks.DestroyFence( m_pDevice->Handle, m_SubmitFence, nullptr );
            m_SubmitFence = VK_NULL_HANDLE;
        }

        if( m_pStablePowerStateHandle != nullptr )
        {
            ProfilerPlatformFunctions::ResetStablePowerState( m_pStablePowerStateHandle );
        }

        m_NextFrameIndex = 0;
        m_pDevice = nullptr;
    }

    /***********************************************************************************\

    Function:
        SetSamplingMode

    Description:
        Set granularity of timestamp queries in the command buffers.
        Does not affect command buffers that were already recorded.

    \***********************************************************************************/
    VkResult DeviceProfiler::SetSamplingMode( VkProfilerModeEXT mode )
    {
        m_Config.m_SamplingMode = mode;

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        SetFrameDelimiter

    Description:
        Set which API call delimits frames reported by the profiler.
        VK_PROFILER_FRAME_DELIMITER_PRESENT_EXT - vkQueuePresentKHR
        VK_PROFILER_FRAME_DELIMITER_SUBMIT_EXT - vkQueueSumit, vkQueueSubmit2(KHR)

    \***********************************************************************************/
    VkResult DeviceProfiler::SetFrameDelimiter( VkProfilerFrameDelimiterEXT frameDelimiter )
    {
        // Check if frame delimiter is supported by current implementation
        if( frameDelimiter != VK_PROFILER_FRAME_DELIMITER_PRESENT_EXT &&
            frameDelimiter != VK_PROFILER_FRAME_DELIMITER_SUBMIT_EXT )
        {
            return VK_ERROR_VALIDATION_FAILED_EXT;
        }

        m_Config.m_FrameDelimiter = frameDelimiter;

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        SetDataBufferSize

    Description:
        Set the maximum number of buffered frames.

    \***********************************************************************************/
    VkResult DeviceProfiler::SetDataBufferSize( uint32_t size )
    {
        std::scoped_lock lk( m_DataMutex );

        size = std::max( size, m_MinDataBufferSize );

        m_DataAggregator.SetDataBufferSize( size );
        m_DataBufferSize = size;

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        SetMinDataBufferSize

    Description:
        Set the minimum number of buffered frames.

    \***********************************************************************************/
    VkResult DeviceProfiler::SetMinDataBufferSize( uint32_t size )
    {
        std::scoped_lock lk( m_DataMutex );

        if( size > m_DataBufferSize )
        {
            m_DataAggregator.SetDataBufferSize( size );
            m_DataBufferSize = size;
        }

        m_MinDataBufferSize = size;

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    \***********************************************************************************/
    std::shared_ptr<DeviceProfilerFrameData> DeviceProfiler::GetData()
    {
        std::scoped_lock lk( m_DataMutex );
        std::shared_ptr<DeviceProfilerFrameData> pData = nullptr;

        if( !m_pData.empty() )
        {
            pData = m_pData.front();
            m_pData.pop_front();
        }

        return pData;
    }

    /***********************************************************************************\
    \***********************************************************************************/
    ProfilerCommandBuffer& DeviceProfiler::GetCommandBuffer( VkCommandBuffer commandBuffer )
    {
        return *m_pCommandBuffers.at( commandBuffer );
    }

    /***********************************************************************************\
    \***********************************************************************************/
    DeviceProfilerCommandPool& DeviceProfiler::GetCommandPool( VkCommandPool commandPool )
    {
        return *m_pCommandPools.at( commandPool );
    }

    /***********************************************************************************\
    \***********************************************************************************/
    DeviceProfilerPipeline& DeviceProfiler::GetPipeline( VkPipeline pipeline )
    {
        return m_Pipelines.at( pipeline );
    }

    /***********************************************************************************\
    \***********************************************************************************/
    DeviceProfilerRenderPass& DeviceProfiler::GetRenderPass( VkRenderPass renderPass )
    {
        return m_RenderPasses.at( renderPass );
    }

    /***********************************************************************************\
    \***********************************************************************************/
    ProfilerShader& DeviceProfiler::GetShader( VkShaderEXT handle )
    {
        return m_Shaders.at( handle );
    }

    /***********************************************************************************\
    \***********************************************************************************/
    VkObject DeviceProfiler::GetObjectHandle( VkObject object ) const
    {
        if( object.m_CreateTime == 0 )
        {
            m_ObjectCreateTimes.find( object, &object.m_CreateTime );
        }

        return object;
    }

    /***********************************************************************************\

    Function:
        ShouldCapturePipelineExecutableProperties

    Description:
        Checks whether pipeline executable properties should be captured.
        The feature is enabled only if VK_KHR_pipeline_executable_properties extension
        is enabled and it is not disabled in the configuration.

    \***********************************************************************************/
    bool DeviceProfiler::ShouldCapturePipelineExecutableProperties() const
    {
        return m_PipelineExecutablePropertiesEnabled;
    }

    /***********************************************************************************\

    Function:
        CreateCommandPool

    Description:
        Create wrapper for VkCommandPool object.

    \***********************************************************************************/
    void DeviceProfiler::CreateCommandPool( VkCommandPool commandPool, const VkCommandPoolCreateInfo* pCreateInfo )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        m_pCommandPools.insert( commandPool,
            std::make_unique<DeviceProfilerCommandPool>( *this, commandPool, *pCreateInfo ) );
    }

    /***********************************************************************************\

    Function:
        DestroyCommandPool

    Description:
        Destroy wrapper for VkCommandPool object and all command buffers allocated from
        that pool.

    \***********************************************************************************/
    void DeviceProfiler::DestroyCommandPool( VkCommandPool commandPool )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        std::scoped_lock lk( m_pCommandBuffers );

        for( auto it = m_pCommandBuffers.begin(); it != m_pCommandBuffers.end(); )
        {
            it = (it->second->GetCommandPool().GetHandle() == commandPool)
                ? FreeCommandBuffer( it )
                : std::next( it );
        }

        m_pCommandPools.remove( commandPool );
    }

    /***********************************************************************************\

    Function:
        RegisterCommandBuffers

    Description:
        Create wrappers for VkCommandBuffer objects.

    \***********************************************************************************/
    void DeviceProfiler::AllocateCommandBuffers( VkCommandPool commandPool, VkCommandBufferLevel level, uint32_t count, VkCommandBuffer* pCommandBuffers )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        std::scoped_lock lk( m_pCommandBuffers );

        DeviceProfilerCommandPool& profilerCommandPool = GetCommandPool( commandPool );

        for( uint32_t i = 0; i < count; ++i )
        {
            VkCommandBuffer commandBuffer = pCommandBuffers[ i ];
            RegisterObject( commandBuffer );

            SetDefaultObjectName( commandBuffer );

            m_pCommandBuffers.unsafe_insert( commandBuffer,
                std::make_unique<ProfilerCommandBuffer>( *this, profilerCommandPool, commandBuffer, level ) );
        }
    }

    /***********************************************************************************\

    Function:
        UnregisterCommandBuffers

    Description:
        Destroy wrappers for VkCommandBuffer objects.

    \***********************************************************************************/
    void DeviceProfiler::FreeCommandBuffers( uint32_t count, const VkCommandBuffer* pCommandBuffers )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        std::scoped_lock lk( m_pCommandBuffers );

        for( uint32_t i = 0; i < count; ++i )
        {
            FreeCommandBuffer( pCommandBuffers[ i ] );
            UnregisterObject( pCommandBuffers[ i ] );
        }
    }

    /***********************************************************************************\

    Function:
        CreateDeferredOperation

    Description:
        Register deferred host operation.

    \***********************************************************************************/
    void DeviceProfiler::CreateDeferredOperation( VkDeferredOperationKHR deferredOperation )
    {
        m_DeferredOperationCallbacks.insert( deferredOperation, nullptr );
    }

    /***********************************************************************************\

    Function:
        DestroyDeferredOperation

    Description:
        Unregister deferred host operation.

    \***********************************************************************************/
    void DeviceProfiler::DestroyDeferredOperation( VkDeferredOperationKHR deferredOperation )
    {
        m_DeferredOperationCallbacks.remove( deferredOperation );
    }

    /***********************************************************************************\

    Function:
        SetDeferredOperationCallback

    Description:
        Associate an action with a deferred host operation. The action will be executed
        when the deferred operation is joined.

    \***********************************************************************************/
    void DeviceProfiler::SetDeferredOperationCallback( VkDeferredOperationKHR deferredOperation, DeferredOperationCallback callback )
    {
        m_DeferredOperationCallbacks.at( deferredOperation ) = std::move( callback );
    }

    /***********************************************************************************\

    Function:
        ExecuteDeferredOperationCallback

    Description:
        Execute an action associated with the deferred host operation.

    \***********************************************************************************/
    void DeviceProfiler::ExecuteDeferredOperationCallback( VkDeferredOperationKHR deferredOperation )
    {
        auto& callback = m_DeferredOperationCallbacks.at( deferredOperation );
        if( callback )
        {
            // Execute the custom action.
            callback( deferredOperation );

            // Clear the callback once it is executed.
            // Note that callback is a reference to the element in the m_DeferredOperationCallbacks map.
            callback = nullptr;
        }
    }

    /***********************************************************************************\

    Function:
        CreatePipelines

    Description:
        Register graphics pipelines.

    \***********************************************************************************/
    void DeviceProfiler::CreatePipelines( uint32_t pipelineCount, const VkGraphicsPipelineCreateInfo* pCreateInfos, VkPipeline* pPipelines )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        for( uint32_t i = 0; i < pipelineCount; ++i )
        {
            DeviceProfilerPipeline profilerPipeline;
            profilerPipeline.m_Handle = RegisterObject( pPipelines[i] );
            profilerPipeline.m_BindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            profilerPipeline.m_Type = DeviceProfilerPipelineType::eGraphics;

            const VkGraphicsPipelineCreateInfo& createInfo = pCreateInfos[i];

            SetPipelineShaderProperties( profilerPipeline, createInfo.stageCount, createInfo.pStages );
            SetDefaultPipelineName( profilerPipeline );

            profilerPipeline.m_pCreateInfo = DeviceProfilerPipeline::CopyPipelineCreateInfo( &createInfo );

            m_Pipelines.insert( pPipelines[i], profilerPipeline );
        }
    }

    /***********************************************************************************\

    Function:
        CreatePipelines

    Description:
        Register compute pipelines.

    \***********************************************************************************/
    void DeviceProfiler::CreatePipelines( uint32_t pipelineCount, const VkComputePipelineCreateInfo* pCreateInfos, VkPipeline* pPipelines )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        for( uint32_t i = 0; i < pipelineCount; ++i )
        {
            DeviceProfilerPipeline profilerPipeline;
            profilerPipeline.m_Handle = RegisterObject( pPipelines[i] );
            profilerPipeline.m_BindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
            profilerPipeline.m_Type = DeviceProfilerPipelineType::eCompute;
            
            SetPipelineShaderProperties( profilerPipeline, 1, &pCreateInfos[i].stage );
            SetDefaultPipelineName( profilerPipeline );

            m_Pipelines.insert( pPipelines[ i ], profilerPipeline );
        }
    }

    /***********************************************************************************\

    Function:
        CreatePipelines

    Description:
        Register ray-tracing pipelines.

    \***********************************************************************************/
    void DeviceProfiler::CreatePipelines( uint32_t pipelineCount, const VkRayTracingPipelineCreateInfoKHR* pCreateInfos, VkPipeline* pPipelines, bool deferred )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        for( uint32_t i = 0; i < pipelineCount; ++i )
        {
            DeviceProfilerPipeline profilerPipeline;
            profilerPipeline.m_Handle = RegisterObject( pPipelines[i] );
            profilerPipeline.m_BindPoint = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
            profilerPipeline.m_Type = DeviceProfilerPipelineType::eRayTracingKHR;
            
            const VkRayTracingPipelineCreateInfoKHR& createInfo = pCreateInfos[i];

            SetPipelineShaderProperties( profilerPipeline, createInfo.stageCount, createInfo.pStages );
            SetDefaultPipelineName( profilerPipeline, deferred );

            profilerPipeline.m_pCreateInfo = DeviceProfilerPipeline::CopyPipelineCreateInfo( &createInfo );

            // Calculate default pipeline stack size.
            VkDeviceSize rayGenStackMax = 0;
            VkDeviceSize closestHitStackMax = 0;
            VkDeviceSize missStackMax = 0;
            VkDeviceSize intersectionStackMax = 0;
            VkDeviceSize anyHitStackMax = 0;
            VkDeviceSize callableStackMax = 0;

            for( uint32_t groupIndex = 0; groupIndex < createInfo.groupCount; ++groupIndex )
            {
                const VkRayTracingShaderGroupCreateInfoKHR& group = createInfo.pGroups[groupIndex];
                switch( group.type )
                {
                case VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR:
                {
                    // Ray generation, miss and callable shaders.
                    VkDeviceSize stackSize = m_pDevice->Callbacks.GetRayTracingShaderGroupStackSizeKHR(
                        m_pDevice->Handle,
                        profilerPipeline.m_Handle,
                        groupIndex,
                        VK_SHADER_GROUP_SHADER_GENERAL_KHR );

                    switch( createInfo.pStages[group.generalShader].stage )
                    {
                    case VK_SHADER_STAGE_RAYGEN_BIT_KHR:
                        rayGenStackMax = std::max( rayGenStackMax, stackSize );
                        break;

                    case VK_SHADER_STAGE_MISS_BIT_KHR:
                        missStackMax = std::max( missStackMax, stackSize );
                        break;

                    case VK_SHADER_STAGE_CALLABLE_BIT_KHR:
                        callableStackMax = std::max( callableStackMax, stackSize );
                        break;

                    default:
                        assert( !"Unsupported general shader group." );
                        break;
                    }
                    break;
                }

                case VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR:
                case VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR:
                {
                    // Closest-hit, any-hit and intersection shaders.
                    if( group.closestHitShader != VK_SHADER_UNUSED_KHR )
                    {
                        closestHitStackMax = std::max( closestHitStackMax,
                            m_pDevice->Callbacks.GetRayTracingShaderGroupStackSizeKHR(
                                m_pDevice->Handle,
                                profilerPipeline.m_Handle,
                                groupIndex,
                                VK_SHADER_GROUP_SHADER_CLOSEST_HIT_KHR ) );
                    }
                    if( group.anyHitShader != VK_SHADER_UNUSED_KHR )
                    {
                        anyHitStackMax = std::max( anyHitStackMax,
                            m_pDevice->Callbacks.GetRayTracingShaderGroupStackSizeKHR(
                                m_pDevice->Handle,
                                profilerPipeline.m_Handle,
                                groupIndex,
                                VK_SHADER_GROUP_SHADER_ANY_HIT_KHR ) );
                    }
                    if( group.intersectionShader != VK_SHADER_UNUSED_KHR )
                    {
                        intersectionStackMax = std::max( intersectionStackMax,
                            m_pDevice->Callbacks.GetRayTracingShaderGroupStackSizeKHR(
                                m_pDevice->Handle,
                                profilerPipeline.m_Handle,
                                groupIndex,
                                VK_SHADER_GROUP_SHADER_INTERSECTION_KHR ) );
                    }
                    break;
                }

                default:
                {
                    assert( !"Unsupported shader group type." );
                    break;
                }
                }
            }

            // Calculate the default pipeline stack size according to the Vulkan spec.
            const uint32_t maxRayRecursionDepth = std::min( 1U, createInfo.maxPipelineRayRecursionDepth );
            const VkDeviceSize closestHitAndMissStackMax = std::max( closestHitStackMax, missStackMax );

            profilerPipeline.m_RayTracingPipelineStackSize =
                rayGenStackMax +
                ( maxRayRecursionDepth ) * std::max( closestHitAndMissStackMax, intersectionStackMax + anyHitStackMax ) +
                ( maxRayRecursionDepth - 1 ) * closestHitAndMissStackMax +
                2 * callableStackMax;

            m_Pipelines.insert( pPipelines[ i ], profilerPipeline );
        }
    }

    /***********************************************************************************\

    Function:
        DestroyPipeline

    Description:

    \***********************************************************************************/
    void DeviceProfiler::DestroyPipeline( VkPipeline pipeline )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        UnregisterObject( pipeline );

        m_Pipelines.remove( pipeline );
    }

    /***********************************************************************************\

    Function:
        CreateShaderModule

    Description:

    \***********************************************************************************/
    void DeviceProfiler::CreateShaderModule( VkShaderModule module, const VkShaderModuleCreateInfo* pCreateInfo )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        RegisterObject( module );

        VkShaderModuleIdentifierEXT shaderModuleIdentifier = {};
        shaderModuleIdentifier.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_IDENTIFIER_EXT;

        if( m_ShaderModuleIdentifierEnabled )
        {
            // Get shader module identifier.
            m_pDevice->Callbacks.GetShaderModuleIdentifierEXT( m_pDevice->Handle, module, &shaderModuleIdentifier );
        }

        m_pShaderModules.insert( module,
            std::make_shared<ProfilerShaderModule>(
                pCreateInfo->pCode,
                pCreateInfo->codeSize,
                shaderModuleIdentifier.identifier,
                shaderModuleIdentifier.identifierSize ) );
    }

    /***********************************************************************************\

    Function:
        DestroyShaderModule

    Description:

    \***********************************************************************************/
    void DeviceProfiler::DestroyShaderModule( VkShaderModule module )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        UnregisterObject( module );

        m_pShaderModules.remove( module );
    }

    /***********************************************************************************\

    Function:
        CreateShader

    Description:

    \***********************************************************************************/
    void DeviceProfiler::CreateShader( VkShaderEXT handle, const VkShaderCreateInfoEXT* pCreateInfo )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        ProfilerShader shader;
        shader.m_Index = UINT32_MAX;
        shader.m_Stage = pCreateInfo->stage;
        shader.m_EntryPoint = pCreateInfo->pName;

        if( pCreateInfo->codeType == VK_SHADER_CODE_TYPE_SPIRV_EXT )
        {
            VkShaderModuleIdentifierEXT shaderModuleIdentifier = {};
            shaderModuleIdentifier.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_IDENTIFIER_EXT;

            if( m_ShaderModuleIdentifierEnabled )
            {
                // Get shader module identifier from a temporary shader module info structure based on the provided shader.
                VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
                shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
                shaderModuleCreateInfo.codeSize = pCreateInfo->codeSize;
                shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>( pCreateInfo->pCode );

                m_pDevice->Callbacks.GetShaderModuleCreateInfoIdentifierEXT(
                    m_pDevice->Handle,
                    &shaderModuleCreateInfo,
                    &shaderModuleIdentifier );
            }

            // Create a shader module for the shader.
            shader.m_pShaderModule = std::make_shared<ProfilerShaderModule>(
                reinterpret_cast<const uint32_t*>( pCreateInfo->pCode ),
                pCreateInfo->codeSize,
                shaderModuleIdentifier.identifier,
                shaderModuleIdentifier.identifierSize );

            shader.m_Hash = shader.m_pShaderModule->m_Hash;
        }

        // Hash the entrypoint and append it to the final hash
        shader.m_Hash ^= Farmhash::Fingerprint32( shader.m_EntryPoint.data(), shader.m_EntryPoint.length() );

        // Save the shader
        m_Shaders.insert( handle, std::move( shader ) );
    }

    /***********************************************************************************\

    Function:
        DestroyShader

    Description:

    \***********************************************************************************/
    void DeviceProfiler::DestroyShader( VkShaderEXT handle )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        m_Shaders.remove( handle );
    }

    /***********************************************************************************\

    Function:
        CreateRenderPass

    Description:

    \***********************************************************************************/
    void DeviceProfiler::CreateRenderPass( VkRenderPass renderPass, const VkRenderPassCreateInfo* pCreateInfo )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        DeviceProfilerRenderPass deviceProfilerRenderPass;
        deviceProfilerRenderPass.m_Handle = RegisterObject( renderPass );
        deviceProfilerRenderPass.m_Type = DeviceProfilerRenderPassType::eGraphics;
        
        for( uint32_t subpassIndex = 0; subpassIndex < pCreateInfo->subpassCount; ++subpassIndex )
        {
            const VkSubpassDescription& subpass = pCreateInfo->pSubpasses[ subpassIndex ];

            DeviceProfilerSubpass deviceProfilerSubpass;
            deviceProfilerSubpass.m_Index = subpassIndex;
            
            // Check if this subpass resolves any attachments at the end
            CountSubpassAttachmentResolves( deviceProfilerSubpass, subpass );

            deviceProfilerRenderPass.m_Subpasses.push_back( deviceProfilerSubpass );
        }

        // Count clear attachments
        CountRenderPassAttachmentClears( deviceProfilerRenderPass, pCreateInfo );

        // Assign default debug name for the render pass
        SetDefaultObjectName( renderPass );

        // Store render pass
        m_RenderPasses.insert( renderPass, deviceProfilerRenderPass );
    }

    /***********************************************************************************\

    Function:
        CreateRenderPass

    Description:

    \***********************************************************************************/
    void DeviceProfiler::CreateRenderPass( VkRenderPass renderPass, const VkRenderPassCreateInfo2* pCreateInfo )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        DeviceProfilerRenderPass deviceProfilerRenderPass;
        deviceProfilerRenderPass.m_Handle = RegisterObject( renderPass );
        deviceProfilerRenderPass.m_Type = DeviceProfilerRenderPassType::eGraphics;

        for( uint32_t subpassIndex = 0; subpassIndex < pCreateInfo->subpassCount; ++subpassIndex )
        {
            const VkSubpassDescription2& subpass = pCreateInfo->pSubpasses[ subpassIndex ];

            DeviceProfilerSubpass deviceProfilerSubpass;
            deviceProfilerSubpass.m_Index = subpassIndex;

            // Check if this subpass resolves any attachments at the end
            CountSubpassAttachmentResolves( deviceProfilerSubpass, subpass );

            // Check if this subpass resolves depth-stencil attachment
            for( const auto& it : PNextIterator( subpass.pNext ) )
            {
                if( it.sType == VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE )
                {
                    const VkSubpassDescriptionDepthStencilResolve& depthStencilResolve =
                        reinterpret_cast<const VkSubpassDescriptionDepthStencilResolve&>(it);

                    // Check if depth-stencil resolve is actually enabled for this subpass
                    if( (depthStencilResolve.pDepthStencilResolveAttachment) &&
                        (depthStencilResolve.pDepthStencilResolveAttachment->attachment != VK_ATTACHMENT_UNUSED) )
                    {
                        if( (depthStencilResolve.depthResolveMode != VK_RESOLVE_MODE_NONE) ||
                            (depthStencilResolve.stencilResolveMode != VK_RESOLVE_MODE_NONE) )
                        {
                            deviceProfilerSubpass.m_ResolveCount++;
                        }

                        // Check if independent resolve is used - it will count as 2 resolves
                        if( (depthStencilResolve.depthResolveMode != VK_RESOLVE_MODE_NONE) &&
                            (depthStencilResolve.stencilResolveMode != VK_RESOLVE_MODE_NONE) &&
                            (depthStencilResolve.stencilResolveMode != depthStencilResolve.depthResolveMode) )
                        {
                            deviceProfilerSubpass.m_ResolveCount++;
                        }
                    }
                }
            }

            deviceProfilerRenderPass.m_Subpasses.push_back( deviceProfilerSubpass );
        }

        // Count clear attachments
        CountRenderPassAttachmentClears( deviceProfilerRenderPass, pCreateInfo );

        // Assign default debug name for the render pass
        SetDefaultObjectName( renderPass );

        // Store render pass
        m_RenderPasses.insert( renderPass, deviceProfilerRenderPass );
    }

    /***********************************************************************************\

    Function:
        DestroyRenderPass

    Description:

    \***********************************************************************************/
    void DeviceProfiler::DestroyRenderPass( VkRenderPass renderPass )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        UnregisterObject( renderPass );

        m_RenderPasses.remove( renderPass );
    }

    /***********************************************************************************\

    Function:
        PreSubmitCommandBuffers

    Description:

    \***********************************************************************************/
    void DeviceProfiler::PreSubmitCommandBuffers( const DeviceProfilerSubmitBatch& submitBatch )
    {
    }

    /***********************************************************************************\

    Function:
        PostSubmitCommandBuffers

    Description:

    \***********************************************************************************/
    void DeviceProfiler::PostSubmitCommandBuffers( const DeviceProfilerSubmitBatch& submitBatch )
    {
        TipRangeId tip = m_pDevice->TIP.BeginFunction( __func__ );

        // Append the submit batch for aggregation
        m_DataAggregator.AppendSubmit( submitBatch );

        if( m_Config.m_FrameDelimiter == VK_PROFILER_FRAME_DELIMITER_SUBMIT_EXT )
        {
            // Begin the next frame
            BeginNextFrame();
        }

        // Get data captured during the last frame
        ResolveFrameData( tip );
    }

    /***********************************************************************************\

    Function:
        CreateSubmitBatchInfoImpl

    Description:
        Structure-independent implementation of CreateSubmitBatchInfo.

    \***********************************************************************************/
    template<typename SubmitInfoT>
    void DeviceProfiler::CreateSubmitBatchInfoImpl( VkQueue queue, uint32_t count, const SubmitInfoT* pSubmitInfo, DeviceProfilerSubmitBatch* pSubmitBatch )
    {
        using T = SubmitInfoTraits<SubmitInfoT>;

        TipGuard tip( m_pDevice->TIP, __func__ );

        // Synchronize read access to m_pCommandBuffers
        std::shared_lock lk( m_pCommandBuffers );

        // Store submitted command buffers and get results
        pSubmitBatch->m_Handle = queue;
        pSubmitBatch->m_Timestamp = m_CpuTimestampCounter.GetCurrentValue();
        pSubmitBatch->m_ThreadId = ProfilerPlatformFunctions::GetCurrentThreadId();

        for( uint32_t submitIdx = 0; submitIdx < count; ++submitIdx )
        {
            const SubmitInfoT& submitInfo = pSubmitInfo[ submitIdx ];

            // Wrap submit info into our structure
            DeviceProfilerSubmit submit;
            submit.m_pCommandBuffers.reserve( T::CommandBufferCount( submitInfo ) );
            submit.m_SignalSemaphores.reserve( T::SignalSemaphoreCount( submitInfo ) );
            submit.m_WaitSemaphores.reserve( T::WaitSemaphoreCount( submitInfo ) );

            for( uint32_t commandBufferIdx = 0; commandBufferIdx < T::CommandBufferCount( submitInfo ); ++commandBufferIdx )
            {
                // Get command buffer handle
                VkCommandBuffer commandBuffer = T::CommandBuffer( submitInfo, commandBufferIdx );
                ProfilerCommandBuffer* pProfilerCommandBuffer = m_pCommandBuffers.unsafe_at( commandBuffer ).get();

                // Dirty command buffer profiling data
                pProfilerCommandBuffer->Submit();

                submit.m_pCommandBuffers.push_back( pProfilerCommandBuffer );
            }

            // Copy semaphores
            for( uint32_t semaphoreIdx = 0; semaphoreIdx < T::SignalSemaphoreCount( submitInfo ); ++semaphoreIdx )
            {
                submit.m_SignalSemaphores.push_back( T::SignalSemaphore( submitInfo, semaphoreIdx ) );
            }

            for( uint32_t semaphoreIdx = 0; semaphoreIdx < T::WaitSemaphoreCount( submitInfo ); ++semaphoreIdx )
            {
                submit.m_WaitSemaphores.push_back( T::WaitSemaphore( submitInfo, semaphoreIdx ) );
            }

            // Store the submit wrapper
            pSubmitBatch->m_Submits.push_back( std::move( submit ) );
        }
    }

    /***********************************************************************************\

    Function:
        CreateSubmitBatchInfo

    Description:

    \***********************************************************************************/
    void DeviceProfiler::CreateSubmitBatchInfo( VkQueue queue, uint32_t count, const VkSubmitInfo* pSubmitInfo, DeviceProfilerSubmitBatch* pSubmitBatch )
    {
        CreateSubmitBatchInfoImpl<VkSubmitInfo>( queue, count, pSubmitInfo, pSubmitBatch );
    }

    /***********************************************************************************\

    Function:
        CreateSubmitBatchInfo

    Description:

    \***********************************************************************************/
    void DeviceProfiler::CreateSubmitBatchInfo( VkQueue queue, uint32_t count, const VkSubmitInfo2* pSubmitInfo, DeviceProfilerSubmitBatch* pSubmitBatch )
    {
        CreateSubmitBatchInfoImpl<VkSubmitInfo2>( queue, count, pSubmitInfo, pSubmitBatch );
    }

    /***********************************************************************************\

    Function:
        FinishFrame

    Description:

    \***********************************************************************************/
    void DeviceProfiler::FinishFrame()
    {
        TipRangeId tip = m_pDevice->TIP.BeginFunction( __func__ );

        // Update FPS counter
        m_CpuFpsCounter.Update();

        if( m_Config.m_FrameDelimiter == VK_PROFILER_FRAME_DELIMITER_PRESENT_EXT )
        {
            // Begin the next frame
            BeginNextFrame();
        }

        // Get data captured during the last frame
        ResolveFrameData( tip );
    }

    /***********************************************************************************\

    Function:
        BeginNextFrame

    Description:

    \***********************************************************************************/
    void DeviceProfiler::BeginNextFrame()
    {
        // Prepare aggregator for the next frame.
        DeviceProfilerFrame frame = {};
        frame.m_FrameIndex = m_NextFrameIndex++;
        frame.m_ThreadId = ProfilerPlatformFunctions::GetCurrentThreadId();
        frame.m_Timestamp = m_CpuTimestampCounter.GetCurrentValue();
        frame.m_FramesPerSec = m_CpuFpsCounter.GetValue();
        frame.m_FrameDelimiter = static_cast<VkProfilerFrameDelimiterEXT>( m_Config.m_FrameDelimiter.value );
        frame.m_SyncTimestamps = m_Synchronization.GetSynchronizationTimestamps();

        m_DataAggregator.AppendFrame( frame );
    }

    /***********************************************************************************\

    Function:
        ResolveFrameData

    Description:

    \***********************************************************************************/
    void DeviceProfiler::ResolveFrameData( TipRangeId& tip )
    {
        if( !m_DataAggregator.IsDataCollectionThreadRunning() )
        {
            // Collect data from the submitted command buffers
            m_DataAggregator.Aggregate();
        }

        m_pDevice->TIP.EndFunction( tip );

        // Check if new data is available
        auto pResolvedData = m_DataAggregator.GetAggregatedData();
        if( !pResolvedData.empty() )
        {
            std::scoped_lock lk( m_DataMutex );

            m_pData.insert( m_pData.end(), pResolvedData.begin(), pResolvedData.end() );

            // Return TIP data
            m_pData.back()->m_TIP = m_pDevice->TIP.GetData();

            // Free frames above the buffer size
            if( m_DataBufferSize )
            {
                while( m_pData.size() > m_DataBufferSize )
                {
                    m_pData.pop_front();
                }
            }
        }
    }

    /***********************************************************************************\

    Function:
        AllocateMemory

    Description:

    \***********************************************************************************/
    void DeviceProfiler::AllocateMemory( VkDeviceMemory allocatedMemory, const VkMemoryAllocateInfo* pAllocateInfo )
    {
        if( !m_Config.m_EnableMemoryProfiling )
        {
            return;
        }

        m_MemoryTracker.RegisterAllocation( RegisterObject( allocatedMemory ), pAllocateInfo );
    }

    /***********************************************************************************\

    Function:
        FreeMemory

    Description:

    \***********************************************************************************/
    void DeviceProfiler::FreeMemory( VkDeviceMemory allocatedMemory )
    {
        if( !m_Config.m_EnableMemoryProfiling )
        {
            return;
        }

        m_MemoryTracker.UnregisterAllocation( GetObjectHandle( allocatedMemory ) );

        UnregisterObject( allocatedMemory );
    }

    /***********************************************************************************\

    Function:
        CreateAccelerationStructure

    Description:

    \***********************************************************************************/
    void DeviceProfiler::CreateAccelerationStructure( VkAccelerationStructureKHR accelerationStructure, const VkAccelerationStructureCreateInfoKHR* pCreateInfo )
    {
        if( !m_Config.m_EnableMemoryProfiling )
        {
            return;
        }

        m_MemoryTracker.RegisterAccelerationStructure(
            RegisterObject( accelerationStructure ),
            GetObjectHandle( pCreateInfo->buffer ),
            pCreateInfo );
    }

    /***********************************************************************************\

    Function:
        DestroyAccelerationStructure

    Description:

    \***********************************************************************************/
    void DeviceProfiler::DestroyAccelerationStructure( VkAccelerationStructureKHR accelerationStructure )
    {
        if( !m_Config.m_EnableMemoryProfiling )
        {
            return;
        }

        m_MemoryTracker.UnregisterAccelerationStructure( GetObjectHandle( accelerationStructure ) );

        UnregisterObject( accelerationStructure );
    }

    /***********************************************************************************\

    Function:
        CreateMicromap

    Description:

    \***********************************************************************************/
    void DeviceProfiler::CreateMicromap( VkMicromapEXT micromap, const VkMicromapCreateInfoEXT* pCreateInfo )
    {
        if( !m_Config.m_EnableMemoryProfiling )
        {
            return;
        }

        m_MemoryTracker.RegisterMicromap(
            RegisterObject( micromap ),
            GetObjectHandle( pCreateInfo->buffer ),
            pCreateInfo );
    }

    /***********************************************************************************\

    Function:
        DestroyAccelerationStructure

    Description:

    \***********************************************************************************/
    void DeviceProfiler::DestroyMicromap( VkMicromapEXT micromap )
    {
        if( !m_Config.m_EnableMemoryProfiling )
        {
            return;
        }

        m_MemoryTracker.UnregisterMicromap( GetObjectHandle( micromap ) );

        UnregisterObject( micromap );
    }

    /***********************************************************************************\

    Function:
        CreateBuffer

    Description:

    \***********************************************************************************/
    void DeviceProfiler::CreateBuffer( VkBuffer buffer, const VkBufferCreateInfo* pCreateInfo )
    {
        if( !m_Config.m_EnableMemoryProfiling )
        {
            return;
        }

        m_MemoryTracker.RegisterBuffer( RegisterObject( buffer ), pCreateInfo );
    }

    /***********************************************************************************\

    Function:
        DestroyBuffer

    Description:

    \***********************************************************************************/
    void DeviceProfiler::DestroyBuffer( VkBuffer buffer )
    {
        if( !m_Config.m_EnableMemoryProfiling )
        {
            return;
        }

        m_MemoryTracker.UnregisterBuffer( GetObjectHandle( buffer ) );

        UnregisterObject( buffer );
    }

    /***********************************************************************************\

    Function:
        BindBufferMemory

    Description:

    \***********************************************************************************/
    void DeviceProfiler::BindBufferMemory( VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize offset )
    {
        if( !m_Config.m_EnableMemoryProfiling )
        {
            return;
        }

        m_MemoryTracker.BindBufferMemory(
            GetObjectHandle( buffer ),
            GetObjectHandle( memory ),
            offset );
    }

    /***********************************************************************************\

    Function:
        BindBufferMemory

    Description:

    \***********************************************************************************/
    void DeviceProfiler::BindBufferMemory( VkBuffer buffer, uint32_t bindCount, const VkSparseMemoryBind* pBinds )
    {
        if( !m_Config.m_EnableMemoryProfiling )
        {
            return;
        }

        const VkObjectHandle<VkBuffer> bufferHandle = GetObjectHandle( buffer );

        for( uint32_t i = 0; i < bindCount; ++i )
        {
            m_MemoryTracker.BindSparseBufferMemory(
                bufferHandle,
                pBinds[i].resourceOffset,
                GetObjectHandle( pBinds[i].memory ),
                pBinds[i].memoryOffset,
                pBinds[i].size,
                pBinds[i].flags );
        }
    }

    /***********************************************************************************\

    Function:
        CreateImage

    Description:

    \***********************************************************************************/
    void DeviceProfiler::CreateImage( VkImage image, const VkImageCreateInfo* pCreateInfo )
    {
        if( !m_Config.m_EnableMemoryProfiling )
        {
            return;
        }

        m_MemoryTracker.RegisterImage( RegisterObject( image ), pCreateInfo );
    }

    /***********************************************************************************\

    Function:
        DestroyImage

    Description:

    \***********************************************************************************/
    void DeviceProfiler::DestroyImage( VkImage image )
    {
        if( !m_Config.m_EnableMemoryProfiling )
        {
            return;
        }

        m_MemoryTracker.UnregisterImage( GetObjectHandle( image ) );

        UnregisterObject( image );
    }

    /***********************************************************************************\

    Function:
        BindImageMemory

    Description:

    \***********************************************************************************/
    void DeviceProfiler::BindImageMemory( VkImage image, VkDeviceMemory memory, VkDeviceSize offset )
    {
        if( !m_Config.m_EnableMemoryProfiling )
        {
            return;
        }

        m_MemoryTracker.BindImageMemory(
            GetObjectHandle( image ),
            GetObjectHandle( memory ),
            offset );
    }

    /***********************************************************************************\

    Function:
        BindImageMemory

    Description:

    \***********************************************************************************/
    void DeviceProfiler::BindImageMemory( VkImage image, uint32_t bindCount, const VkSparseMemoryBind* pBinds )
    {
        if( !m_Config.m_EnableMemoryProfiling )
        {
            return;
        }

        const VkObjectHandle<VkImage> imageHandle = GetObjectHandle( image );

        for( uint32_t i = 0; i < bindCount; ++i )
        {
            m_MemoryTracker.BindSparseImageMemory(
                imageHandle,
                pBinds[i].resourceOffset,
                GetObjectHandle( pBinds[i].memory ),
                pBinds[i].memoryOffset,
                pBinds[i].size,
                pBinds[i].flags );
        }
    }

    /***********************************************************************************\

    Function:
        BindImageMemory

    Description:

    \***********************************************************************************/
    void DeviceProfiler::BindImageMemory( VkImage image, uint32_t bindCount, const VkSparseImageMemoryBind* pBinds )
    {
        if( !m_Config.m_EnableMemoryProfiling )
        {
            return;
        }

        const VkObjectHandle<VkImage> imageHandle = GetObjectHandle( image );

        for( uint32_t i = 0; i < bindCount; ++i )
        {
            m_MemoryTracker.BindSparseImageMemory(
                imageHandle,
                pBinds[i].subresource,
                pBinds[i].offset,
                pBinds[i].extent,
                GetObjectHandle( pBinds[i].memory ),
                pBinds[i].memoryOffset,
                pBinds[i].flags );
        }
    }

    /***********************************************************************************\

    Function:
        CreateShaderTuple

    Description:

    \***********************************************************************************/
    void DeviceProfiler::SetPipelineShaderProperties( DeviceProfilerPipeline& pipeline, uint32_t stageCount, const VkPipelineShaderStageCreateInfo* pStages )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        // Capture pipeline executable properties
        if( ShouldCapturePipelineExecutableProperties() )
        {
            VkPipelineInfoKHR pipelineInfo = {};
            pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INFO_KHR;
            pipelineInfo.pipeline = pipeline.m_Handle;

            // Get number of executables collected for this pipeline
            uint32_t pipelineExecutablesCount = 0;
            VkResult result = m_pDevice->Callbacks.GetPipelineExecutablePropertiesKHR(
                m_pDevice->Handle,
                &pipelineInfo,
                &pipelineExecutablesCount,
                nullptr );

            std::vector<VkPipelineExecutablePropertiesKHR> pipelineExecutables( 0 );
            if( result == VK_SUCCESS && pipelineExecutablesCount > 0 )
            {
                pipelineExecutables.resize( pipelineExecutablesCount );
                memset( pipelineExecutables.data(), 0,
                    sizeof( VkPipelineExecutablePropertiesKHR ) * pipelineExecutablesCount );

                for( uint32_t i = 0; i < pipelineExecutablesCount; ++i )
                {
                    pipelineExecutables[ i ].sType = VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_PROPERTIES_KHR;
                }

                m_pDevice->Callbacks.GetPipelineExecutablePropertiesKHR(
                    m_pDevice->Handle,
                    &pipelineInfo,
                    &pipelineExecutablesCount,
                    pipelineExecutables.data() );
            }

            // Preallocate space for the shader executables
            pipeline.m_ShaderTuple.m_ShaderExecutables.resize( pipelineExecutablesCount );

            std::vector<VkPipelineExecutableStatisticKHR> executableStatistics( 0 );
            std::vector<VkPipelineExecutableInternalRepresentationKHR> executableInternalRepresentations( 0 );

            for( uint32_t i = 0; i < pipelineExecutablesCount; ++i )
            {
                VkPipelineExecutableInfoKHR executableInfo = {};
                executableInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_INFO_KHR;
                executableInfo.executableIndex = i;
                executableInfo.pipeline = pipeline.m_Handle;

                // Enumerate shader statistics for the executable
                uint32_t executableStatisticsCount = 0;
                result = m_pDevice->Callbacks.GetPipelineExecutableStatisticsKHR(
                    m_pDevice->Handle,
                    &executableInfo,
                    &executableStatisticsCount,
                    nullptr );

                executableStatistics.clear();

                if( result == VK_SUCCESS && executableStatisticsCount > 0 )
                {
                    executableStatistics.resize( executableStatisticsCount );
                    memset( executableStatistics.data(), 0,
                        sizeof( VkPipelineExecutableStatisticKHR ) * executableStatisticsCount );

                    for( uint32_t j = 0; j < executableStatisticsCount; ++j )
                    {
                        executableStatistics[ j ].sType = VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_STATISTIC_KHR;
                    }

                    result = m_pDevice->Callbacks.GetPipelineExecutableStatisticsKHR(
                        m_pDevice->Handle,
                        &executableInfo,
                        &executableStatisticsCount,
                        executableStatistics.data() );

                    if( result != VK_SUCCESS )
                    {
                        executableStatistics.clear();
                    }
                }

                // Enumerate shader internal representations
                uint32_t executableInternalRepresentationsCount = 0;
                result = m_pDevice->Callbacks.GetPipelineExecutableInternalRepresentationsKHR(
                    m_pDevice->Handle,
                    &executableInfo,
                    &executableInternalRepresentationsCount,
                    nullptr );

                executableInternalRepresentations.clear();

                if( result == VK_SUCCESS && executableInternalRepresentationsCount > 0 )
                {
                    executableInternalRepresentations.resize( executableInternalRepresentationsCount );
                    memset( executableInternalRepresentations.data(), 0,
                        sizeof( VkPipelineExecutableInternalRepresentationKHR ) * executableInternalRepresentationsCount );

                    for( uint32_t j = 0; j < executableInternalRepresentationsCount; ++j )
                    {
                        executableInternalRepresentations[ j ].sType = VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_INTERNAL_REPRESENTATION_KHR;
                    }

                    result = m_pDevice->Callbacks.GetPipelineExecutableInternalRepresentationsKHR(
                        m_pDevice->Handle,
                        &executableInfo,
                        &executableInternalRepresentationsCount,
                        executableInternalRepresentations.data() );

                    if( result != VK_SUCCESS )
                    {
                        executableInternalRepresentations.clear();
                    }
                }

                // Initialize the shader executable
                result = pipeline.m_ShaderTuple.m_ShaderExecutables[ i ].Initialize(
                    &pipelineExecutables[ i ],
                    executableStatisticsCount,
                    executableStatistics.data(),
                    executableInternalRepresentationsCount,
                    executableInternalRepresentations.data() );

                if( result == VK_INCOMPLETE )
                {
                    // Call vkGetPipelineExecutableInternalRepresentationsKHR to write the internal representations
                    // to the shader executable's internal memory.
                    m_pDevice->Callbacks.GetPipelineExecutableInternalRepresentationsKHR(
                        m_pDevice->Handle,
                        &executableInfo,
                        &executableInternalRepresentationsCount,
                        executableInternalRepresentations.data() );
                }
            }
        }

        // Preallocate memory for the pipeline shader stages
        pipeline.m_ShaderTuple.m_Shaders.resize( stageCount );

        for( uint32_t i = 0; i < stageCount; ++i )
        {
            std::shared_ptr<ProfilerShaderModule> pShaderModule = nullptr;

            // If module is VK_NULL_HANDLE, either the pNext chain contains a VkShaderModuleCreateInfo, or an identifier is provided.
            // In the latter case the bytecode may not be available if it is cached.
            if( pStages[i].module == VK_NULL_HANDLE )
            {
                for( const auto& it : PNextIterator( pStages[i].pNext ) )
                {
                    if( it.sType == VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO )
                    {
                        const VkShaderModuleCreateInfo& shaderModuleCreateInfo =
                            reinterpret_cast<const VkShaderModuleCreateInfo&>( it );

                        // Get shader identifier from the shader module create info.
                        VkShaderModuleIdentifierEXT shaderModuleIdentifier = {};
                        shaderModuleIdentifier.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_IDENTIFIER_EXT;

                        if( m_ShaderModuleIdentifierEnabled )
                        {
                            m_pDevice->Callbacks.GetShaderModuleCreateInfoIdentifierEXT(
                                m_pDevice->Handle,
                                &shaderModuleCreateInfo,
                                &shaderModuleIdentifier );
                        }

                        // Create shader object from the provided bytecode.
                        pShaderModule = std::make_shared<ProfilerShaderModule>(
                            reinterpret_cast<const uint32_t*>( shaderModuleCreateInfo.pCode ),
                            shaderModuleCreateInfo.codeSize,
                            shaderModuleIdentifier.identifier,
                            shaderModuleIdentifier.identifierSize );

                        break;
                    }

                    if( it.sType == VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_MODULE_IDENTIFIER_CREATE_INFO_EXT )
                    {
                        const VkPipelineShaderStageModuleIdentifierCreateInfoEXT& moduleIdentifierCreateInfo =
                            reinterpret_cast<const VkPipelineShaderStageModuleIdentifierCreateInfoEXT&>( it );

                        // Construct a shader module with no bytecode.
                        pShaderModule = std::make_shared<ProfilerShaderModule>(
                            nullptr, 0,
                            moduleIdentifierCreateInfo.pIdentifier,
                            moduleIdentifierCreateInfo.identifierSize );
                    }
                }
            }
            else
            {
                // VkShaderModule entry should already be in the map.
                pShaderModule = m_pShaderModules.at( pStages[i].module );
            }

            ProfilerShader& shader = pipeline.m_ShaderTuple.m_Shaders[ i ];
            shader.m_Hash = pShaderModule ? pShaderModule->m_Hash : 0;
            shader.m_Index = i;
            shader.m_Stage = pStages[ i ].stage;
            shader.m_EntryPoint = pStages[ i ].pName;
            shader.m_pShaderModule = std::move( pShaderModule );

            // Hash the entrypoint and append it to the final hash
            shader.m_Hash ^= Farmhash::Fingerprint32( shader.m_EntryPoint.data(), shader.m_EntryPoint.length() );
        }

        // Finalize pipeline creation
        pipeline.Finalize();
    }

    /***********************************************************************************\

    Function:
        GetObjectName

    Description:
        Get object name.

    \***********************************************************************************/
    const char* DeviceProfiler::GetObjectName( VkObject object ) const
    {
        // Grab the latest hangle to the object.
        object = GetObjectHandle( object );

        std::shared_lock lock( m_ObjectNames );
        auto it = m_ObjectNames.unsafe_find( object );
        if( it != m_ObjectNames.end() )
        {
            return it->second.c_str();
        }

        return nullptr;
    }

    /***********************************************************************************\

    Function:
        SetObjectName

    Description:
        Set custom object name.

    \***********************************************************************************/
    void DeviceProfiler::SetObjectName( VkObject object, const char* pName )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        // Grab the latest handle to the object.
        object = GetObjectHandle( object );

        m_ObjectNames.insert_or_assign( object, pName );
    }

    /***********************************************************************************\

    Function:
        SetDefaultObjectName

    Description:
        Set default object name.

    \***********************************************************************************/
    void DeviceProfiler::SetDefaultObjectName( VkObject object )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        // There is special function for VkPipeline objects
        if( object.m_Type == VK_OBJECT_TYPE_PIPELINE )
        {
            SetDefaultObjectName( VkObject_Traits<VkPipeline>::GetObjectHandleAsVulkanHandle( object.m_Handle ) );
            return;
        }

        VkObject_Runtime_Traits traits = VkObject_Runtime_Traits::FromObjectType( object.m_Type );
        assert( traits.ObjectTypeName != nullptr );

        char pObjectDebugName[ 64 ] = {};
        ProfilerStringFunctions::Format( pObjectDebugName, "%s 0x%016" PRIx64, traits.ObjectTypeName, object.m_Handle );

        m_ObjectNames.insert_or_assign( object, pObjectDebugName );
    }

    /***********************************************************************************\

    Function:
        SetDefaultObjectName

    Description:
        Set default pipeline name consisting of shader tuple hashes.

    \***********************************************************************************/
    void DeviceProfiler::SetDefaultObjectName( VkPipeline object )
    {
        SetDefaultPipelineName( GetPipeline( object ) );
    }

    /***********************************************************************************\

    Function:
        SetDefaultPipelineName

    Description:
        Set default pipeline name consisting of shader tuple hashes.

    \***********************************************************************************/
    void DeviceProfiler::SetDefaultPipelineName( const DeviceProfilerPipeline& pipeline, bool deferred )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        std::string pipelineName;

        if( pipeline.m_BindPoint == VK_PIPELINE_BIND_POINT_GRAPHICS )
        {
            pipelineName = pipeline.m_ShaderTuple.GetShaderStageHashesString(
                VK_SHADER_STAGE_VERTEX_BIT |
                VK_SHADER_STAGE_TASK_BIT_EXT |
                VK_SHADER_STAGE_MESH_BIT_EXT |
                VK_SHADER_STAGE_FRAGMENT_BIT,
                true /*skipEmptyStages*/ );
        }

        if( pipeline.m_BindPoint == VK_PIPELINE_BIND_POINT_COMPUTE )
        {
            pipelineName = pipeline.m_ShaderTuple.GetShaderStageHashesString(
                VK_SHADER_STAGE_COMPUTE_BIT );
        }

        if( pipeline.m_BindPoint == VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR )
        {
            pipelineName = pipeline.m_ShaderTuple.GetShaderStageHashesString(
                VK_SHADER_STAGE_RAYGEN_BIT_KHR |
                VK_SHADER_STAGE_ANY_HIT_BIT_KHR |
                VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR );
        }

        if( !pipelineName.empty() )
        {
            // When deferred operation is joined, the application may have already set a name for the pipeline.
            // Don't set the default in such case.
            if( deferred )
            {
                m_ObjectNames.insert( pipeline.m_Handle, std::move( pipelineName ) );
            }
            else
            {
                m_ObjectNames.insert_or_assign( pipeline.m_Handle, std::move( pipelineName ) );
            }
        }
    }

    /***********************************************************************************\

    Function:
        CreateInternalPipeline

    Description:
        Create internal pipeline to track drawcalls which don't require any user-provided
        pipelines but execude some tasks on the GPU.

    \***********************************************************************************/
    void DeviceProfiler::CreateInternalPipeline( DeviceProfilerPipelineType type, const char* pName )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        DeviceProfilerPipeline internalPipeline;
        internalPipeline.m_Handle = RegisterObject( (VkPipeline)type );
        internalPipeline.m_ShaderTuple.m_Hash = (uint32_t)type;
        internalPipeline.m_Type = type;
        internalPipeline.m_Internal = true;

        // Assign name for the internal pipeline
        SetObjectName( internalPipeline.m_Handle, pName );

        m_Pipelines.insert( internalPipeline.m_Handle, internalPipeline );
    }

    /***********************************************************************************\

    Function:
        FreeCommandBuffer

    Description:

    \***********************************************************************************/
    decltype(DeviceProfiler::m_pCommandBuffers)::iterator DeviceProfiler::FreeCommandBuffer( VkCommandBuffer commandBuffer )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        // Assume m_CommandBuffers map is already locked
        auto it = m_pCommandBuffers.unsafe_find( commandBuffer );

        // Collect command buffer data now, command buffer won't be available later
        m_DataAggregator.Aggregate( it->second.get() );

        return m_pCommandBuffers.unsafe_remove( it );
    }

    /***********************************************************************************\

    Function:
        FreeCommandBuffer

    Description:

    \***********************************************************************************/
    decltype(DeviceProfiler::m_pCommandBuffers)::iterator DeviceProfiler::FreeCommandBuffer( decltype(m_pCommandBuffers)::iterator it )
    {
        TipGuard tip( m_pDevice->TIP, __func__ );

        // Collect command buffer data now, command buffer won't be available later
        m_DataAggregator.Aggregate( it->second.get() );

        // Assume m_CommandBuffers map is already locked
        return m_pCommandBuffers.unsafe_remove( it );
    }

    /***********************************************************************************\

    Function:
        RegisterObject

    Description:
        Save object handle and its creation time to distinguish between instances of
        the same object handle in time.

    \***********************************************************************************/
    template<typename ObjectT>
    VkObjectHandle<ObjectT> DeviceProfiler::RegisterObject( ObjectT object )
    {
        uint64_t creationTime = m_CpuTimestampCounter.GetCurrentValue();

        m_ObjectCreateTimes.insert_or_assign( object, creationTime );

        return VkObjectHandle<ObjectT>( object, creationTime );
    }

    /***********************************************************************************\

    Function:
        UnregisterObject

    Description:
        Remove the object handle from the profiler.

    \***********************************************************************************/
    template<typename ObjectT>
    void DeviceProfiler::UnregisterObject( ObjectT object )
    {
        m_ObjectCreateTimes.remove( object );
    }
}

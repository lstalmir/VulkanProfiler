// Copyright (c) 2026 Lukasz Stalmirski
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

#include <profiler/profiler_helpers.h>
#include <profiler_overlay/profiler_overlay.h>
#include <profiler_overlay/profiler_overlay_layer_backend.h>
#include <profiler_layer_objects/VkQueue_object.h>

#include <iostream>

#define SDL_ENABLE_OLD_NAMES
#include <SDL3/SDL.h>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

/***********************************************************************************\

Class:
    SDLOverlayBackend

Description:

\***********************************************************************************/
class SDLOverlayBackend : public Profiler::OverlayBackend
{
public:
    explicit SDLOverlayBackend( SDL_Window* pWindow );

    bool PrepareImGuiBackend() override;
    void DestroyImGuiBackend() override;

    void WaitIdle() override;

    bool NewFrame() override;
    void RenderDrawData( ImDrawData* draw_data ) override;

    float GetDPIScale() const override;
    ImVec2 GetRenderArea() const override;

    uint64_t CreateImage( int width, int height, const void* pData ) override;
    void DestroyImage( uint64_t image ) override;
    void CreateFontsImage() override;
    void DestroyFontsImage() override;

    void ProcessEvent( SDL_Event* pEvent );
    void Present();

private:
    SDL_Window* m_pWindow;
    SDL_Renderer* m_pRenderer;
    SDL_GPUDevice* m_pDevice;
};

SDLOverlayBackend::SDLOverlayBackend( SDL_Window* pWindow )
    : m_pWindow( pWindow )
    , m_pRenderer( nullptr )
    , m_pDevice( nullptr )
{
}

bool SDLOverlayBackend::PrepareImGuiBackend()
{
    m_pRenderer = SDL_CreateRenderer( m_pWindow, nullptr );

    if( !m_pRenderer )
    {
        std::cerr << "Failed to create SDL renderer: " << SDL_GetError() << std::endl;
        return false;
    }

    if( !ImGui_ImplSDL3_InitForSDLRenderer( m_pWindow, m_pRenderer ) )
    {
        std::cerr << "Failed to initialize ImGui SDL3 backend" << std::endl;

        SDL_DestroyRenderer( m_pRenderer );
        m_pRenderer = nullptr;

        return false;
    }

    if( !ImGui_ImplSDLRenderer3_Init( m_pRenderer ) )
    {
        std::cerr << "Failed to initialize ImGui SDL Renderer backend" << std::endl;

        ImGui_ImplSDL3_Shutdown();

        SDL_DestroyRenderer( m_pRenderer );
        m_pRenderer = nullptr;

        return false;
    }

    m_pDevice = SDL_GetGPURendererDevice( m_pRenderer );

    return true;
}

void SDLOverlayBackend::DestroyImGuiBackend()
{
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();

    SDL_DestroyRenderer( m_pRenderer );
    m_pRenderer = nullptr;
    m_pDevice = nullptr;
}

void SDLOverlayBackend::WaitIdle()
{
    if( m_pDevice )
    {
        SDL_WaitForGPUIdle( m_pDevice );
    }
}

bool SDLOverlayBackend::NewFrame()
{
    SDL_RenderClear( m_pRenderer );

    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplSDLRenderer3_NewFrame();
    return true;
}

void SDLOverlayBackend::RenderDrawData( ImDrawData* draw_data )
{
    ImGui_ImplSDLRenderer3_RenderDrawData( draw_data, m_pRenderer );
}

float SDLOverlayBackend::GetDPIScale() const
{
    return 1.0f;
}

ImVec2 SDLOverlayBackend::GetRenderArea() const
{
    int w, h;
    SDL_GetWindowSize( m_pWindow, &w, &h );
    return ImVec2( static_cast<float>( w ), static_cast<float>( h ) );
}

uint64_t SDLOverlayBackend::CreateImage( int width, int height, const void* pData )
{
    SDL_Surface* pSurface = SDL_CreateSurface( width, height, SDL_PIXELFORMAT_RGBA8888 );
    if( !pSurface )
    {
        return 0;
    }

    if( pData )
    {
        if( !SDL_LockSurface( pSurface ) )
        {
            SDL_DestroySurface( pSurface );
            return 0;
        }

        std::memcpy( pSurface->pixels, pData, static_cast<size_t>( width * height * 4 ) );
        SDL_UnlockSurface( pSurface );
    }

    SDL_Texture* pTexture = SDL_CreateTextureFromSurface(
        m_pRenderer,
        pSurface );

    SDL_DestroySurface( pSurface );

    return static_cast<uint64_t>( reinterpret_cast<uintptr_t>( pTexture ) );
}

void SDLOverlayBackend::DestroyImage( uint64_t image )
{
    SDL_Texture* pTexture = reinterpret_cast<SDL_Texture*>( static_cast<uintptr_t>( image ) );
    if( pTexture )
    {
        SDL_DestroyTexture( pTexture );
    }
}

void SDLOverlayBackend::CreateFontsImage()
{
    ImGui_ImplSDLRenderer3_CreateFontsTexture();
}

void SDLOverlayBackend::DestroyFontsImage()
{
    ImGui_ImplSDLRenderer3_DestroyFontsTexture();
}

void SDLOverlayBackend::ProcessEvent( SDL_Event* pEvent )
{
    ImGui_ImplSDL3_ProcessEvent( pEvent );
}

void SDLOverlayBackend::Present()
{
    SDL_RenderPresent( m_pRenderer );
}

/***********************************************************************************\

Class:
    MockProfilerFrontend

Description:

\***********************************************************************************/
class MockProfilerFrontend : public Profiler::DeviceProfilerFrontend
{
public:
    bool IsAvailable() override;

    const VkApplicationInfo& GetApplicationInfo() override;
    const VkPhysicalDeviceProperties& GetPhysicalDeviceProperties() override;
    const VkPhysicalDeviceMemoryProperties& GetPhysicalDeviceMemoryProperties() override;
    const std::vector<VkQueueFamilyProperties>& GetQueueFamilyProperties() override;

    const std::unordered_set<std::string>& GetEnabledInstanceExtensions() override;
    const std::unordered_set<std::string>& GetEnabledDeviceExtensions() override;

    const std::unordered_map<VkQueue, Profiler::VkQueue_Object>& GetDeviceQueues() override;

    bool SupportsCustomPerformanceMetricsSets() override;
    uint32_t CreateCustomPerformanceMetricsSet( const VkProfilerCustomPerformanceMetricsSetCreateInfoEXT* pCreateInfo ) override;
    void DestroyCustomPerformanceMetricsSet( uint32_t setIndex ) override;
    void UpdateCustomPerformanceMetricsSets( uint32_t updateCount, const VkProfilerCustomPerformanceMetricsSetUpdateInfoEXT* pUpdateInfos ) override;
    uint32_t GetPerformanceCounterProperties( uint32_t counterCount, VkProfilerPerformanceCounterProperties2EXT* pCounters ) override;
    uint32_t GetPerformanceMetricsSets( uint32_t setCount, VkProfilerPerformanceMetricsSetProperties2EXT* pSets ) override;
    void GetPerformanceMetricsSetProperties( uint32_t setIndex, VkProfilerPerformanceMetricsSetProperties2EXT* pProperties ) override;
    uint32_t GetPerformanceMetricsSetCounterProperties( uint32_t setIndex, uint32_t counterCount, VkProfilerPerformanceCounterProperties2EXT* pCounters ) override;
    uint32_t GetPerformanceCounterRequiredPasses( uint32_t counterCount, const uint32_t* pCounters ) override;
    void GetAvailablePerformanceCounters( uint32_t selectedCounterCount, const uint32_t* pSelectedCounters, uint32_t& availableCounterCount, uint32_t* pAvailableCounters ) override;
    VkResult SetPreformanceMetricsSetIndex( uint32_t setIndex ) override;
    uint32_t GetPerformanceMetricsSetIndex() override;
    VkProfilerPerformanceCountersSamplingModeEXT GetPerformanceCountersSamplingMode() override;

    uint64_t GetDeviceCreateTimestamp( VkTimeDomainEXT timeDomain ) override;
    uint64_t GetHostTimestampFrequency( VkTimeDomainEXT timeDomain ) override;

    const Profiler::DeviceProfilerConfig& GetProfilerConfig() override;

    VkProfilerFrameDelimiterEXT GetProfilerFrameDelimiter() override;
    VkResult SetProfilerFrameDelimiter( VkProfilerFrameDelimiterEXT frameDelimiter ) override;

    VkProfilerModeEXT GetProfilerSamplingMode() override;
    VkResult SetProfilerSamplingMode( VkProfilerModeEXT mode ) override;

    std::string GetObjectName( const Profiler::VkObject& object ) override;
    void SetObjectName( const Profiler::VkObject& object, const std::string& name ) override;

    std::shared_ptr<Profiler::DeviceProfilerFrameData> GetData() override;
    void SetDataBufferSize( uint32_t maxFrames ) override;
};

bool MockProfilerFrontend::IsAvailable()
{
    return true;
}

const VkApplicationInfo& MockProfilerFrontend::GetApplicationInfo()
{
    static const VkApplicationInfo applicationInfo = {
        VK_STRUCTURE_TYPE_APPLICATION_INFO,
        nullptr,
        "Mock Application",
        VK_MAKE_API_VERSION( 0, 1, 0, 0 ),
        "Mock Engine",
        VK_MAKE_API_VERSION( 0, 1, 0, 0 )
    };
    return applicationInfo;
}

const VkPhysicalDeviceProperties& MockProfilerFrontend::GetPhysicalDeviceProperties()
{
    static const VkPhysicalDeviceProperties deviceProperties = {
        VK_API_VERSION_1_0,
        VK_MAKE_API_VERSION( 0, 1, 0, 0 ),
        0,
        0,
        VK_PHYSICAL_DEVICE_TYPE_OTHER,
        "Mock GPU"
    };
    return deviceProperties;
}

const VkPhysicalDeviceMemoryProperties& MockProfilerFrontend::GetPhysicalDeviceMemoryProperties()
{
    static const VkPhysicalDeviceMemoryProperties memoryProperties = {
        2,
        { { VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0 }, { VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 1 } },
        2,
        { { 1024 * 1024 * 1024, VK_MEMORY_HEAP_DEVICE_LOCAL_BIT }, { 512 * 1024 * 1024, 0 } }
    };
    return memoryProperties;
}

const std::vector<VkQueueFamilyProperties>& MockProfilerFrontend::GetQueueFamilyProperties()
{
    static const std::vector<VkQueueFamilyProperties> queueFamilyProperties = {
        { VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT, 1, 64 },
        { VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT, 1, 64 },
        { VK_QUEUE_TRANSFER_BIT, 1, 0 }
    };
    return queueFamilyProperties;
}

const std::unordered_set<std::string>& MockProfilerFrontend::GetEnabledInstanceExtensions()
{
    static const std::unordered_set<std::string> instanceExtensions = {};
    return instanceExtensions;
}

const std::unordered_set<std::string>& MockProfilerFrontend::GetEnabledDeviceExtensions()
{
    static const std::unordered_set<std::string> deviceExtensions = {};
    return deviceExtensions;
}

const std::unordered_map<VkQueue, Profiler::VkQueue_Object>& MockProfilerFrontend::GetDeviceQueues()
{
    static const std::unordered_map<VkQueue, Profiler::VkQueue_Object> deviceQueues = {
        //{ (VkQueue)0x1, Profiler::VkQueue_Object( (VkQueue)0x1, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT, 0, 0 ) }
    };
    return deviceQueues;
}

bool MockProfilerFrontend::SupportsCustomPerformanceMetricsSets()
{
    return false;
}

uint32_t MockProfilerFrontend::CreateCustomPerformanceMetricsSet( const VkProfilerCustomPerformanceMetricsSetCreateInfoEXT* pCreateInfo )
{
    return UINT32_MAX;
}

void MockProfilerFrontend::DestroyCustomPerformanceMetricsSet( uint32_t setIndex )
{
}

void MockProfilerFrontend::UpdateCustomPerformanceMetricsSets( uint32_t updateCount, const VkProfilerCustomPerformanceMetricsSetUpdateInfoEXT* pUpdateInfos )
{
}

uint32_t MockProfilerFrontend::GetPerformanceCounterProperties( uint32_t counterCount, VkProfilerPerformanceCounterProperties2EXT* pCounters )
{
    return 0;
}

uint32_t MockProfilerFrontend::GetPerformanceMetricsSets( uint32_t setCount, VkProfilerPerformanceMetricsSetProperties2EXT* pSets )
{
    return 0;
}

void MockProfilerFrontend::GetPerformanceMetricsSetProperties( uint32_t setIndex, VkProfilerPerformanceMetricsSetProperties2EXT* pProperties )
{
}

uint32_t MockProfilerFrontend::GetPerformanceMetricsSetCounterProperties( uint32_t setIndex, uint32_t counterCount, VkProfilerPerformanceCounterProperties2EXT* pCounters )
{
    return 0;
}

uint32_t MockProfilerFrontend::GetPerformanceCounterRequiredPasses( uint32_t counterCount, const uint32_t* pCounters )
{
    return 0;
}

void MockProfilerFrontend::GetAvailablePerformanceCounters( uint32_t selectedCounterCount, const uint32_t* pSelectedCounters, uint32_t& availableCounterCount, uint32_t* pAvailableCounters )
{
    availableCounterCount = 0;
}

VkResult MockProfilerFrontend::SetPreformanceMetricsSetIndex( uint32_t setIndex )
{
    return VK_ERROR_FEATURE_NOT_PRESENT;
}

uint32_t MockProfilerFrontend::GetPerformanceMetricsSetIndex()
{
    return 0;
}

VkProfilerPerformanceCountersSamplingModeEXT MockProfilerFrontend::GetPerformanceCountersSamplingMode()
{
    return VK_PROFILER_PERFORMANCE_COUNTERS_SAMPLING_MODE_QUERY_EXT;
}

uint64_t MockProfilerFrontend::GetDeviceCreateTimestamp( VkTimeDomainEXT timeDomain )
{
    return 0;
}

uint64_t MockProfilerFrontend::GetHostTimestampFrequency( VkTimeDomainEXT timeDomain )
{
    return 1000000000; // 1 GHz
}

const Profiler::DeviceProfilerConfig& MockProfilerFrontend::GetProfilerConfig()
{
    static const Profiler::DeviceProfilerConfig config = {};
    return config;
}

VkProfilerFrameDelimiterEXT MockProfilerFrontend::GetProfilerFrameDelimiter()
{
    return VK_PROFILER_FRAME_DELIMITER_PRESENT_EXT;
}

VkResult MockProfilerFrontend::SetProfilerFrameDelimiter( VkProfilerFrameDelimiterEXT frameDelimiter )
{
    return VK_ERROR_FEATURE_NOT_PRESENT;
}

VkProfilerModeEXT MockProfilerFrontend::GetProfilerSamplingMode()
{
    return VK_PROFILER_MODE_PER_DRAWCALL_EXT;
}

VkResult MockProfilerFrontend::SetProfilerSamplingMode( VkProfilerModeEXT mode )
{
    return VK_ERROR_FEATURE_NOT_PRESENT;
}

std::string MockProfilerFrontend::GetObjectName( const Profiler::VkObject& object )
{
    return std::string();
}

void MockProfilerFrontend::SetObjectName( const Profiler::VkObject& object, const std::string& name )
{
}

std::shared_ptr<Profiler::DeviceProfilerFrameData> MockProfilerFrontend::GetData()
{
    auto pFrame = std::make_shared<Profiler::DeviceProfilerFrameData>();

    auto& memoryProperties = GetPhysicalDeviceMemoryProperties();
    for( uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i )
        pFrame->m_Memory.m_Types.emplace_back();
    for( uint32_t i = 0; i < memoryProperties.memoryHeapCount; ++i )
        pFrame->m_Memory.m_Heaps.emplace_back();

    return pFrame;
}

void MockProfilerFrontend::SetDataBufferSize( uint32_t maxFrames )
{
}

/***********************************************************************************\

Function:
    main

Description:

\***********************************************************************************/
int main()
{
    if( !SDL_Init( SDL_INIT_EVENTS | SDL_INIT_VIDEO ) )
    {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Window* pWindow = SDL_CreateWindow( "Profiler Overlay Test", 1280, 720, 0 );
    if( !pWindow )
    {
        std::cerr << "Failed to create SDL window: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDLOverlayBackend backend( pWindow );
    MockProfilerFrontend frontend;

    Profiler::ProfilerOverlayOutput overlay( frontend, backend );
    if( !overlay.Initialize() )
    {
        std::cerr << "Failed to initialize Profiler Overlay" << std::endl;
        return -1;
    }

    bool running = true;
    while( running )
    {
        SDL_Event event;
        while( SDL_PollEvent( &event ) )
        {
            if( event.type == SDL_QUIT )
            {
                running = false;
            }

            backend.ProcessEvent( &event );
        }

        if( running )
        {
            overlay.Update();
            overlay.Present();

            backend.Present();
        }
    }

    overlay.Destroy();
    return 0;
}

#include <imgui_impl_sdl3.cpp>
#include <imgui_impl_sdlrenderer3.cpp>

// Copyright (c) 2019-2026 Lukasz Stalmirski
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

#pragma once

// Vulkan headers
#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>

#include "VkLayer_profiler_layer.generated.h"
#include "profiler_vulkan_state.h"

// Profiler extension
#include "profiler_ext/VkProfilerEXT.h"
#include "profiler_ext/VkProfilerObjectEXT.h"
#include "profiler/profiler.h"

// Linux: Undefine names conflicting in gtest
#undef None
#undef Bool

// GTest headers
#include <gtest/gtest.h>

// Windows: Undefine names conflicting in tests
#undef SetEnvironmentVariable
#undef GetEnvironmentVariable

#define SkipIfUnsupported( feature )                    \
    do                                                  \
    {                                                   \
        const auto& f = ( feature );                    \
        if( !f.Enabled )                                \
        {                                               \
            GTEST_SKIP() << f.Name << " not supported"; \
            return;                                     \
        }                                               \
    } while( 0 )

namespace Profiler
{
    /***********************************************************************************\

    Class:
        ProfilerBaseULT

    Description:
        Base class for all profiler tests.

    \***********************************************************************************/
    class ProfilerBaseULT : public testing::Test
    {
    protected:
        VulkanState* Vk = {};
        DeviceProfiler* Prof = {};

        // Executed before each test
        inline void SetUp() override
        {
            Test::SetUp();

            VulkanState::CreateInfo createInfo;
            SetUpVulkan( createInfo );

            try
            {
                Vk = new VulkanState( createInfo );

                // Get layer objects
                {
                    auto vkGetProfilerEXT = (PFN_vkGetProfilerEXT)vkGetDeviceProcAddr( Vk->Device, "vkGetProfilerEXT" );
                    if( !vkGetProfilerEXT )
                    {
                        throw VulkanError( VK_ERROR_EXTENSION_NOT_PRESENT, "vkGetProfilerEXT" );
                    }

                    VkProfilerEXT profiler = VK_NULL_HANDLE;
                    vkGetProfilerEXT( Vk->Device, &profiler );

                    Prof = (DeviceProfiler*)( profiler );
                }
            }
            catch( const VulkanError& error )
            {
                if( (error.Result == VK_ERROR_FEATURE_NOT_PRESENT) ||
                    (error.Result == VK_ERROR_EXTENSION_NOT_PRESENT) ||
                    (error.Result == VK_ERROR_LAYER_NOT_PRESENT) )
                {
                    GTEST_SKIP() << "Required extension, feature or layer is not present (" << error.Message << ")";
                }
                else
                {
                    GTEST_FAIL() << "Failed to set up Vulkan. " << error.Message << " (VkResult = " << error.Result << ")";
                }
            }
        }

        // Executed after each test
        inline void TearDown() override
        {
            delete Vk;

            Test::TearDown();
        }

        // Overridden in tests that require specific extensions or features
        inline virtual void SetUpVulkan( VulkanState::CreateInfo& createInfo ) {}
    };

    /***********************************************************************************\

    Class:
        EnvironmentVariableScope

    Description:
        RAII class for managing environment variables within a scope.

    \***********************************************************************************/
    class EnvironmentVariableScope
    {
    public:
        EnvironmentVariableScope( const std::string& name, const std::optional<std::string>& value )
            : m_Name( name )
            , m_OriginalValue( Get() )
        {
            Set( value );
        }

        ~EnvironmentVariableScope()
        {
            Set( m_OriginalValue );
        }

        inline bool Unset()
        {
            return Set( std::nullopt );
        }

        inline bool Set( const std::optional<std::string>& value )
        {
#ifdef WIN32
            const char* pValue = nullptr;

            if( value.has_value() )
            {
                pValue = value->c_str();
            }

            return ::SetEnvironmentVariableA( m_Name.c_str(), pValue );
#else
            if( value.has_value() )
            {
                return setenv( m_Name.c_str(), value->c_str(), 1 /*overwrite*/ ) == 0;
            }
            else
            {
                return unsetenv( m_Name.c_str() ) == 0;
            }
#endif
        }

        inline std::optional<std::string> Get() const
        {
#ifdef WIN32
            DWORD length = ::GetEnvironmentVariableA( m_Name.c_str(), nullptr, 0 );
            if( length == 0 )
            {
                return std::nullopt;
            }

            std::vector<char> buffer( length );

            if( ::GetEnvironmentVariableA( m_Name.c_str(), buffer.data(), length ) == 0 )
            {
                return std::nullopt;
            }

            return std::string( buffer.data() );
#else
            const char* pValue = getenv( m_Name.c_str() );
            if( !pValue )
            {
                return std::nullopt;
            }

            return std::string( pValue );
#endif
        }

    private:
        std::string m_Name;
        std::optional<std::string> m_OriginalValue;
    };
}

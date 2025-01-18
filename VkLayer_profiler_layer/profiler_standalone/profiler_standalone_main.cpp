// Copyright (c) 2025 Lukasz Stalmirski
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

#include "profiler_standalone_client.h"

int main()
{
    Profiler::NetworkClient client;
    client.Initialize( "127.0.0.1", 27015 );
    client.Update();

    // Print application info
    const VkApplicationInfo& appInfo = client.GetApplicationInfo();
    printf( "Application name: %s\n", appInfo.pApplicationName );
    printf( "Application version: %u.%u.%u\n",
        VK_API_VERSION_MAJOR( appInfo.applicationVersion ),
        VK_API_VERSION_MINOR( appInfo.applicationVersion ),
        VK_API_VERSION_PATCH( appInfo.applicationVersion ) );
    printf( "Engine name: %s\n", appInfo.pEngineName );
    printf( "Engine version: %u.%u.%u\n",
        VK_API_VERSION_MAJOR( appInfo.engineVersion ),
        VK_API_VERSION_MINOR( appInfo.engineVersion ),
        VK_API_VERSION_PATCH( appInfo.engineVersion ) );
    printf( "API version: %u.%u.%u\n",
        VK_API_VERSION_MAJOR( appInfo.apiVersion ),
        VK_API_VERSION_MINOR( appInfo.apiVersion ),
        VK_API_VERSION_PATCH( appInfo.apiVersion ) );

    client.Destroy();
    return 0;
}

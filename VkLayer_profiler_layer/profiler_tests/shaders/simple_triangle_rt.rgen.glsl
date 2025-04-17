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

#version 460 core
#extension GL_EXT_ray_tracing : enable

layout( binding = 0, set = 0 ) uniform accelerationStructureEXT accelerationStructure;
layout( binding = 1, set = 0, rgba8 ) uniform image2D outputImage;
layout( location = 0 ) rayPayloadEXT vec3 outputColor;

void main()
{
    vec3 cameraPosition = vec3( 0, 0, -1 );
    vec3 cameraDirection = vec3( 0, 0, 1 );

    // compute ray direction
    uvec2 rayIndex = gl_LaunchIDEXT.xy;
    uvec2 dispatchSize = gl_LaunchSizeEXT.xy;
    vec2 screenUV = ( vec2( rayIndex ) + vec2( 0.5f, 0.5f ) ) / vec2( dispatchSize.xy );
    vec3 rayDirection = normalize( vec3( screenUV.x * 2 - 1, 1 - screenUV.y * 2, 1 ) );

    // trace ray
    outputColor = vec3( 0.0f );
    traceRayEXT(
        accelerationStructure,
        gl_RayFlagsNoneEXT,
        0xFF, 0, 0, 0,
        cameraPosition, 0.0f,
        rayDirection, 1e+30f, 0 );

    imageStore( outputImage, ivec2( rayIndex ), vec4( outputColor, 1.0f ) );
}

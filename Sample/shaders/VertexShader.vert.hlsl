
float4 main( int vertId : SV_VertexID ) : SV_Position
{
    const float2 triangleVertices[3] = {
        float2(0, -0.5),
        float2(-0.5, 0.5),
        float2(0.5, 0.5) };

    return float4(triangleVertices[vertId], 0, 1);
}

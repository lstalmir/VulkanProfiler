
struct VS_Out
{
    float4 position : SV_Position;
    float3 color : COLOR;
};

VS_Out main( int vertId : SV_VertexID )
{
    const VS_Out triangleVertices[3] = {
        { float4(0, -0.5, 0, 1), float3(1, 0, 0) },
        { float4(-0.5, 0.5, 0, 1), float3(0, 0, 1) },
        { float4(0.5, 0.5, 0, 1), float3(0, 1, 0) } };

    return triangleVertices[vertId];
}

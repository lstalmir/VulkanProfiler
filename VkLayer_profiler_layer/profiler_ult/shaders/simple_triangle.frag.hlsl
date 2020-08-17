
struct PS_In
{
    float4 position : SV_Position;
    float3 color : COLOR;
};

float4 main( PS_In input ) : SV_Target
{
    return float4(input.color, 1);
}



struct PS_INPUT
{
    float4 Position    : SV_POSITION;
    float3 WorldPos    : POSITION;
    float3 Normal      : NORMAL;
    float2 UV          : TEXCOORD0;
};

struct PS_OUTPUT
{
    float4 color : SV_Target0;
};

PS_OUTPUT mainPS(PS_INPUT input)
{
    PS_OUTPUT output;
    output.color = float4 (1, 1, 1, 1);  // test
    
    return output;
}
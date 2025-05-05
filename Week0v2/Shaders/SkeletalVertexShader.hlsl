
cbuffer FSkeletalMatrixConstant : register(b0)
{
    row_major float4x4 Model;
    row_major float4x4 ViewProj;
    row_major float4x4 MInverseTranspose;
};

struct VS_INPUT {
    float3 Position    : POSITION;
    float3 Normal      : NORMAL;
    float2 UV          : TEXCOORD0;
};

struct PS_INPUT
{
    float4 Position    : SV_POSITION;
    float3 WorldPos    : POSITION;
    float3 Normal      : NORMAL;
    float2 UV          : TEXCOORD0;
};

PS_INPUT mainVS(VS_INPUT IN)
{
    PS_INPUT OUT;
    float4 worldPos = mul(float4(IN.Position,1), Model);
    OUT.Position    = mul(worldPos, ViewProj);
    OUT.WorldPos    = worldPos.xyz;
    OUT.Normal      = normalize(mul(float4(IN.Normal, 0), MInverseTranspose).xyz);  
    OUT.UV          = IN.UV;

    return OUT;
}
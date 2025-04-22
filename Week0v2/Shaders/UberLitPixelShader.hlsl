#include "ShaderHeaders/GSamplers.hlsli"


Texture2D Texture : register(t0);
Texture2D NormalTexture : register(t1);
Texture2D SpotLightShadowMap[8] : register(t3);
TextureCube CubeShadowMap : register(t11);

StructuredBuffer<uint> TileLightIndices : register(t2);

#define MAX_POINTLIGHT_COUNT 16

cbuffer FMaterialConstants : register(b0)
{
    float3 DiffuseColor;
    float TransparencyScalar;
    float3 MatAmbientColor;
    float DensityScalar;
    float3 SpecularColor;
    float SpecularScalar;
    float3 EmissiveColor;
    uint bHasNormalTexture;
}

cbuffer FConstatntBufferActor : register(b1)
{
    float4 UUID; // 임시
    uint IsSelectedActor;
    float3 padding;
}

struct FDirectionalLight
{
    float3 Direction;
    float Intensity;
    float4 Color;
};

struct FPointLight
{
    float3 Position;
    float Radius;
    
    float4 Color;
    
    float Intensity;
    float AttenuationFalloff;
    float2 pad;
};

struct FSpotLight
{
    float3 Position;
    float Intensity;
    
    float4 Color;
    
    float3 Direction;
    float InnerAngle;
    
    float OuterAngle;
    float3 pad;
    
    row_major float4x4 View;
    row_major float4x4 Proj;
};

cbuffer FLightingConstants : register(b2)
{
    uint NumDirectionalLights;
    uint NumPointLights;
    uint NumSpotLights;
    float pad;

    FDirectionalLight DirLights[4];
    FPointLight PointLights[16];
    FSpotLight SpotLights[8];
};

cbuffer FFlagConstants : register(b3)
{
    uint IsLit;
    uint IsNormal;
    float2 flagPad0;
}

cbuffer FSubUVConstant : register(b4)
{
    float indexU;
    float indexV;
    float2 subUVpadding;
}

cbuffer FCameraConstant : register(b5)
{
    row_major matrix ViewMatrix;
    row_major matrix ProjMatrix;
    row_major matrix ViewProjMatrix;
    
    float3 CameraPos;
    float NearPlane;
    float3 CameraForward;
    float FarPlane;
};

cbuffer FComputeConstants : register(b6){
    row_major matrix InverseView;
    row_major matrix InverseProj;
    float screenWidth;
    float screenHeight;
    int numTilesX;
    int numTilesY;
}

struct PS_INPUT
{
    float4 position : SV_POSITION; // 변환된 화면 좌표
    float3 worldPos : POSITION;
    float4 color : COLOR; // 전달할 색상
    float2 texcoord : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3x3 TBN: TEXCOORD3;
};

struct PS_OUTPUT
{
    float4 color : SV_Target0;
    float4 UUID : SV_Target1;
};

float CalculatePointLightShadow(float3 worldPos, float3 lightPos,float radius)
{
    float3 lightToPos = worldPos - lightPos;
    float currentDistance = length(lightToPos);
    float3 direction = normalize(lightToPos);
    float normalizedDistance = currentDistance / radius;
    // 큰 바이어스 값 사용

    float shadow = CubeShadowMap.SampleCmpLevelZero(shadowSampler, direction, normalizedDistance);
    
    return shadow;
    
    float bias = 0.1; // 매우 큰 값으로 시작
    return (normalizedDistance <= shadow + bias) ? 1.0 : 0.0;
}

float3 CalculateDirectionalLight(  
    FDirectionalLight Light,  
    float3 Normal,  
    float3 ViewDir,  
    float3 Albedo)  
{  
    // 광원 방향  
    float3 LightDir = normalize(-Light.Direction);  

    // 디퓨즈 계산  
    float NdotL = max(dot(Normal, LightDir), 0.0);  
    float3 Diffuse = Light.Color.rgb * Albedo * NdotL;  

    // 스페큘러 (Blinn-Phong)  
    float3 HalfVec = normalize(LightDir + ViewDir);  
    float NdotH = max(dot(Normal, HalfVec), 0.0);  
    float Specular = pow(NdotH, SpecularScalar * 128.0) * SpecularScalar;  
    float3 specularColor = Light.Color.rgb * Specular * SpecularColor;  

    // 최종 광원 영향  
    return (Diffuse + specularColor) * Light.Intensity;  
}

float3 CalculatePointLight(  
    FPointLight Light,  
    float3 WorldPos,  
    float3 Normal,  
    float3 ViewDir,  
    float3 Albedo)  
{  
    // 광원 거리/방향  
    float3 LightDir = Light.Position - WorldPos;
    float Distance = length(LightDir);  
    LightDir = normalize(LightDir);  

    // 반경 체크  
    if(Distance > Light.Radius) return float3(0,0,0);  

    // 감쇠 계산  
    float Attenuation = Light.Intensity / (1.0 + Light.AttenuationFalloff * Distance * Distance);  
    Attenuation *= 1.0 - smoothstep(0.0, Light.Radius, Distance);  

    // 디퓨즈  
    float NdotL = max(dot(Normal, LightDir), 0.0);
    float3 Diffuse = Light.Color.rgb * Albedo * NdotL;
    //return float3(abs(LightDir));
    //return float3(NdotL.xxx);

#if LIGHTING_MODEL_LAMBERT
    return Diffuse * Light.Intensity * Attenuation;
#endif
    // 스페큘러  
    float3 HalfVec = normalize(LightDir + ViewDir);  
    float NdotH = max(dot(Normal, HalfVec), 0.0);  
    float Specular = pow(NdotH, SpecularScalar * 128.0) * SpecularScalar;
    float3 specularColor = Light.Color.rgb * Specular * SpecularColor;

    return (Diffuse + specularColor) * Light.Intensity * Attenuation;  
}  

float3 CalculateSpotLight(
    FSpotLight Light,
    float3 WorldPos,
    float3 Normal,
    float3 ViewDir,
    float3 Albedo)
{
    // 조명 방향 벡터 (광원 위치 → 픽셀)
    float3 LightDir = normalize(Light.Position - WorldPos);
    float Distance = length(Light.Position - WorldPos);

    // Spot Light 중심 방향
    float3 SpotDirection = normalize(-Light.Direction);

    // 각도 감쇠 계산
    float CosInner = cos(Light.InnerAngle);
    float CosOuter = cos(Light.OuterAngle);
    float CosAngle = dot(SpotDirection, LightDir); // 광원 기준 → 픽셀 방향

    if (CosAngle < CosOuter)
        return float3(0, 0, 0);

    float SpotAttenuation = saturate((CosAngle - CosOuter) / (CosInner - CosOuter));
    float DistanceAttenuation = (1.0 / (1.0 + Distance * Distance * 0.01)); // 간단 거리 감쇠

    // 디퓨즈
    float NdotL = max(dot(Normal, SpotDirection), 0.0);
    float3 Diffuse = Light.Color.rgb * Albedo * NdotL;

#if LIGHTING_MODEL_LAMBERT
    return Diffuse * Light.Intensity * SpotAttenuation * DistanceAttenuation;
#endif
    // 스페큘러 (Blinn-Phong)
    float3 HalfVec = normalize(SpotDirection + ViewDir);
    float NdotH = max(dot(Normal, HalfVec), 0.0);
    float Specular = pow(NdotH, SpecularScalar * 128.0) * SpecularScalar;
    float3 specularColor = Light.Color.rgb * Specular * SpecularColor;

    return (Diffuse + specularColor) * Light.Intensity * SpotAttenuation * DistanceAttenuation;
}

// 타일 크기 설정
static const uint TILE_SIZE_X = 16;
static const uint TILE_SIZE_Y = 16;

float4 WorldToLight(float3 WorldPos, row_major float4x4 View, row_major float4x4 Proj)
{
    float4 ViewPos = mul(float4(WorldPos, 1.0), View);
    float4 LightViewPos = mul(ViewPos, Proj);
    return LightViewPos;
}

PS_OUTPUT mainPS(PS_INPUT input)
{
    PS_OUTPUT output;
    output.UUID = UUID;
    float2 uvAdjusted = input.texcoord;
  
    // 기본 색상 추출  
    float4 baseColor = Texture.Sample(linearSampler, uvAdjusted) + float4(DiffuseColor, 1.0);  

    if (!IsLit && !IsNormal)
    {
        output.color = float4(baseColor.rgb, 1.0);
        return output;
    }
    
#if LIGHTING_MODEL_GOURAUD
    if (IsSelectedActor == 1)
        input.color = input.color * 5;

    output.color = float4(baseColor.rgb * input.color.rgb, 1.0);
    return output;
#endif
    float4 normalTex = ((NormalTexture.Sample(linearSampler, uvAdjusted)- 0.5) * 2);

    float2 tileSize = float2(TILE_SIZE_X, TILE_SIZE_Y);
    
    uint2 pixelCoord = uint2(input.position.xy);
    uint2 tileCoord = pixelCoord / tileSize; // 각 성분별 나눔
    uint tileIndex = tileCoord.x + tileCoord.y * numTilesX;
    
    float3 Normal = input.normal;
    
    if (bHasNormalTexture)
    {
        Normal = normalize(mul(normalTex.rgb, input.TBN));

        if (length(Normal) < 0.001) // tangent 값이 없을때 ( uv 없을때 )
        {
            Normal = input.normal;
        }        
    }
    
    if (IsNormal)
    {
        //Normal = input.normal;
        Normal = Normal * 0.5 + 0.5;
        output.color = float4(Normal.rgb, 1.0);
        return output;
    }
    
    float3 ViewDir = normalize(CameraPos - input.worldPos); // CameraPos도 안 들어오고, ViewDir은 카메라의 Foward 아닌가요?
    
    //float3 TotalLight = MatAmbientColor; // 전역 앰비언트
    // TODO : Lit이면 낮은 값 Unlit이면 float3(1.0f,1.0f,1.0f)면 됩니다.
    float3 TotalLight = float3(0.01f,0.01f,0.01f); // 전역 앰비언트  
    if (IsSelectedActor == 1)
         TotalLight = TotalLight * 10.0f;
    TotalLight += EmissiveColor; // 자체 발광  
    
    // 방향광 처리  s
    for(uint i=0; i<NumDirectionalLights; ++i)  
        TotalLight += CalculateDirectionalLight(DirLights[i], Normal, ViewDir, baseColor.rgb);  

    // 점광 처리  
    [unroll(16)]
    for(uint j=0; j<NumPointLights; ++j)
    {
        uint listIndex = tileIndex * MAX_POINTLIGHT_COUNT + j;
        uint lightIndex = TileLightIndices[listIndex];
        if (lightIndex == 0xFFFFFFFF)
        {
            break;
        }
        
      
          // 먼저 포인트 라이트 조명 계산
        float3 lightColor = CalculatePointLight(PointLights[lightIndex], input.worldPos, Normal, ViewDir, baseColor.rgb);
    
    // 포인트 라이트 그림자 계산
        float shadow = CalculatePointLightShadow(input.worldPos, PointLights[lightIndex].Position,PointLights[lightIndex].Radius);
    
    // 그림자를 조명 계산에 적용
        TotalLight += lightColor * shadow;
    }
    
    for (uint k = 0; k < NumSpotLights; ++k)
    {
        bool bIsShadow = false;
        float shadow = 0;
        float3 LightColor = CalculateSpotLight(SpotLights[k], input.worldPos, input.normal, ViewDir, baseColor.rgb);
        TotalLight += LightColor;
        if (length(LightColor) > 0.0)
        {
            float4 LightViewPos = WorldToLight(input.worldPos, SpotLights[k].View, SpotLights[k].Proj);
            
            float2 shadowUV = LightViewPos.xy / LightViewPos.w * 0.5 + 0.5;
            shadowUV.y = 1.0 - shadowUV.y;
            float worldDepth = LightViewPos.z / LightViewPos.w;

            if(shadowUV.x >= 0 && shadowUV.x <= 1 &&
                shadowUV.y >= 0 && shadowUV.y <= 1 && 
                worldDepth >= 0 && worldDepth <= 1)
            {
                float bias = max(0.01 * (1.0 - dot(Normal, -SpotLights[k].Direction)), 0.001);
                for (int x = -1; x <= 1; ++x)
                {
                    for (int y = -1; y <= 1; ++y)
                    {
                        uint textureWidth, textureHeight;
                        SpotLightShadowMap[k].GetDimensions(textureWidth, textureHeight);
                        float2 texelSize = 1.0 / float2(textureWidth, textureHeight);
                        float2 offset = float2(x, y) * texelSize;
                        float sample = SpotLightShadowMap[k].Sample(pointSampler, shadowUV + offset).r;
                        shadow += (worldDepth >= sample + bias) ? 1.0 : 0.0;
                    }
                }
                shadow = shadow / 9.0;
                bIsShadow = true;
            }
        }
        float shadowFactor = bIsShadow ? (1.0 - shadow) : 1.0;
        if (bIsShadow)
            TotalLight += LightColor * shadowFactor;
    }
    
    float4 FinalColor = float4(TotalLight * baseColor.rgb, baseColor.a * TransparencyScalar);
    // 최종 색상 
    output.color = FinalColor;
    return output;  
}
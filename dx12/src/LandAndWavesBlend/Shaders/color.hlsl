#include "LightingUtils.hlsl"
cbuffer cbPerObject : register(b0) {
  float4x4 gWorld;
  float4x4 gInvTransWorld;
  float4x4 gTexTransform;
};

cbuffer cbMaterial : register(b1) {
  float4 gDiffuseAlbedo;
  float3 gFresnelR0;
  float gRoughness;
  float4x4 gMatTransform;
};

Texture2D gDiffuseMap : register(t0);
SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

struct VertexIn {
  float3 PosL : POSITION;
  float3 Normal : NORMAL;
  float2 TexC : TEXCOORD;
};

struct VertexOut {
  float4 PosH : SV_Position;
  float3 PosW : Position;
  float3 NormalW : NORMAL;
  float2 TexC : TEXCOORD;
};

cbuffer cbPass : register(b2) {
  float4x4 gView;
  float4x4 gInvView;
  float4x4 gProj;
  float4x4 gInvProj;
  float4x4 gViewProj;
  float4x4 gInvViewProj;
  float3 gEyePosW;
  float cbPerObjectPad1;
  float2 gRenderTargetSize;
  float2 gInvRenderTargetSize;
  float gNearZ;
  float gFarZ;
  float gTotalTime;
  float gDeltaTime;

  float4 gAmbientLight;
  float4 gFogColor;
  float gFogStart = 5.0f;
  float gFogRange = 150.0f;
  Light gLights[MaxLights];
};
VertexOut VSMain(VertexIn vin) {
  VertexOut vout;

  vout.NormalW = mul(vin.Normal, (float3x3)(gInvTransWorld));

  float4 PosW = mul(float4(vin.PosL, 1.0), gWorld);
  vout.PosW = PosW.xyz;
  vout.PosH = mul(PosW, gViewProj);
  float4 TexObject = mul(float4(vin.TexC, 0.0, 1.0), gTexTransform);
  vout.TexC = mul(TexObject, gMatTransform);
  return vout;
};

float4 PSMain(VertexOut vout) : SV_Target {

  vout.NormalW = normalize(vout.NormalW); // re-normalize
  // return float4(0.5 * vout.NormalW + 0.5, 1);
  float3 toEye = gEyePosW - vout.PosW;
  float distToEye = length(toEye);
  float3 viewDir = toEye / distToEye;

  float4 texColor0 = gDiffuseMap.Sample(gsamAnisotropicWrap, vout.TexC);

  float4 diffuseAlbedo = texColor0 * gDiffuseAlbedo;

  float3 ambient = gAmbientLight * gDiffuseAlbedo;
  Material mat;
  mat.DiffuseAlbedo = diffuseAlbedo;
  mat.FresnelR0 = gFresnelR0;
  mat.Shininess = 1 - gRoughness;
  float4 litColor =
      float4(ambient + ComputeDirLight(gLights[0], mat, vout.NormalW, viewDir) +
                 ComputeDirLight(gLights[1], mat, vout.NormalW, viewDir) +
                 ComputeDirLight(gLights[2], mat, vout.NormalW, viewDir),
             1.0);

#ifdef FOG
  float fogAmount = saturate((distToEye - gFogStart) / gFogRange);
  litColor = lerp(litColor, gFogColor, fogAmount);
#endif
  litColor.a = gDiffuseAlbedo.a;
  return litColor;
}
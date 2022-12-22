#include "LightingUtils.hlsl"
cbuffer cbPerObject : register(b0) {
  float4x4 gWorld;
  float4x4 gInvTransWorld;
};

cbuffer cbMaterial : register(b1) {
  float4 gDiffuseAlbedo;
  float3 gFresnelR0;
  float gRoughness;
  float4x4 gMatTransform;
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
  Light gLights[MaxLights];
};

struct VertexIn {
  float3 PosL : POSITION;
  float3 Normal : NORMAL;
};

struct VertexOut {
  float4 PosH : SV_Position;
  float3 PosW : Position;
  float3 NormalW : NORMAL;
};

VertexOut VSMain(VertexIn vin) {
  VertexOut vout;

  vout.NormalW = mul(vin.Normal, (float3x3)(gInvTransWorld));

  float4 PosW = mul(float4(vin.PosL, 1.0), gWorld);
  vout.PosW = PosW.xyz;
  vout.PosH = mul(PosW, gViewProj);
  return vout;
};

float4 PSMain(VertexOut vout) : SV_Target {

  vout.NormalW = normalize(vout.NormalW); // re-normalize
  // return float4(0.5 * vout.NormalW + 0.5, 1);
  float3 viewDir = normalize(gEyePosW - vout.PosW);
  float3 ambient = gAmbientLight * gDiffuseAlbedo;
  Material mat;
  mat.DiffuseAlbedo = gDiffuseAlbedo;
  mat.FresnelR0 = gFresnelR0;
  mat.Shininess = 1 - gRoughness;

  float4 litColor = float4(
      ambient + ComputeDirLight(gLights[0], mat, vout.NormalW, viewDir), 1.0);
  litColor.a = gDiffuseAlbedo.a;
  return litColor;
}
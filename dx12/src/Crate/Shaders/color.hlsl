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
Texture2D gDiffuseMap1 : register(t1);
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

struct PixelOut {
  float4 color : SV_Target;
  float depth : SV_Depth;
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

PixelOut PSMain(VertexOut vout) {
  PixelOut Res;
  vout.NormalW = normalize(vout.NormalW); // re-normalize
  // return float4(0.5 * vout.NormalW + 0.5, 1);
  float3 viewDir = normalize(gEyePosW - vout.PosW);

  float4 texColor0 = gDiffuseMap.Sample(gsamLinearClamp, vout.TexC);
  float4 texColor1 = gDiffuseMap1.Sample(gsamLinearClamp, vout.TexC);

  float4 diffuseAlbedo = lerp(texColor0, texColor1, 0.2) * gDiffuseAlbedo;

  float3 ambient = gAmbientLight * gDiffuseAlbedo;
  Material mat;
  mat.DiffuseAlbedo = diffuseAlbedo;
  mat.FresnelR0 = gFresnelR0;
  mat.Shininess = 1 - gRoughness;
  float4 litColor = float4(
      ambient + 
      ComputeDirLight(gLights[0], mat, vout.NormalW, viewDir) + 
      ComputeDirLight(gLights[1], mat, vout.NormalW, viewDir) + 
      ComputeDirLight(gLights[2], mat, vout.NormalW, viewDir),1.0);
  litColor.a = gDiffuseAlbedo.a;
  Res.color = litColor;
  Res.depth =
      vout.PosH.z + 0.0001f; // slightly move NDC-z by 0.0001 to aviod early-z
  return Res;
}
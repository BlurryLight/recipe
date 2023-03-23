#include "LightingUtils.hlsl"
cbuffer cbPerObject : register(b0) {
  float4x4 gWorld;
  float4x4 gInvTransWorld;
  float4x4 gTexTransform;

  uint gMaterialIndex;
  uint padding;
  uint padding1;
  uint padding2;
};

struct MaterialData {
  float4 gDiffuseAlbedo;
  float3 gFresnelR0;
  float gRoughness;
  float4x4 gMatTransform;
  uint DiffuseMapIndex;
  uint padding;
  uint padding1;
  uint padding2;
};

// need /enable_unbounded_descriptor_tables
Texture2D gDiffuseMap[] : register(t0);
// 确保从t0开始，但是不和diffuseMap发生冲突
StructuredBuffer<MaterialData> gMaterialData : register(t0, space1);

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

cbuffer cbPass : register(b1) {
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
  VertexOut vout = (VertexOut)0.0f;

  vout.NormalW = mul(vin.Normal, (float3x3)(gInvTransWorld));

  float4 PosW = mul(float4(vin.PosL, 1.0), gWorld);
  vout.PosW = PosW.xyz;
  vout.PosH = mul(PosW, gViewProj);

  MaterialData matData = gMaterialData[gMaterialIndex];
  float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
  vout.TexC = mul(texC, matData.gMatTransform).xy;

  return vout;
};

float4 PSMain(VertexOut vout) : SV_Target {

  vout.NormalW = normalize(vout.NormalW); // re-normalize
  // return float4(0.5 * vout.NormalW + 0.5, 1);
  float3 viewDir = normalize(gEyePosW - vout.PosW);
  MaterialData matData = gMaterialData[gMaterialIndex];
  float3 ambient = gAmbientLight * gDiffuseMap[matData.DiffuseMapIndex].Sample(
                                       gsamLinearWrap, vout.TexC);
  Material mat;
  mat.DiffuseAlbedo = matData.gDiffuseAlbedo;
  mat.FresnelR0 = matData.gFresnelR0;
  mat.Shininess = max(1 - matData.gRoughness, 0.001);
  float4 litColor =
      float4(ambient + ComputeDirLight(gLights[0], mat, vout.NormalW, viewDir) +
                 ComputeDirLight(gLights[1], mat, vout.NormalW, viewDir) +
                 ComputeDirLight(gLights[2], mat, vout.NormalW, viewDir),
             1.0);
  litColor.a = mat.DiffuseAlbedo.a;
  return litColor;
}
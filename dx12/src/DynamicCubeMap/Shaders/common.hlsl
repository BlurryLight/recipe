#ifndef __COMMON_HLSL
#define __COMMON_HLSL
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

TextureCube gCubeMap : register(t0);
// need /enable_unbounded_descriptor_tables
Texture2D gDiffuseMap[] : register(t1);
// 确保从t0开始，但是不和diffuseMap发生冲突
StructuredBuffer<MaterialData> gMaterialData : register(t0, space1);

SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

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

#endif
#include "common.hlsl"

struct VertexIn {
  float3 PosL : POSITION;
  float3 Normal : NORMAL;
  float2 TexC : TEXCOORD;
};

struct VertexOut {
  float4 PosH : SV_Position;
  float3 PosL : POSITION;
};

VertexOut VSMain(VertexIn vin) {
  VertexOut vout = (VertexOut)(0);
  vout.PosL = vin.PosL;
  // Transform to world space.
  float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);

  // Always center sky about camera.
  posW.xyz += gEyePosW;

  // Set z = w so that z/w = 1 (i.e., skydome always on far plane).
  vout.PosH = mul(posW, gViewProj).xyww;

  return vout;
}

float4 PSMain(VertexOut vout) : SV_TARGET {
  return gCubeMap.Sample(gsamLinearWrap, vout.PosL.xyz);
}
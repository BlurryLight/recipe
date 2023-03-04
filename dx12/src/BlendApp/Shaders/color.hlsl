cbuffer cbPerObject : register(b0) {
  float4x4 gWorld;
  float4x4 gInvTransWorld;
  float4x4 gTexTransform;
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
};
VertexOut VSMain(VertexIn vin) {
  VertexOut vout = (VertexOut)(0);
  float4 Tex = mul(float4(vin.TexC, 0.0, 1.0), gTexTransform);
  vout.TexC = Tex;
  float4 PosW = mul(float4(vin.PosL, 1.0), gWorld);
  vout.PosH = mul(PosW, gViewProj);
  return vout;
};

float4 PSMain(VertexOut vout) : SV_Target {
  return gDiffuseMap.Sample(gsamLinearClamp, vout.TexC);
}
cbuffer cbPerObject : register(b0) { float4x4 gWorld; };

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

struct VertexIn {
  float3 PosL : POSITION;
  float3 Normal: NORMAL;
};

struct VertexOut {
  float4 PosH : SV_Position;
  float4 Color : COLOR;
};

VertexOut VSMain(VertexIn vin) {
  VertexOut vout;
  vout.Color = 0.5 * float4(vin.Normal,1.0) + 0.5;

  float4 PosW = mul(float4(vin.PosL,1.0), gWorld);
  vout.PosH = mul(PosW, gViewProj);
  return vout;
};

float4 PSMain(VertexOut vout) : SV_Target { return float4(vout.Color.xyz, 1); }
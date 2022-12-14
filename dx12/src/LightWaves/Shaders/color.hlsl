cbuffer cbPerObject : register(b0) { float4x4 gWorld; };

cbuffer cbMaterial : register(b1) {
  float4 gDiffuseAlbedo;
  float3 gFresnelR0;
  float gRoughness;
  float4x4 gMatTransform;
};

#define MaxLights 16
struct Light {
  float3 gStrength;
  float FalloffStart; // point/spot
  float3 Direction;   // dir/spot
  float FalloffEnd;
  float3 Position; // point/spot
  float SpotPower; // spot only
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
  float3 NormalW : NORMAL;
};

VertexOut VSMain(VertexIn vin) {
  VertexOut vout;

  // avoid inv-transpose
  vout.NormalW = mul(vin.Normal, (float3x3)(gWorld));

  float4 PosW = mul(float4(vin.PosL, 1.0), gWorld);
  vout.PosH = mul(PosW, gViewProj);
  return vout;
};

float4 PSMain(VertexOut vout) : SV_Target {

  return 0.5 * float4(vout.NormalW, 1.0) + 0.5 + gAmbientLight + float4(gLights[0].gStrength,1.0);
}